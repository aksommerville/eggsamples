#include "tetris.h"

/* Render fixed cells.
 * Caller adjusts (dstx,dsty) to the center of the top-left cell.
 */
 
static void field_render_cellv(struct field *field,int16_t dstx,int16_t dsty) {
  int16_t dstx0=dstx;
  const uint8_t *v=field->cellv;
  int yi=FIELDH; for (;yi-->0;dsty+=NS_sys_tilesize) {
    int xi=FIELDW; for (dstx=dstx0;xi-->0;dstx+=NS_sys_tilesize,v++) {
      if (!*v) continue; // Zero means empty, transparent.
      graf_draw_tile(&g.graf,g.texid_tiles,dstx,dsty,*v,0);
    }
  }
}

/* Render one player's falling piece.
 * Caller adjusts (dstx,dsty) to the center of the top-left cell.
 */
 
static void field_render_player(struct field *field,const struct player *player,int16_t dstx,int16_t dsty) {
  struct tetr_tile tilev[4];
  int tilec=tetr_tile_shape(tilev,4,player->tetr,player->xform);
  if (tilec>4) return;
  const struct tetr_tile *tile=tilev;
  for (;tilec-->0;tile++) {
    int col=player->x+tile->x; if ((col<0)||(col>=FIELDW)) continue;
    int row=player->y+tile->y; if ((row<0)||(row>=FIELDH)) continue;
    graf_draw_tile(&g.graf,g.texid_tiles,dstx+col*NS_sys_tilesize,dsty+row*NS_sys_tilesize,0x01+player->tetr,0);
  }
}

/* Render main portion.
 */
 
void field_render(struct field *field,int16_t dstx,int16_t dsty) {
  dstx+=NS_sys_tilesize>>1;
  dsty+=NS_sys_tilesize>>1;
  field_render_cellv(field,dstx,dsty);
  const struct player *player=field->playerv;
  int i=field->playerc;
  for (;i-->0;player++) field_render_player(field,player,dstx,dsty);
}

/* Render upcoming tetromino.
 */
 
void field_render_next(struct field *field,int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth) {
  int tetr=bag_peek(field->readhead);
  
  // If we have at least 4x4 tiles, render the tetromino exactly as it appears in play.
  if ((dstw>=NS_sys_tilesize<<2)&&(dsth>=NS_sys_tilesize<<2)) {
    struct tetr_tile tilev[4];
    int tilec=tetr_tile_shape(tilev,4,tetr,0);
    if ((tilec<1)||(tilec>4)) return;
    int xa=tilev[0].x,xz=tilev[0].x,ya=tilev[0].y,yz=tilev[0].y;
    int i=1; for (;i<tilec;i++) {
      if (tilev[i].x<xa) xa=tilev[i].x; else if (tilev[i].x>xz) xz=tilev[i].x;
      if (tilev[i].y<ya) ya=tilev[i].y; else if (tilev[i].y>yz) yz=tilev[i].y;
    }
    int16_t figw=(xz-xa+1)*NS_sys_tilesize;
    int16_t figh=(yz-ya+1)*NS_sys_tilesize;
    int16_t figx=dstx+(dstw>>1)-(figw>>1)+(NS_sys_tilesize>>1);
    int16_t figy=dsty+(dsth>>1)-(figh>>1)+(NS_sys_tilesize>>1);
    uint8_t tileid=0x01+tetr; // TODO In a multi-player field, I think we should use a neutral tile.
    const struct tetr_tile *tile=tilev;
    for (i=tilec;i-->0;tile++) {
      graf_draw_tile(&g.graf,g.texid_tiles,figx+(tile->x-xa)*NS_sys_tilesize,figy+(tile->y-ya)*NS_sys_tilesize,tileid,0);
    }
    
  // If too small, use the single-tile thumbnails in row 1.
  } else {
    graf_draw_tile(&g.graf,g.texid_tiles,dstx+(dstw>>1),dsty+(dsth>>1),0x11+tetr,0);
  }
}

/* Render score for single-field game.
 */
 
void field_render_single_score(struct field *field,int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth) {
  //TODO
}

/* Render score for double-field game.
 */
 
void field_render_combined_score(struct field *l,struct field *r,int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth) {
  //TODO
}
