/* autosolve.c
 * A software player of minesweeper.
 * If this works, we'll use it to cheat around unsolvable puzzles.
 * In other words, if the player is forced to guess, she will always guess right.
 */

#include "sweep.h"

#define TILE_ASSUME_VACANT (TILE_EXPOSE_8+1) /* During brute force, assume there's no egg here but we don't know how many neighbor eggs */

int autosolve(const uint8_t *map);

/* Nonzero if there's at least one exposed neighbor to the given cell.
 */
 
static inline int cell_has_exposed_neighbors(int x,int y,const uint8_t *v) {
  #define CHECK(tileid) { if ((tileid>=TILE_EXPOSE_0)&&(tileid<=TILE_EXPOSE_8)) return 1; }
  if (x>0) {
    if (y>0) CHECK(v[-COLC-1])
    CHECK(v[-1])
    if (y<ROWC-1) CHECK(v[COLC-1])
  }
  if (y>0) CHECK(v[-COLC])
  if (y<ROWC-1) CHECK(v[COLC])
  if (x<COLC-1) {
    if (y>0) CHECK(v[-COLC+1])
    CHECK(v[1])
    if (y<ROWC-1) CHECK(v[COLC+1])
  }
  #undef CHECK
  return 0;
}

static inline int cell_has_hidden_neighbors(int x,int y,const uint8_t *v) {
  #define CHECK(tileid) { if ((tileid==TILE_HIDDEN_EMPTY)||(tileid==TILE_HIDDEN_EGG)) return 1; }
  if (x>0) {
    if (y>0) CHECK(v[-COLC-1])
    CHECK(v[-1])
    if (y<ROWC-1) CHECK(v[COLC-1])
  }
  if (y>0) CHECK(v[-COLC])
  if (y<ROWC-1) CHECK(v[COLC])
  if (x<COLC-1) {
    if (y>0) CHECK(v[-COLC+1])
    CHECK(v[1])
    if (y<ROWC-1) CHECK(v[COLC+1])
  }
  #undef CHECK
  return 0;
}

static inline int count_hidden_neighbors(int x,int y,const uint8_t *v) {
  int c=0;
  #define CHECK(tileid) { if ((tileid==TILE_HIDDEN_EMPTY)||(tileid==TILE_HIDDEN_EGG)) c++; }
  if (x>0) {
    if (y>0) CHECK(v[-COLC-1])
    CHECK(v[-1])
    if (y<ROWC-1) CHECK(v[COLC-1])
  }
  if (y>0) CHECK(v[-COLC])
  if (y<ROWC-1) CHECK(v[COLC])
  if (x<COLC-1) {
    if (y>0) CHECK(v[-COLC+1])
    CHECK(v[1])
    if (y<ROWC-1) CHECK(v[COLC+1])
  }
  #undef CHECK
  return c;
}

/* How many adjacent flags?
 */
 
static int count_flags(int x,int y,const uint8_t *v) {
  int c=0;
  #define CHECK(tileid) { if ((tileid==TILE_FLAG_EMPTY)||(tileid==TILE_FLAG_EGG)) c++; }
  if (x>0) {
    if (y>0) CHECK(v[-COLC-1])
    CHECK(v[-1])
    if (y<ROWC-1) CHECK(v[COLC-1])
  }
  if (y>0) CHECK(v[-COLC])
  if (y<ROWC-1) CHECK(v[COLC])
  if (x<COLC-1) {
    if (y>0) CHECK(v[-COLC+1])
    CHECK(v[1])
    if (y<ROWC-1) CHECK(v[COLC+1])
  }
  #undef CHECK
  return c;
}
 
static int count_adjacent_eggs(int x,int y,const uint8_t *v) {
  int c=0;
  #define CHECK(tileid) { if ((tileid==TILE_HIDDEN_EGG)||(tileid==TILE_FLAG_EGG)) c++; }
  if (x>0) {
    if (y>0) CHECK(v[-COLC-1])
    CHECK(v[-1])
    if (y<ROWC-1) CHECK(v[COLC-1])
  }
  if (y>0) CHECK(v[-COLC])
  if (y<ROWC-1) CHECK(v[COLC])
  if (x<COLC-1) {
    if (y>0) CHECK(v[-COLC+1])
    CHECK(v[1])
    if (y<ROWC-1) CHECK(v[COLC+1])
  }
  #undef CHECK
  return c;
}

