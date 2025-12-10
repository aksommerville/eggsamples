#ifndef SWEEP_H
#define SWEEP_H

#include "egg/egg.h"
#include "util/stdlib/egg-stdlib.h"
#include "util/graf/graf.h"
#include "util/font/font.h"
#include "egg_res_toc.h"
#include "shared_symbols.h"

/* >0 to let the CPU play; it's the interval between plays.
 * Player can still interfere.
 */
#define AUTOPLAY 0.0

#define FBW 320
#define FBH 248

/* For now at least, we're not doing adjustable difficulty.
 * The field size and egg count are constant.
 * On top of that, the field is expected to exactly cover the framebuffer.
 */
#define COLC 20
#define ROWC 15
#define EGGC 40

/* The "HIDDEN" and "FLAG" tiles are duplicated, so we can use tileid for everything -- it records whether an egg is actually there or not.
 */
#define TILE_HIDDEN_EMPTY 0x00
#define TILE_HIDDEN_EGG   0x01
#define TILE_FLAG_EMPTY   0x02
#define TILE_FLAG_EGG     0x03
#define TILE_EXPOSE_0     0x04
#define TILE_EXPOSE_1     0x05
#define TILE_EXPOSE_2     0x06
#define TILE_EXPOSE_3     0x07
#define TILE_EXPOSE_4     0x08
#define TILE_EXPOSE_5     0x09
#define TILE_EXPOSE_6     0x0a
#define TILE_EXPOSE_7     0x0b
#define TILE_EXPOSE_8     0x0c
#define TILE_FALSE_FLAG   0x0d
#define TILE_EGGSPLODE    0x0e
#define TILE_CURSOR       0x0f

extern struct g {
  void *rom;
  int romc;
  struct graf graf;
  struct font *font;
  int pvinput;
  uint8_t map[COLC*ROWC];
  int selx,sely;
  int cursorframe; // 0..3
  double cursorclock;
  int running;
  int suspend;
  int victory;
  int flagc;
  double autoplay_clock; // Counts up.
  int song_playing;
} g;

// sweep.c
void sweep_reset();
void sweep_move(int dx,int dy);
void sweep_expose();
void sweep_flag(); // toggle
void sweep_lose();
void sweep_expose_empty_tile(uint8_t *map,int x,int y);

/* autosolve.c
 * Returns one of:
 *  - COLC*ROWC: Not solvable.
 *  - -COLC*ROWC-1: Invalid. Only happens in certain false-flag cases.
 *  - COLC*ROWC+1: Already solved.
 *  - 0..(COLC*ROWC-1): Expose a cell.
 *  - -1..(-COLC*ROWC): Flag cell (-n-1).
 * Does not read or modify any globals, or modify the provided map.
 */
int autosolve(const uint8_t *map);

/* autosolve.c
 * When the map is unsolvable and (x,y) is TILE_HIDDEN_EGG,
 * look for somewhere we can move this egg to, such that all exposed clues remain valid.
 * If that's possible, do it and return >=0.
 * <0 if we can't.
 */
int autosolve_repair(int x,int y);

void sweep_song(int rid);
void sweep_sound(int rid);

#endif
