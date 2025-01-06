#include "tetris.h"

/* Set level and update scoring constants.
 * No harm in calling redundantly, or with (level) too high.
 */
 
static void field_next_level(struct field *field,int level) {

  /* NES Tetris I'm told goes up to like 30?
   * But I've only been able to test the first 20, so that's all we imitate.
   */
  if (level<0) level=0;
  else if (level>19) level=19;
  if (level!=field->level) {
    // We're not responsible for rendering it, but we are responsible for this clock.
    // Proceed with initialization even if (level==field->level); eg it might be zero at init.
    field->disp_level_clock=DISP_LEVEL_TIME;
  }
  field->level=level;
  
  /* Scoring increases linearly with level.
   */
  field->linevalue[0]=(field->level+1)*40;
  field->linevalue[1]=(field->level+1)*100;
  field->linevalue[2]=(field->level+1)*300;
  field->linevalue[3]=(field->level+1)*1200;
  
  /* Drop time is kind of arbitrary.
   * I got these numbers by counting frames in NES Tetris.
   * We use continuous time, so we actually don't need to make eg (10,11,12) the same.
   * But NES did it that way and I want to match as close as possible.
   */
  const double FRAMETIME=0.016666;
  switch (field->level) {
    case  0: field->droptime=FRAMETIME*48.0; break;
    case  1: field->droptime=FRAMETIME*43.0; break;
    case  2: field->droptime=FRAMETIME*38.0; break;
    case  3: field->droptime=FRAMETIME*33.0; break;
    case  4: field->droptime=FRAMETIME*28.0; break;
    case  5: field->droptime=FRAMETIME*23.0; break;
    case  6: field->droptime=FRAMETIME*18.0; break;
    case  7: field->droptime=FRAMETIME*13.0; break;
    case  8: field->droptime=FRAMETIME* 8.0; break;
    case  9: field->droptime=FRAMETIME* 6.0; break;
    case 10: field->droptime=FRAMETIME* 5.0; break;
    case 11: field->droptime=FRAMETIME* 5.0; break;
    case 12: field->droptime=FRAMETIME* 5.0; break;
    case 13: field->droptime=FRAMETIME* 4.0; break;
    case 14: field->droptime=FRAMETIME* 4.0; break;
    case 15: field->droptime=FRAMETIME* 4.0; break;
    case 16: field->droptime=FRAMETIME* 3.0; break;
    case 17: field->droptime=FRAMETIME* 3.0; break;
    case 18: field->droptime=FRAMETIME* 3.0; break;
    case 19: field->droptime=FRAMETIME* 2.0; break;
  }
}

/* Init.
 */
 
int field_init(struct field *field,int readhead,int playerc,int level) {
  if ((playerc<0)||(playerc>PLAYER_LIMIT)) return -1;
  if ((readhead<0)||(readhead>1)) return -1;
  if ((level<0)||(level>19)) return -1;
  
  egg_texture_del(field->score_texid);
  egg_texture_del(field->level_texid);

  memset(field,0,sizeof(struct field));
  field->readhead=readhead;
  if (!playerc) {
    field->finished=1;
    field->end_ack=1;
    return 0;
  }
  field->playerc=playerc;
  struct player *player=field->playerv;
  int i=0;
  for (;i<playerc;i++,player++) {
    player->playerid=1+i;
    player->pvinput=0;
    player->tetr=-1;
    player->xform=0;
    player->x=0;
    player->y=0;
  }
  field_next_level(field,level);
  field->disp_level_clock=0.0; // Don't declare the level when starting out.
  field->dropscore=1; // *rowc
  field->dirty=0;
  field->disp_score=-1;
  return 0;
}

/* Cement the player's tetromino into the static grid, and arrange to draw a fresh one at the next update.
 */
 