/* Position of any hidden neighbor of (x,y).
 */
 
static int get_hidden_neighbor(const uint8_t *map,int x,int y) {
  int dy=-1; for (;dy<=1;dy++) {
    int oy=y+dy;
    if ((oy<0)||(oy>=ROWC)) continue;
    int dx=-1; for (;dx<=1;dx++) {
      int ox=x+dx;
      if ((ox<0)||(ox>=COLC)) continue;
      int op=oy*COLC+ox;
      if (map[op]==TILE_HIDDEN_EMPTY) return op;
      if (map[op]==TILE_HIDDEN_EGG) return op;
    }
  }
  return -1;
}

/* Component of speculate().
 * Check each cell in (refv) and make any mandatory changes to (map).
 * (flags_available) is in/out, we decrement if we place a new flag.
 * Returns 0 if the current map is invalid, or 1 if valid. (same semantics as speculate()).
 */

static int ramify_speculative_change(uint8_t *map,const int *refv,int refc,int *flags_available) {
 _again_:;
  int i=refc,modified=0;
  for (;i-->0;) {
    int p=refv[i];
    int x=p%COLC,y=p/COLC;
    int expectc=map[p]-TILE_EXPOSE_0;
    if ((expectc<1)||(expectc>8)) return 0;
    int actualc=count_flags(x,y,map+p);
    int needc=expectc-actualc;
    if (needc<0) return 0;
    if (needc>*flags_available) return 0;
    int freec=count_hidden_neighbors(x,y,map+p);
    if (needc>freec) return 0;
    if (needc==freec) { // We have exactly enough free space -- fill it.
      int dx=-1; for (;dx<=1;dx++) {
        int ox=x+dx;
        if ((ox<0)||(ox>=COLC)) continue;
        int dy=-1; for (;dy<=1;dy++) {
          int oy=y+dy;
          if ((oy<0)||(oy>=ROWC)) continue;
          int op=oy*COLC+ox;
          switch (map[op]) {
            case TILE_HIDDEN_EMPTY: modified=1; map[op]=TILE_FLAG_EMPTY; (*flags_available)--; break;
            case TILE_HIDDEN_EGG: modified=1; map[op]=TILE_FLAG_EGG; (*flags_available)--; break;
          }
        }
      }
    }
  }
  if (modified) goto _again_;
  return 1;
}

/* Run a speculative test to completion.
 * Caller should first set one candidate cell to either TILE_FLAG_EGG or TILE_ASSUME_VACANT.
 * We return nonzero if there is at least one valid disposition for the remaining candidates.
 * We may damage (map) in the process.
 */
 
static int speculate(uint8_t *map,const int *candidatev,int candidatec,const int *refv,int refc,int flags_available) {
  if (!ramify_speculative_change(map,refv,refc,&flags_available)) {
    return 0;
  }
  uint8_t map0[COLC*ROWC];
  memcpy(map0,map,COLC*ROWC);
  while (candidatec>0) {
    int p=candidatev[--candidatec];
    uint8_t tileid0=map[p];
    if ((tileid0!=TILE_HIDDEN_EMPTY)&&(tileid0!=TILE_HIDDEN_EGG)) continue;
    int eggok=0;
    if (flags_available>0) {
      map[p]=TILE_FLAG_EGG;
      int flags_available0=flags_available;
      eggok=ramify_speculative_change(map,refv,refc,&flags_available);
      memcpy(map,map0,COLC*ROWC);
      flags_available=flags_available0;
    }
    map[p]=TILE_ASSUME_VACANT;
    int flags_available0=flags_available;
    int vacantok=ramify_speculative_change(map,refv,refc,&flags_available);
    memcpy(map,map0,COLC*ROWC);
    flags_available=flags_available0;
    if (!eggok&&!vacantok) return 0;
  }
  return 1;
}

