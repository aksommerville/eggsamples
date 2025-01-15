#include "game/rpg.h"
#include "textbox.h"

#define BORDERW 2
#define BORDERCOLOR 0x366cb4ff /* Match image/3-uitiles.png, tiles 0x10 and 0x11 */

/* Border.
 */
 
void textbox_render_backdrop(int16_t x,int16_t y,int16_t w,int16_t h) {
  if ((w<BORDERW*2)||(h<BORDERW*2)) {
    graf_draw_rect(&g.graf,x,y,w,h,0x000000ff);
  } else {
    graf_draw_rect(&g.graf,x+BORDERW,y+BORDERW,w-BORDERW*2,h-BORDERW*2,0x000000ff);
  }
}

void textbox_render_border(int16_t x,int16_t y,int16_t w,int16_t h) {

  /* Below 1, don't draw anything, and below the tilesize, draw a rectangle.
   * The corner border tiles have legs, so their extent is (NS_sys_tilesize>>1)+(BORDERW>>1).
   * The legs may overlap.
   */
  if ((w<1)||(h<1)) return;
  if ((w<NS_sys_tilesize)||(h<NS_sys_tilesize)) {
    graf_draw_rect(&g.graf,x,y,w,1,BORDERCOLOR);
    graf_draw_rect(&g.graf,x,y,1,h,BORDERCOLOR);
    graf_draw_rect(&g.graf,x+w-1,y,1,h,BORDERCOLOR);
    graf_draw_rect(&g.graf,x,y+h-1,w,1,BORDERCOLOR);
    return;
  }
  int texid=texcache_get_image(&g.texcache,RID_image_uitiles);
  
  /* An edge tile at the tail end of each edge, if they're more than a tile apart.
   * This one gets partially covered by the broad edge rendering below.
   * One tile or smaller, the corners cover it.
   * If we're two tiles or smaller, draw this filler tile in the middle, and it's all we need.
   */
  if (w<=NS_sys_tilesize+BORDERW) {
  } else if (w<=NS_sys_tilesize*2+BORDERW) {
    graf_draw_tile(&g.graf,texid,x+(w>>1),y+(BORDERW>>1),0x11,0);
    graf_draw_tile(&g.graf,texid,x+(w>>1),y+h-(BORDERW>>1),0x11,EGG_XFORM_YREV);
  } else {
    graf_draw_tile(&g.graf,texid,x+w-NS_sys_tilesize,y+(BORDERW>>1),0x11,0);
    graf_draw_tile(&g.graf,texid,x+w-NS_sys_tilesize,y+h-(BORDERW>>1),0x11,EGG_XFORM_YREV);
  }
  if (h<=NS_sys_tilesize+BORDERW) {
  } else if (h<=NS_sys_tilesize*2+BORDERW) {
    graf_draw_tile(&g.graf,texid,x+w-(BORDERW>>1),y+(h>>1),0x11,EGG_XFORM_SWAP|EGG_XFORM_YREV);
    graf_draw_tile(&g.graf,texid,x+(BORDERW>>1),y+(h>>1),0x11,EGG_XFORM_SWAP);
  } else {
    graf_draw_tile(&g.graf,texid,x+w-(BORDERW>>1),y+h-NS_sys_tilesize,0x11,EGG_XFORM_SWAP|EGG_XFORM_YREV);
    graf_draw_tile(&g.graf,texid,x+(BORDERW>>1),y+h-NS_sys_tilesize,0x11,EGG_XFORM_SWAP);
  }
  
  /* A tile at each corner.
   */
  graf_draw_tile(&g.graf,texid,x+(BORDERW>>1),y+(BORDERW>>1),0x10,0);
  graf_draw_tile(&g.graf,texid,x+w-(BORDERW>>1),y+(BORDERW>>1),0x10,EGG_XFORM_SWAP|EGG_XFORM_YREV);
  graf_draw_tile(&g.graf,texid,x+w-(BORDERW>>1),y+h-(BORDERW>>1),0x10,EGG_XFORM_XREV|EGG_XFORM_YREV);
  graf_draw_tile(&g.graf,texid,x+(BORDERW>>1),y+h-(BORDERW>>1),0x10,EGG_XFORM_SWAP|EGG_XFORM_XREV);
  
  /* Multiple edge tiles along all four edges.
   */
  int16_t dst=x+NS_sys_tilesize;
  int i=(w-NS_sys_tilesize)/NS_sys_tilesize;
  for (;i-->0;dst+=NS_sys_tilesize) {
    graf_draw_tile(&g.graf,texid,dst,y+(BORDERW>>1),0x11,0);
    graf_draw_tile(&g.graf,texid,dst,y+h-(BORDERW>>1),0x11,EGG_XFORM_YREV);
  }
  for (dst=y+NS_sys_tilesize,i=(h-NS_sys_tilesize)/NS_sys_tilesize;i-->0;dst+=NS_sys_tilesize) {
    graf_draw_tile(&g.graf,texid,x+(BORDERW>>1),dst,0x11,EGG_XFORM_SWAP);
    graf_draw_tile(&g.graf,texid,x+w-(BORDERW>>1),dst,0x11,EGG_XFORM_SWAP|EGG_XFORM_YREV);
  }
}

