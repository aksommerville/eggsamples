#include "tetris.h"

/* Private renderer state.
 */
 
struct rect { int16_t x,y,w,h; };
 
static struct {
  int fieldc; // If !=g.fieldc, we need to relayout.
  
  // Layout. 'l' fields are left or only. 'r' fields only when fieldc==2.
  int16_t lineh;
  struct rect score;
  struct rect fieldl,fieldr;
  struct rect nextl,nextr;
  struct rect hiscores;
  struct rect unused;
  
  int dbmodseq;
  int hstexid,hsw,hsh;
} r={0};

/* Generic box.
 * (x,y,w,h) is the client space. We draw a border outside it.
 * (w,h) should be at least one tile, but don't need to be multiples of tile size.
 * We black out the interior.
 */

static void draw_box_loose(int16_t x,int16_t y,int16_t w,int16_t h) {
  graf_draw_rect(&g.graf,x,y,w,h,0x000000ff);
  if ((w<NS_sys_tilesize)||(h<NS_sys_tilesize)) return; // Don't attempt the border if either axis smaller than a tile.
  
  // Corner positions. They are not necessarily a clean tile width apart, but always at least 2 tile widths.
  int xa=x-(NS_sys_tilesize>>1);
  int ya=y-(NS_sys_tilesize>>1);
  int xz=x+w+(NS_sys_tilesize>>1);
  int yz=y+h+(NS_sys_tilesize>>1);
  
  // Draw the corners, and one leg extending leftward from the 'z' sides, to cover non-tilesize slack.
  // The edge tiles are safe to render over each other misaligned (ie no alpha, and no off-axis features).
  graf_draw_tile(&g.graf,g.texid_tiles,xa,ya,0x20,0);
  graf_draw_tile(&g.graf,g.texid_tiles,xz,ya,0x22,0);
  graf_draw_tile(&g.graf,g.texid_tiles,xa,yz,0x40,0);
  graf_draw_tile(&g.graf,g.texid_tiles,xz,yz,0x42,0);
  if (w%NS_sys_tilesize) {
    graf_draw_tile(&g.graf,g.texid_tiles,xz-NS_sys_tilesize,ya,0x21,0);
    graf_draw_tile(&g.graf,g.texid_tiles,xz-NS_sys_tilesize,yz,0x41,0);
  }
  if (h%NS_sys_tilesize) {
    graf_draw_tile(&g.graf,g.texid_tiles,xa,yz-NS_sys_tilesize,0x30,0);
    graf_draw_tile(&g.graf,g.texid_tiles,xz,yz-NS_sys_tilesize,0x32,0);
  }
  
  // Edges.
  int i=w/NS_sys_tilesize;
  int16_t p=x+(NS_sys_tilesize>>1);
  for (;i-->0;p+=NS_sys_tilesize) {
    graf_draw_tile(&g.graf,g.texid_tiles,p,ya,0x21,0);
    graf_draw_tile(&g.graf,g.texid_tiles,p,yz,0x41,0);
  }
  for (i=h/NS_sys_tilesize,p=y+(NS_sys_tilesize>>1);i-->0;p+=NS_sys_tilesize) {
    graf_draw_tile(&g.graf,g.texid_tiles,xa,p,0x30,0);
    graf_draw_tile(&g.graf,g.texid_tiles,xz,p,0x32,0);
  }
}

static void draw_box(const struct rect *r) {
  draw_box_loose(r->x,r->y,r->w,r->h);
}

/* "Unused" section, a corny joke because I don't know what else to do with the giant gap in the one-field layout.
 */
 
static void draw_unused(const struct rect *r) {
  if (r->w<NS_sys_tilesize*4) return;
  if (r->h<NS_sys_tilesize*2) return;
  int16_t w=NS_sys_tilesize*3;
  int16_t h=NS_sys_tilesize*1;
  int16_t srcx=NS_sys_tilesize*3;
  int16_t srcy=NS_sys_tilesize*2;
  int16_t dstx=r->x+(r->w>>1)-(w>>1);
  int16_t dsty=r->y+(r->h>>1)-(h>>1);
  graf_draw_decal(&g.graf,g.texid_tiles,dstx,dsty,srcx,srcy,w,h,0);
}

/* Fetch hiscores and write into an RGBA buffer.
 */

