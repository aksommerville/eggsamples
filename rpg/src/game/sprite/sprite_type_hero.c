/* sprite_type_hero.c
 */
 
#include "game/rpg.h"
#include "game/world/world.h"
#include "game/world/map.h"
#include "sprite.h"
#include "opt/rom/rom.h"

#define HERO_WALK_IMPULSE_BLACKOUT_TIME 0.100
#define HERO_BUMP_REJECT_TIME 0.250
#define HERO_BUMP_REJECT_DISTANCE 2 /* pixels */

/* Object definition.
 */
 
struct sprite_hero {
  struct sprite hdr;
  int indx,indy; // dpad state
  uint8_t facedir;
  double animclock;
  int animframe;
  int col,row; // Position in discrete map space. Updates before animation.
  uint8_t walkdir; // Nonzero when walking.
  double walk_impulse_blackout; // Brief delay after pressing dpad, to allow changing direction without moving.
  double bump_reject_clock; // Counts down while reacting to having walked into a wall.
  int boat; // Nonzero if we're in a boat.
};

#define SPRITE ((struct sprite_hero*)sprite)

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {
  sprite->imageid=RID_image_sprites1;
  sprite->tileid=0x00;
  sprite->xform=0;
  SPRITE->facedir=DIR_S;
  SPRITE->col=(int)sprite->x;
  SPRITE->row=(int)sprite->y;
  
  // Start in the boat, if our initial position is water or dock.
  if ((SPRITE->col>=0)&&(SPRITE->row>=0)&&(SPRITE->col<g.world->map->w)&&(SPRITE->row<g.world->map->h)) {
    switch (g.world->map->physics[g.world->map->v[SPRITE->row*g.world->map->w+SPRITE->col]]) {
      case NS_physics_dock:
      case NS_physics_water: {
          SPRITE->boat=1;
        } break;
    }
  }
  
  return 0;
}

/* Find commands for the given cell and trigger them.
 * (how) is 'b'=bump or 't'=treadle.
 */
 
static void hero_trigger_poi(struct sprite *sprite,int x,int y,char how) {
  struct rom_command_reader reader={.v=g.world->map->cmdv,.c=g.world->map->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    // Every POI command has its position in the first two bytes.
    if ((cmd.argc<2)||(cmd.argv[0]!=x)||(cmd.argv[1]!=y)) continue;
    switch (cmd.opcode) {
      case CMD_map_toggle: {
          int flag=(cmd.argv[2]<<8)|cmd.argv[3];
          int v=world_get_flag(g.world,flag)?0:1;
          egg_play_sound((how=='b')?RID_sound_switch:RID_sound_treadle);
          if (world_set_flag(g.world,flag,v)) world_apply_map_flags(g.world);
        } break;
      case CMD_map_door: if (how=='t') {
          fprintf(stderr,"TODO Enter door at (%d,%d)\n",x,y);//TODO
        } break;
    }
  }
}

/* End of step.
 * This doesn't necessarily mean she's stopped walking.
 * It will fire for each cell along the path.
 */
 
static void hero_step_completed(struct sprite *sprite) {
  if (world_poi_search(g.world,SPRITE->col,SPRITE->row)>=0) {
    hero_trigger_poi(sprite,SPRITE->col,SPRITE->row,'t');
  }
  //TODO Random battles.
}

/* Start of step.
 */
 
static void hero_step_started(struct sprite *sprite) {
}

/* Consider starting a step to (x,y).
 * If there's some other thing that should happen immediately upon starting travel to a cell, call it out here.
 * I think most map triggers will prefer to happen at hero_step_completed() instead.
 */
 
#define HERO_STEP_BLOCKED 0
#define HERO_STEP_WALK 1
#define HERO_STEP_BOAT 2
 
