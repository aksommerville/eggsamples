/* snake.c
 * The game model.
 * We're responsible for everything except rendering and client hooks (both in main.c).
 */
 
#include "snake.h"

/* Choose a random position for a new snack.
 */
 
void snake_place_snack() {
  /* We're going to murder the stack by making a bitmap of cells, and then a list of valid positions.
   */
  uint8_t map[COLC*ROWC]={0}; // Nonzero at illegal positions (ie snaked).
  int candidatev[COLC*ROWC];
  int candidatec=0;
  const struct snegment *snegment=g.snegmentv;
  int i=g.snegmentc;
  for (;i-->0;snegment++) {
    map[snegment->y*COLC+snegment->x]=1;
  }
  for (i=COLC*ROWC;i-->0;) {
    if (map[i]) continue;
    candidatev[candidatec++]=i;
  }
  if (candidatec<1) { // Shouldn't be possible.
    g.snakx=0xff;
    g.snaky=0xff;
    return;
  }
  int candidatep=rand()%candidatec;
  int mapp=candidatev[candidatep];
  g.snakx=mapp%COLC;
  g.snaky=mapp/COLC;
}
 
/* Reset game and begin play.
 */
 
void snake_reset() {
  
  g.snegmentc=2;
  struct snegment *head=g.snegmentv+0;
  struct snegment *tail=g.snegmentv+1;
  head->x=COLC>>1;
  head->y=ROWC>>1;
  head->tileid=TILE_HEAD;
  head->xform=XF_N;
  tail->x=head->x;
  tail->y=head->y+1;
  tail->tileid=TILE_TAIL;
  tail->xform=XF_N;
  
  snake_place_snack();
  
  g.tick_dur=0.500;
  g.motion_clock=0.0;
  g.running=1;
  g.clock=0.0;
  
  egg_play_song(RID_song_snake_shakin,0,1);
}

/* Enter initial state.
 * Could just call snake_reset() and be done with it, but I think it's better to await a keystroke before starting.
 */
 
void snake_init() {
  g.running=0;
  g.snegmentc=0;
  g.snakx=g.snaky=0xff;
  g.clock=0.0;
  egg_play_song(RID_song_lounge_lizard,0,1);
}

/* End game.
 */
 
static void snake_finish(int victory,const char *reason) {
  g.running=0;
  egg_play_song(RID_song_lounge_lizard,0,1);
  if (victory) {
  } else if (reason) {
    //egg_log(reason);
  }
}

/* Assess one map position.
 */
 
#define POS_OOB 1
#define POS_SNAKE 2
#define POS_SNACK 3
#define POS_OPEN 4

static int snake_examine_position(int x,int y) {
  if ((x<0)||(y<0)||(x>=COLC)||(y>=ROWC)) return POS_OOB;
  const struct snegment *snegment=g.snegmentv;
  int i=g.snegmentc;
  for (;i-->0;snegment++) {
    if (x!=snegment->x) continue;
    if (y!=snegment->y) continue;
    if (snegment->tileid==TILE_TAIL) break; // You're allowed to chase your own tail directly.
    return POS_SNAKE;
  }
  if ((x==g.snakx)&&(y==g.snaky)) return POS_SNACK;
  return POS_OPEN;
}

/* Advance snake position.
 */
 
