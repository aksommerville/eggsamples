#include "battle_internal.h"

/* Object.
 */
 
struct scene_sprite {
  int side;
  const struct fighter *fighter; // WEAK
  int16_t rx,ry; // Resting position.
  int16_t x,y; // Actual position.
  int imageid; // Zero to skip rendering.
  uint8_t tileid; // Base tileid. Constant.
  uint8_t xform; // Entirely determined by (side). Natural orientation faces left.
};
 
struct scene {
  int x,y,w,h;
  struct battle *battle;
  struct scene_sprite *spritev;
  int spritec,spritea;
};

/* Delete.
 */
 
void scene_del(struct scene *scene) {
  if (!scene) return;
  if (scene->spritev) {
    free(scene->spritev);
  }
  free(scene);
}

/* New.
 */
 
struct scene *scene_new(struct battle *battle) {
  struct scene *scene=calloc(1,sizeof(struct scene));
  if (!scene) return 0;
  scene->battle=battle;
  return scene;
}

/* Set bounds.
 */
 
void scene_set_bounds(struct scene *scene,int x,int y,int w,int h) {
  scene->x=x;
  scene->y=y;
  scene->w=w;
  scene->h=h;
}

/* Add sprite.
 * Fails if there isn't room; caller is responsible for realloc.
 * Position is (0,0) until later. They get positioned after they're all created.
 */
 
static int scene_add_sprite(struct scene *scene,const struct fighter *fighter,int side) {
  if (scene->spritec>=scene->spritea) return -1;
  struct scene_sprite *sprite=scene->spritev+scene->spritec++;
  memset(sprite,0,sizeof(struct scene_sprite));
  
  sprite->side=side;
  sprite->fighter=fighter;
  sprite->imageid=fighter->imageid;
  sprite->tileid=fighter->tileid;
  
  return 0;
}

/* Position all sprites.
 */
 
static void scene_pack_sprites(struct scene *scene) {

  /* Count per side.
   */
  int leftc=0,rightc=0,i=scene->spritec;
  struct scene_sprite *sprite=scene->spritev;
  for (;i-->0;sprite++) {
    if (sprite->side==0) leftc++;
    else if (sprite->side==1) rightc++;
  }
  
  int topy=scene->y+BATTLE_SCENE_TOP_H;
  int availh=scene->h-BATTLE_SCENE_TOP_H;
  if (availh<1) availh=1;
  
  /* There shouldn't be more than 4 fighters in a party.
   * Align them in columns of two, but if a fighter ends up with a column to himself, center instead of thirds.
   */
  int leftp=0,rightp=0,xspacing=(NS_sys_tilesize*3)/2;
  for (sprite=scene->spritev,i=scene->spritec;i-->0;sprite++) {
    if (sprite->side==0) {
      sprite->xform=EGG_XFORM_XREV;
      sprite->x=scene->x+NS_sys_tilesize+xspacing*(leftp>>1);
      if ((leftc&1)&&(leftp==leftc-1)) { // Center.
        sprite->y=topy+(availh>>1);
      } else { // Thirds.
        sprite->y=topy+(((leftp&1)+1)*availh)/3;
      }
      leftp++;
    } else if (sprite->side==1) {
      sprite->xform=0;
      sprite->x=scene->x+scene->w-NS_sys_tilesize-xspacing*(rightp>>1);
      if ((rightc&1)&&(rightp==rightc-1)) { // Center.
        sprite->y=topy+(availh>>1);
      } else {
        sprite->y=topy+(((rightp&1)+1)*availh)/3;
      }
      rightp++;
    } else {
      sprite->imageid=0;
    }
    sprite->rx=sprite->x;
    sprite->ry=sprite->y;
  }
  
  //TODO Once it's all in place, sort by imageid (assuming they don't overlap vertically -- if so, sort by (y)).
}

/* Update.
 */
 
void scene_update(struct scene *scene,double elapsed) {

  /* Ensure our sprite count matches the battle's fighter count.
   * If not, chuck them and rebuild from scratch.
   * This should only happen once; fighter list is constant even if they die.
   * (and dead fighters should keep their sprite, just it renders differently).
   * We can't do this at init because the UI elements are instantiated before the battle model gets populated.
   */
  int fighterc=scene->battle->teamv[0].fighterc+scene->battle->teamv[1].fighterc;
  if (fighterc!=scene->spritec) {
    scene->spritec=0; // Clean up, if sprites ever need that.
    if (scene->spritea<fighterc) {
      void *nv=realloc(scene->spritev,sizeof(struct scene_sprite)*fighterc);
      if (!nv) return;
      scene->spritev=nv;
      scene->spritea=fighterc;
    }
    const struct fighter *fighter;
    int i;
    for (fighter=scene->battle->teamv[0].fighterv,i=scene->battle->teamv[0].fighterc;i-->0;fighter++) scene_add_sprite(scene,fighter,0);
    for (fighter=scene->battle->teamv[1].fighterv,i=scene->battle->teamv[1].fighterc;i-->0;fighter++) scene_add_sprite(scene,fighter,1);
    scene_pack_sprites(scene);
  }
  
  //TODO animation etc
}

/* Render.
 */
 
void scene_render(struct scene *scene) {

  /* Background.
   * TODO Get these from whoever launched the battle. They depend on world, shouldn't be part of the battle resource.
   */
  int bgimageid=RID_image_battlebg,bgcol=0,bgrow=0;
  int texid=texcache_get_image(&g.texcache,bgimageid);
  graf_draw_decal(&g.graf,texid,scene->x,scene->y,bgcol*BATTLE_SCENE_W,bgrow*BATTLE_SCENE_H,BATTLE_SCENE_W,BATTLE_SCENE_H,0);
  
  /* Sprites.
   */
  int imageid=bgimageid;
  struct scene_sprite *sprite=scene->spritev;
  int i=scene->spritec;
  for (;i-->0;sprite++) {
    if (!sprite->imageid) continue;
    if (sprite->imageid!=imageid) {
      texid=texcache_get_image(&g.texcache,imageid=sprite->imageid);
    }
    uint8_t tileid=sprite->tileid;
    if (sprite->fighter->hp<=0) tileid+=0x01;
    graf_draw_tile(&g.graf,texid,sprite->x,sprite->y,tileid,sprite->xform);
    //TODO Other decorations, like swinging a sword...
  }
  
  //TODO Damage indicators.
  //TODO Focus indicators.
  
  textbox_render_border(scene->x,scene->y,scene->w,scene->h);
}
