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
#include "opt/stdlib/egg-stdlib.h"
 
int bag_reset(int readheadc,int randseed) {
  if ((readheadc<1)||(readheadc>2)) return -1;
  fprintf(stderr,"%s seed=0x%08x\n",__func__,randseed);
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

/* Tetromino shapes and transforms.
 * It's not related to bagging but this seems like a reasonable place for it.
 */
 
#define ________ 0x0
#define ______XX 0x1
#define ____XX__ 0x2
#define ____XXXX 0x3
#define __XX____ 0x4
#define __XX__XX 0x5
#define __XXXX__ 0x6
#define __XXXXXX 0x7
#define XX______ 0x8
#define XX____XX 0x9
#define XX__XX__ 0xa
#define XX__XXXX 0xb
#define XXXX____ 0xc
#define XXXX__XX 0xd
#define XXXXXX__ 0xe
#define XXXXXXXX 0xf
 
#define SHAPE(name,n0,e0,s0,w0,n1,e1,s1,w1,n2,e2,s2,w2,n3,e3,s3,w3,dummy) \
  [TETR_##name*4+0]=((n0<<12)|(n1<<8)|(n2<<4)|n3), \
  [TETR_##name*4+1]=((e0<<12)|(e1<<8)|(e2<<4)|e3), \
  [TETR_##name*4+2]=((s0<<12)|(s1<<8)|(s2<<4)|s3), \
  [TETR_##name*4+3]=((w0<<12)|(w1<<8)|(w2<<4)|w3),
static const unsigned short shapes[7*4]={
  SHAPE(I,
    ________,____XX__,________,____XX__,
    ________,____XX__,________,____XX__,
    XXXXXXXX,____XX__,XXXXXXXX,____XX__,
    ________,____XX__,________,____XX__,
  )
  SHAPE(L,
    ________,XXXX____,____XX__,__XX____,
    XXXXXX__,__XX____,XXXXXX__,__XX____,
    XX______,__XX____,________,__XXXX__,
    ________,________,________,________,
  )
  SHAPE(J,
    ________,__XX____,XX______,__XXXX__,
    XXXXXX__,__XX____,XXXXXX__,__XX____,
    ____XX__,XXXX____,________,__XX____,
    ________,________,________,________,
  )
  SHAPE(S,
    ________,__XX____,________,__XX____,
    __XXXX__,__XXXX__,__XXXX__,__XXXX__,
    XXXX____,____XX__,XXXX____,____XX__,
    ________,________,________,________,
  )
  SHAPE(Z,
    ________,____XX__,________,____XX__,
    XXXX____,__XXXX__,XXXX____,__XXXX__,
    __XXXX__,__XX____,__XXXX__,__XX____,
    ________,________,________,________,
  )
  SHAPE(O,
    ________,________,________,________,
    __XXXX__,__XXXX__,__XXXX__,__XXXX__,
    __XXXX__,__XXXX__,__XXXX__,__XXXX__,
    ________,________,________,________,
  )
  SHAPE(T,
    ________,____XX__,____XX__,____XX__,
    __XXXXXX,__XXXX__,__XXXXXX,____XXXX,
    ____XX__,____XX__,________,____XX__,
    ________,________,________,________,
  )
};
#undef SHAPE
 
int tetr_tile_shape(struct tetr_tile *dstv,int dsta,int tetr,int xform) {
  if ((tetr<0)||(tetr>=7)) return 0;
  if (!dstv||(dsta<4)) return 0;
  int dstc=0;
  unsigned short bits=shapes[tetr*4+(xform&3)];
  unsigned short mask=0x8000;
  int y=0; for (;y<4;y++) {
    int x=0; for (;x<4;x++,mask>>=1) {
      if (bits&mask) {
        if (dstc>=dsta) return dstc;
        dstv[dstc++]=(struct tetr_tile){x,y};
      }
    }
  }
  return dstc;
}