static void snake_advance(int dx,int dy) {
  if (g.snegmentc<2) { snake_finish(0,"NO SNAKE"); return; }
  struct snegment *head=g.snegmentv;
  
  /* If a direction was provided, validate it and change the xform.
   */
  if (dx||dy) {
    if ((dx&&dy)||(dx<-1)||(dx>1)||(dy<-1)||(dy>1)) return;
         if (dx<0) head->xform=XF_W;
    else if (dx>0) head->xform=XF_E;
    else if (dy<0) head->xform=XF_N;
    else           head->xform=XF_S;
  
  /* Read dpad state and select the next move. Don't commit it yet.
   * Do commit xform changes to the head immediately.
   * If you turn directly into a wall, it will break your neck, which I think is appropriate.
   */
  } else {
    switch (g.pvinput&(EGG_BTN_LEFT|EGG_BTN_RIGHT|EGG_BTN_UP|EGG_BTN_DOWN)) {
      case EGG_BTN_LEFT:  dx=-1; head->xform=XF_W; break;
      case EGG_BTN_RIGHT: dx= 1; head->xform=XF_E; break;
      case EGG_BTN_UP:    dy=-1; head->xform=XF_N; break;
      case EGG_BTN_DOWN:  dy= 1; head->xform=XF_S; break;
      default: switch (head->xform) {
          case XF_W: dx=-1; break;
          case XF_E: dx= 1; break;
          case XF_N: dy=-1; break;
          case XF_S: dy= 1; break;
          default: snake_finish(0,"INVALID SNAKE"); return;
        }
    }
  }
  
  /* Check for collisions and the snack, and commit head motion if we're clear.
   */
  int pvx=head->x,pvy=head->y;
  int nx=head->x+dx,ny=head->y+dy,snacked=0;
  switch (snake_examine_position(nx,ny)) {
    case POS_OOB: snake_finish(0,"OOB"); return;
    case POS_SNAKE: snake_finish(0,"COLLISION"); return;
    case POS_SNACK: snacked=1; egg_play_sound(RID_sound_snack); // pass
    case POS_OPEN: {
        head->x=nx;
        head->y=ny;
      } break;
    default: snake_finish(0,"MAP FAULT"); return;
  }
  
  /* Capture the tail's original position, in case we're growing.
   */
  int tailx,taily;
  {
    struct snegment *tail=g.snegmentv+g.snegmentc-1;
    tailx=tail->x;
    taily=tail->y;
  }
  
  /* Move the rest of the snake to the predecessor's position.
   * Ignore tileid and xform for now.
   */
  int i=g.snegmentc-1;
  struct snegment *snegment=g.snegmentv+1;
  for (;i-->0;snegment++) {
    int tmpx=snegment->x;
    int tmpy=snegment->y;
    snegment->x=pvx;
    snegment->y=pvy;
    pvx=tmpx;
    pvy=tmpy;
  }
  
  /* One more pass along the body only, setting tileid and xform.
   * Everything we visit here has a neighbor on both sides.
   */
  for (i=g.snegmentc-2,snegment=g.snegmentv+1;i-->0;snegment++) {
    struct snegment *prev=snegment-1,*next=snegment+1;
    int pvd,nxd;
         if (prev->x<snegment->x) pvd=XF_W;
    else if (prev->x>snegment->x) pvd=XF_E;
    else if (prev->y<snegment->y) pvd=XF_N;
    else                          pvd=XF_S;
         if (next->x<snegment->x) nxd=XF_W;
    else if (next->x>snegment->x) nxd=XF_E;
    else if (next->y<snegment->y) nxd=XF_N;
    else                          nxd=XF_S;
    // There are 12 valid neighbor combinations (and if we get one of the 4 invalids, just don't do anything).
    // (pvd) is the direction toward the head, so that is always this segment's xform, regardless of the tile.
    snegment->xform=pvd;
    switch (pvd) {
      case XF_W: switch (nxd) {
          case XF_E: snegment->tileid=TILE_BODY; break;
          case XF_N: snegment->tileid=TILE_DEAS; break;
          case XF_S: snegment->tileid=TILE_CLOK; break;
        } break;
      case XF_E: switch (nxd) {
          case XF_W: snegment->tileid=TILE_BODY; break;
          case XF_N: snegment->tileid=TILE_CLOK; break;
          case XF_S: snegment->tileid=TILE_DEAS; break;
        } break;
      case XF_N: switch (nxd) {
          case XF_W: snegment->tileid=TILE_CLOK; break;
          case XF_E: snegment->tileid=TILE_DEAS; break;
          case XF_S: snegment->tileid=TILE_BODY; break;
        } break;
      case XF_S: switch (nxd) {
          case XF_W: snegment->tileid=TILE_DEAS; break;
          case XF_E: snegment->tileid=TILE_CLOK; break;
          case XF_N: snegment->tileid=TILE_BODY; break;
        } break;
    }
  }
  
  /* Set xform for the tail. Its tileid is constant.
   */
  {
    struct snegment *tail=g.snegmentv+g.snegmentc-1;
    struct snegment *lead=tail-1;
         if (lead->x<tail->x) tail->xform=XF_W;
    else if (lead->x>tail->x) tail->xform=XF_E;
    else if (lead->y<tail->y) tail->xform=XF_N;
    else                      tail->xform=XF_S;
  }
  
  if (snacked) {
    if (g.snegmentc<COLC*ROWC) {
      struct snegment *ntail=g.snegmentv+g.snegmentc++;
      struct snegment *otail=ntail-1;
      ntail->x=tailx;
      ntail->y=taily;
      ntail->tileid=TILE_TAIL;
           if (otail->x<ntail->x) ntail->xform=XF_W;
      else if (otail->x>ntail->x) ntail->xform=XF_E;
      else if (otail->y<ntail->y) ntail->xform=XF_N;
      else                        ntail->xform=XF_S;
      if (otail->xform==ntail->xform) {
        otail->tileid=TILE_BODY;
      } else switch (otail->xform) {
        case XF_W: switch (ntail->xform) {
            case XF_N: otail->tileid=TILE_CLOK; break;
            case XF_S: otail->tileid=TILE_DEAS; break;
          } break;
        case XF_E: switch (ntail->xform) {
            case XF_N: otail->tileid=TILE_DEAS; break;
            case XF_S: otail->tileid=TILE_CLOK; break;
          } break;
        case XF_N: switch (ntail->xform) {
            case XF_W: otail->tileid=TILE_DEAS; break;
            case XF_E: otail->tileid=TILE_CLOK; break;
          } break;
        case XF_S: switch (ntail->xform) {
            case XF_W: otail->tileid=TILE_CLOK; break;
            case XF_E: otail->tileid=TILE_DEAS; break;
          } break;
      }
    }
    snake_place_snack();
  }
}

/* Move immediately.
 */
 
void snake_move(int dx,int dy) {
  if (!g.running) return;
  snake_advance(dx,dy);
  g.motion_clock=0.0;
}

/* Update.
 */
 
void snake_update(double elapsed) {
  if (!g.running) return;
  g.clock+=elapsed;
  if (g.snegmentc>=COLC*ROWC) { snake_finish(1,""); return; }
  if (g.tick_dur<0.001) { snake_finish(0,"CLOCK FAULT"); return; }
  g.motion_clock+=elapsed;
  while (g.running&&(g.motion_clock>=g.tick_dur)) {
    g.motion_clock-=g.tick_dur;
    snake_advance(0,0);
  }
}
