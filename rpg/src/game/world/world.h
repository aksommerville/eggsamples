/* world.h
 * World represents one session of play.
 * One exists pretty much any time you're not at the main menu.
 * This may have been reconstituted from a saved state or initialized from scratch.
 * I've encapsulated it with an object context you pass around, but in fact it's always a global (g.world).
 */
 
#ifndef WORLD_H
#define WORLD_H

struct map;
struct sprite;

/* Doors, treadles, any map command that can be actuated by hero movement.
 * You can't exceed this arbitrary limit per map.
 */
#define POI_LIMIT 128

/* Everything that gets saved, in the exact shape of the save.
 */
struct world_state {
  uint16_t mapid;
  uint8_t herox,heroy;
  uint16_t hp;
  uint16_t hpmax;
  uint16_t xp;
  uint16_t gold;
  // Inventory, etc...
  uint8_t flags[FLAG_COUNT];
};

struct world {
  int save_result;
  struct world_state state;
  struct map *map;
  int16_t camx,camy,camw,camh; // Camera bounds in world pixels. May be smaller than framebuffer.
  struct sprite **spritev;
  int spritec,spritea;
  int sprite_sort_dir;
  int state_dirty;
  struct poi { uint8_t x,y; } poiv[POI_LIMIT];
  int poic;
};

void world_del(struct world *world);

/* We'll decode this save state and restore it.
 * If anything goes wrong with that, we'll initialize as a new game.
 * The curious may examine (world->save_result) after construction:
 *  <0 A non-empty save was supplied but failed to decode. We're a New Game.
 *   0 New Game due to empty or default save.
 *  >0 Saved game was restored.
 */
struct world *world_new(const char *save,int savec);

/* A new world does not have any map loaded yet.
 * Call world_start() right after world_new, to load the first map and build up the initial model.
 */
int world_start(struct world *world);

/* Maintenance hooks designed to match layer.
 * At render, we overwrite the entire framebuffer.
 */
void world_suspend(struct world *world);
void world_resume(struct world *world,int state);
void world_input(struct world *world,int btnid,int value,int state);
void world_update(struct world *world,double elapsed);
void world_render(struct world *world);

int world_load_map(struct world *world,int mapid);

void world_sort_sprites_fully(struct world *world);

struct sprite *world_get_hero(const struct world *world);

/* Setting a flag returns nonzero if changed.
 * It does not apply to the map. Caller should call world_apply_map_flags() after changing flags.
 * It does set world->state_dirty if appropriate.
 */
int world_get_flag(struct world *world,int flag);
int world_set_flag(struct world *world,int flag,int v);
void world_apply_map_flags(struct world *world);

/* Returns >=0 if there is a command at (x,y).
 * We don't record the command itself here; you'll have to scan for it.
 */
int world_poi_search(const struct world *world,int x,int y);

#endif
