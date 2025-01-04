#include "bag.h"

/* Globals.
 */
 
#define WEIGHT_INITIAL 600
#define WEIGHT_LIMIT 5000
#define WEIGHT_UNLIKELY 10
#define WEIGHT_INCREASE 20
 
static struct bag {
  int headc;
  struct head {
    unsigned int prng;
    int next; // -1 if not drawn, otherwise 0..6
    int wv[7];
  } headv[2];
} bag={0};

/* Reset.
 */
 
int bag_reset(int readheadc,int randseed) {
  if ((readheadc<1)||(readheadc>2)) return -1;
  bag.headc=readheadc;
  struct head *head=bag.headv;
  int i=bag.headc; 
  for (;i-->0;head++) {
    head->prng=randseed?randseed:1;
    int wi=7; 
    int *w=head->wv;
    for (;wi-->0;w++) *w=WEIGHT_INITIAL;
  }
  return 0;
}

/* Advance readhead if (next) unset.
 */
 
static void head_require(struct head *head) {
  if (head->next>=0) return;
  
  // Xorshift PRNG.
  head->prng^=head->prng<<13;
  head->prng^=head->prng>>17;
  head->prng^=head->prng<<5;
  
  // Mod by the sum of weights.
  int *w=head->wv;
  int range=0;
  int i=7;
  for (;i-->0;w++) range+=*w;
  int choice=head->prng%range;
  
  // That weighted choice tells us which tetromino.
  head->next=0;
  for (w=head->wv;head->next<6;w++,head->next++) {
    if ((choice-=*w)<0) break;
  }
  
  // Adjust weights: The chosen tetromino drops to a constant and the others increase, clamping at some constant.
  for (i=0,w=head->wv;i<7;i++,w++) {
    if (i==head->next) *w=WEIGHT_UNLIKELY;
    else if (((*w)+=WEIGHT_INCREASE)>WEIGHT_LIMIT) *w=WEIGHT_LIMIT;
  }
}

/* Draw.
 */
 
int bag_draw(int which) {
  if ((which<0)||(which>=bag.headc)) which=0;
  struct head *head=bag.headv+which;
  head_require(head);
  int tetr=head->next;
  head->next=-1;
  return tetr;
}

/* Peek.
 */
 
int bag_peek(int which) {
  if ((which<0)||(which>=bag.headc)) which=0;
  struct head *head=bag.headv+which;
  head_require(head);
  return head->next;
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
