#include "battle_internal.h"

#define BORDERW 4

/* Object.
 */
 
struct bmenu {
  int x,y,w,h;
  int teamp;
  struct battle *battle;
  struct team *team;
  struct bmenu_option {
    int texid,texw,texh;
    int id;
  } *optionv;
  int optionc,optiona;
  int colc,rowc,rowc_total,colw,rowh; // Reestablished at pack. Plain (rowc) is the count of visible rows.
  int selp; // Index in (optionv), also (row*colc+col).
  int yscroll; // In rows.
  uint32_t cursor_color;
  int state;
  int chosen; // (id) from the activated option, <0 if cancelled, or 0 if pending
};

/* Delete.
 */
 
static void bmenu_option_cleanup(struct bmenu_option *option) {
  egg_texture_del(option->texid);
}
 
void bmenu_del(struct bmenu *bmenu) {
  if (!bmenu) return;
  if (bmenu->optionv) {
    while (bmenu->optionc-->0) bmenu_option_cleanup(bmenu->optionv+bmenu->optionc);
    free(bmenu->optionv);
  }
  free(bmenu);
}

/* New.
 */
 
struct bmenu *bmenu_new(struct battle *battle,int teamp) {
  if ((teamp<0)||(teamp>1)) return 0;
  struct bmenu *bmenu=calloc(1,sizeof(struct bmenu));
  if (!bmenu) return 0;
  bmenu->teamp=teamp;
  bmenu->battle=battle;
  bmenu->team=battle->teamv+teamp;
  bmenu->cursor_color=0x203040ff;
  return bmenu;
}

/* Set bounds.
 */
 
void bmenu_set_bounds(struct bmenu *bmenu,int x,int y,int w,int h) {
  bmenu->x=x;
  bmenu->y=y;
  bmenu->w=w;
  bmenu->h=h;
}

/* Cursor motion.
 */
 
static void bmenu_move(struct bmenu *bmenu,int dx,int dy) {
  bmenu->chosen=0;
  if ((bmenu->colc<1)||(bmenu->rowc_total<1)) return;
  egg_play_sound(RID_sound_ui_move);
  int col=bmenu->selp%bmenu->colc+dx; if (col<0) col=bmenu->colc-1; else if (col>=bmenu->colc) col=0;
  int row=bmenu->selp/bmenu->colc+dy; if (row<0) row=bmenu->rowc_total-1; else if (row>=bmenu->rowc_total) row=0;
  bmenu->selp=row*bmenu->colc+col;
  if (row<bmenu->yscroll) bmenu->yscroll=row;
  else if (row>=bmenu->yscroll+bmenu->rowc) bmenu->yscroll=row+1-bmenu->rowc;
}

/* Activate.
 */
 
static void bmenu_activate(struct bmenu *bmenu) {
  if ((bmenu->selp>=0)&&(bmenu->selp<bmenu->optionc)) bmenu->chosen=bmenu->optionv[bmenu->selp].id;
  else bmenu->chosen=0;
}

/* Cancel.
 */
 
static void bmenu_cancel(struct bmenu *bmenu) {
  bmenu->chosen=-1;
}

/* Input.
 */
 
void bmenu_input(struct bmenu *bmenu,int btnid,int value,int state) {
  if (bmenu->optionc<1) return;
  if (value) switch (btnid) {
    case EGG_BTN_LEFT: bmenu_move(bmenu,-1,0); break;
    case EGG_BTN_RIGHT: bmenu_move(bmenu,1,0); break;
    case EGG_BTN_UP: bmenu_move(bmenu,0,-1); break;
    case EGG_BTN_DOWN: bmenu_move(bmenu,0,1); break;
    case EGG_BTN_SOUTH: bmenu_activate(bmenu); break;
    case EGG_BTN_WEST: bmenu_cancel(bmenu); break;
  }
}

/* Update.
 */
 
void bmenu_update(struct bmenu *bmenu,double elapsed) {
}

/* Render.
 */
 