/* Autosolve.
 */
 
int autosolve(const uint8_t *map) {
  int hiddenc=0,flagc=0;
  int x,y;
  const uint8_t *v;
  
  /* First pass through the map, short-circuit for easy plays:
   *  - Exposed and satisfied cell, expose any hidden unflagged neighbor.
   *  - Exposed cell with exactly the right count of outstanding neighbors, flag them.
   *  - More flags than declared eggs, abort, we don't operate on false-flag boards.
   * While we're at it, count hidden and flagged cells.
   */
  for (y=0,v=map;y<ROWC;y++) {
    for (x=0;x<COLC;x++,v++) {
      if ((*v==TILE_HIDDEN_EMPTY)||(*v==TILE_HIDDEN_EGG)) {
        hiddenc++;
      } else if ((*v==TILE_FLAG_EMPTY)||(*v==TILE_FLAG_EGG)) {
        flagc++;
      } else if ((*v>=TILE_EXPOSE_1)&&(*v<=TILE_EXPOSE_8)) {
        int expectc=(*v)-TILE_EXPOSE_0;
        int actualc=count_flags(x,y,v);
        if (actualc>expectc) {
          return COLC*ROWC;
        }
        if (actualc==expectc) {
          int p=get_hidden_neighbor(map,x,y);
          if (p>=0) {
            return p;
          }
          continue;
        }
        // Can we flag the unflagged neighbors?
        int unflagc=0,unflagp=-1;
        int dy=-1; for (;dy<=1;dy++) {
          int oy=y+dy;
          if ((oy<0)||(oy>=ROWC)) continue;
          int dx=-1; for (;dx<=1;dx++) {
            int ox=x+dx;
            if ((ox<0)||(ox>=COLC)) continue;
            int up=oy*COLC+ox;
            uint8_t tileid=map[up];
            if ((tileid==TILE_HIDDEN_EMPTY)||(tileid==TILE_HIDDEN_EGG)) {
              unflagc++;
              unflagp=up;
            }
          }
        }
        if (unflagc==expectc-actualc) {
          return -unflagp-1;
        }
      }
    }
  }
  if (flagc>=EGGC) return COLC*ROWC;
  if (!hiddenc) return COLC*ROWC;
  if (hiddenc>=COLC*ROWC) return COLC*ROWC;
  
  /* Gather two cell lists:
   *  - candidatev: Hidden cells with an adjacent nonzero exposed cell. That's the only ones we might play on.
   *  - refv: The exposed cells abutting all of (candidatev). These will tell us whether a play is legal.
   */
  int candidatev[COLC*ROWC],refv[COLC*ROWC];
  int candidatec=0,refc=0;
  for (y=0,v=map;y<ROWC;y++) {
    for (x=0;x<COLC;x++,v++) {
      switch (*v) {
        case TILE_HIDDEN_EMPTY: case TILE_HIDDEN_EGG: {
            if (cell_has_exposed_neighbors(x,y,v)) candidatev[candidatec++]=v-map;
          } break;
        case TILE_EXPOSE_1: case TILE_EXPOSE_2: case TILE_EXPOSE_3: case TILE_EXPOSE_4:
        case TILE_EXPOSE_5: case TILE_EXPOSE_6: case TILE_EXPOSE_7: case TILE_EXPOSE_8: {
            if (cell_has_hidden_neighbors(x,y,v)) refv[refc++]=v-map;
          } break;
      }
    }
  }
  if (candidatec>31) {
    return COLC*ROWC;
  }
  int flags_available=EGGC-flagc,i;
  if (!candidatec||!refc) return COLC*ROWC;
  
  /* Make a scratch map for speculative testing.
   * Pop each candidate from the list and try it as both flag and vacant.
   * If it can only work one way, that's our answer.
   * If we fall through, it must be that there's more than one solution, or the map is invalid (eg false flags).
   */
  uint8_t scratch[COLC*ROWC];
  memcpy(scratch,map,COLC*ROWC);
  int lo=candidatev[0],hi=candidatev[0];
  for (i=1;i<candidatec;i++) {
    if (candidatev[i]<lo) lo=candidatev[i];
    else if (candidatev[i]>hi) hi=candidatev[i];
  }
  int cpc=hi-lo+1;
  while (candidatec>0) {
    int p=candidatev[--candidatec];
    memcpy(scratch+lo,map+lo,cpc);
    scratch[p]=TILE_FLAG_EGG;
    int eggok=speculate(scratch,candidatev,candidatec,refv,refc,flags_available-1);
    memcpy(scratch+lo,map+lo,cpc);
    scratch[p]=TILE_ASSUME_VACANT;
    int vacantok=speculate(scratch,candidatev,candidatec,refv,refc,flags_available);
    if (eggok&&!vacantok) {
      return -p-1;
    }
    if (!eggok&&vacantok) {
      return p;
    }
  }
  
  return COLC*ROWC;
}