static void player_commit_piece(struct field *field,struct player *player) {
  struct tetr_tile tilev[4];
  int tilec=tetr_tile_shape(tilev,4,player->tetr,player->xform);
  if ((tilec>0)&&(tilec<=4)) {
    uint8_t tileid=(field->playerc>1)?0x08/*(0x00+player->playerid)*/:(0x01+player->tetr);
    const struct tetr_tile *tile=tilev;
    int ti=tilec;
    for (;ti-->0;tile++) {
      int x=player->x+tile->x,y=player->y+tile->y;
      if ((x<0)||(y<0)||(x>=FIELDW)||(y>=FIELDH)) continue;
      field->cellv[y*FIELDW+x]=tileid;
    }
    field->dirty=1;
  }
  player->tetr=-1;
  player->down=0; // The fast fall ends when the piece lands, you'll have to let go and press it again, intentionally.
}

/* Check collisions.
 * Left, right, and bottom edges of field do count.
 * Other tetrominoes in flight do count.
 * Top edge of field does not count.
 */
 
#define COLLIDE_FLOOR 1 /* Bottom edge of field, or a static tile -- highest priority. */
#define COLLIDE_WALL 2 /* Left or right edge of field. */
#define COLLIDE_OTHER 3 /* Another player. */
 
static int player_collision(const struct field *field,const struct player *player) {
  struct tetr_tile tilev[4];
  int tilec=tetr_tile_shape(tilev,4,player->tetr,player->xform);
  if ((tilec<1)||(tilec>4)) return 0;
  const struct tetr_tile *tile=tilev;
  int hitwall=0;
  int ti=tilec;
  for (;ti-->0;tile++) {
    int y=player->y+tile->y;
    if (y>=FIELDH) {
      //fprintf(stderr,"y=%d tetr=%d xform=%d player=%d,%d tile=%d,%d\n",y,player->tetr,player->xform,player->x,player->y,tile->x,tile->y);
      return COLLIDE_FLOOR;
    }
    int x=player->x+tile->x;
    if ((x<0)||(x>=FIELDW)) hitwall=1;
    else if ((y>=0)&&field->cellv[y*FIELDW+x]) {
      //fprintf(stderr,"static collision at %d,%d (%d) = 0x%02x\n",x,y,y*FIELDW+x,field->cellv[y*FIELDW+x]);
      return COLLIDE_FLOOR;
    }
  }
  if (hitwall) return COLLIDE_WALL;
  if (field->playerc>1) {
    const struct player *other=field->playerv;
    int i=field->playerc;
    for (;i-->0;other++) {
      if (other==player) continue;
      struct tetr_tile otilev[4];
      int otilec=tetr_tile_shape(otilev,4,other->tetr,other->xform);
      if ((otilec<1)||(otilec>4)) continue;
      int ti=tilec;
      for (tile=tilev;ti-->0;tile++) {
        int tx=player->x+tile->x,ty=player->y+tile->y;
        const struct tetr_tile *otile=otilev;
        int oi=otilec;
        for (;oi-->0;otile++) {
          if ((other->x+otile->x==tx)&&(other->y+otile->y==ty)) return COLLIDE_OTHER;
        }
      }
    }
  }
  return 0;
}

/* Drop one player's tetromino.
 * Returns >0 if we were stopped by another piece.
 * Caller should arrange to try again next frame in that case.
 */
 
static int player_fall(struct field *field,struct player *player) {
  player->y++;
  switch (player_collision(field,player)) {
    case COLLIDE_FLOOR: {
        if (player->down&&player->dropc) {
          if ((field->score+=player->dropc*field->dropscore)>SCORE_LIMIT) field->score=SCORE_LIMIT;
        }
        player->y--;
        player_commit_piece(field,player);
        egg_play_sound(RID_sound_dropped);
        player->dropc=0;
      } return 0;
    case COLLIDE_WALL: // Wall shouldn't be possible on a vertical move.
    case COLLIDE_OTHER: {
        player->y--;
        return 1;
      }
  }
  player->dropc++;
  return 0;
}

/* Rotate player's tetromino.
 */
 
static void player_rotate(struct field *field,struct player *player,int d) {
  #define HOW_BOUT_NOW { if (!player_collision(field,player)) { \
    egg_play_sound(RID_sound_rotate); \
    return; \
  }}
  player->xform+=d;
  HOW_BOUT_NOW
  
  // If a small adjustment to (x) can make it valid, go with it. We prefer not to reject rotation.
  int x0=player->x;
  int dx=1; for (;dx<=2;dx++) {
    player->x=x0-dx;
    HOW_BOUT_NOW
    player->x=x0+dx;
    HOW_BOUT_NOW
  }

  player->x=x0;
  player->xform-=d;
  egg_play_sound(RID_sound_reject);
  #undef HOW_BOUT_NOW
}

