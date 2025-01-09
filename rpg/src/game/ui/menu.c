#include "game/rpg.h"
#include "menu.h"

/* Delete.
 */
 
void menu_del(struct menu *menu) {
  if (!menu) return;
  if (menu->buttonv) {
    while (menu->buttonc-->0) button_del(menu->buttonv[menu->buttonc]);
    free(menu->buttonv);
  }
  free(menu);
}

/* New.
 */
 
struct menu *menu_new() {
  struct menu *menu=calloc(1,sizeof(struct menu));
  if (!menu) return 0;
  
  menu->focus_anim_time=0.250;
  menu->render_focus_before_buttons=0;
  
  return menu;
}

/* Motion.
 */

void menu_motion(struct menu *menu,int dx,int dy) {
  if (menu->buttonc<1) return;
  if (menu->focus_to) {
    struct button *next=menu_get_next_focus(menu,menu->focus_to,dx,dy);
    if (next&&(next!=menu->focus_to)) {
      menu->focus_from=menu->focus_to;
      menu->focus_to=next;
      menu->focus_clock=0.0;
    }
  } else {
    menu->focus_to=menu_get_initial_focus(menu,(dx<0)||(dy<0));
    menu->focus_from=0;
  }
}

/* Activate.
 */
 
void menu_activate(struct menu *menu) {
  if (!menu->focus_to) return;
  //TODO Permit per-button override callbacks.
  if (menu->cb_activate) menu->cb_activate(menu,menu->focus_to);
}

/* Cancel.
 */
 
void menu_cancel(struct menu *menu) {
  // XXX I think menu generically is not going to watch for cancel -- that's layer's problem.
}

/* Input event.
 */

void menu_input(struct menu *menu,int btnid,int value,int state) {
  if (value) switch (btnid) {
    case EGG_BTN_LEFT: menu_motion(menu,-1,0); break;
    case EGG_BTN_RIGHT: menu_motion(menu,1,0); break;
    case EGG_BTN_UP: menu_motion(menu,0,-1); break;
    case EGG_BTN_DOWN: menu_motion(menu,0,1); break;
    case EGG_BTN_SOUTH: menu_activate(menu); break;
    case EGG_BTN_WEST: menu_cancel(menu); break;
  }
}

/* Update.
 */
 
void menu_update(struct menu *menu,double elapsed) {
  if (menu->focus_from&&menu->focus_to) {
    if ((menu->focus_clock+=elapsed)>=menu->focus_anim_time) {
      menu->focus_clock=0.0;
      menu->focus_from=0;
    }
  }
}

/* Render outline around some box.
 */
 
static void menu_render_outline(struct menu *menu,int x,int y,int w,int h,int linew,uint32_t rgba) {
  int pad=1;
  graf_draw_rect(&g.graf,x-linew-pad,y-linew-pad,w+(linew<<1)+(pad<<1),linew,rgba);
  graf_draw_rect(&g.graf,x-linew-pad,y-linew-pad,linew,h+(linew<<1)+(pad<<1),rgba);
  graf_draw_rect(&g.graf,x+w+pad,y-linew-pad,linew,h+(linew<<1)+(pad<<1),rgba);
  graf_draw_rect(&g.graf,x-linew-pad,y+h+pad,w+(linew<<1)+(pad<<1),linew,rgba);
}

/* Render focus.
 */

static void menu_render_focus_animated(struct menu *menu,struct button *a,struct button *b,double t) {
  if (menu->cb_render_focus) {
    menu->cb_render_focus(menu,a,b,t);
  } else if (a==b) {
    //menu_render_outline(menu,b->x,b->y,b->w,b->h,1,0xffff00ff);
    //graf_draw_rect(&g.graf,b->x,b->y,b->w,b->h,0x30507080);
    graf_draw_tile(&g.graf,texcache_get_image(&g.texcache,RID_image_uitiles),b->x-(NS_sys_tilesize>>1)-4,b->y+(b->h>>1),0x01,0);
  } else {
    double T=1.0-t;
    int x=(int)(a->x*T+b->x*t);
    int y=(int)(a->y*T+b->y*t);
    //int w=(int)(a->w*T+b->w*t);
    int h=(int)(a->h*T+b->h*t);
    //menu_render_outline(menu,x,y,w,h,1,0xffff00ff);
    //graf_draw_rect(&g.graf,x,y,w,h,0x30507080);
    graf_draw_tile(&g.graf,texcache_get_image(&g.texcache,RID_image_uitiles),x-(NS_sys_tilesize>>1)-4,y+(h>>1),0x01,0);
  }
}
 