/* Nonzero if the hidden eggs in this map agree with all exposed clues.
 */
 
static int trial_map_valid(const uint8_t *map) {
  const uint8_t *v=map;
  int y=0; for (;y<ROWC;y++) {
    int x=0; for (;x<COLC;x++,v++) {
      if ((*v>=TILE_EXPOSE_1)&&(*v<=TILE_EXPOSE_8)) {
        int expectc=(*v)-TILE_EXPOSE_0;
        int actualc=count_adjacent_eggs(x,y,v);
        if (expectc!=actualc) return 0;
      }
    }
  }
  return 1;
}

/* Advance this list of indices in a predictable way, so they have unique values in 0..(locationc-1).
 * Must start with unique values 0,1,2,...
 * Returns zero if iteration complete.
 */
 
static int trial_next_eggs(int *eggv,int eggc,int locationc) {
  if (eggc>locationc) return 0;
  int i=1;
  for (;i<=eggc;i++) {
    int eggp=eggc-i;
    if (eggv[eggp]<locationc-i) {
      eggv[eggp]++;
      for (i=eggp+1;i<eggc;i++) {
        eggv[i]=eggv[eggp]+i-eggp;
      }
      return 1;
    }
  }
  return 0;
}

/* Sneakily move an egg, if possible.
 */
 