/* Move one cell horizontally, and schedule repeat.
 * (d==0) to repeat the most recent motion.
 */
 
static void player_move(struct field *field,struct player *player,int d) {
  if (!d) {
    if (!(d=player->motion)) return;
  } else {
    player->motion=d;
    player->motionclock=0.0-MOTION_INITIAL_DELAY;
  }
  player->x+=d;
  // After a collision, try again immediately next frame. This is necessary for sliding into gaps at high speed.
  if (player_collision(field,player)) {
    player->motionclock=MOTION_TIME-0.001;
    player->x-=d;
  } else {
    egg_play_sound(RID_sound_motion);
  }
}

/* End motion.
 */
 
static void player_stop(struct field *field,struct player *player,int d) {
  if (d==player->motion) {
    player->motion=0;
  }
}

/* Fast fall.
 */
 
static void player_begin_fast_fall(struct field *field,struct player *player) {
  player->down=1;
  player->dropc=0;
  player->dropclock=FAST_FALL_TIME;
}

static void player_end_fast_fall(struct field *field,struct player *player) {
  player->down=0;
  player->dropc=0;
}

/* Check for completed lines.
 */
 
static void field_check_cells(struct field *field) {
  memset(field->rmrowv,0,sizeof(field->rmrowv));
  int lines_scored=0;
  uint8_t *p=field->cellv+sizeof(field->cellv);
  int row=FIELDH,oldrow=FIELDH;
  while (row-->0) {
    oldrow--;
    int line_complete=1;
    int col=FIELDW; while (col-->0) {
      p--;
      if (!*p) line_complete=0;
    }
    if (line_complete) {
      field->rmrowv[oldrow]=1;
      memmove(field->cellv+FIELDW,field->cellv,row*FIELDW);
      memset(field->cellv,0,FIELDW);
      lines_scored++;
      row++;
      p+=FIELDW;
    }
  }
  if (lines_scored<1) return;
  
  field->rmclock=RMANIMTIME;
  switch (lines_scored) {
    case 1: case 2: case 3: egg_play_sound(RID_sound_lines); break;
    default: egg_play_sound(RID_sound_tetris); break;
  }
  
  /* If the new lines advance us to another level, they are scored against the new level's constants.
   * NES Tetris had some funny rules about level-up: Beyond 99 lines, it's every ten lines, regardless of the current level.
   * I don't know why that rule exists. Maybe it's a bug.
   * We are going to handle levels more simply: Level is just linec/10.
   */
  int nlc=field->linec[0]+field->linec[1]*2+field->linec[2]*3+field->linec[3]*4+lines_scored;
  int to_level=nlc/10;
  if ((to_level>field->level)&&(to_level<20)) {
    field_next_level(field,to_level);
  }
  
  // In theory, there can be more than 4 at once. In practice, I don't think it can be done reliably (depends on the order of players in our list).
  while (lines_scored>4) {
    lines_scored-=4;
    field->linec[3]++;
    if ((field->score+=field->linevalue[3])>SCORE_LIMIT) field->score=SCORE_LIMIT;
  }
  field->linec[lines_scored-1]++;
  if ((field->score+=field->linevalue[lines_scored-1])>SCORE_LIMIT) field->score=SCORE_LIMIT;
}

/* Select a position for a new tetromino that doesn't collide with anything existing.
 * Returns -1 if no position is possible.
 */
 
