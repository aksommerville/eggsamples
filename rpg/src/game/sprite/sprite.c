#include "game/rpg.h"
#include "game/world/world.h"
#include "sprite.h"
#include "opt/rom/rom.h"

/* Type IDs.
 */
 
const struct sprite_type *sprite_type_by_id(int id) {
  switch (id) {
    #define _(tag) case NS_spritetype_##tag: return &sprite_type_##tag;
    FOR_EACH_spritetype
    #undef _
  }
  return 0;
}

const struct sprite_type *sprite_type_from_commands(const void *src,int srcc) {
  struct rom_command_reader reader={.v=src,.c=srcc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    if (cmd.opcode==CMD_sprite_type) return sprite_type_by_id((cmd.argv[0]<<8)|cmd.argv[1]);
  }
  return 0;
}

/* Delete.
 */
 
void sprite_del(struct sprite *sprite) {
  if (!sprite) return;
  if (sprite->type->del) sprite->type->del(sprite);
  free(sprite);
}

/* New, programmatic.
 */

struct sprite *sprite_new(const struct sprite_type *type,int rid,const void *cmdv,int cmdc,double x,double y,uint32_t arg) {
  if (!g.world) return 0;
  if (!type) return 0;
  struct sprite *sprite=calloc(1,type->objlen);
  if (!sprite) return 0;
  
  sprite->type=type;
  sprite->rid=rid;
  sprite->cmdv=cmdv;
  sprite->cmdc=cmdc;
  sprite->arg=arg;
  sprite->x=x;
  sprite->y=y;
  sprite->layer=0x80;
  
  /* Apply generic commands.
   */
  struct rom_command_reader reader={.v=cmdv,.c=cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_sprite_solid: sprite->solid=1; break;
      case CMD_sprite_image: sprite->imageid=(cmd.argv[0]<<8)|cmd.argv[1]; break;
      case CMD_sprite_tile: sprite->tileid=cmd.argv[0]; sprite->xform=cmd.argv[1]; break;
      case CMD_sprite_layer: sprite->layer=cmd.argv[0]; break;
    }
  }
  
  /* Type-specific initialization.
   */
  int err=0;
  if (type->init) err=type->init(sprite);
  if ((err<0)||sprite->defunct) {
    sprite_del(sprite);
    return 0;
  }
  
  /* Add to global list.
   */
  if (g.world->spritec>=g.world->spritea) {
    int na=g.world->spritea+32;
    if (na>INT_MAX/sizeof(void*)) { sprite_del(sprite); return 0; }
    void *nv=realloc(g.world->spritev,sizeof(void*)*na);
    if (!nv) { sprite_del(sprite); return 0; }
    g.world->spritev=nv;
    g.world->spritea=na;
  }
  g.world->spritev[g.world->spritec++]=sprite;
  
  return sprite;
}

/* New, from resource.
 */

struct sprite *sprite_new_res(int rid,uint8_t x,uint8_t y,uint32_t arg) {
  const void *serial=0;
  int serialc=rpg_res_get(&serial,EGG_TID_sprite,rid);
  struct rom_sprite rspr;
  if (rom_sprite_decode(&rspr,serial,serialc)<0) return 0;
  const struct sprite_type *type=sprite_type_from_commands(rspr.cmdv,rspr.cmdc);
  return sprite_new(type,rid,rspr.cmdv,rspr.cmdc,x+0.5,y+0.5,arg);
}
