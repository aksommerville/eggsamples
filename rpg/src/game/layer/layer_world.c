/* layer_world.c
 * View of the outer world, sprites walking around a map etc.
 * This layer has session scope, ie it lasts until you quit or return to Hello.
 */

#include "game/rpg.h"
#include "game/world/world.h"

/* Object definition.
 */
 
struct layer_world {
  struct layer hdr;
};

#define LAYER ((struct layer_world*)layer)

/* Layer hooks route directly to the world instance.
 */
 
static void _world_del(struct layer *layer) {
  world_del(g.world);
  g.world=0;
}
 
static void _world_input(struct layer *layer,int btnid,int value,int state) {
  if (!g.world) { layer->defunct=1; return; }
  if (btnid==EGG_BTN_FOCUS) {
    if (value) world_suspend(g.world);
    else world_resume(g.world,state);
  } else {
    world_input(g.world,btnid,value,state);
  }
}
 
static void _world_update(struct layer *layer,double elapsed) {
  if (!g.world) { layer->defunct=1; return; }
  world_update(g.world,elapsed);
}
 
static void _world_render(struct layer *layer) {
  if (!g.world) { layer->defunct=1; return; }
  world_render(g.world);
}

/* New.
 */
 
struct layer *layer_spawn_world(const char *save,int savec) {

  /* We own (g.world).
   * If it already exists, something is horribly wrong.
   */
  if (g.world) return 0;

  struct layer *layer=layer_spawn(sizeof(struct layer_world));
  if (!layer) return 0;
  
  layer->opaque=1;
  layer->magic=layer_spawn_world;
  layer->del=_world_del;
  layer->input=_world_input;
  layer->update=_world_update;
  //layer->update_bg=_world_update;
  //layer->update_hidden=_world_update;
  layer->render=_world_render;
  
  if (!(g.world=world_new(save,savec))) { layer->defunct=1; return 0; }
  if (g.world->save_result<0) {
    fprintf(stderr,"Saved game failed to load, starting new game instead.\n");
  } else if (g.world->save_result>0) {
    fprintf(stderr,"Loaded saved game.\n");
  } else {
    fprintf(stderr,"Starting new game.\n");
  }
  if (world_start(g.world)<0) { layer->defunct=1; return 0; }
  
  return layer;
}