static int check_new_tetromino_position(const struct field *field,int x,int y,const struct tetr_tile *tilev,int tilec) {
  struct xy { int x,y; } xyv[4];
  int xyc=0;
  const struct tetr_tile *tile=tilev;
  int i=tilec;
  for (;i-->0;tile++) {
    int tx=x+tile->x; if ((tx<0)||(tx>=FIELDW)) return -1;
    int ty=y+tile->y; if ((ty<0)||(ty>=FIELDH)) return -1;
    if (field->cellv[ty*FIELDW+tx]) return -1;
    xyv[xyc++]=(struct xy){tx,ty};
  }
  const struct player *player=field->playerv;
  int pi=field->playerc;
  for (;pi-->0;player++) {
    struct tetr_tile otiles[4];
    int otilec=tetr_tile_shape(otiles,4,player->tetr,player->xform);
    if ((otilec<1)||(otilec>4)) continue;
    for (tile=otiles,i=otilec;i-->0;tile++) {
      int tx=player->x+tile->x;
      int ty=player->y+tile->y;
      int xyi=xyc; while (xyi-->0) {
        if ((tx==xyv[xyi].x)&&(ty==xyv[xyi].y)) return -1;
      }
    }
  }
  return 0;
}
 
static int choose_position_for_tiles(int *x,int *y,const struct tetr_tile *tilev,int tilec,const struct field *field) {
  
  // First identify the tiles' bounds.
  int xa=tilev[0].x,xz=tilev[0].x,ya=tilev[0].y,yz=tilev[0].y;
  int i=tilec-1;
  const struct tetr_tile *tile=tilev+1;
  for (;i-->0;tile++) {
    if (tile->x<xa) xa=tile->x; else if (tile->x>xz) xz=tile->x;
    if (tile->y<ya) ya=tile->y; else if (tile->y>yz) yz=tile->y;
  }
  int w=xz-xa+1,h=yz-ya+1;
  
  // (y) is fixed; we only return positions that put the tetromino flush against the top.
  *y=-ya;
  
  // Start in the middle of the field, and proceed outward from there.
  // Give up when we've breached both horz edges.
  int xmid=(FIELDW>>1)-(w>>1)-xa;
  int r=0,leftdone=0,rightdone=0;
  for (;!leftdone||!rightdone;r++) {
    if (xmid-r+xa<0) {
      leftdone=1;
    } else {
      *x=xmid-r;
      if (check_new_tetromino_position(field,*x,*y,tilev,tilec)>=0) return 0;
    }
    if (!r) continue;
    if (xmid+r+xz>=FIELDW) {
      rightdone=1;
    } else {
      *x=xmid+r;
      if (check_new_tetromino_position(field,*x,*y,tilev,tilec)>=0) return 0;
    }
  }
  return -1;
}
 
static int choose_new_tetromino_position(int *x,int *y,int *xform,int tetr,const struct field *field) {
  // There might be a more graceful solution, but whatever just brute force the xforms.
  // Prefer xform zero.
  for (*xform=0;*xform<4;(*xform)++) {
    struct tetr_tile tilev[4];
    int tilec=tetr_tile_shape(tilev,4,tetr,*xform);
    if ((tilec<1)||(tilec>4)) continue;
    if (choose_position_for_tiles(x,y,tilev,tilec,field)>=0) return 0;
  }
  return -1;
}

/* Draw a tile for a play that's out.
 * Returns:
 *   <0 if the upper field is full, and we've marked the game as lost.
 *    0 if we opt not to just now, but the game proceeds. (eg first pass on an 8-player field, there isn't room for everybody).
 *   >0 created a tile, carry on.
 */
 
static int player_draw(struct field *field,struct player *player) {
  int tetr=bag_peek(field->readhead);
  int xform=0,x=0,y=0;
  if (choose_new_tetromino_position(&x,&y,&xform,tetr,field)<0) {
  
    // If there's a tile in every row of the field, game over, return <0.
    // But if some row is empty, return 0 to keep running and revisit this player later.
    int game_over=1;
    const uint8_t *p=field->cellv+FIELDW*FIELDH;
    int y=FIELDH;
    while (y-->1) { // sic 1 not 0: All tetrominoes can be placed in two rows. Most can't in one. So allow the top row to be empty, otherwise we might stall.
      int x=FIELDW,empty=1;
      while (x-->0) {
        p--;
        if (*p) empty=0;
      }
      if (empty) {
        return 0;
      }
    }
    return -1;
  }
  if (bag_draw(field->readhead)!=tetr) return -1;
  player->tetr=tetr;
  player->xform=xform;
  player->x=x;
  player->y=y;
  player->dropclock=-NEW_PIECE_GRACE_TIME;
  return 1;
}

