#include "game/rpg.h"
#include "game/world/world.h"
#include "game/world/map.h"
#include "game/sprite/sprite.h"
#include "opt/password/password.h"
#include "opt/rom/rom.h"

/* Delete.
 */
 
void world_del(struct world *world) {
  if (!world) return;
  if (world->spritev) {
    while (world->spritec-->0) sprite_del(world->spritev[world->spritec]);
    free(world->spritev);
  }
  free(world);
}

/* Initialize a fresh world from a saved state.
 * Fails if anything goes wrong -- the fallback is not our job.
 * We only touch (world->state) and may leave it dirty on failures.
 */
 
static int world_init_save(struct world *world,const char *save,int savec) {
  char password[PASSWORD_ENCODED_SIZE(struct world_state)];
  int passwordc=egg_store_get(password,sizeof(password),"save",4);
  if ((passwordc<1)||(passwordc>sizeof(password))) return -1;
  PASSWORD_DECODE(world->state,password,passwordc)
  if (!decode_ok) return -1;
  //TODO Business-level consistency checks eg (hp<=hpmax)
  return 0;
}

/* Initialize world->state for a new game.
 * It's not necessarily straight zeroes -- there may have been a failed load first.
 */
 
static int world_init_new(struct world *world) {
  memset(&world->state,0,sizeof(world->state));
  world->state.mapid=RID_map_start;
  world->state.hp=world->state.hpmax=100;
  return 0;
}

/* New.
 */

struct world *world_new(const char *save,int savec) {
  if (!save||(savec<0)) savec=0;
  struct world *world=calloc(1,sizeof(struct world));
  if (!world) return 0;
  if (savec) {
    if (world_init_save(world,save,savec)<0) {
      world->save_result=-1;
      if (world_init_new(world)<0) {
        world_del(world);
        return 0;
      }
    } else {
      world->save_result=1;
    }
  } else {
    if (world_init_new(world)<0) {
      world_del(world);
      return 0;
    }
  }
  return world;
}

/* Start.
 */
 
int world_start(struct world *world) {
  if (!world) return -1;
  if (world_load_map(world,world->state.mapid)<0) return -1;
  //TODO
  return 0;
}

/* Suspend.
 * Drop any input state.
 */

void world_suspend(struct world *world) {
  //TODO
}

/* Resume.
 * Force input state, restart music...
 */
 
void world_resume(struct world *world,int state) {
  //TODO
}

/* Input event.
 */
 
void world_input(struct world *world,int btnid,int value,int state) {
  struct sprite *hero=world_get_hero(world);
  if (hero) {
    switch (btnid) {
      case EGG_BTN_LEFT: hero_motion(hero,-1,0,value); break;
      case EGG_BTN_RIGHT: hero_motion(hero,1,0,value); break;
      case EGG_BTN_UP: hero_motion(hero,0,-1,value); break;
      case EGG_BTN_DOWN: hero_motion(hero,0,1,value); break;
      //TODO Attack or context-sensitive action?
    }
  }
  //TODO Pause.
}

/* Update.
 */
 
void world_update(struct world *world,double elapsed) {

  /* Update all sprites that can.
   */
  int i=world->spritec;
  while (i-->0) {
    struct sprite *sprite=world->spritev[i];
    if (sprite->defunct) continue;
    if (sprite->type->update) sprite->type->update(sprite,elapsed);
  }
  
  /* Reap defunct sprites.
   */
  for (i=world->spritec;i-->0;) {
    struct sprite *sprite=world->spritev[i];
    if (!sprite->defunct) continue;
    world->spritec--;
    memmove(world->spritev+i,world->spritev+i+1,sizeof(void*)*(world->spritec-i));
    sprite_del(sprite);
  }
}

/* Load map.
 */
 
