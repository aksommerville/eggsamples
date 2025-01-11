/* rpg_res.c
 * Global resource TOC.
 */

#include "rpg.h"
#include "world/map.h"
#include "opt/rom/rom.h"

/* Globals.
 */
 
static struct {
  
  // All interesting resources, we record as stored.
  struct rom_res *resv;
  int resc,resa;
  
  // Types that need a live object that remains immutable-ish, we can track those objects too.
  struct map *mapv; int mapc,mapa;
  uint8_t **tsv; int tsc,tsa; // Tilesheet physics tables. We're not tracking rids, this is just storage.
  
} gres={0};

/* Which resources do we need to record?
 * Should be pretty much all the custom types.
 * No need for strings, image, sound, song: Those are managed via different mechanisms.
 */
 
static int rpg_res_should_keep_type(uint8_t tid) {
  switch (tid) {
    case EGG_TID_map:
    case EGG_TID_tilesheet:
    case EGG_TID_sprite:
    case EGG_TID_battle:
    case EGG_TID_fighter:
      return 1;
  }
  return 0;
}

/* Map and tilesheet resources.
 */
 
static int rpg_res_add_map(const struct rom_res *res) {
  if (gres.mapc>=gres.mapa) {
    int na=gres.mapa+32;
    if (na>INT_MAX/sizeof(struct map)) return -1;
    void *nv=realloc(gres.mapv,sizeof(struct map)*na);
    if (!nv) return -1;
    gres.mapv=nv;
    gres.mapa=na;
  }
  if (gres.mapc&&(res->rid<=gres.mapv[gres.mapc-1].rid)) return -1;
  struct map *map=gres.mapv+gres.mapc++;
  memset(map,0,sizeof(struct map));
  map->rid=res->rid;
  int err=map_init(map,res->v,res->c);
  if (err<0) {
    if (err!=-2) fprintf(stderr,"map:%d: Unspecified error decoding map.\n",res->rid);
    return -2;
  }
  return 0;
}

static int rpg_res_link_map(struct map *map) {
  // We actually don't need this.
  // The only thing to link is tilesheet, but those are done in the other direction.
  return 0;
}
 
struct map *rpg_map_get(int rid) {
  int lo=0,hi=gres.mapc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    struct map *q=gres.mapv+ck;
         if (rid<q->rid) hi=ck;
    else if (rid>q->rid) lo=ck+1;
    else return q;
  }
  return 0;
}

static int rpg_res_link_tilesheet(const struct rom_res *res) {
  if (gres.tsc>=gres.tsa) {
    int na=gres.tsa+16;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(gres.tsv,na*sizeof(void*));
    if (!nv) return -1;
    gres.tsv=nv;
    gres.tsa=na;
  }
  uint8_t *dst=calloc(1,256);
  if (!dst) return -1;
  gres.tsv[gres.tsc++]=dst;
  
  struct rom_tilesheet_reader reader;
  if (rom_tilesheet_reader_init(&reader,res->v,res->c)<0) return -1;
  struct rom_tilesheet_entry entry;
  while (rom_tilesheet_reader_next(&entry,&reader)>0) {
    if (entry.tableid==NS_tilesheet_physics) { // The only table we care about
      memcpy(dst+entry.tileid,entry.v,entry.c);
    }
  }
  
  struct map *map=gres.mapv;
  int i=gres.mapc;
  for (;i-->0;map++) {
    if (map->imageid!=res->rid) continue;
    map->physics=dst;
  }
  
  return 0;
}

/* Initialize TOC.
 */
 
int rpg_res_init(const void *rom,int romc) {
  int err,i;
  if (gres.resc) return -1;
  
  /* Build up the raw TOC, and instantiate and store interesting objects.
   */
  struct rom_reader reader;
  if (rom_reader_init(&reader,rom,romc)<0) return -1;
  struct rom_res *res;
  while (res=rom_reader_next(&reader)) {
    if (!rpg_res_should_keep_type(res->tid)) continue;
    if (gres.resc>=gres.resa) {
      int na=gres.resa+64;
      if (na>INT_MAX/sizeof(struct rom_res)) return -1;
      void *nv=realloc(gres.resv,sizeof(struct rom_res)*na);
      if (!nv) return -1;
      gres.resv=nv;
      gres.resa=na;
    }
    gres.resv[gres.resc++]=*res;
    switch (res->tid) {
    
      case EGG_TID_map: if ((err=rpg_res_add_map(res))<0) return err; break;
      // Add new object types here.
      
    }
  }
  if (!gres.resc) return -1;
  
  /* There are some relationships where linkage makes more sense child-to-parent.
   * Pick those off here.
   */
  for (res=gres.resv,i=gres.resc;i-->0;res++) {
    switch (res->tid) {
    
      case EGG_TID_tilesheet: if ((err=rpg_res_link_tilesheet(res))<0) return err; break;
      // Add new child-to-parent linkages here.
      
    }
  }
  
  /* Parent-to-child linkage, the more obvious way.
   */
  for (i=gres.mapc;i-->0;) if ((err=rpg_res_link_map(gres.mapv+i))<0) return err;
  // Add new linkable types here.
  
  return 0;
}

/* Public access.
 */
 
int rpg_res_get(void *dstpp,int tid,int rid) {
  int lo=0,hi=gres.resc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct rom_res *res=gres.resv+ck;
         if (tid<res->tid) hi=ck;
    else if (tid>res->tid) lo=ck+1;
    else if (rid<res->rid) hi=ck;
    else if (rid>res->rid) lo=ck+1;
    else {
      *(const void**)dstpp=res->v;
      return res->c;
    }
  }
  return 0;
}
