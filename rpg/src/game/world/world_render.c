#include "game/rpg.h"
#include "game/sprite/sprite.h"
#include "map.h"
#include "world.h"

/* Calculate camera position.
 */
 
static void world_camera_refresh(struct world *world) {
  int mapw=world->map->w*NS_sys_tilesize;
  int maph=world->map->h*NS_sys_tilesize;
  
  /* If either axis is smaller than the framebuffer, center it.
   * If they're both smaller, that's all.
   */
  int needx=0,needy=0;
  if (mapw<=FBW) {
    world->camw=mapw;
    world->camx=(FBW>>1)-(mapw>>1);
  } else needx=1;
  if (maph<=FBH) {
    world->camh=maph;
    world->camy=(FBH>>1)-(maph>>1);
  } else needy=1;
  if (!needx&&!needy) return;
  
  /* Locate the hero sprite and center on her, on any axis not established by the small rule.
   * After centering, clamp to the map's boundary.
   */
  struct sprite *hero=world_get_hero(world);
  if (!hero) return;
  int herox=(int)(hero->x*NS_sys_tilesize);
  int heroy=(int)(hero->y*NS_sys_tilesize);
  if (needx) {
    world->camx=herox-(FBW>>1);
    if (world->camx<0) world->camx=0;
    else if (world->camx>mapw-FBW) world->camx=mapw-FBW;
    world->camw=FBW;
  }
  if (needy) {
    world->camy=heroy-(FBH>>1);
    if (world->camy<0) world->camy=0;
    else if (world->camy>maph-FBH) world->camy=maph-FBH;
    world->camh=FBH;
  }
}

/* Render map and if necessary, the blotter behind it.
 */
 
static void world_render_map(struct world *world) {

  /* For small maps, draw a blotter first.
   */
  if ((world->camw<FBW)||(world->camh<FBH)) {
    graf_draw_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
  }
  
  /* graf has a convenience that does exactly what we need for the map.
   * We just need to do some dumb math for the projection.
   */
  int srccol=world->camx/NS_sys_tilesize;
  int srcrow=world->camy/NS_sys_tilesize;
  int colc=(world->camx+world->camw-1)/NS_sys_tilesize-srccol+1;
  int rowc=(world->camy+world->camh-1)/NS_sys_tilesize-srcrow+1;
  if (srccol<0) { colc+=srccol; srccol=0; }
  if (srcrow<0) { rowc+=srcrow; srcrow=0; }
  if (srccol+colc>world->map->w) colc=world->map->w-srccol;
  if (srcrow+rowc>world->map->h) rowc=world->map->h-srcrow;
  int dstx=(NS_sys_tilesize>>1)+srccol*NS_sys_tilesize-world->camx;
  int dsty=(NS_sys_tilesize>>1)+srcrow*NS_sys_tilesize-world->camy;
  graf_draw_tile_buffer(
    &g.graf,
    texcache_get_image(&g.texcache,world->map->imageid),
    dstx,dsty,
    world->map->v+srcrow*world->map->w+srccol,
    colc,rowc,world->map->w
  );
}

/* Compare sprites for render order.
 * Returns strictly (-1,0,1).
 */
 
static int sprites_rendercmp(const struct sprite *a,const struct sprite *b) {
  if (a->layer<b->layer) return -1;
  if (a->layer>b->layer) return 1;
  if (a->y<b->y) return -1;
  if (a->y>b->y) return 1;
  return 0;
}

/* Sort sprites.
 * We normally sort incrementally to avoid churn.
 * But after a major change like loading a new map, call this to sort them all the way.
 */
 
void world_sort_sprites_fully(struct world *world) {
  if (world->spritec<2) return;
  int lo=0,hi=world->spritec-1,d=1;
  while (lo<hi) {
    int first,last,i,done=1;
    if (d>0) { first=lo; last=hi; }
    else { first=hi; last=lo; }
    for (i=first;i!=last;i+=d) {
      int cmp=sprites_rendercmp(world->spritev[i],world->spritev[i+d]);
      if (cmp==d) {
        done=0;
        struct sprite *tmp=world->spritev[i];
        world->spritev[i]=world->spritev[i+d];
        world->spritev[i+d]=tmp;
      }
    }
    if (done) return;
    if (d>0) { hi--; d=-1; }
    else { lo++; d=1; }
  }
}

/* Render sprites.
 */
 
static void world_render_sprites(struct world *world) {
  if (world->spritec<1) return;

  /* Do one pass of a bubble sort to push them toward render order.
   * Don't complete the sort; that would be a lot of extra churn every frame.
   * I figure it's OK for the order to be out of whack for a frame or two when something changes.
   */
  if (world->sprite_sort_dir>0) {
    int i=0,stop=world->spritec-1;
    for (;i!=stop;i++) {
      int cmp=sprites_rendercmp(world->spritev[i],world->spritev[i+1]);
      if (cmp==1) {
        struct sprite *tmp=world->spritev[i];
        world->spritev[i]=world->spritev[i+1];
        world->spritev[i+1]=tmp;
      }
    }
    world->sprite_sort_dir=-1;
  } else {
    int i=world->spritec-1,stop=0;
    for (;i!=stop;i--) {
      int cmp=sprites_rendercmp(world->spritev[i],world->spritev[i-1]);
      if (cmp==-1) {
        struct sprite *tmp=world->spritev[i];
        world->spritev[i]=world->spritev[i-1];
        world->spritev[i-1]=tmp;
      }
    }
    world->sprite_sort_dir=1;
  }
  
  /* Render them all in order.
   */
  int texid=0,imageid=0;
  int i=world->spritec;
  struct sprite **p=world->spritev;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->defunct) continue;
    
    // Project to framebuffer and if they haven't told us not to, cull anything at least one tile OOB.
    int dstx=(int)(sprite->x*NS_sys_tilesize)-world->camx;
    int dsty=(int)(sprite->y*NS_sys_tilesize)-world->camy;
    if (!sprite->always_render) {
      if (dstx<=-NS_sys_tilesize) continue;
      if (dsty<=-NS_sys_tilesize) continue;
      if (dstx>=FBW+NS_sys_tilesize) continue;
      if (dsty>=FBH+NS_sys_tilesize) continue;
    }
    
    // If it implements (render), we need to flush.
    if (sprite->type->render) {
      graf_flush(&g.graf);
      texid=imageid=0;
      sprite->type->render(sprite,dstx,dsty);
    
    // For normal tile-based sprites, we can lump them in one GPU command sometimes.
    } else {
      if (sprite->imageid!=imageid) {
        graf_flush(&g.graf);
        imageid=sprite->imageid;
        texid=texcache_get_image(&g.texcache,imageid);
      }
      graf_draw_tile(&g.graf,texid,dstx,dsty,sprite->tileid,sprite->xform);
    }
  }
  graf_flush(&g.graf);
}

/* Render, main entry point.
 */
 
void world_render(struct world *world) {
  world_camera_refresh(world);
  world_render_map(world);
  world_render_sprites(world);
}
