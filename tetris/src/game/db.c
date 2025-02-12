/* db.c
 * Manages persistence of high scores.
 *
 * We'll store the top ten by score for each player count.
 * Key: "hsN" N in '1'..'8'.
 * Value is up to ten lines (LF) of space-delimited text: SCORE LINEC TIME ...
 * Values must be sorted by score descending, and scores must be >0.
 *
 * Also, the most recent play goes under key "recent": SCORE LINEC TIME PLAYERC
 * I want that in the store in case we monitor the save file for a cluster high score bot at conventions.
 *
 * 999999 9999 2000-01-01T00:00 8 = 31 bytes including the LF
 */

#include "tetris.h"

/* Globals.
 * Buckets are encoded at rest. We decode everything from scratch on demand.
 */
 
static struct {
  struct bucket {
    char text[512]; // Shouldn't exceed 310.
    int textc;
  } bucketv[8]; // Index is playerc-1
  int modseq;
} db={0};

/* Init.
 */

void db_init() {
  db.modseq=0;
  struct bucket *bucket=db.bucketv;
  int playerc=1;
  for (;playerc<=8;playerc++,bucket++) {
    char k[4]={'h','s','0'+playerc,0};
    bucket->textc=egg_store_get(bucket->text,sizeof(bucket->text),k,3);
    if ((bucket->textc<0)||(bucket->textc>sizeof(bucket->text))) bucket->textc=0;
    if (bucket->textc) db.modseq=1;
  }
}

/* Merge an encoded bucket with incoming text.
 */
 
static int db_merge(char *dst,int dsta,const char *pv,int pvc,int nscore,const char *n,int nc) {
  int dstc=0,pvp=0,dstlinec=0,copied=0;
  while (pvp<pvc) {
  
    // Read the next line from the previous text.
    if ((unsigned char)pv[pvp]<=0x20) { pvp++; continue; }
    const char *pvline=pv+pvp;
    int pvlinec=0;
    while ((pvp<pvc)&&(pv[pvp++]!=0x0a)) pvlinec++;
    while (pvlinec&&((unsigned char)pvline[pvlinec-1]<=0x20)) pvlinec--;
    if (!pvlinec) continue;
    
    // Decode its score.
    int pvscore=0;
    int pvlinep=0;
    while ((pvlinep<pvlinec)&&(pvline[pvlinep]>='0')&&(pvline[pvlinep]<='9')) {
      pvscore*=10;
      pvscore+=pvline[pvlinep]-'0';
      pvlinep++;
      if (pvscore>SCORE_LIMIT) { pvscore=0; break; }
    }
    
    // Incoming score goes here if the score >=previous. (ie ties break to the newer one).
    if (!copied&&(nscore>=pvscore)) {
      copied=1;
      if (dstc<=dsta-nc) memcpy(dst+dstc,n,nc);
      dstc+=nc;
      dstlinec++;
      if (dstlinec>=10) break;
    }
    
    // Emit this previous score. Mind that we've trimmed the newline.
    if (dstc<dsta-pvlinec) {
      memcpy(dst+dstc,pvline,pvlinec);
      dst[dstc+pvlinec]=0x0a;
    }
    dstc+=pvlinec+1;
    dstlinec++;
    if (dstlinec>=10) break;
  }
  
  // If we haven't rolled the new one in yet, and there's fewer than ten, append it.
  if (!copied&&(dstlinec<10)) {
    if (dstc<=dsta-nc) memcpy(dst+dstc,n,nc);
    dstc+=nc;
    copied=1;
  }
  
  if (copied) {
    if (db.modseq==INT_MAX) db.modseq=1; // one admires this user's tenacity
    else db.modseq++;
  }
  
  return dstc;
}

/* Add encoded score to the appropriate bucket.
 */
 
static int db_add_to_bucket(int playerc,int score,const char *text,int textc) {
  if ((playerc<1)||(playerc>8)) return -1;
  struct bucket *bucket=db.bucketv+playerc-1;
  char ntext[512];
  int ntextc=db_merge(ntext,sizeof(ntext),bucket->text,bucket->textc,score,text,textc);
  if ((ntextc<0)||(ntextc>sizeof(ntext))) return -1;
  memcpy(bucket->text,ntext,ntextc);
  bucket->textc=ntextc;
  char k[4]={'h','s','0'+playerc,0};
  return egg_store_set(k,3,ntext,ntextc);
}