static int hero_consider_step(struct sprite *sprite,int x,int y) {

  // Walking OOB is never allowed.
  if ((x<0)||(y<0)||(x>=g.world->map->w)||(y>=g.world->map->h)) return HERO_STEP_BLOCKED;
  
  // Check map physics.
  int fromp=SPRITE->row*g.world->map->w+SPRITE->col;
  uint8_t fromtileid=g.world->map->v[fromp];
  uint8_t fromphysics=g.world->map->physics[fromtileid];
  int p=y*g.world->map->w+x;
  uint8_t tileid=g.world->map->v[p];
  uint8_t physics=g.world->map->physics[tileid];
  switch (physics) {
    // Dock is always allowed:
    case NS_physics_dock:
      break;
    // Water and vacant (or similar) are allowed from themselves, and from the dock:
    case NS_physics_vacant: switch (fromphysics) {
        case NS_physics_vacant:
        case NS_physics_dock:
          break;
        default: return HERO_STEP_BLOCKED;
      } break;
    case NS_physics_water: switch (fromphysics) {
        case NS_physics_water:
        case NS_physics_dock:
          break;
        default: return HERO_STEP_BLOCKED;
      } break;
    // Some are never allowed:
    case NS_physics_solid:
    default:
      return HERO_STEP_BLOCKED;
  }
  
  // Check solid sprites.
  struct sprite **spritep=g.world->spritev;
  int i=g.world->spritec;
  for (;i-->0;spritep++) {
    struct sprite *other=*spritep;
    if (!other->solid) continue;
    if (other==sprite) continue;
    int ox=(int)other->x,oy=(int)other->y;
    if ((ox!=x)||(oy!=y)) continue;
    if (other->type->bump) other->type->bump(other,sprite);
    return HERO_STEP_BLOCKED;
  }

  // It's allowed. Tell the caller whether we're a boat or a man.
  switch (physics) {
    case NS_physics_water:
    case NS_physics_dock:
      return HERO_STEP_BOAT;
  }
  return HERO_STEP_WALK;
}

/* Update walking.
 */
 
