#ifndef RPG_H
#define RPG_H

#define FBW 320
#define FBH 180

struct world; // game/world/world.h

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "opt/text/text.h"
#include "egg_rom_toc.h"
#include "shared_symbols.h"
#include "layer/layer.h"

extern struct g {
  void *rom;
  int romc;
  struct graf graf;
  struct texcache texcache;
  struct font *font;
  int pvinput;
  
  struct layer **layerv;
  int layerc,layera;
  int layerdirty;
  struct layer *pvfocus; // WEAK and possibly nonresident
  
  struct world *world; // Present during gameplay (even non-world modes like combat). Null at main menu.
} g;

/* Helpers for scalar direction, converting to and from deltas.
 */

#define DIR_N 0x40
#define DIR_W 0x10
#define DIR_E 0x08
#define DIR_S 0x02
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

#endif
