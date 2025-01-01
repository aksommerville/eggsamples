#ifndef SNAKE_H
#define SNAKE_H

#include "egg/egg.h"
#include "egg_rom_toc.h"
#include "shared_symbols.h"

// Must agree with metadata. We validate at init.
#define FBW 248
#define FBH 168

// Play field size in tiles. n*NS_sys_tilesize must be less than (FBW,FBH), and they should both be odd.
#define COLC 15
#define ROWC  9

// Transforms, for a given direction toward the head.
#define XF_W 0
#define XF_E (EGG_XFORM_XREV|EGG_XFORM_YREV)
#define XF_N (EGG_XFORM_SWAP|EGG_XFORM_YREV)
#define XF_S (EGG_XFORM_SWAP|EGG_XFORM_XREV)

// Tileid.
#define TILE_SNAK 1
#define TILE_HEAD 2
#define TILE_BODY 3
#define TILE_TAIL 4
#define TILE_CLOK 5
#define TILE_DEAS 6

extern struct g {
  int pvinput;
  int texid; // 1-tiles; all our graphics are in a single tilesheet
  
  /* When the game is running, there will always be at least 2 snegments.
   * [0] is the head, and [c-1] the tail.
   * Each snegment after the head is exactly one cardinal space from its predecessor.
   */
  struct snegment {
    uint8_t x,y;
    uint8_t tileid;
    uint8_t xform;
  } snegmentv[COLC*ROWC];
  int snegmentc;
  uint8_t snakx,snaky; // OOB for none.
  double motion_clock; // Counts up. Snake moves when it reaches (tick_dur).
  double tick_dur; // Snake moves every so many seconds.
  int running;
  double clock; // Total runtime, resets with game.
} g;

// rand.c
void srand_auto();
int rand();

// snake.c
void snake_init();
void snake_reset();
void snake_move(int dx,int dy); // For immediate moves. Not necessary for scheduled moves.
void snake_update(double elapsed);

#endif
