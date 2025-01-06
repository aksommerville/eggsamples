#ifndef TETRIS_H
#define TETRIS_H

#define FBW 640
#define FBH 360

#define FIELDW 10
#define FIELDH 20

#define PLAYER_LIMIT 8

#define FAST_FALL_TIME 0.040

#define SCORE_LIMIT 999999

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
  struct cursor { // Half-assed menu model.
    int pvinput;
    int p; // 0..9=left, 10..19=right, 20=fieldc-L, 21=fieldc-R
    int decline; // Pressed B to opt out.
    uint32_t color;
  } cursorv[8];
  double input_blackout;
} g;

// render.c
void render();

// db.c
void db_init();
int db_add(int playerc,int linec,int score);
int db_modseq(); // => Changes whenever the internal state changes. Zero only if no scores loaded.

/* Populate up to (dsta), sorted by (score) descending.
 * This interleaves playerc buckets as needed.
 * Never returns >dsta or <0.
 */
struct db_score {
  int playerc,linec,score,year,month,day,hour,minute;
};
int db_get_scores(struct db_score *dst,int dsta);

#endif