/* Render, main entry point.
 */
 
void textbox_render(struct textbox *textbox) {
  textbox->clock++;
  textbox_render_backdrop(textbox->dstx,textbox->dsty,textbox->dstw,textbox->dsth);
  
  if (textbox->focus==2) {
    if (textbox->selp>=0) {
      const int blink_period=50;
      int col=textbox->selp%textbox->colc;
      int row=textbox->selp/textbox->colc;
      int16_t dstx=textbox->dstx+BORDERW+col*textbox->colw;
      int16_t dsty=textbox->dsty+BORDERW+row*textbox->rowh;
      int16_t dstw=textbox->colw+4;
      int16_t dsth=textbox->rowh+2;
      uint32_t color=((textbox->clock%blink_period)>=(blink_period>>1))?0x402080ff:0x301060ff;
      graf_draw_rect(&g.graf,dstx,dsty,dstw,dsth,color);
    }
  }
  
  struct tb_option *option=textbox->optionv;
  int i=textbox->optionc,row=0,col=0;
  int16_t celly=textbox->dsty+BORDERW+2;
  int16_t cellx=textbox->dstx+BORDERW+2;
  for (;i-->0;option++) {
    if (option->texid) {
      int16_t dstx=cellx,dsty=celly;
      switch (option->align&(DIR_W|DIR_E)) {
        case DIR_W: break;
        case DIR_E: dstx+=textbox->colw-option->texw; break;
        default: dstx+=(textbox->colw>>1)-(option->texw>>1);
      }
      switch (option->align&(DIR_N|DIR_S)) {
        case DIR_N: break;
        case DIR_S: dsty+=textbox->rowh-option->texh; break;
        default: dsty+=(textbox->rowh>>1)-(option->texh>>1);
      }
      int16_t srcx=0,srcy=0;
      int16_t cpw=option->texw;
      int16_t cph=option->texh;
      if (dstx<textbox->dstx) { cpw-=textbox->dstx-dstx; srcx+=textbox->dstx-dstx; dstx=textbox->dstx; }
      if (dsty<textbox->dsty) { cph-=textbox->dsty-dsty; srcy+=textbox->dsty-dsty; dsty=textbox->dsty; }
      if (dstx+cpw>textbox->dstx+textbox->dstw) cpw=textbox->dstx+textbox->dstw-dstx;
      if (dsty+cph>textbox->dsty+textbox->dsth) cph=textbox->dsty+textbox->dsth-dsty;
      if ((cpw<1)||(cph<1)) continue;
      graf_draw_decal(&g.graf,option->texid,dstx,dsty,srcx,srcy,cpw,cph,0);
    }
    cellx+=textbox->colw;
    if (++col>=textbox->colc) {
      col=0;
      row++;
      cellx=textbox->dstx+BORDERW+2;
      celly+=textbox->rowh;
    }
  }

  textbox_render_border(textbox->dstx,textbox->dsty,textbox->dstw,textbox->dsth);
  
  if (textbox->focus==1) {
    const int caret_period=50; // In video frames.
    uint8_t tileid=0x12+(textbox->clock%caret_period)/(caret_period>>1);
    int16_t dstx=textbox->dstx+textbox->dstw-10;
    int16_t dsty=textbox->dsty+textbox->dsth-1;
    graf_draw_tile(&g.graf,texcache_get_image(&g.texcache,RID_image_uitiles),dstx,dsty,tileid,0);
  }
}

/* Animated acknowledge-me or scroll-me carets.
 */
 
void textbox_render_ack(int16_t x,int16_t y,int16_t w,int16_t h) {
  uint8_t tileid=0x12;
  if (g.framec%50>=30) tileid++;
  int16_t dstx=x+w-10;
  int16_t dsty=y+h-1;
  graf_draw_tile(&g.graf,texcache_get_image(&g.texcache,RID_image_uitiles),dstx,dsty,tileid,0);
}

void textbox_render_more(int16_t x,int16_t y,int16_t w,int16_t h,uint8_t mask) {
  uint8_t tileid=0x12;
  if (g.framec%50>=30) tileid++;
  int texid=texcache_get_image(&g.texcache,RID_image_uitiles);
  if (mask&DIR_N) graf_draw_tile(&g.graf,texid,x+(w>>1),y+1,tileid,EGG_XFORM_YREV);
  if (mask&DIR_S) graf_draw_tile(&g.graf,texid,x+(w>>1),y+h-1,tileid,0);
  if (mask&DIR_W) graf_draw_tile(&g.graf,texid,x+1,y+(h>>1),tileid,EGG_XFORM_SWAP|EGG_XFORM_YREV);
  if (mask&DIR_E) graf_draw_tile(&g.graf,texid,x+w-1,y+(h>>1),tileid,EGG_XFORM_SWAP);
}