// If (dsta) short, we produce incorrect text, but never return >dsta.
static int decuint_repr(char *dst,int dsta,int v) {
  if (dsta<1) return 0;
  if (v<0) v=0;
  int dstc=0;
  if ((v>=1000000000)&&(dstc<dsta)) dst[dstc++]='0'+(v/1000000000)%10;
  if ((v>=100000000 )&&(dstc<dsta)) dst[dstc++]='0'+(v/100000000 )%10;
  if ((v>=10000000  )&&(dstc<dsta)) dst[dstc++]='0'+(v/10000000  )%10;
  if ((v>=1000000   )&&(dstc<dsta)) dst[dstc++]='0'+(v/1000000   )%10;
  if ((v>=100000    )&&(dstc<dsta)) dst[dstc++]='0'+(v/100000    )%10;
  if ((v>=10000     )&&(dstc<dsta)) dst[dstc++]='0'+(v/10000     )%10;
  if ((v>=1000      )&&(dstc<dsta)) dst[dstc++]='0'+(v/1000      )%10;
  if ((v>=100       )&&(dstc<dsta)) dst[dstc++]='0'+(v/100       )%10;
  if ((v>=10        )&&(dstc<dsta)) dst[dstc++]='0'+(v/10        )%10;
  if (dstc<dsta) dst[dstc++]='0'+v%10;
  return dstc;
}
 
static void render_generate_hiscores_rgba(void *dst,int dstw,int dsth) {
  int lineh=font_get_line_height(g.font);
  struct db_score scorev[20];
  int scorec=db_get_scores(scorev,20);
  int16_t dsty=0;
  const struct db_score *score=scorev;
  int i=scorec;
  for (;i-->0;score++) {
    if (dsty+lineh>dsth) break;
    // Let's try three columns per record: playerc, score, linec. (year,month,day,hour,minute) are available if we want them.
    char pcv[2],lcv[5],sv[7];
    int pcc=0,lcc=0,sc=0;
    if ((score->playerc<1)||(score->playerc>8)) continue;
    pcv[0]='0'+score->playerc;
    pcv[1]='p';
    pcc=2;
    lcc=decuint_repr(lcv,sizeof(lcv),score->linec);
    sc=decuint_repr(sv,sizeof(sv),score->score);
    int lw=font_measure_line(g.font,lcv,lcc);
    int sw=font_measure_line(g.font,sv,sc);
    font_render_string(dst,dstw,dsth,dstw<<2,      0,dsty,g.font,pcv,pcc,0xffffffff);
    font_render_string(dst,dstw,dsth,dstw<<2,  60-lw,dsty,g.font,lcv,lcc,0xffffffff);
    font_render_string(dst,dstw,dsth,dstw<<2,dstw-sw,dsty,g.font,sv,sc,0xffffffff);
    dsty+=lineh;
  }
}

/* Reacquire hiscores texture.
 */
 
static void render_acquire_hiscores() {
  r.hsw=r.hsh=0;
  if ((r.hiscores.w<1)||(r.hiscores.h<1)) return;

  if (!r.hstexid) {
    if ((r.hstexid=egg_texture_new())<1) return;
  }
  
  char *rgba=calloc(r.hiscores.w*r.hiscores.h,4);
  if (!rgba) return;
  render_generate_hiscores_rgba(rgba,r.hiscores.w,r.hiscores.h);
  if (egg_texture_load_raw(r.hstexid,EGG_TEX_FMT_RGBA,r.hiscores.w,r.hiscores.h,r.hiscores.w<<2,rgba,r.hiscores.w*r.hiscores.h*4)>=0) {
    r.hsw=r.hiscores.w;
    r.hsh=r.hiscores.h;
  }
  free(rgba);
}

/* High scores.
 */
 
static void draw_hiscores(const struct rect *bounds) {

  // Rebuild the texture if needed.
  int modseq=db_modseq();
  if (modseq!=r.dbmodseq) {
    render_acquire_hiscores();
    r.dbmodseq=modseq;
  }
  
  // Sometimes we have nothing, no worries.
  if ((r.hsw<1)||(r.hsh<1)) return;
  
  // Anchor to top-left of bounds and clip.
  // I don't think bounds will ever change, so it should be exactly right every time, but let's be safe.
  int cpw=r.hsw; if (cpw>bounds->w) cpw=bounds->w;
  int cph=r.hsh; if (cph>bounds->h) cph=bounds->h;
  graf_draw_decal(&g.graf,r.hstexid,bounds->x,bounds->y,0,0,cpw,cph,0);
}