int world_load_map(struct world *world,int mapid) {
  struct map *map=map_by_rid(mapid);
  if (!map) {
    fprintf(stderr,"map:%d not found\n",mapid);
    return -1;
  }
  int pvimageid=0;
  if (world->map) pvimageid=world->map->imageid;
  world->state.mapid=mapid;
  world->map=map;
  
  /* Flush cells, in case they were dirty from last time.
   */
  memcpy(map->v,map->ov,map->w*map->h);
  
  /* Defunct all the sprites, but if there's a hero sprite, keep it and do not defunct.
   * It's a bit heavy-handed to defunct instead of deleting, but I'm doing that in the interest of safety.
   * This way, a map change could be triggered by any sprite's update, and there's no danger of deleting the sprite.
   * But realistically, it's hard to imagine map changes being triggered by anyone but the hero, which is managed special regardless.
   */
  struct sprite *hero=0;
  int i=world->spritec;
  while (i-->0) {
    struct sprite *sprite=world->spritev[i];
    if (!hero&&(sprite->type=&sprite_type_hero)) hero=sprite;
    else sprite->defunct=1;
  }
  
  /* Apply map commands.
   * We'll do a second pass after, for flag-related commands.
   */
  world->poic=0;
  struct rom_command_reader reader={.v=map->cmdv,.c=map->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_map_hero: {
          if (!hero) {
            double x=cmd.argv[0]+0.5;
            double y=cmd.argv[1]+0.5;
            hero=sprite_new(&sprite_type_hero,0,0,0,x,y,0);
          }
        } break;
      case CMD_map_sprite: {
          uint8_t x=cmd.argv[0];
          uint8_t y=cmd.argv[1];
          uint16_t spriteid=(cmd.argv[2]<<8)|cmd.argv[3];
          uint32_t arg=(cmd.argv[4]<<24)|(cmd.argv[5]<<16)|(cmd.argv[6]<<8)|cmd.argv[7];
          struct sprite *sprite=sprite_new_res(spriteid,x,y,arg);
        } break;
      case CMD_map_door: // All POI commands with position in the first two bytes.
      case CMD_map_toggle:
        if (world->poic<POI_LIMIT) {
          uint8_t x=cmd.argv[0];
          uint8_t y=cmd.argv[1];
          int p=world_poi_search(world,x,y);
          if (p<0) {
            p=-p-1;
            memmove(world->poiv+p+1,world->poiv+p,sizeof(struct poi)*(world->poic-p));
            world->poic++;
            world->poiv[p]=(struct poi){x,y};
          }
        } break;
    }
  }
  
  /* Poke the song, sort the sprite list.
   * This is a good place for miscellaneous "starting a new map" stuff.
   */
  world_apply_map_flags(world);
  egg_play_song(map->songid,0,1);
  world_sort_sprites_fully(world);
  
  return 0;
}

/* Get hero.
 */
 
struct sprite *world_get_hero(const struct world *world) {
  if (!world) return 0;
  int i=world->spritec;
  struct sprite **p=world->spritev;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->type==&sprite_type_hero) return sprite;
  }
  return 0;
}

/* Access to flags.
 */
 
int world_get_flag(struct world *world,int flag) {
  if ((flag<0)||(flag>=FLAG_COUNT)) return 0;
  return (world->state.flags[flag>>3]&(1<<(flag&7)))?1:0;
}

int world_set_flag(struct world *world,int flag,int v) {
  if ((flag<0)||(flag>=FLAG_COUNT)) return 0;
  uint8_t *dst=world->state.flags+(flag>>3);
  uint8_t mask=1<<(flag&7);
  if (v) {
    if ((*dst)&mask) return 0;
    (*dst)|=mask;
  } else {
    if (!((*dst)&mask)) return 0;
    (*dst)&=~mask;
  }
  world->state_dirty=1;
  return 1;
}

/* Apply flags.
 */
 
void world_apply_map_flags(struct world *world) {
  struct rom_command_reader reader={.v=world->map->cmdv,.c=world->map->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_map_flagtile: {
          uint8_t x=cmd.argv[0];
          uint8_t y=cmd.argv[1];
          uint16_t flag=(cmd.argv[2]<<8)|cmd.argv[3];
          if ((x<world->map->w)&&(y<world->map->h)) {
            int p=y*world->map->w+x;
            world->map->v[p]=world->map->ov[p];
            if (world_get_flag(world,flag)) world->map->v[p]++;
          }
        } break;
    }
  }
}

/* Search POI.
 */
 
int world_poi_search(const struct world *world,int x,int y) {
  int lo=0,hi=world->poic;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct poi *q=world->poiv+ck;
         if (y<q->y) hi=ck;
    else if (y>q->y) lo=ck+1;
    else if (x<q->x) hi=ck;
    else if (x>q->x) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}
