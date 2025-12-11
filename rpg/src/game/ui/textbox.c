#include "game/rpg.h"
#include "textbox.h"

#define BORDERW 2
#define TEXTBOX_SIZE_MIN NS_sys_tilesize
#define TEXTBOX_CELLS_MAX 32

/* Delete.
 */
 
static void tb_option_cleanup(struct tb_option *option) {
  if (option->text) free(option->text);
  if (option->texid) egg_texture_del(option->texid);
}
 
void textbox_del(struct textbox *textbox) {
  if (!textbox) return;
  if (textbox->optionv) {
    while (textbox->optionc-->0) tb_option_cleanup(textbox->optionv+textbox->optionc);
    free(textbox->optionv);
  }
  free(textbox);
}

/* New.
 */

struct textbox *textbox_new(
  int dstx,int dsty,int dstw,int dsth,
  int colc,int rowc
) {
  if (dstw<TEXTBOX_SIZE_MIN) dstw=TEXTBOX_SIZE_MIN;
  if (dsth<TEXTBOX_SIZE_MIN) dsth=TEXTBOX_SIZE_MIN;
  if ((colc<0)||(colc>TEXTBOX_CELLS_MAX)) return 0;
  if ((rowc<0)||(rowc>TEXTBOX_CELLS_MAX)) return 0;
  
  struct textbox *textbox=calloc(1,sizeof(struct textbox));
  if (!textbox) return 0;
  
  textbox->dstx=dstx;
  textbox->dsty=dsty;
  textbox->dstw=dstw;
  textbox->dsth=dsth;
  
  if (colc||rowc) {
    textbox->colc=colc;
    textbox->rowc=rowc;
  }
  
  return textbox;
}

/* Set option.
 */
 
int textbox_set_option(struct textbox *textbox,int col,int row,const char *text,int textc,uint8_t align,int enable) {
  if (!textbox) return -1;
  if ((col<0)||(col>=textbox->colc)) return -1;
  if ((row<0)||(row>=textbox->rowc)) return -1;
  if (!text) textc=0; else if (textc<0) { textc=0; while (text[textc]) textc++; }
  
  int p=row*textbox->colc+col;
  if (p>=textbox->optiona) {
    int na=(p+16)&~15;
    if (na>INT_MAX/sizeof(struct tb_option)) return -1;
    void *nv=realloc(textbox->optionv,sizeof(struct tb_option)*na);
    if (!nv) return -1;
    textbox->optionv=nv;
    textbox->optiona=na;
  }
  if (p>=textbox->optionc) {
    memset(textbox->optionv+textbox->optionc,0,sizeof(struct tb_option)*(p-textbox->optionc+1));
    textbox->optionc=p+1;
  }
  struct tb_option *option=textbox->optionv+p;
  
  char *ntext=0;
  if (textc) {
    if (!(ntext=malloc(textc))) return -1;
    memcpy(ntext,text,textc);
  }
  if (option->text) free(option->text);
  option->text=ntext;
  if (option->texid) egg_texture_del(option->texid);
  if (option->textc=textc) {
    option->texid=font_render_to_texture(0,g.font,text,textc,FBW,font_get_line_height(g.font),0xffffffff);
    egg_texture_get_size(&option->texw,&option->texh,option->texid);
  } else {
    option->texid=0;
    option->texw=option->texh=0;
  }
  
  option->align=align;
  option->enable=enable;
  return 0;
}

/* Set option from strings resource.
 */
 
int textbox_set_option_string(struct textbox *textbox,int col,int row,int rid,int ix,uint8_t align,int enable) {
  const char *src=0;
  int srcc=strings_get(&src,rid,ix);
  if (srcc<0) srcc=0;
  return textbox_set_option(textbox,col,row,src,srcc,align,enable);
}

/* Remove all options.
 */
 
void textbox_clear(struct textbox *textbox) {
  while (textbox->optionc>0) {
    textbox->optionc--;
    tb_option_cleanup(textbox->optionv+textbox->optionc);
  }
}

/* Comments.
 */
 
void textbox_set_comment(struct textbox *textbox,int col,int row,int comment) {
  if (!textbox||(col<0)||(row<0)) return;
  int p=row*textbox->colc+col;
  if (p>=textbox->optionc) return;
  textbox->optionv[p].comment=comment;
}

int textbox_get_comment(const struct textbox *textbox,int col,int row) {
  if (!textbox||(col<0)||(row<0)) return 0;
  int p=row*textbox->colc+col;
  if (p>=textbox->optionc) return 0;
  return textbox->optionv[p].comment;
}

int textbox_find_option_by_comment(int *col,int *row,const struct textbox *textbox,int comment) {
  if (!textbox) return -1;
  int p=0;
  struct tb_option *option=textbox->optionv;
  for (;p<textbox->optionc;p++,option++) {
    if (option->comment!=comment) continue;
    if (col) *col=p%textbox->colc;
    if (row) *row=p/textbox->colc;
    return p;
  }
  return -1;
}

/* Pack.
 */
 
void textbox_pack(struct textbox *textbox) {
  if (!textbox) return;
  textbox->colw=1;
  textbox->rowh=1;
  struct tb_option *option=textbox->optionv;
  int i=textbox->optionc;
  for (;i-->0;option++) {
    if (option->texw>textbox->colw) textbox->colw=option->texw;
    if (option->texh>textbox->rowh) textbox->rowh=option->texh;
  }
  textbox->colw+=TEXTBOX_CURSOR_W;
  if (textbox->colc==1) textbox->colw=textbox->dstw-BORDERW*2-4;
  if (textbox->rowc==1) textbox->rowh=textbox->dsth-BORDERW*2-4;
}

/* If the current selection is OOB or disabled, put it somewhere valid.
 */
 
static void textbox_require_selection(struct textbox *textbox) {
  if ((textbox->selp>=0)&&(textbox->selp<textbox->optionc)&&textbox->optionv[textbox->selp].enable) return;
  const struct tb_option *option=textbox->optionv;
  int i=0;
  for (;i<textbox->optionc;i++,option++) {
    if (option->enable) {
      textbox->selp=i;
      return;
    }
  }
}

/* Focus.
 */
 
void textbox_focus(struct textbox *textbox,int focus) {
  if (focus) {
    textbox->focus=1;
    const struct tb_option *option=textbox->optionv;
    int i=textbox->optionc;
    for (;i-->0;option++) {
      if (option->enable) {
        textbox->focus=2;
        textbox_require_selection(textbox);
        return;
      }
    }
  } else {
    textbox->focus=0;
  }
}

/* Move cursor.
 */
 
void textbox_move(struct textbox *textbox,int dx,int dy) {
  if (!textbox) return;
  if (textbox->focus!=2) return;
  if (textbox->selp<0) textbox->selp=0;
  int col=textbox->selp%textbox->colc+dx;
  int row=textbox->selp/textbox->colc+dy;
  if (col<0) col=textbox->colc-1; else if (col>=textbox->colc) col=0;
  if (row<0) row=textbox->rowc-1; else if (row>=textbox->rowc) row=0;
  textbox->selp=row*textbox->colc+col;
}

/* Get cursor.
 */
 
void textbox_get_selection(int *x,int *y,const struct textbox *textbox) {
  if (!textbox) return;
  if (textbox->selp<0) {
    if (x) *x=-1;
    if (y) *y=-1;
    return;
  }
  if (x) *x=textbox->selp%textbox->colc;
  if (y) *y=textbox->selp/textbox->colc;
}
