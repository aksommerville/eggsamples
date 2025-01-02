#ifndef SWEEP_H
#define SWEEP_H

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "opt/text/text.h"
#include "egg_rom_toc.h"
#include "shared_symbols.h"

#define FBW 320
#define FBH 240

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
  struct texcache texcache;
  struct font *font;
  int pvinput;
  uint8_t map[COLC*ROWC];
  int selx,sely;
  int cursorframe; // 0..3
  double cursorclock;
  int running;
} g;

// sweep.c
void sweep_reset();
void sweep_move(int dx,int dy);
void sweep_expose();
void sweep_flag(); // toggle

#endif
