#include "tetris.h"

static int textid=0;//XXX
static int textw=0,texth=0;

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

//XXX testing
static void draw_field(int16_t dstx,int16_t dsty) {
  int16_t dstx0=dstx+(NS_sys_tilesize>>1);
  dsty+=NS_sys_tilesize>>1;
  uint8_t tileid=0;
  int row=0; for (;row<FIELDH;row++,dsty+=NS_sys_tilesize) {
    int col=0; for (dstx=dstx0;col<FIELDW;col++,dstx+=NS_sys_tilesize) {
      if ((row>4)||((row==4)&&(col&1))) {
        graf_draw_tile(&g.graf,g.texid_tiles,dstx,dsty,tileid,0);
      }
      if (++tileid>=7) tileid=0;
    }
  }
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
  
  if (!textid) {//XXX
    textid=font_tex_multiline(g.font,"Score: 0123456789\nHigh: 9876543210",-1,11*NS_sys_tilesize,4*NS_sys_tilesize,0xffffffff);
    egg_texture_get_status(&textw,&texth,textid);
  }
  
  /* Common elements.
   */
  draw_box(&r.score);
  
  /* Left-side elements always.
   */
  draw_box(&r.fieldl);
  draw_field(r.fieldl.x,r.fieldl.y);//TODO
  { // TODO
    int16_t dstx=r.score.x+(r.score.w>>1)-(textw>>1);
    int16_t dsty=r.score.y+(r.score.h>>1)-(texth>>1);
    graf_draw_decal(&g.graf,textid,dstx,dsty,0,0,textw,texth,0);
  }
  draw_box(&r.nextl);
  graf_draw_tile_buffer(&g.graf,g.texid_tiles,r.nextl.x+NS_sys_tilesize,r.nextl.y+NS_sys_tilesize,
    (const uint8_t*)//TODO
    "\xff\xff\xff\xff"
    "\xff\x01\xff\xff"
    "\xff\x01\xff\xff"
    "\xff\x01\x01\xff",
    4,4,4
  );
  
  /* Right-side elements if requested.
   */
  if (r.fieldc==2) {
    draw_box(&r.fieldr);
    draw_field(r.fieldr.x,r.fieldr.y);//TODO
    { // TODO
      int16_t dstx=r.score.x+(r.score.w>>1)-(textw>>1);
      int16_t dsty=r.score.y+(r.score.h>>1)-(texth>>1);
      graf_draw_decal(&g.graf,textid,dstx,dsty,0,0,textw,texth,0);
    }
    draw_box(&r.nextr);
    graf_draw_tile_buffer(&g.graf,g.texid_tiles,r.nextr.x+NS_sys_tilesize,r.nextr.y+NS_sys_tilesize,
      (const uint8_t*)//TODO
      "\xff\xff\xff\xff"
      "\xff\x03\x03\xff"
      "\xff\x03\x03\xff"
      "\xff\xff\xff\xff",
      4,4,4
    );
  }
}
