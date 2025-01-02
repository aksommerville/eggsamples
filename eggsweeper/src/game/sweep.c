#include "sweep.h"

/* Put an egg somewhere.
 */
 
static void sweep_add_random_egg() {
  int panic=1000;
  while (panic-->0) {
    int p=rand()%(COLC*ROWC);
    if (g.map[p]==TILE_HIDDEN_EMPTY) {
      g.map[p]=TILE_HIDDEN_EGG;
      return;
    }
  }
}

/* Reset.
 */
 
void sweep_reset() {
  memset(g.map,TILE_HIDDEN_EMPTY,COLC*ROWC);
  int i=EGGC; while (i-->0) sweep_add_random_egg();
  g.selx=COLC>>1;
  g.sely=ROWC>>1;
  g.flagc=0;
  g.running=1;
  g.victory=0;
}

/* Lose game: Reveal all of the false flags and missed eggs, and set g.running false.
 */
 
static void sweep_lose() {
  g.running=0;
  g.victory=0;
  uint8_t *v=g.map;
  int i=COLC*ROWC;
  for (;i-->0;v++) {
    switch (*v) {
      case TILE_HIDDEN_EGG: *v=TILE_EGGSPLODE; break;
      case TILE_FLAG_EMPTY: *v=TILE_FALSE_FLAG; break;
    }
  }
}

/* Review map, after a flag change.
 * If every egg is flagged, and no false flags, terminate.
 */
 
static void sweep_check_victory() {
  const uint8_t *v=g.map;
  int i=COLC*ROWC;
  for (;i-->0;v++) {
    switch (*v) {
      case TILE_HIDDEN_EGG:
      case TILE_FLAG_EMPTY:
        return;
    }
  }
  egg_play_sound(RID_sound_win);
  g.running=0;
  g.victory=1;
}

/* How many eggs adjacent to this cell? 0..8, no errors
 */
 
static int sweep_count_eggs(int x,int y) {
  int c=0;
  int dx=-1; for (;dx<=1;dx++) {
    int ox=x+dx;
    if ((ox<0)||(ox>=COLC)) continue;
    int dy=-1; for (;dy<=1;dy++) {
      if (!dx&&!dy) continue;
      int oy=y+dy;
      if ((oy<0)||(oy>=ROWC)) continue;
      int p=oy*COLC+ox;
      switch (g.map[p]) {
        case TILE_HIDDEN_EGG:
        case TILE_FLAG_EGG:
        case TILE_EGGSPLODE:
          c++;
          break;
      }
    }
  }
  return c;
}

/* Expose an empty tile, and if it's a zero, expose its neighbors recursively.
 * Safe to call on OOB on not-exposable tiles.
 */
 
static void sweep_expose_empty_tile(int x,int y) {
  if ((x<0)||(y<0)||(x>=COLC)||(y>=ROWC)) return;
  int p=y*COLC+x;
  if (g.map[p]!=TILE_HIDDEN_EMPTY) return;
  int eggc=sweep_count_eggs(x,y);
  g.map[p]=TILE_EXPOSE_0+eggc;
  if (!eggc) {
    int dx=-1; for (;dx<=1;dx++) {
      int dy=-1; for (;dy<=1;dy++) {
        if (dx||dy) sweep_expose_empty_tile(x+dx,y+dy);
      }
    }
  }
}

/* Move cursor.
 */

void sweep_move(int dx,int dy) {
  int ok=1;
  g.selx+=dx;
  g.sely+=dy;
  if (g.selx<0) {
    ok=0;
    g.selx=0;
  } else if (g.selx>=COLC) {
    ok=0;
    g.selx=COLC-1;
  }
  if (g.sely<0) {
    ok=0;
    g.sely=0;
  } else if (g.sely>=ROWC) {
    ok=0;
    g.sely=ROWC-1;
  }
  if (ok) egg_play_sound(RID_sound_move);
  else egg_play_sound(RID_sound_reject);
}

/* Expose tile.
 */
 
void sweep_expose() {
  switch (g.map[g.sely*COLC+g.selx]) {
    case TILE_HIDDEN_EMPTY: egg_play_sound(RID_sound_expose); sweep_expose_empty_tile(g.selx,g.sely); break;
    case TILE_HIDDEN_EGG: egg_play_sound(RID_sound_eggsplode); sweep_lose(); break;
    default: egg_play_sound(RID_sound_reject);
  }
}

/* Toggle flag.
 */
 
void sweep_flag() {
  int p=g.sely*COLC+g.selx;
  switch (g.map[p]) {
    case TILE_HIDDEN_EMPTY: egg_play_sound(RID_sound_flag); g.map[p]=TILE_FLAG_EMPTY; g.flagc++; break;
    case TILE_HIDDEN_EGG: egg_play_sound(RID_sound_flag); g.map[p]=TILE_FLAG_EGG; g.flagc++; sweep_check_victory(); break;
    case TILE_FLAG_EMPTY: egg_play_sound(RID_sound_unflag); g.map[p]=TILE_HIDDEN_EMPTY; g.flagc--; sweep_check_victory(); break;
    case TILE_FLAG_EGG: egg_play_sound(RID_sound_unflag); g.map[p]=TILE_HIDDEN_EGG; g.flagc--; break;
    default: egg_play_sound(RID_sound_reject);
  }
}
