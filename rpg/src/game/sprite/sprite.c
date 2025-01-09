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

/* Global store.
 */
 
static struct sprites {
  struct sprite_res {
    int rid;
    const void *cmdv;
    int cmdc;
    const struct sprite_type *type;
  } *v;
  int c,a;
} gsprites={0};

/* Initialize global store.
 */
 
int sprites_init(const void *rom,int romc) {
  struct rom_reader reader;
  if (rom_reader_init(&reader,rom,romc)<0) return -1;
  struct rom_res *res;
  while (res=rom_reader_next(&reader)) {
    if (res->tid<EGG_TID_sprite) continue;
    if (res->tid>EGG_TID_sprite) break;
    
    /* Extract type first.
     * At instantiation, we're going to need this before anything else.
     */
    const struct sprite_type *type=0;
    struct rom_sprite rspr;
    if (rom_sprite_decode(&rspr,res->v,res->c)<0) {
      fprintf(stderr,"sprite:%d malformed\n",res->rid);
      return -1;
    }
    struct rom_command_reader sreader={.v=rspr.cmdv,.c=rspr.cmdc};
    struct rom_command cmd;
    while (rom_command_reader_next(&cmd,&sreader)>0) {
      if (cmd.opcode==CMD_sprite_type) {
        int sprite_type_id=(cmd.argv[0]<<8)|cmd.argv[1];
        if (!(type=sprite_type_by_id(sprite_type_id))) {
          fprintf(stderr,"sprite:%d refers to unknown type %d\n",res->rid,sprite_type_id);
          return -1;
        }
        break;
      }
    }
    if (!type) {
      fprintf(stderr,"sprite:%d missing 'type' command\n",res->rid);
      return -1;
    }
    
    if (gsprites.c>=gsprites.a) {
      int na=gsprites.c+32;
      if (na>INT_MAX/sizeof(struct sprite_res)) return -1;
      void *nv=realloc(gsprites.v,sizeof(struct sprite_res)*na);
      if (!nv) return -1;
      gsprites.v=nv;
      gsprites.a=na;
    }
    struct sprite_res *sres=gsprites.v+gsprites.c++;
    sres->rid=res->rid;
    sres->cmdv=rspr.cmdv;
    sres->cmdc=rspr.cmdc;
    sres->type=type;
  }
  return 0;
}

/* Get resource from store.
 */
 
int sprite_res_get(const struct sprite_type **type,void *dstpp,int rid) {
  if (rid<1) return 0;
  int lo=0,hi=gsprites.c;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct sprite_res *q=gsprites.v+ck;
         if (rid<q->rid) hi=ck;
    else if (rid>q->rid) lo=ck+1;
    else {
      *type=q->type;
      *(const void**)dstpp=q->cmdv;
      return q->cmdc;
    }
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
  const struct sprite_type *type=0;
  const void *cmdv=0;
  int cmdc=sprite_res_get(&type,&cmdv,rid);
  if (cmdc<1) return 0;
  return sprite_new(type,rid,cmdv,cmdc,x+0.5,y+0.5,arg);
}
