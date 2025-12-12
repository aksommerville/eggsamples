#ifndef RPG_H
#define RPG_H

#define FBW 320
#define FBH 180

/* Size of the small animated view in battle.
 * You must supply backgrounds at this size.
 * The exact measurements of battle view are largely dependent on these.
 * TOP_H pushes the fighter images down somewhat, if you have a horizon. Zero is fine to balance vertically.
 */
#define BATTLE_SCENE_W 128
#define BATTLE_SCENE_H 80
#define BATTLE_SCENE_TOP_H 20

struct world; // game/world/world.h

#include "egg/egg.h"
#include "util/stdlib/egg-stdlib.h"
#include "util/graf/graf.h"
#include "util/font/font.h"
#include "util/res/res.h"
#include "util/text/text.h"
#include "egg_res_toc.h"
#include "shared_symbols.h"
#include "layer/layer.h"

extern struct g {
  struct graf graf;
  struct font *font;
  int pvinput;
  int framec; // Global render count, for cheap animations. Where it's worth being correct, use a private game-time clock.
  int song_playing;
  
  struct layer **layerv;
  int layerc,layera;
  int layerdirty;
  struct layer *pvfocus; // WEAK and possibly nonresident
  
  struct world *world; // Present during gameplay (even non-world modes like combat). Null at main menu.
} g;

/* Global resource TOC.
 * We track certain resource types (not all of them).
 * Maps and tilesheets are partly decoded during load.
 */
int rpg_res_init();
int rpg_res_get(void *dstpp,int tid,int rid);
struct map *rpg_map_get(int rid);

/* Helpers for scalar direction, converting to and from deltas.
 */

#define DIR_NW 0x80
#define DIR_N  0x40
#define DIR_NE 0x20
#define DIR_W  0x10
#define DIR_E  0x08
#define DIR_SW 0x04
#define DIR_S  0x02
#define DIR_SE 0x01
static inline void delta_from_dir(int *dx,int *dy,uint8_t dir) {
  switch (dir&(DIR_W|DIR_E)) {
    case DIR_W: *dx=-1; break;
    case DIR_E: *dx=1; break;
    default: *dx=0;
  }
  switch (dir&(DIR_N|DIR_S)) {
    case DIR_N: *dy=-1; break;
    case DIR_S: *dy=1; break;
    default: *dy=0;
  }
}
static inline uint8_t dir_from_delta(int dx,int dy) {
  uint8_t dir=0;
  if (dx<0) dir|=DIR_W; else if (dx>0) dir|=DIR_E;
  if (dy<0) dir|=DIR_N; else if (dy>0) dir|=DIR_S;
  return dir;
}

void rpg_song(int rid);
void rpg_sound(int rid);

#endif
