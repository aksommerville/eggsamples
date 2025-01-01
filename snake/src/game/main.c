#include "snake.h"

struct g g={0};

void egg_client_quit(int status) {
}

int egg_client_init() {
  int fbw=0,fbh=0;
  egg_texture_get_status(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) return -1;
  if (egg_texture_load_image(g.texid=egg_texture_new(),RID_image_tiles)<0) return -1;
  srand_auto();
  snake_init();
  return 0;
}

void egg_client_update(double elapsed) {
  int input=egg_input_get_one(0);
  if (input!=g.pvinput) {
    if ((input&EGG_BTN_AUX3)&&!(g.pvinput&EGG_BTN_AUX3)) egg_terminate(0);
    
    /* Preemptive Turbo Mode: Tapping the dpad moves the snake immediately.
     * It also samples the dpad during scheduled moves.
     */
    if ((input&EGG_BTN_LEFT)&&!(g.pvinput&EGG_BTN_LEFT)) snake_move(-1,0);
    if ((input&EGG_BTN_RIGHT)&&!(g.pvinput&EGG_BTN_RIGHT)) snake_move(1,0);
    if ((input&EGG_BTN_UP)&&!(g.pvinput&EGG_BTN_UP)) snake_move(0,-1);
    if ((input&EGG_BTN_DOWN)&&!(g.pvinput&EGG_BTN_DOWN)) snake_move(0,1);
    
    if ((input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
      if (!g.running) {
        snake_reset();
      }
    }
    g.pvinput=input;
  }
  snake_update(elapsed);
}

// (dstx,dsty) is the center of the first glyph. Glyphs are NS_sys_tilesize square.
static void draw_string(int16_t dstx,int16_t dsty,const char *src,int srcc) {
  if (!src) return;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  struct egg_draw_tile vtxv[32];
  int vtxc=0;
  for (;srcc-->0;src++,dstx+=NS_sys_tilesize) {
    // We only do rows 2..5 of ASCII. (6 and 7 are mostly lowercase roman, render the letters uppercase).
    int ch=*src;
    if ((ch>='a')&&(ch<='z')) ch-=0x20;
    if ((ch<=0x20)||(ch>0x5f)) continue;
    if (vtxc>=sizeof(vtxv)/sizeof(vtxv[0])) {
      egg_draw_tile(1,g.texid,vtxv,vtxc);
      vtxc=0;
    }
    vtxv[vtxc++]=(struct egg_draw_tile){
      .dstx=dstx,
      .dsty=dsty,
      .tileid=ch,
      .xform=0,
    };
  }
  if (vtxc) {
    egg_draw_tile(1,g.texid,vtxv,vtxc);
  }
}

static void draw_int(int16_t dstx,int16_t dsty,int v) {
  if (v<0) v=0;
  char tmp[16];
  int tmpc=0;
  if (v>=1000000000) tmp[tmpc++]='0'+(v/1000000000)%10;
  if (v>=100000000 ) tmp[tmpc++]='0'+(v/100000000 )%10;
  if (v>=10000000  ) tmp[tmpc++]='0'+(v/10000000  )%10;
  if (v>=1000000   ) tmp[tmpc++]='0'+(v/1000000   )%10;
  if (v>=100000    ) tmp[tmpc++]='0'+(v/100000    )%10;
  if (v>=10000     ) tmp[tmpc++]='0'+(v/10000     )%10;
  if (v>=1000      ) tmp[tmpc++]='0'+(v/1000      )%10;
  if (v>=100       ) tmp[tmpc++]='0'+(v/100       )%10;
  if (v>=10        ) tmp[tmpc++]='0'+(v/10        )%10;
  tmp[tmpc++]='0'+v%10;
  draw_string(dstx,dsty,tmp,tmpc);
}

// Draw the constant background, a grid of tile zero. (dstx,dsty) is the center of the top-left cell.
static void draw_background(int16_t dstx,int16_t dsty) {
  struct egg_draw_tile vtxv[COLC*ROWC];
  struct egg_draw_tile *vtx=vtxv;
  int16_t dstx0=dstx;
  int yi=ROWC; for (;yi-->0;dsty+=NS_sys_tilesize) {
    int xi=COLC,dstx=dstx0; for (;xi-->0;dstx+=NS_sys_tilesize,vtx++) {
      vtx->dstx=dstx;
      vtx->dsty=dsty;
      vtx->tileid=0;
      vtx->xform=0;
    }
  }
  egg_draw_tile(1,g.texid,vtxv,COLC*ROWC);
}

static void draw_snake(int16_t dstx,int16_t dsty) {
  struct egg_draw_tile vtxv[COLC*ROWC];
  struct egg_draw_tile *vtx=vtxv;
  const struct snegment *snegment=g.snegmentv;
  int i=g.snegmentc;
  for (;i-->0;vtx++,snegment++) {
    vtx->dstx=dstx+snegment->x*NS_sys_tilesize;
    vtx->dsty=dsty+snegment->y*NS_sys_tilesize;
    vtx->tileid=snegment->tileid;
    vtx->xform=snegment->xform;
  }
  egg_draw_tile(1,g.texid,vtxv,g.snegmentc);
}

static void draw_snak(int16_t dstx,int16_t dsty) {
  if ((g.snakx>=COLC)||(g.snaky>=ROWC)) return;
  struct egg_draw_tile vtx={
    .dstx=dstx+g.snakx*NS_sys_tilesize,
    .dsty=dsty+g.snaky*NS_sys_tilesize,
    .tileid=TILE_SNAK,
    .xform=0,
  };
  egg_draw_tile(1,g.texid,&vtx,1);
}

void egg_client_render() {
  egg_draw_clear(1,0x102030ff);
  
  /* Game. Draw even when not running.
   */
  int fldw=COLC*NS_sys_tilesize;
  int fldh=ROWC*NS_sys_tilesize;
  int fldx=(FBW>>1)-(fldw>>1);
  int fldy=FBH-fldx-fldh; // Repeat the left margin at the bottom, leaves room at the top for messages.
  int fldxt=fldx+(NS_sys_tilesize>>1);
  int fldyt=fldy+(NS_sys_tilesize>>1);
  draw_background(fldxt,fldyt);
  draw_snake(fldxt,fldyt);
  draw_snak(fldxt,fldyt);
  
  /* Time and score.
   * Show during and after play, but not during Hello.
   * (g.snegmentc) is only zero before the first game start.
   */
  if (g.snegmentc) {
    draw_string(20,fldy>>1,"T:",2);
    draw_int(20+NS_sys_tilesize*2,fldy>>1,(int)g.clock);
    draw_string(FBW-20-NS_sys_tilesize*4,fldy>>1,"S:",2);
    draw_int(FBW-20-NS_sys_tilesize*2,fldy>>1,g.snegmentc);
  }
  
  /* Hello and Game Over details.
   */
  if (!g.running) {
    struct egg_draw_rect rect={fldx,fldy,fldw,fldh,0x00,0x00,0x00,0xa0};
    egg_draw_rect(1,&rect,1);
    if (g.snegmentc) { // Game Over.
    } else { // Hello.
      int16_t srcx=0;
      int16_t srcy=NS_sys_tilesize*6;
      int16_t srcw=NS_sys_tilesize*9;
      int16_t srch=NS_sys_tilesize*3;
      int16_t dstx=(FBW>>1)-(srcw>>1);
      int16_t dsty=fldy+(fldh>>1)-(srch>>1);
      struct egg_draw_decal decal={dstx,dsty,srcx,srcy,srcw,srch};
      egg_draw_decal(1,g.texid,&decal,1);
    }
  }
}

/* We're building without libc, but I guess clang inserts calls to memset somewhere, and then doesn't insert an implementation.
 */
typedef int size_t;
void *memset(void *v,int ch,size_t c) {
  uint8_t *V=v;
  for (;c-->0;V++) *V=ch;
  return v;
}