void bmenu_render(struct bmenu *bmenu) {
  if ((bmenu->colc>0)&&bmenu->optionc) {
    if (1) { // Cursor. TODO conditional
      int selcol=bmenu->selp%bmenu->colc;
      int selrow=bmenu->selp/bmenu->colc;
      if ((selcol>=0)&&(selcol<bmenu->colc)&&(selrow>=bmenu->yscroll)&&(selrow<bmenu->yscroll+bmenu->rowc)) {
        graf_draw_rect(&g.graf,
          bmenu->x+BORDERW+selcol*bmenu->colw-1,
          bmenu->y+BORDERW+(selrow-bmenu->yscroll)*bmenu->rowh-1,
          bmenu->colw+1,
          bmenu->rowh+1,
          bmenu->cursor_color
        );
      }
    }
    // Labels.
    int16_t ylimit=bmenu->y+bmenu->h-BORDERW;
    int16_t row=bmenu->yscroll,dsty=bmenu->y+BORDERW,optp=bmenu->yscroll*bmenu->colc;
    for (;row<bmenu->rowc_total;row++,dsty+=bmenu->rowh) {
      if (dsty>=ylimit) break;
      int16_t col=0,dstx=bmenu->x+BORDERW;
      for (;col<bmenu->colc;col++,dstx+=bmenu->colw,optp++) {
        if (optp>=bmenu->optionc) break;
        const struct bmenu_option *option=bmenu->optionv+optp;
        int16_t cpw=option->texw;
        int16_t cph=option->texh;
        if (dstx+cpw>bmenu->x+bmenu->w) cpw=bmenu->x+bmenu->w-dstx;
        if (dsty+cph>bmenu->y+bmenu->h) cph=bmenu->y+bmenu->h-dsty;
        graf_draw_decal(&g.graf,option->texid,dstx,dsty,0,0,cpw,cph,0);
      }
    }
  }
  textbox_render_border(bmenu->x,bmenu->y,bmenu->w,bmenu->h);
  uint8_t mask=0;
  if (bmenu->yscroll) mask|=DIR_N;
  if (bmenu->yscroll+bmenu->rowc<bmenu->rowc_total) mask|=DIR_S;
  if (mask) textbox_render_more(bmenu->x,bmenu->y,bmenu->w,bmenu->h,mask);
}

/* Drop all options.
 */
 
void bmenu_clear(struct bmenu *bmenu) {
  while (bmenu->optionc>0) {
    bmenu->optionc--;
    bmenu_option_cleanup(bmenu->optionv+bmenu->optionc);
  }
  bmenu->state=BMENU_STATE_NONE;
}

/* Add unconfigured option.
 */
 
static struct bmenu_option *bmenu_optionv_add(struct bmenu *bmenu) {
  if (bmenu->optionc>=bmenu->optiona) {
    int na=bmenu->optiona+8;
    if (na>INT_MAX/sizeof(struct bmenu_option)) return 0;
    void *nv=realloc(bmenu->optionv,sizeof(struct bmenu_option)*na);
    if (!nv) return 0;
    bmenu->optionv=nv;
    bmenu->optiona=na;
  }
  struct bmenu_option *option=bmenu->optionv+bmenu->optionc++;
  memset(option,0,sizeof(struct bmenu_option));
  return option;
}

/* Add option.
 */
 
void bmenu_add_option(struct bmenu *bmenu,const char *text,int textc,int id) {
  struct bmenu_option *option=bmenu_optionv_add(bmenu);
  if (!option) return;
  option->id=id;
  option->texid=font_tex_oneline(g.font,text,textc,bmenu->w,0xffffffff);
  egg_texture_get_status(&option->texw,&option->texh,option->texid);
}

/* Pack options.
 */
 
void bmenu_pack(struct bmenu *bmenu) {
  int wmax=1,hmax=1;
  const struct bmenu_option *option=bmenu->optionv;
  int i=bmenu->optionc;
  for (;i-->0;option++) {
    if (option->texw>wmax) wmax=option->texw;
    if (option->texh>hmax) hmax=option->texh;
  }
  int wlimit=bmenu->w-BORDERW*2; if (wlimit<1) wlimit=1;
  int hlimit=bmenu->h-BORDERW*2; if (hlimit<1) hlimit=1;
  if ((bmenu->colw=wmax)>wlimit) bmenu->colw=wlimit;
  if ((bmenu->rowh=hmax)>hlimit) bmenu->rowh=hlimit;
  if ((bmenu->colc=wlimit/bmenu->colw)<1) bmenu->colc=1;
  if ((bmenu->rowc=hlimit/bmenu->rowh)<1) bmenu->rowc=1;
  if (bmenu->rowc>=bmenu->optionc) bmenu->colc=1; // Single column if they fit.
  if (bmenu->colc==1) bmenu->colw=wlimit; // Fill width if single column.
  bmenu->rowc_total=(bmenu->optionc+bmenu->colc-1)/bmenu->colc;
  bmenu->selp=0;
  bmenu->yscroll=0;
}

/* State.
 */
 
void bmenu_set_state(struct bmenu *bmenu,int state) {
  bmenu->state=state;
}

int bmenu_get_state(const struct bmenu *bmenu) {
  return bmenu->state;
}

int bmenu_get_chosen(const struct bmenu *bmenu) {
  return bmenu->chosen;
}

int bmenu_get_focus(const struct bmenu *bmenu) {
  if ((bmenu->selp<0)||(bmenu->selp>=bmenu->optionc)) return 0;
  return bmenu->optionv[bmenu->selp].id;
}
