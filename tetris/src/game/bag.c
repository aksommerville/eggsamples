#include "bag.h"

/* Globals.
 */
 
static struct {
  int twin; // Nonzero if right readhead enabled.
  unsigned int prngl,prngr;
  int lv[14],rv[14];
  int lp,rp; // 0..13
} bag={0};

/* PRNG.
 * Same algorithm Egg's rand() uses, which I yoinked off Wikipedia some time ago.
 * I've confirmed that it is very uniform, and that it only fails when the initial seed is zero.
 * People who actually know what they're talking about recommend other more complex algorithms, but I don't get it.
 */
 
static unsigned int bag_rand_l() {
  bag.prngl^=bag.prngl<<13;
  bag.prngl^=bag.prngl>>17;
  bag.prngl^=bag.prngl<<5;
  return bag.prngl;
}
 
static unsigned int bag_rand_r() {
  bag.prngr^=bag.prngr<<13;
  bag.prngr^=bag.prngr>>17;
  bag.prngr^=bag.prngr<<5;
  return bag.prngr;
}

static void bag_srand(int seed) {
  // Every seed is fine except zero, so force that to one instead.
  bag.prngl=bag.prngr=seed?seed:1;
}

/* Reset.
 */
 
int bag_reset(int readheadc,int randseed) {
  if ((readheadc<1)||(readheadc>2)) return -1;
  bag.twin=readheadc-1;
  bag_srand(randseed);
  bag.lp=bag.rp=14; // Both sides get their next bag, next time they're drawn.
  return 0;
}

/* Refill one bag.
 * (src) is a fresh random number.
 * There's an upper limit of ((7**7)*7!) possible states for a bag.
 * That just happens to be 4150656720, just a bit under 2**32, lucky!
 */
 
static void bag_shuffle(int *dst,unsigned int src) {
  int remaining_by_id[7]={2,2,2,2,2,2,2};
  int idv[7]={0,1,2,3,4,5,6}; // Shuffles down as ids get eliminated.
  int remaining=14;
  int modulus=7;
  for (;remaining-->0;dst++) {
    int selection=src%modulus;
    src/=modulus;
    int id=idv[selection];
    if (remaining_by_id[id]==1) {
      remaining_by_id[id]=0;
      int i=modulus; while (i-->0) {
        if (idv[i]==id) {
          i++; for (;i<modulus;i++) idv[i-1]=idv[i];
          modulus--;
          break;
        }
      }
    } else {
      remaining_by_id[id]=1;
    }
    *dst=id;
  }
}

/* Draw.
 */
 
int bag_draw(int which) {
  int tetr=bag_peek(which);
  if (which&&bag.twin) { // R
    bag.rp++;
  } else { // L
    bag.lp++;
  }
  return tetr;
}

/* Peek.
 */

int bag_peek(int which) {
  if (which&&bag.twin) { // R
    if (bag.rp>=14) { bag_shuffle(bag.rv,bag_rand_r()); bag.rp=0; }
    return bag.rv[bag.rp];
  } else { // L
    if (bag.lp>=14) { bag_shuffle(bag.lv,bag_rand_l()); bag.lp=0; }
    return bag.lv[bag.lp];
  }
}