/* Update all players of a field.
 */
 
static void field_update_players(struct field *field,double elapsed) {
  struct player *player=field->playerv;
  int i=field->playerc;
  for (;i-->0;player++) {
  
    // Draw tile if we're out.
    if (player->tetr<0) {
      int err=player_draw(field,player);
      if (err<0) {
        egg_play_sound(RID_sound_fatal);
        field->finished=1;
        int linec=field->linec[0]+field->linec[1]*2+field->linec[2]*3+field->linec[3]*4;
        db_add(field->playerc,linec,field->score);
        return;
      }
      if (!err) continue;
    }
  
    // Check input.
    int input=egg_input_get_one(player->playerid);
    if (input!=player->pvinput) {
           if ((input&EGG_BTN_LEFT)&&!(player->pvinput&EGG_BTN_LEFT)) player_move(field,player,-1);
      else if (!(input&EGG_BTN_LEFT)&&(player->pvinput&EGG_BTN_LEFT)) player_stop(field,player,-1);
           if ((input&EGG_BTN_RIGHT)&&!(player->pvinput&EGG_BTN_RIGHT)) player_move(field,player,1);
      else if (!(input&EGG_BTN_RIGHT)&&(player->pvinput&EGG_BTN_RIGHT)) player_stop(field,player,1);
           if ((input&EGG_BTN_DOWN)&&!(player->pvinput&EGG_BTN_DOWN)) player_begin_fast_fall(field,player);
      else if (!(input&EGG_BTN_DOWN)&&(player->pvinput&EGG_BTN_DOWN)) player_end_fast_fall(field,player);
      if ((input&EGG_BTN_SOUTH)&&!(player->pvinput&EGG_BTN_SOUTH)) player_rotate(field,player,1);
      if ((input&EGG_BTN_WEST)&&!(player->pvinput&EGG_BTN_WEST)) player_rotate(field,player,-1);
      player->pvinput=input;
    }
    
    // Repeat horizontal motion when held, per a clock.
    if (player->motion) {
      player->motionclock+=elapsed;
      while (player->motionclock>=MOTION_TIME) {
        player->motionclock-=MOTION_TIME;
        player_move(field,player,0);
      }
    }
    
    // Advance dropclock and fall if needed.
    double droptime=field->droptime;
    if (player->down&&(FAST_FALL_TIME<field->droptime)) droptime=FAST_FALL_TIME;
    player->dropclock+=elapsed;
    while (player->dropclock>=droptime) {
      player->dropclock-=droptime;
      if (player_fall(field,player)>0) {
        // Bumped into another player. Spam falling, make sure it triggers next frame too.
        player->dropclock=droptime-0.001;
        break;
      }
    }
  }
}

/* Poll players for end acknowledgement.
 */
 
static void field_check_end_ack(struct field *field) {
  struct player *player=field->playerv;
  int i=field->playerc;
  for (;i-->0;player++) {
    int input=egg_input_get_one(player->playerid);
    if (input!=player->pvinput) {
      if ((input&EGG_BTN_SOUTH)&&!(player->pvinput&EGG_BTN_SOUTH)) field->end_ack=1;
      player->pvinput=input;
    }
  }
}

/* Update.
 */

void field_update(struct field *field,double elapsed) {
  if (field->finished) {
    if (field->end_ack) return;
    if ((field->end_blackout-=elapsed)>0.0) return;
    field_check_end_ack(field);
    return;
  }
  
  if (field->disp_level_clock>0.0) {
    field->disp_level_clock-=elapsed;
  }
  
  // Pause hard during line removal.
  if (field->rmclock<=0.0) {
    field_update_players(field,elapsed);
  }
  
  // Tick the removal animation clock.
  if (field->rmclock>0.0) {
    field->rmclock-=elapsed;
  }
  
  // If the field is dirty, check for lines. Defer this when the removal animation is running.
  if (field->dirty&&(field->rmclock<=0.0)) {
    field->dirty=0;
    field_check_cells(field);
  }
}
