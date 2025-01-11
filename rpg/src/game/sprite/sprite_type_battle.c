/* sprite_type_battle.c
 * Bump this sprite to begin an explicit battle.
 * arg: (u16)battleid (u16)flag
 * If (flag) nonzero, we self-destroy at initiate.
 */
 
#include "game/rpg.h"
#include "game/world/world.h"
#include "sprite.h"

struct sprite_battle {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_battle*)sprite)

static int _battle_init(struct sprite *sprite) {
  if (world_get_flag(g.world,sprite->arg&0xffff)) return -1;
  return 0;
}

static void _battle_bump(struct sprite *sprite,struct sprite *hero) {
  int battleid=sprite->arg>>16;
  struct layer *layer=layer_spawn_battle(battleid);
  if (!layer) {
    fprintf(stderr,"Failed to start battle:%d\n",battleid);
    return;
  }
}

const struct sprite_type sprite_type_battle={
  .name="battle",
  .objlen=sizeof(struct sprite_battle),
  .init=_battle_init,
  .bump=_battle_bump,
};
