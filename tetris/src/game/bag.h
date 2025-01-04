/* bag.h
 * Bag is the source of tetrominoes.
 * The one bag has two readheads, so in a two-field game, each field gets exactly the same draw.
 * Players within a field draw from the same source -- bag itself has no concept of players.
 * We use a 14-bag strategy: In each set of 14 draws, each tetromino will appear exactly twice.
 * Bag uses its own globals, independent of the app's globals.
 * Also, we're 100% finite-state and self-contained. No stdlib, nothing. About 150 bytes of globals (which you could reduce to more like 40, by using 8-bit ints where possible).
 */
 
#ifndef BAG_H
#define BAG_H

/* Tetromino IDs.
 */
#define TETR_I 0
#define TETR_L 1
#define TETR_J 2
#define TETR_S 3
#define TETR_Z 4
#define TETR_O 5
#define TETR_T 6

/* Initialize a new bag.
 * (readheadc) must be 1 or 2.
 * Provide (randseed) from the system clock, or some other source of entropy. All numbers are valid.
 */
int bag_reset(int readheadc,int randseed);

/* Draw a tetromino and advance the bag's state.
 * If we're not initialized, it's TETR_I (zero) every time.
 * For single-readhead bags, the argument is ignored.
 * Double-readhead, (which) must be zero or one.
 */
int bag_draw(int which);

/* Same as 'draw' but do not advance the internal state.
 */
int bag_peek(int which);

/* Fill (dstv) with the positions of four tiles describing tetromino (tetr) with the given (xform).
 * We only look at the low 2 bits of (xform), and interpret them as quarter-turns clockwise.
 * The origin of the returned space is the point around which they rotate.
 * (dsta) should always be 4, and the return value is either 4, or 0 if (tetr) invalid.
 */
struct tetr_tile {
  int x,y; // 0..3
};
int tetr_tile_shape(struct tetr_tile *dstv,int dsta,int tetr,int xform);

#endif
