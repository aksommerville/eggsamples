#include "sweep.h"

/* Encode map to a simple text string, so we can replay later for troubleshooting.
 */
 
#define ENCODED_MAP_LENGTH ((COLC*ROWC+5)/6)
 
static void sweep_explicit_map(const char *src) {
  const char *src0=src;
  memset(g.map,TILE_HIDDEN_EMPTY,COLC*ROWC);
  uint8_t *dst=g.map;
  int i=COLC*ROWC;
  uint8_t buf,mask;
  #define NEXT { \
         if ((*src>='A')&&(*src<='Z')) buf=(*src)-'A'; \
    else if ((*src>='a')&&(*src<='z')) buf=(*src)-'a'+26; \
    else if ((*src>='0')&&(*src<='9')) buf=(*src)-'0'+52; \
    else if (*src=='_') buf=62; \
    else if (*src=='-') buf=63; \
    else return; \
    src++; \
    mask=1; \
  }
  NEXT
  int eggc=0;
  for (;i-->0;dst++,mask<<=1) {
    if (mask>=0x40) NEXT
    if (buf&mask) {
      *dst=TILE_HIDDEN_EGG;
      eggc++;
    }
  }
  #undef NEXT
  fprintf(stderr,"Restored map: %s ; eggc=%d\n",src0,eggc);
}

static void sweep_log_map() {
  char encoded[ENCODED_MAP_LENGTH];
  int encodedc=0;
  uint8_t buf=0,mask=1;
  const uint8_t *v=g.map;
  int i=COLC*ROWC;
  for (;i-->0;v++) {
    if (*v==TILE_HIDDEN_EGG) buf|=mask;
    if ((mask<<=1)>=0x40) {
      if (encodedc>=sizeof(encoded)) {
        fprintf(stderr,"%s:%d: ENCODED_MAP_LENGTH too small!\n",__FILE__,__LINE__);
        return;
      }
      encoded[encodedc++]=
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789_-"[buf];
      buf=0;
      mask=1;
    }
  }
  if ((mask!=1)&&(encodedc<sizeof(encoded))) {
    encoded[encodedc++]=
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789_-"[buf];
  }
  fprintf(stderr,"%s: %.*s\n",__func__,encodedc,encoded);
}

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
  if (0) { // For debugging, enable explicit map here.
    // iMQAASACACAIAABoAwEAsBQBAAAAAIIEAAHABEyBggCAAAAIEg : autosolve started in the upper-right, flagged one egg, then gave up. ...fixed
    // gAAEAIAEoAAEgJAACVQgRJAEQAAkoAABDCBAAAAAIACAgIQgoQ : Needs to guess immediately.
    // IIAQAACgAQgAFxIzAEoAAAEABQYAAAAAEiCAAAAAEAgGAAGTiA : Solvable but autosolve fails ...fixed
    // AGABFBAggA4IwAABBERAAARCFgAAAAAEAoCAAEAQKAQoEBAAIQ : Autosolve places an incorrect flag at 6,0 ...fixed
    // CACIEIGAkIAAAJEAigAEAASUAgIgIAgAABAUAAEEBQKICCBAQA : UNSOLVABLE? Autosolve incorrectly determines the upper-right to be unsolvable. It's solvable in light of the flag count.
    //                                                      The lower region is a classic unsolvable foursquare.
    //                                                      !!! If I solve the upper part first, then guess wrong, we think the bottom is solvable (and we would have played wrong on it).
    //                                                      ...fixed
    //                                                      Still not solving every case but I think it's close enough. Undersolving works to the players' benefit.
    //                                                      Now we need autosolve_repair() to work for these weird cases, still a fair test case for it. ...fixed
    // gAAAIAggAAh4AAQQEgHBBIAAEBAgAkAoIQAAAkgAAQAoBQAANB : Finished with one egg still listed pending. And automation fails. Replayed and it was fine. Huh?
    sweep_explicit_map("gAAAIAggAAh4AAQQEgHBBIAAEBAgAkAoIQAAAkgAAQAoBQAANB");
  } else {
    memset(g.map,TILE_HIDDEN_EMPTY,COLC*ROWC);
    int i=EGGC; while (i-->0) sweep_add_random_egg();
    sweep_log_map();
  }
  //int eggc=0,i=COLC*ROWC; while (i-->0) if (g.map[i]==TILE_HIDDEN_EGG) eggc++;
  //fprintf(stderr,"I count %d eggs.\n",eggc);
  g.selx=COLC>>1;
  g.sely=ROWC>>1;
  g.flagc=0;
  g.running=1;
  g.suspend=0;
  g.victory=0;
  g.autoplay_clock=0.0;
  egg_play_song(RID_song_around_here_somewhere,0,1);
}

/* Lose game: Reveal all of the false flags and missed eggs, and set g.running false.
 */
 
void sweep_lose() {
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
  egg_play_song(RID_song_eternal_torment,0,1);
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
  egg_play_song(RID_song_hold_your_fire,0,1);
}

/* How many eggs adjacent to this cell? 0..8, no errors
 */
 
static int sweep_count_eggs(const uint8_t *map,int x,int y) {
  int c=0;
  int dx=-1; for (;dx<=1;dx++) {
    int ox=x+dx;
    if ((ox<0)||(ox>=COLC)) continue;
    int dy=-1; for (;dy<=1;dy++) {
      if (!dx&&!dy) continue;
      int oy=y+dy;
      if ((oy<0)||(oy>=ROWC)) continue;
      int p=oy*COLC+ox;
      switch (map[p]) {
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
 
void sweep_expose_empty_tile(uint8_t *map,int x,int y) {
  if ((x<0)||(y<0)||(x>=COLC)||(y>=ROWC)) return;
  int p=y*COLC+x;
  if (map[p]!=TILE_HIDDEN_EMPTY) return;
  int eggc=sweep_count_eggs(map,x,y);
  map[p]=TILE_EXPOSE_0+eggc;
  if (!eggc) {
    int dx=-1; for (;dx<=1;dx++) {
      int dy=-1; for (;dy<=1;dy++) {
        if (dx||dy) sweep_expose_empty_tile(map,x+dx,y+dy);
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
    case TILE_HIDDEN_EMPTY: egg_play_sound(RID_sound_expose); sweep_expose_empty_tile(g.map,g.selx,g.sely); break;
    case TILE_HIDDEN_EGG: {
        int solution=autosolve(g.map);
        if (solution==COLC*ROWC) {
          if (autosolve_repair(g.selx,g.sely)>=0) {
            egg_play_sound(RID_sound_expose);
            sweep_expose_empty_tile(g.map,g.selx,g.sely);
            return;
          }
        }
        egg_play_sound(RID_sound_eggsplode);
        sweep_lose();
      } break;
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
