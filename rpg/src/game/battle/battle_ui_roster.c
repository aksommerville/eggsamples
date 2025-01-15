#include "battle_internal.h"

/* Object.
 */
 
struct roster {
  int x,y,w,h;
  int teamp;
  struct battle *battle;
  struct team *team;
  struct label { // Parallel to (team->fighterv).
    int texid,texw,texh;
    int disphp;
    int highlight; // Reference count. Highlit if nonzero.
  } *labelv;
  int labelc,labela;
  int labelv_dirty;
  uint32_t highlight_color;
};

/* Delete.
 */
 
void roster_del(struct roster *roster) {
  if (!roster) return;
  if (roster->labelv) {
    while (roster->labelc-->0) {
      egg_texture_del(roster->labelv[roster->labelc].texid);
    }
    free(roster->labelv);
  }
  free(roster);
}

/* New.
 */
 
struct roster *roster_new(struct battle *battle,int teamp) {
  if ((teamp<0)||(teamp>1)) return 0;
  struct roster *roster=calloc(1,sizeof(struct roster));
  if (!roster) return 0;
  roster->teamp=teamp;
  roster->battle=battle;
  roster->team=battle->teamv+teamp;
  roster->labelv_dirty=1;
  roster->highlight_color=0x401020ff;
  return roster;
}

/* Set bounds.
 */
 
void roster_set_bounds(struct roster *roster,int x,int y,int w,int h) {
  roster->x=x;
  roster->y=y;
  roster->w=w;
  roster->h=h;
}

/* Update.
 */
 
void roster_update(struct roster *roster,double elapsed) {
}

/* Rebuild labels list.
 * This should only happen once: Labels only contain the fighters' names, and those aren't allowed to change.
 */
 
static void roster_rebuild_labels(struct roster *roster) {
  while (roster->labelc>0) {
    roster->labelc--;
    egg_texture_del(roster->labelv[roster->labelc].texid);
  }
  if (roster->labela<roster->team->fighterc) {
    int na=roster->team->fighterc;
    void *nv=realloc(roster->labelv,sizeof(struct label)*na);
    if (!nv) return;
    roster->labelv=nv;
    roster->labela=na;
  }
  while (roster->labelc<roster->team->fighterc) {
    const struct fighter *fighter=roster->team->fighterv+roster->labelc;
    struct label *label=roster->labelv+roster->labelc++;
    label->texid=font_tex_oneline(g.font,fighter->name,fighter->namec,roster->w,0xffffffff);
    egg_texture_get_status(&label->texw,&label->texh,label->texid);
    //label->disphp=0;
    label->disphp=fighter->hp;
    label->highlight=0;
  }
}

/* Render HP bar.
 * Provided bounds are the total available space; don't necessarily fill it.
 */
 
static void roster_draw_hp(struct roster *roster,int16_t x,int16_t y,int16_t w,int16_t h,int hp,int max,int texid) {
  const int16_t xstride=5;
  int16_t dstx=x+w-4;
  int16_t dsty=y+(h>>1);
  if (hp<0) hp=0; else if (hp>9999) hp=9999;
  if (max<0) max=0; else if (max>9999) max=9999;
  int limit=10,div=1;
  while (max>div-1) {
    int digit=(max/div)%10;
    graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+digit,0);
    dstx-=xstride;
    div=limit;
    limit*=10;
  }
  graf_draw_tile(&g.graf,texid,dstx,dsty,'/',0);
  dstx-=xstride;
  limit=10;
  div=1;
  while (hp>=div-1) {
    int digit=(hp/div)%10;
    graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+digit,0);
    dstx-=xstride;
    div=limit;
    limit*=10;
  }
  if (max>0) {
    int16_t barx=x+xstride;
    int16_t bary=dsty-2;
    int16_t barw=x+(w>>1)-barx;
    int16_t barh=3;
    if ((barw>0)&&(barh>0)) {
      graf_draw_rect(&g.graf,barx,bary,barw,barh,0x404040ff);
      int thumbw=(hp*barw)/max;
      if (thumbw>barw) thumbw=barw;
      graf_draw_rect(&g.graf,barx,bary,thumbw,barh,0xffff00ff);
    }
  }
}

/* Step displayed HP to approach model HP.
 * We do this statelessly, so we can't enforce an overall duration for the animation.
 */
 
static int roster_hp_approach(int to,int from) {
  int d=to-from;
       if (d<-100) return from-20;
  else if (d< -10) return from-2;
  else if (d<   0) return from-1;
  else if (d<   1) return from;
  else if (d<  10) return from+1;
  else if (d< 100) return from+2;
  else             return from+20;
}

/* Render.
 */
 
void roster_render(struct roster *roster) {

  // Freshen labels if they need it.
  if (roster->labelv_dirty) {
    roster->labelv_dirty=0;
    roster_rebuild_labels(roster);
  }
  
  /* Draw a row for each fighter.
   * Labels and fighters must be the same length. If not, panic and render nothing but the border.
   * TODO We might need clipping here, and would be polite to do it even if not necessary yet.
   */
  if ((roster->labelc==roster->team->fighterc)&&(roster->labelc>0)) {
    const int16_t margin_left=4;
    const int16_t margin_top=4;
    const int16_t rowh=roster->labelv[0].texh;
    int texid_tiles=texcache_get_image(&g.texcache,RID_image_uitiles);
    int16_t dsty=roster->y+margin_top;
    struct label *label=roster->labelv;
    const struct fighter *fighter=roster->team->fighterv;
    int i=roster->labelc;
    for (;i-->0;label++,fighter++) {
      if (label->highlight) {
        graf_draw_rect(&g.graf,roster->x,dsty-1,roster->w,rowh*2+1,roster->highlight_color);
      }
      graf_draw_decal(&g.graf,label->texid,roster->x+margin_left,dsty,0,0,label->texw,label->texh,0);
      dsty+=rowh;
      label->disphp=roster_hp_approach(fighter->hp,label->disphp);
      roster_draw_hp(roster,roster->x+margin_left,dsty,roster->w-margin_left*2,rowh,label->disphp,fighter->hpmax,texid_tiles);
      dsty+=rowh;
    }
  }
  
  textbox_render_border(roster->x,roster->y,roster->w,roster->h);
}

/* Highlight.
 */

void roster_clear_highlights(struct roster *roster) {
  struct label *label=roster->labelv;
  int i=roster->labelc;
  for (;i-->0;label++) {
    label->highlight=0;
  }
}

void roster_highlight_id(struct roster *roster,int id,int highlight) {
  if (roster->labelc!=roster->team->fighterc) return;
  struct label *label=roster->labelv;
  const struct fighter *fighter=roster->team->fighterv;
  int i=roster->labelc;
  for (;i-->0;label++,fighter++) {
    if (fighter->id!=id) continue;
    if (highlight) {
      label->highlight++;
    } else if (label->highlight) {
      label->highlight--;
    }
  }
}