/* Recalculate layout.
 */
 
static void render_layout_single() {
  r.fieldl.x=(FBW>>1)-(r.fieldl.w>>1);
  r.fieldl.y=(FBH>>1)-(r.fieldl.h>>1);
    
  r.score.x=r.fieldl.x+r.fieldl.w+NS_sys_tilesize;
  r.score.y=r.fieldl.y;
  r.score.w=NS_sys_tilesize*11;
  r.score.h=r.lineh*2+4;
    
  r.nextl.x=r.fieldl.x+r.fieldl.w+NS_sys_tilesize;
  r.nextl.y=r.score.y+r.score.h+NS_sys_tilesize;
  r.nextl.w=NS_sys_tilesize*5;
  r.nextl.h=NS_sys_tilesize*5;
  
  r.hiscores.x=r.score.x;
  r.hiscores.y=r.nextl.y+r.nextl.h+NS_sys_tilesize;
  r.hiscores.w=r.score.w;
  r.hiscores.h=r.fieldl.y+r.fieldl.h-r.hiscores.y;
  
  r.unused.x=0;
  r.unused.y=0;
  r.unused.w=r.fieldl.x;
  r.unused.h=FBH;
}

static void render_layout_double() {
  int16_t xmargin=NS_sys_tilesize*3;
  r.fieldl.x=xmargin;
  r.fieldr.x=FBW-xmargin-r.fieldr.w;
  r.fieldl.y=(FBH>>1)-(r.fieldl.h>>1);
  r.fieldr.y=(FBH>>1)-(r.fieldr.h>>1);
    
  r.score.w=NS_sys_tilesize*11;
  r.score.h=r.lineh*2+5;
  r.score.x=(FBW>>1)-(r.score.w>>1);
  r.score.y=r.fieldl.y;
    
  r.nextl.w=r.nextr.w=NS_sys_tilesize*5;
  r.nextl.h=r.nextr.h=NS_sys_tilesize*5;
  r.nextl.x=r.score.x;
  r.nextr.x=r.score.x+r.score.w-r.nextr.w;
  r.nextl.y=r.nextr.y=r.score.y+r.score.h+NS_sys_tilesize;
  
  r.hiscores.x=r.score.x;
  r.hiscores.y=r.nextl.y+r.nextl.h+NS_sys_tilesize;
  r.hiscores.w=r.score.w;
  r.hiscores.h=r.fieldl.y+r.fieldl.h-r.hiscores.y;
  
  r.unused.w=0;
  r.unused.h=0;
}
 
static void render_layout() {
  
  r.fieldl.w=r.fieldr.w=FIELDW*NS_sys_tilesize;
  r.fieldl.h=r.fieldr.h=FIELDH*NS_sys_tilesize;
  r.lineh=font_get_line_height(g.font);
  
  switch (r.fieldc) {
    case 1: render_layout_single(); break;
    case 2: render_layout_double(); break;
  }
}

/* Main entry point.
 */
 
void render() {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x323c78ff);
  
  if (g.fieldc!=r.fieldc) {
    r.fieldc=g.fieldc;
    render_layout();
  }
  
  /* Common elements and left side boxes always.
   * Right side boxes if enabled.
   */
  draw_box(&r.score);
  draw_box(&r.fieldl);
  draw_box(&r.nextl);
  draw_box(&r.hiscores);
  if (r.fieldc==2) {
    draw_box(&r.fieldr);
    draw_box(&r.nextr);
  }
  
  /* If running, draw the games.
   */
  if (g.running) {
    field_render(&g.l,r.fieldl.x,r.fieldl.y);
    field_render_next(&g.l,r.nextl.x,r.nextl.y,r.nextl.w,r.nextl.h);
    if (g.fieldc==2) {
      field_render_combined_score(&g.l,&g.r,r.score.x,r.score.y,r.score.w,r.score.h);
      field_render(&g.r,r.fieldr.x,r.fieldr.y);
      field_render_next(&g.r,r.nextr.x,r.nextr.y,r.nextr.w,r.nextr.h);
    } else {
      field_render_single_score(&g.l,r.score.x,r.score.y,r.score.w,r.score.h);
    }
  }
  
  draw_hiscores(&r.hiscores);
  draw_unused(&r.unused);
}