static void menu_render_focus(struct menu *menu) {
  if (menu->focus_to) {
    if (menu->focus_from) {
      double t=menu->focus_clock/menu->focus_anim_time;
      if (t<=0.0) menu_render_focus_animated(menu,menu->focus_from,menu->focus_from,0.0);
      else if (t>=1.0) menu_render_focus_animated(menu,menu->focus_to,menu->focus_to,1.0);
      else menu_render_focus_animated(menu,menu->focus_from,menu->focus_to,t);
    } else {
      menu_render_focus_animated(menu,menu->focus_to,menu->focus_to,1.0);
    }
  }
}

/* Render.
 */
 
void menu_render(struct menu *menu) {
  if (menu->render_focus_before_buttons) menu_render_focus(menu);
  struct button **p=menu->buttonv;
  int i=menu->buttonc;
  for (;i-->0;p++) button_render(*p);
  if (!menu->render_focus_before_buttons) menu_render_focus(menu);
}

/* Spawn button.
 */
 
struct button *button_spawn(struct menu *menu) {
  if (!menu) return 0;
  if (menu->buttonc>=menu->buttona) {
    int na=menu->buttona+16;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(menu->buttonv,sizeof(void*)*na);
    if (!nv) return 0;
    menu->buttonv=nv;
    menu->buttona=na;
  }
  struct button *button=button_new();
  if (!button) return 0;
  button->menu=menu;
  menu->buttonv[menu->buttonc++]=button;
  return button;
}

/* Pack buttons in a column.
 */
 
void menu_pack_col(struct menu *menu,int x,int y,int w,int h,int xalign,int yalign,int uniform_widths) {
  struct button **p;
  int i;
  if (!menu) return;
  if (menu->buttonc<1) return;
  int yspacing=0; // TODO?
  
  int hsum=0,wmax=0;
  for (p=menu->buttonv,i=menu->buttonc;i-->0;p++) {
    struct button *button=*p;
    hsum+=button->h;
    if (button->w>wmax) wmax=button->w;
  }
  hsum+=yspacing*(menu->buttonc-1);
  if (uniform_widths) {
    for (p=menu->buttonv,i=menu->buttonc;i-->0;p++) {
      struct button *button=*p;
      button->w=wmax;
    }
  }
  
  int dsty=y;
  if (yalign<0) ;
  else if (yalign>0) dsty=y+h-hsum;
  else dsty=y+(h>>1)-(hsum>>1);
  for (p=menu->buttonv,i=menu->buttonc;i-->0;p++) {
    struct button *button=*p;
    button->y=dsty;
    dsty+=button->h+yspacing;
  }
  
  if (xalign<0) {
    for (p=menu->buttonv,i=menu->buttonc;i-->0;p++) {
      struct button *button=*p;
      button->x=x;
    }
  } else if (xalign>0) {
    for (p=menu->buttonv,i=menu->buttonc;i-->0;p++) {
      struct button *button=*p;
      button->x=x+w-button->w;
    }
  } else {
    for (p=menu->buttonv,i=menu->buttonc;i-->0;p++) {
      struct button *button=*p;
      button->x=x+(w>>1)-(button->w>>1);
    }
  }
}

/* Pack buttons in a row.
 */
 
