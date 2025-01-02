#include "sweep.h"

struct g g={0};

void egg_client_quit(int status) {
}

int egg_client_init() {
  
  int fbw=0,fbh=0;
  egg_texture_get_status(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) {
    fprintf(stderr,"Got %dx%d framebuffer (ie metadata) but expected %dx%d (ie sweep.h)\n",fbw,fbh,FBW,FBH);
    return -1;
  }
  
  if ((g.romc=egg_get_rom(0,0))<=0) return -1;
  if (!(g.rom=malloc(g.romc))) return -1;
  if (egg_get_rom(g.rom,g.romc)!=g.romc) return -1;
  strings_set_rom(g.rom,g.romc);
  
  if (!(g.font=font_new())) return -1;
  if (font_add_image_resource(g.font,0x0020,RID_image_font9_0020)<0) return -1;
  
  srand_auto();
  
  sweep_reset();
  
  return 0;
}

void egg_client_update(double elapsed) {
  int input=egg_input_get_one(0);
  if (input!=g.pvinput) {
    if ((input&EGG_BTN_AUX3)&&!(g.pvinput&EGG_BTN_AUX3)) egg_terminate(0);
    if (g.running) {
      if ((input&EGG_BTN_LEFT)&&!(g.pvinput&EGG_BTN_LEFT)) sweep_move(-1,0);
      if ((input&EGG_BTN_RIGHT)&&!(g.pvinput&EGG_BTN_RIGHT)) sweep_move(1,0);
      if ((input&EGG_BTN_UP)&&!(g.pvinput&EGG_BTN_UP)) sweep_move(0,-1);
      if ((input&EGG_BTN_DOWN)&&!(g.pvinput&EGG_BTN_DOWN)) sweep_move(0,1);
      if ((input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) sweep_expose();
      if ((input&EGG_BTN_WEST)&&!(g.pvinput&EGG_BTN_WEST)) sweep_flag();
    } else {
      if ((input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) sweep_reset();
    }
    g.pvinput=input;
  }
  if ((g.cursorclock-=elapsed)<=0.0) {
    g.cursorclock+=0.200;
    if (++(g.cursorframe)>=4) g.cursorframe=0;
  }
}

void egg_client_render() {
  const int status_bar_height=8;
  int texid=texcache_get_image(&g.texcache,RID_image_tiles);
  graf_reset(&g.graf);
  
  /* Status bar: A row of eggs or flags at the top, telling you how much outstanding.
   */
  graf_draw_rect(&g.graf,0,0,FBW,status_bar_height,0x000000ff);
  uint8_t tileid=0x10;
  int iconc=EGGC-g.flagc;
  if (iconc<0) {
    tileid=0x11;
    iconc=-iconc;
  }
  int16_t dstx=4,dsty=status_bar_height>>1;
  for (;iconc-->0;dstx+=8) {
    graf_draw_tile(&g.graf,texid,dstx,dsty,tileid,0);
  }
  
  /* The main event.
   */
  graf_draw_tile_buffer(&g.graf,texid,NS_sys_tilesize>>1,status_bar_height+(NS_sys_tilesize>>1),g.map,COLC,ROWC,COLC);
  
  /* Animated cursor.
   */
  if (g.running) {
    uint8_t cursorxform=0;
    switch (g.cursorframe) {
      case 0: cursorxform=0; break;
      case 1: cursorxform=EGG_XFORM_SWAP|EGG_XFORM_XREV; break;
      case 2: cursorxform=EGG_XFORM_XREV|EGG_XFORM_YREV; break;
      case 3: cursorxform=EGG_XFORM_YREV|EGG_XFORM_SWAP; break;
    }
    graf_draw_tile(&g.graf,texid,g.selx*NS_sys_tilesize+(NS_sys_tilesize>>1),status_bar_height+g.sely*NS_sys_tilesize+(NS_sys_tilesize>>1),TILE_CURSOR,cursorxform);
  }
  
  /* Final message.
   */
  if (!g.running) {
    int16_t srcx,srcy,srcw,srch;
    if (g.victory) {
      srcx=0;
      srcy=NS_sys_tilesize*2;
      srcw=NS_sys_tilesize*7;
      srch=NS_sys_tilesize*3;
    } else {
      srcx=NS_sys_tilesize*7;
      srcy=NS_sys_tilesize*2;
      srcw=NS_sys_tilesize*3;
      srch=NS_sys_tilesize*3;
    }
    int16_t dstx=(FBW>>1)-(srcw>>1);
    int16_t dsty=(FBH>>1)-(srch>>1);
    graf_draw_decal(&g.graf,texid,dstx,dsty,srcx,srcy,srcw,srch,0);
  }
  
  graf_flush(&g.graf);
}
