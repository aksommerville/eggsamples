#include "tetris.h"

/* Render fixed cells.
 * Caller adjusts (dstx,dsty) to the center of the top-left cell.
 */
 
static void field_render_cellv_rm(struct field *field,int16_t dstx,int16_t dsty) {
  /* It's substantially more complex when the removal animation is running.
   * Start from the bottom.
   * When we encounter a removing row, draw the special thing and advance output but not input position -- model is already updated.
   */
  uint32_t hilite=(((int)(field->rmclock*15.0))&1)?0xff0000ff:0x200000ff;
  dsty+=NS_sys_tilesize*(FIELDH-1);
  const uint8_t *rowv=field->cellv+FIELDW*(FIELDH-1);
  const uint8_t *rmv=field->rmrowv+(FIELDH-1);
  int yi=FIELDH; for (;yi-->0;rmv--,dsty-=NS_sys_tilesize) {
    if (*rmv) {
      graf_draw_rect(&g.graf,dstx-(NS_sys_tilesize>>1),dsty-(NS_sys_tilesize>>1),FIELDW*NS_sys_tilesize,NS_sys_tilesize,hilite);
    } else {
      int xi=FIELDW;
      const uint8_t *p=rowv;
      int16_t dstx1=dstx;
      for (;xi-->0;dstx1+=NS_sys_tilesize,p++) {
        if (!*p) continue;
        graf_draw_tile(&g.graf,g.texid_tiles,dstx1,dsty,*p,0);
      }
      rowv-=FIELDW;
    }
  }
}
 
