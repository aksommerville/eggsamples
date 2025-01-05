/* field.h
 * Model of one field of tetris play.
 * There can be two fields at once (which is main's problem, not ours, and also not a problem).
 * Each field may have up to 8 players.
 * All fields draw from the global bag -- you initialize us with the readhead id (0,1).
 */
 
#ifndef FIELD_H
#define FIELD_H

/* During the removal animation, the rendered field does not match the model, and some operations are suspended.
 * So don't let it last too long.
 */
//#define RMANIMTIME 0.375
#define RMANIMTIME (20.0*0.016666)

/* New pieces stay at the top row for so long before their first drop.
 * This does not change per level.
 * I've measured it in NES Tetris to be exactly 10 frames.
 */
#define NEW_PIECE_GRACE_TIME 0.166666

#define MOTION_TIME (6.0*0.016666)
#define MOTION_INITIAL_DELAY (4.0*0.016666)

#define DISP_LEVEL_TIME 2.000

struct field {
  int readhead; // For talking to the bag.
  struct player {
    int playerid; // Unrelated to index in this list.
    int pvinput;
    int motion; // -1,0,1
    double motionclock;
    int down;
    int dropc;
    int tetr,xform;
    int x,y; // Add the tetriminoes' relative positions. (x,y) is often <0
    double dropclock;
  } playerv[PLAYER_LIMIT];
  int playerc;
  uint8_t cellv[FIELDW*FIELDH]; // Cemented tiles only.
  double droptime;
  int dirty;
  int level;
  int dropscore; // Per level.
  int linevalue[4]; // How much is each play worth? Varies by level.
  int linec[4]; // [singles,doubles,triples,tetri]
  int score;
  int disp_score;
  int score_texid,score_w,score_h;
  int disp_level;
  int level_texid,level_w,level_h;
  double disp_level_clock;
  int finished;
  uint8_t rmrowv[FIELDH]; // When (rmclock>0), nonzero members here are being removed. Not necessarily contiguous.
  double rmclock; // Counts down while a removal animation in progress.
};

int field_init(struct field *field,int readhead,int playerc,int level);

void field_update(struct field *field,double elapsed);

/* The main render doesn't take dimensions, it's always (FIELDW,FIELDH)*NS_sys_tilesize.
 * Every field has its own "next" area, dimensions negotiable.
 * There's one "score" area, which we render with either one or two fields.
 */
void field_render(struct field *field,int16_t dstx,int16_t dsty);
void field_render_next(struct field *field,int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth);
void field_render_single_score(struct field *field,int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth);
void field_render_combined_score(struct field *l,struct field *r,int16_t dstx,int16_t dsty,int16_t dstw,int16_t dsth);

#endif
