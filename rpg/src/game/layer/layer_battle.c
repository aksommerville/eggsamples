/* layer_battle.c
 * Presents a battle.
 */

#include "game/rpg.h"
#include "game/battle/battle.h"

/* Object definition.
 */
 
struct layer_battle {
  struct layer hdr;
  struct battle *battle;
};

#define LAYER ((struct layer_battle*)layer)

/* Layer hooks route directly to the battle instance.
 */
 
static void _battle_del(struct layer *layer) {
  battle_del(LAYER->battle);
}
 
static void _battle_input(struct layer *layer,int btnid,int value,int state) {
  if (!LAYER->battle) { layer->defunct=1; return; }
  if (btnid==EGG_BTN_FOCUS) {
    if (value) battle_resume(LAYER->battle,state,0);
    else battle_suspend(LAYER->battle);
  } else {
    battle_input(LAYER->battle,1,btnid,value,state);
  }
}
 
static void _battle_update(struct layer *layer,double elapsed) {
  if (!LAYER->battle) { layer->defunct=1; return; }
  battle_update(LAYER->battle,elapsed);
  if (battle_is_finished(LAYER->battle)) {
    layer->defunct=1;
  }
}
 
static void _battle_render(struct layer *layer) {
  if (!LAYER->battle) { layer->defunct=1; return; }
  battle_render(LAYER->battle);
}

/* New.
 */
 
struct layer *layer_spawn_battle(int battleid) {
  if (!g.world) return 0;

  struct layer *layer=layer_spawn(sizeof(struct layer_battle));
  if (!layer) return 0;
  
  layer->opaque=1;
  layer->magic=layer_spawn_battle;
  layer->del=_battle_del;
  layer->input=_battle_input;
  layer->update=_battle_update;
  //layer->update_bg=_battle_update;
  //layer->update_hidden=_battle_update;
  layer->render=_battle_render;
  
  if (
    !(LAYER->battle=battle_new())||
    (battle_setup_res(LAYER->battle,g.world,battleid)<0)
  ) { layer->defunct=1; return 0; }
  
  return layer;
}
