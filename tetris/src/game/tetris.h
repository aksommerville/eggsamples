#ifndef TETRIS_H
#define TETRIS_H

#define FBW 640
#define FBH 360

#define FIELDW 10
#define FIELDH 20

#define PLAYER_LIMIT 8

#define HORZ_REPEAT_TIME 0.125
#define FAST_FALL_TIME 0.040

#include "egg/egg.h"
#include "egg_rom_toc.h"
#include "shared_symbols.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "opt/text/text.h"
#include "bag.h"
#include "field.h"

extern struct g {
  void *rom;
  int romc;
  int texid_tiles;
  struct font *font;
  struct graf graf;
  struct texcache texcache;
  int fieldc; // 1,2: How many fields to render. Modes should try to support both.
  int running;
  struct field l,r; // Game models.
} g;

// render.c
void render();

#endif