static void field_render_cellv(struct field *field,int16_t dstx,int16_t dsty) {
  if (field->rmclock>0.0) {
    field_render_cellv_rm(field,dstx,dsty);
  } else {
    int16_t dstx0=dstx;
    const uint8_t *v=field->cellv;
    int yi=FIELDH; for (;yi-->0;dsty+=NS_sys_tilesize) {
      int xi=FIELDW; for (dstx=dstx0;xi-->0;dstx+=NS_sys_tilesize,v++) {
        if (!*v) continue; // Zero means empty, transparent.
        graf_draw_tile(&g.graf,g.texid_tiles,dstx,dsty,*v,0);
      }
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
  uint8_t tileid=(field->playerc>1)?(0x00+player->playerid):(0x01+player->tetr);
  const struct tetr_tile *tile=tilev;
  for (;tilec-->0;tile++) {
    int col=player->x+tile->x; if ((col<0)||(col>=FIELDW)) continue;
    int row=player->y+tile->y; if ((row<0)||(row>=FIELDH)) continue;
    graf_draw_tile(&g.graf,g.texid_tiles,dstx+col*NS_sys_tilesize,dsty+row*NS_sys_tilesize,tileid,0);
  }
}

/* Render the level indicator.
 */
 
static void field_render_level(struct field *field,int16_t parentx,int16_t parenty) {
  int16_t parentw=NS_sys_tilesize*FIELDW;
  int16_t parenth=NS_sys_tilesize*FIELDH;

  // Regenerate texture if needed.
  if (field->disp_level!=field->level) {
    egg_texture_del(field->level_texid);
    char tmp[64];
    int tmpc=snprintf(tmp,sizeof(tmp),"Level %d",field->level);
    if ((tmpc>0)&&(tmpc<sizeof(tmp))) field->level_texid=font_tex_oneline(g.font,tmp,tmpc,parentw,0xffffffff);
    egg_texture_get_status(&field->level_w,&field->level_h,field->level_texid);
  }
  
  int frame=((int)(field->disp_level_clock*8.0))&1;
  if (frame) graf_set_alpha(&g.graf,0xc0);
  else graf_set_alpha(&g.graf,0x40);
  
  int16_t dstx=parentx+(parentw>>1)-(field->level_w>>1);
  int16_t dsty=parenty+(parenth>>1)-(field->level_h>>1);
  graf_draw_decal(&g.graf,field->level_texid,dstx,dsty,0,0,field->level_w,field->level_h,0);
  
  graf_set_alpha(&g.graf,0xff);
}

/* Render main portion.
 */
 
void field_render(struct field *field,int16_t dstx,int16_t dsty) {
  if (field->end_ack) return;
  if (field->finished) {
    graf_draw_rect(&g.graf,dstx,dsty,FIELDW*NS_sys_tilesize,FIELDH*NS_sys_tilesize,0x400010ff);
  }
  dstx+=NS_sys_tilesize>>1;
  dsty+=NS_sys_tilesize>>1;
  field_render_cellv(field,dstx,dsty);
  const struct player *player=field->playerv;
  int i=field->playerc;
  for (;i-->0;player++) field_render_player(field,player,dstx,dsty);
  if (field->disp_level_clock>0.0) field_render_level(field,dstx-(NS_sys_tilesize>>1),dsty-(NS_sys_tilesize>>1));
}

/* Render upcoming tetromino.
 */
 
void field_render_next(struct field *field,int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth) {
  if (field->finished) return;
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
    uint8_t tileid=(field->playerc>1)?0x08:(0x01+tetr);
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

  /* Rebuild the score texture if needed.
   */
  if (field->disp_score!=field->score) {
    egg_texture_del(field->score_texid);
    char tmp[64];
    int tmpc=snprintf(tmp,sizeof(tmp),"Score: %d\nLines: %d",field->score,field->linec[0]+field->linec[1]*2+field->linec[2]*3+field->linec[3]*4);
    if ((tmpc<0)||(tmpc>sizeof(tmp))) tmpc=0;
    field->score_texid=font_tex_multiline(g.font,tmp,tmpc,dstw,dsth,0xffffffff);
    egg_texture_get_status(&field->score_w,&field->score_h,field->score_texid);
    field->disp_score=field->score;
  }
  
  graf_draw_decal(&g.graf,field->score_texid,dstx,dsty+(dsth>>1)-(field->score_h>>1),0,0,field->score_w,field->score_h,0);
}

/* Render score for double-field game.
 */
 
void field_render_combined_score(struct field *l,struct field *r,int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth) {

  /* Rebuild texture if either score changed.
   * We use the texture from (l).
   */
  if ((l->disp_score!=l->score)||(r->disp_score!=r->score)) {
    l->disp_score=l->score;
    r->disp_score=r->score;
    int llinec=l->linec[0]+l->linec[1]*2+l->linec[2]*3+l->linec[3]*4;
    int rlinec=r->linec[0]+r->linec[1]*2+r->linec[2]*3+r->linec[3]*4;
    int lineh=font_get_line_height(g.font);
    int boty=dsth>>1;
    int topy=boty-lineh;
    char *rgba=calloc(dstw*dsth,4);
    if (!rgba) return;
    char tmp[64];
    int tmpc;
    if (((tmpc=snprintf(tmp,sizeof(tmp),"%d",l->score))>0)&&(tmpc<sizeof(tmp))) {
      font_render_string(rgba,dstw,dsth,dstw<<2,0,topy,g.font,tmp,tmpc,0xffffffff);
    }
    if (((tmpc=snprintf(tmp,sizeof(tmp),"%d",llinec))>0)&&(tmpc<sizeof(tmp))) {
      font_render_string(rgba,dstw,dsth,dstw<<2,0,boty,g.font,tmp,tmpc,0xffffffff);
    }
    if (((tmpc=snprintf(tmp,sizeof(tmp),"%d",r->score))>0)&&(tmpc<sizeof(tmp))) {
      int sw=font_measure_line(g.font,tmp,tmpc);
      font_render_string(rgba,dstw,dsth,dstw<<2,dstw-sw,topy,g.font,tmp,tmpc,0xffffffff);
    }
    if (((tmpc=snprintf(tmp,sizeof(tmp),"%d",rlinec))>0)&&(tmpc<sizeof(tmp))) {
      int sw=font_measure_line(g.font,tmp,tmpc);
      font_render_string(rgba,dstw,dsth,dstw<<2,dstw-sw,boty,g.font,tmp,tmpc,0xffffffff);
    }
    if (!l->score_texid) l->score_texid=egg_texture_new();
    egg_texture_load_raw(l->score_texid,EGG_TEX_FMT_RGBA,dstw,dsth,dstw<<2,rgba,dstw*dsth*4);
    l->score_w=dstw;
    l->score_h=dsth;
    free(rgba);
  }
  
  graf_draw_decal(&g.graf,l->score_texid,dstx+(dstw>>1)-(l->score_w>>1),dsty+(dsth>>1)-(l->score_h>>1),0,0,l->score_w,l->score_h,0);
}