/* Add score.
 */
 
int db_add(int playerc,int linec,int score) {
  if ((playerc<1)||(playerc>8)) return -1;
  if (linec<0) return -1;
  if (linec>9999) linec=9999;
  if ((score<1)||(score>SCORE_LIMIT)) return -1;
  int t[5]={0};
  egg_time_local(t,5);
  char text[64];
  int textc=snprintf(text,sizeof(text),"%d %d %04d-%02d-%02dT%02d:%02d %d\n",score,linec,t[0],t[1],t[2],t[3],t[4],playerc);
  if ((textc<1)||(textc>=sizeof(text))) return -1;
  egg_store_set("recent",6,text,textc);
  db_add_to_bucket(playerc,score,text,textc);
  return 0;
}

/* Trivial accessors.
 */
 
int db_modseq() {
  return db.modseq;
}

/* Parse one score line.
 */
 
static int parse_score(struct db_score *dst,const char *src,int srcc) {
  int srcp;
  memset(dst,0,sizeof(struct db_score));
  
  // Find the LF and ensure we consume exactly that much.
  for (srcp=0;srcp<srcc;srcp++) if (src[srcp]==0x0a) { srcc=srcp+1; break; }
  srcp=0;
  
  #define DECUINT(fldname,limit) { \
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++; \
    while ((srcp<srcc)&&(src[srcp]>='0')&&(src[srcp]<='9')) { \
      dst->fldname*=10; \
      dst->fldname+=src[srcp++]-'0'; \
      if (dst->fldname>limit) dst->fldname=limit; \
    } \
  }
  
  DECUINT(score,999999)
  DECUINT(linec,9999)
  DECUINT(year,9999)
  if ((srcp<srcc)&&(src[srcp]=='-')) {
    srcp++;
    DECUINT(month,99)
    if ((srcp<srcc)&&(src[srcp]=='-')) {
      srcp++;
      DECUINT(day,99)
      if ((srcp<srcc)&&(src[srcp]=='T')) {
        srcp++;
        DECUINT(hour,99)
        if ((srcp<srcc)&&(src[srcp]==':')) {
          srcp++;
          DECUINT(minute,99)
        }
      }
    }
  }
  DECUINT(playerc,8)
  
  #undef DECUINT
  return srcc;
}

/* Fetch sorted interleaved scores.
 */
 
int db_get_scores(struct db_score *dst,int dsta) {
  int dstc=0;
  
  /* Prepare 8 readers, one for each bucket.
   * (reader.score.score) zero means we haven't populated (score) yet.
   */
  struct reader {
    const char *src;
    int srcc;
    int srcp;
    struct db_score score;
  } readerv[8]={0};
  int playerc=8;
  while (playerc-->0) {
    readerv[playerc].src=db.bucketv[playerc].text;
    readerv[playerc].srcc=db.bucketv[playerc].textc;
  }
  
  /* Buckets are always sorted by score descending, the same order we want to output.
   * So just walk thru them incrementally and consume the highest score.
   */
  while (dstc<dsta) {
  
    // Load each reader's score if not loaded yet, detect the highest, and detect completion.
    int done=1,hiscore=0,hiplayerc=0;
    struct reader *reader=readerv;
    for (playerc=0;playerc<8;playerc++,reader++) {
      if (!reader->score.score) {
        if (reader->srcp>=reader->srcc) continue;
        int err=parse_score(&reader->score,reader->src+reader->srcp,reader->srcc-reader->srcp);
        if ((err<=0)||(reader->score.score<1)) {
          reader->score.score=0;
          reader->srcp=reader->srcc;
          continue;
        }
        reader->srcp+=err;
      }
      done=0;
      if (reader->score.score>=hiscore) {
        hiscore=reader->score.score;
        hiplayerc=playerc;
      }
    }
    if (done) break;
    
    dst[dstc]=readerv[hiplayerc].score;
    dst[dstc].playerc=hiplayerc+1; // playerc not necessarily present yet
    dstc++;
    readerv[hiplayerc].score.score=0;
  }
  return dstc;
}