void menu_pack_row(struct menu *menu,int x,int y,int w,int h,int xalign,int yalign,int uniform_heights) {
  struct button **p;
  int i;
  if (!menu) return;
  if (menu->buttonc<1) return;
  int xspacing=0; // TODO?
  
  int wsum=0,hmax=0;
  for (p=menu->buttonv,i=menu->buttonc;i-->0;p++) {
    struct button *button=*p;
    wsum+=button->w;
    if (button->h>hmax) hmax=button->h;
  }
  wsum+=xspacing*(menu->buttonc-1);
  if (uniform_heights) {
    for (p=menu->buttonv,i=menu->buttonc;i-->0;p++) {
      struct button *button=*p;
      button->h=hmax;
    }
  }
  
  int dstx=x;
  if (xalign<0) ;
  else if (xalign>0) dstx+=w-wsum;
  else dstx+=(w>>1)-(wsum>>1);
  for (p=menu->buttonv,i=menu->buttonc;i-->0;p++) {
    struct button *button=*p;
    button->x=dstx;
    dstx+=button->w+xspacing;
  }
  
  if (yalign<0) {
    for (p=menu->buttonv,i=menu->buttonc;i-->0;p++) {
      struct button *button=*p;
      button->y=y;
    }
  } else if (yalign>0) {
    for (p=menu->buttonv,i=menu->buttonc;i-->0;p++) {
      struct button *button=*p;
      button->y=y+h-button->h;
    }
  } else {
    for (p=menu->buttonv,i=menu->buttonc;i-->0;p++) {
      struct button *button=*p;
      button->y=y+(h>>1)-(button->h>>1);
    }
  }
}

/* First button to focus.
 */
 
struct button *menu_get_initial_focus(struct menu *menu,int end) {
  if (!menu) return 0;
  if (end) {
    int i=menu->buttonc;
    while (i-->0) {
      struct button *button=menu->buttonv[i];
      if (button->enable) return button;
    }
  } else {
    int i=0;
    for (;i<menu->buttonc;i++) {
      struct button *button=menu->buttonv[i];
      if (button->enable) return button;
    }
  }
  return 0;
}

/* Focus neighbor.
 */

struct button *menu_get_next_focus(struct menu *menu,struct button *from,int dx,int dy) {
  if (!menu||!from) return 0;
  /* Every enabled button is eligible.
   * Distance to a candidate is:
   *   Cardinal distance to leading edge, wrapping around the framebuffer.
   *   Plus double the absolute value distance to nearest edge in the perpendicular direction -- zero if they overlap on that axis.
   * So picture a narrow cone extending in the direction of travel, and it wraps around at the end.
   */
  int wrap,flead;
  if (dx) { wrap=FBW; flead=from->x+((dx>0)?from->w:0); }
  else { wrap=FBH; flead=from->y+((dy>0)?from->h:0); }
  struct button *best=0;
  int bestdistance=FBW*FBH;
  struct button **p=menu->buttonv;
  int i=menu->buttonc;
  for (;i-->0;p++) {
    struct button *other=*p;
    //if (other==from) continue;
    if (!other->enable) continue;
    
    int distance,offaxis,olead;
    if (dx) {
      if ((offaxis=from->y-other->h-other->y)<0) offaxis=other->y-from->h-from->y;
      if (dx<0) distance=flead-other->x;
      else distance=other->x+other->w-flead;
      if (distance<=0) distance+=wrap;
    } else {
      if ((offaxis=from->x-other->w-other->x)<0) offaxis=other->x-from->w-from->x;
      if (dy<0) distance=flead-other->y;
      else distance=other->y+other->h-flead;
      if (distance<=0) distance+=wrap;
    }
    if (offaxis>0) distance+=offaxis<<1;
    
    if (distance<bestdistance) {
      best=other;
      bestdistance=distance;
    } else if (distance==bestdistance) {
      // In a tie, break toward self. This happens often, when you have a single column or row.
      if (other==from) best=other;
    }
  }
  return best;
}

/* Enable button by id.
 */
 
void menu_enable_button(struct menu *menu,int id,int enable) {
  if (!menu||!id) return;
  struct button **p=menu->buttonv;
  int i=menu->buttonc;
  for (;i-->0;p++) {
    struct button *button=*p;
    if (button->id!=id) continue;
    button->enable=enable;
    return;
  }
}
