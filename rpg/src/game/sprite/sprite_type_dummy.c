/* sprite_type_dummy.c
 */
 
#include "game/rpg.h"
#include "sprite.h"

const struct sprite_type sprite_type_dummy={
  .name="dummy",
  .objlen=sizeof(struct sprite),
};