static void hero_update_walk(struct sprite *sprite,double elapsed) {

  /* If a bump reject is in progress, that supercedes walking.
   */
  if (SPRITE->bump_reject_clock>0.0) {
    if ((SPRITE->bump_reject_clock-=elapsed)>0.0) {
      return;
    }
    SPRITE->walkdir=0;
  }

  /* When we're already walking, continue until we reach the target.
   * Note that (walkdir) and (facedir) are independent: You can Moonwalk your way to the end of a step.
   * It's also independent of (indx,indy): Once a step is started, you have no choice but to finish it.
   */
  if (SPRITE->walkdir) {
    const double walkspeed=6.0; // m/s
    int dx,dy; delta_from_dir(&dx,&dy,SPRITE->walkdir);
    sprite->x+=elapsed*walkspeed*dx;
    sprite->y+=elapsed*walkspeed*dy;
    double tx=SPRITE->col+0.5,ty=SPRITE->row+0.5;
    switch (SPRITE->walkdir) {
      case DIR_W: if (sprite->x<=tx) { sprite->x=tx; SPRITE->walkdir=0; } break;
      case DIR_E: if (sprite->x>=tx) { sprite->x=tx; SPRITE->walkdir=0; } break;
      case DIR_N: if (sprite->y<=ty) { sprite->y=ty; SPRITE->walkdir=0; } break;
      case DIR_S: if (sprite->y>=ty) { sprite->y=ty; SPRITE->walkdir=0; } break;
      default: sprite->x=tx; sprite->y=ty; SPRITE->walkdir=0;
    }
    if (SPRITE->walkdir) return;
    hero_step_completed(sprite);
  }
  
  /* Not walking, or just completed. Should we start the next step?
   * Use facedir as the authority for direction, since it's possible for both axes to be nonzero.
   * Facedir is always the most recent dpad stroke.
   */
  if (!SPRITE->indx&&!SPRITE->indy) return;
  if (SPRITE->walk_impulse_blackout>0.0) {
    if ((SPRITE->walk_impulse_blackout-=elapsed)>0.0) return;
  }
  int dx,dy; delta_from_dir(&dx,&dy,SPRITE->facedir);
  int nx=SPRITE->col+dx;
  int ny=SPRITE->row+dy;
  switch (hero_consider_step(sprite,nx,ny)) {
    case HERO_STEP_WALK: {
        SPRITE->boat=0;
        SPRITE->col+=dx;
        SPRITE->row+=dy;
        SPRITE->walkdir=SPRITE->facedir;
        hero_step_started(sprite);
      } break;
    case HERO_STEP_BOAT: {
        SPRITE->boat=1;
        SPRITE->col+=dx;
        SPRITE->row+=dy;
        SPRITE->walkdir=SPRITE->facedir;
        hero_step_started(sprite);
      } break;
    default: {
        if (world_poi_search(g.world,nx,ny)>=0) {
          hero_trigger_poi(sprite,nx,ny,'b');
        } else {
          egg_play_sound(RID_sound_bump);
        }
        SPRITE->bump_reject_clock=HERO_BUMP_REJECT_TIME;
        SPRITE->walkdir=SPRITE->facedir;
      }
  }
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {

  // Four frames of animation, whether we need them or not.
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.200;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
  }
  
  // Walk if we're doing that.
  hero_update_walk(sprite,elapsed);

  // Update face.XXX do this during render, it's going to be complex
  switch (SPRITE->facedir) {
    case DIR_W: sprite->tileid=0x02; sprite->xform=0; break;
    case DIR_E: sprite->tileid=0x02; sprite->xform=EGG_XFORM_XREV; break;
    case DIR_N: sprite->tileid=0x01; sprite->xform=0; break;
    case DIR_S: sprite->tileid=0x00; sprite->xform=0; break;
  }
  if (SPRITE->walkdir&&(SPRITE->bump_reject_clock<=0.0)) {
    switch (SPRITE->animframe) {
      case 1: sprite->tileid+=0x10; break;
      case 3: sprite->tileid+=0x20; break;
    }
  }
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int16_t x,int16_t y) {
  int texid=texcache_get_image(&g.texcache,sprite->imageid);
  uint8_t tileid=sprite->tileid;
  uint8_t xform=sprite->xform;
  
  /* Shift a little during a bump.
   */
  if (SPRITE->bump_reject_clock>0.0) {
    double t=1.0-(SPRITE->bump_reject_clock/HERO_BUMP_REJECT_TIME);
    t*=2.0;
    t-=1.0;
    if (t<0.0) t+=1.0;
    else t=1.0-t;
    if ((t>=0.0)&&(t<=1.0)) {
      int dx,dy; delta_from_dir(&dx,&dy,SPRITE->walkdir);
      x+=dx*t*HERO_BUMP_REJECT_DISTANCE+0.5;
      y+=dy*t*HERO_BUMP_REJECT_DISTANCE+0.5;
    }
  }
  
  /* In the boat, we use a different face.
   */
  if (SPRITE->boat) {
    tileid=0x13;
    xform=0;
    if (SPRITE->animframe&2) tileid+=0x10;
  }
  
  graf_draw_tile(&g.graf,texid,x,y,tileid,xform);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .init=_hero_init,
  .update=_hero_update,
  .render=_hero_render,
};

/* Change to dpad.
 */
 
void hero_motion(struct sprite *sprite,int dx,int dy,int value) {
  if (value) {
    uint8_t pvfacedir=SPRITE->facedir;
    if (dx) { SPRITE->indx=dx; SPRITE->facedir=dir_from_delta(dx,0); }
    if (dy) { SPRITE->indy=dy; SPRITE->facedir=dir_from_delta(0,dy); }
    if (SPRITE->facedir!=pvfacedir) SPRITE->walk_impulse_blackout=HERO_WALK_IMPULSE_BLACKOUT_TIME;
  } else if (!dx&&!dy) {
    SPRITE->indx=SPRITE->indy=0;
    SPRITE->walk_impulse_blackout=0.0;
    SPRITE->bump_reject_clock=0.0;
  } else {
    if (dx==SPRITE->indx) { SPRITE->indx=0; if (SPRITE->indy) SPRITE->facedir=dir_from_delta(0,SPRITE->indy); }
    if (dy==SPRITE->indy) { SPRITE->indy=0; if (SPRITE->indx) SPRITE->facedir=dir_from_delta(SPRITE->indx,0); }
    if (!SPRITE->indx&&!SPRITE->indy) SPRITE->walk_impulse_blackout=0.0;
  }
}
