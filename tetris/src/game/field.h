/* field.h
 * Model of one field of tetris play.
 * There can be two fields at once (which is main's problem, not ours, and also not a problem).
 * Each field may have up to 8 players.
 * All fields draw from the global bag -- you initialize us with the readhead id (0,1).
 */
 
#ifndef FIELD_H
#define FIELD_H

struct field {
  int readhead; // For talking to the bag.
  struct player {
    int playerid; // Unrelated to index in this list.
    int pvinput;
    int motion; // -1,0,1
    double motionclock;
    int down;
    int tetr,xform;
    int x,y; // Add the tetriminoes' relative positions. (x,y) is often <0
    double dropclock;
  } playerv[PLAYER_LIMIT];
  int playerc;
  uint8_t cellv[FIELDW*FIELDH]; // Cemented tiles only.
  double droptime;
  int dirty;
  int linevalue[4]; // How much is each play worth? Varies by level.
  int linec[4]; // [singles,doubles,triples,tetri]
  int score;
  int disp_score;
  int score_texid,score_w,score_h;
  int finished;
};

int field_init(struct field *field,int readhead,int playerc);

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