int autosolve_repair(int x,int y) {
  
  if ((x<0)||(y<0)||(x>=COLC)||(y>=ROWC)||(g.map[y*COLC+x]!=TILE_HIDDEN_EGG)) {
    return -1;
  }
  
  /* The ideal case arises on the very first play, and is unlikely after that.
   * This is where the selected tile has no exposed neighbors, and a TILE_HIDDEN_EMPTY exists somewhere with no exposed neighbors.
   * Don't drop it in the first candidate, select randomly. Otherwise eggs would gravitate upward.
   */
  if (!cell_has_exposed_neighbors(x,y,g.map+y*COLC+x)) {
    int candidatev[COLC*ROWC];
    int candidatec=0;
    uint8_t *v=g.map;
    int oy=0; for (;oy<ROWC;oy++) {
      int ox=0; for (;ox<COLC;ox++,v++) {
        if (*v!=TILE_HIDDEN_EMPTY) continue;
        if (!cell_has_exposed_neighbors(ox,oy,v)) {
          candidatev[candidatec++]=v-g.map;
        }
      }
    }
    if (candidatec>0) {
      int candidatep=rand()%candidatec;
      int dstx=candidatev[candidatep]%COLC;
      int dsty=candidatev[candidatep]/ROWC;
      g.map[y*COLC+x]=TILE_HIDDEN_EMPTY;
      g.map[dsty*COLC+dstx]=TILE_HIDDEN_EGG;
      return 0;
    }
  }
  
  /* Another likelyish early case: Your first click has neighbor eggs, so you click something adjacent to it, which is the egg.
   * This won't hit the first case, since the egg does have one exposed neighbor.
   * Check for other positions adjacent to the exposed cell, move to one of them if we can.
   * Don't let the general case handle it because the possibility space is way too large. (plus, it would pick something like "all the eggs lined up in the top row")
   * Reproducible under automation: gAAEAIAEoAAEgJAACVQgRJAEQAAkoAABDCBAAAAAIACAgIQgoQ
   */
  if ((x>1)&&(y>1)&&(x<COLC-2)&&(y<ROWC-2)) {
    int exposex,exposey,exposec=0;
    int dy=-1; for (;dy<=1;dy++) {
      int oy=y+dy;
      int dx=-1; for (;dx<=1;dx++) {
        int ox=x+dx;
        uint8_t tileid=g.map[oy*COLC+ox];
        if ((tileid>=TILE_EXPOSE_1)&&(tileid<=TILE_EXPOSE_8)) {
          exposex=ox;
          exposey=oy;
          exposec++;
        }
      }
    }
    if (exposec==1) {
      int eggp=y*COLC+x;
      for (dy=-1;dy<=1;dy++) {
        int oy=exposey+dy;
        int dx=-1; for (;dx<=1;dx++) {
          int ox=exposex+dx;
          int op=oy*COLC+ox;
          uint8_t tileid=g.map[op];
          if (tileid==TILE_HIDDEN_EMPTY) {
            g.map[eggp]=TILE_HIDDEN_EMPTY;
            g.map[op]=TILE_HIDDEN_EGG;
            if (trial_map_valid(g.map)) {
              return 0;
            }
            g.map[eggp]=TILE_HIDDEN_EGG;
            g.map[op]=TILE_HIDDEN_EMPTY;
          }
        }
      }
    }
  }
  
  /* Since we've taken care of the buggersome first-click problem, the remaining cases are best-effort.
   * Not that they don't matter: I really want every puzzle to be solvable.
   * But if we miss a few cases now, not the end of the world.
   */
  #define LOCATION_LIMIT 16 /* Arbitrary limit, but if it's too high we might churn hard. */
  struct location { int x,y,p; uint8_t tileid; } locationv[LOCATION_LIMIT];
  int locationc=0,eggc=0;
  uint8_t *v=g.map;
  int i=COLC*ROWC;
  for (;i-->0;v++) {
    switch (*v) {
      case TILE_HIDDEN_EGG: eggc++; // pass
      case TILE_HIDDEN_EMPTY: {
          if (locationc>=LOCATION_LIMIT) {
            return -1;
          }
          int p=v-g.map;
          int xx=p%COLC;
          int yy=p/COLC;
          locationv[locationc++]=(struct location){.x=xx,.y=yy,.p=p,.tileid=g.map[p]};
        } break;
      case TILE_FLAG_EMPTY: {
          return -1;
        } break;
    }
  }
  if ((locationc<1)||(eggc<1)||(eggc>=locationc)) {
    return -1;
  }
  
  /* OK, there's a finite set of numbers of 16 bits with (n) set.
   * Iterate all of them and use the first arrangement that agrees with all exposures and does not put an egg on (x,y).
   */
  int iterc=0;
  int eggv[LOCATION_LIMIT]; // Index in (locationv); only the first (eggc) are used.
  for (i=eggc;i-->0;) eggv[i]=i;
  for (;;) {
    iterc++;
    for (i=locationc;i-->0;) g.map[locationv[i].p]=TILE_HIDDEN_EMPTY;
    for (i=eggc;i-->0;) g.map[locationv[eggv[i]].p]=TILE_HIDDEN_EGG;
    if (g.map[y*COLC+x]==TILE_HIDDEN_EMPTY) {
      if (trial_map_valid(g.map)) {
        return 0;
      }
    }
    if (!trial_next_eggs(eggv,eggc,locationc)) break;
  }
  
  /* Giving up.
   * Restore locations to their original tiles.
   */
  for (i=locationc;i-->0;) {
    g.map[locationv[i].p]=locationv[i].tileid;
  }
  return -1;
}
