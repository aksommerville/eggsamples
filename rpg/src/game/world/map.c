#include "game/rpg.h"
#include "map.h"
#include "opt/rom/rom.h"

/* Size limit is more or less arbitrary.
 * Technically it could go up to 0xffff.
 * But I've assumed in several places -- both here and in Egg itself -- that map coords can be 8 bits per axis.
 */
#define MAP_SIZE_LIMIT 256

/* Globals.
 */
 
static struct gmaps {
  struct map *v;
  int c,a;
  uint8_t **tsv; // Storage only. We don't even record the rids, which you would need to use them after load.
  int tsc,tsa;
} gmaps={0};

static const uint8_t default_physics[256]={0};

/* List ops.
 */
 
static int maps_search(int rid) {
  int lo=0,hi=gmaps.c;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct map *q=gmaps.v+ck;
         if (rid<q->rid) hi=ck;
    else if (rid>q->rid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static struct map *maps_insert(int p,int rid) {
  if ((p<0)||(p>gmaps.c)) return 0;
  if (p&&(rid<=gmaps.v[p-1].rid)) return 0;
  if ((p<gmaps.c)&&(rid>=gmaps.v[p].rid)) return 0;
  if (gmaps.c>=gmaps.a) {
    int na=gmaps.a+32; // You must know your map count at build time. Put that here, and we only have to allocate once.
    void *nv=realloc(gmaps.v,sizeof(struct map)*na);
    if (!nv) return 0;
    gmaps.v=nv;
    gmaps.a=na;
  }
  struct map *map=gmaps.v+p;
  memmove(map+1,map,sizeof(struct map));
  gmaps.c++;
  memset(map,0,sizeof(struct map));
  map->rid=rid;
  return map;
}

/* Initialize one map, already split by rom.
 * (dst->rid) should already be populated, and the rest zero. (ie how they arrive from maps_insert())
 */
 
static int map_init(struct map *dst,const struct rom_map *src) {
  if ((src->w<1)||(src->h<1)||(src->w>MAP_SIZE_LIMIT)||(src->h>MAP_SIZE_LIMIT)) {
    fprintf(stderr,"map:%d invalid size %dx%d\n",dst->rid,src->w,src->h);
    return -1;
  }
  int len=src->w*src->h;
  if (!(dst->v=malloc(len))) return -1;
  memcpy(dst->v,src->v,len);
  dst->w=src->w;
  dst->h=src->h;
  dst->ov=src->v;
  dst->cmdv=src->cmdv;
  dst->cmdc=src->cmdc;
  dst->physics=default_physics;
  struct rom_command_reader reader={.v=src->cmdv,.c=src->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
    
      case CMD_map_image: dst->imageid=(cmd.argv[0]<<8)|cmd.argv[1]; break;
      case CMD_map_song: dst->songid=(cmd.argv[0]<<8)|cmd.argv[1]; break;
      // Pick off any other commands that we want to record special.
    }
  }
  // Opportunity here for further validation. Return <0 to abort app startup if anything's fishy.
  return 0;
}

/* Add map to global store.
 */
 
static int maps_add(int rid,const void *src,int srcc) {
  int p=maps_search(rid);
  if (p>=0) return -1;
  p=-p-1;
  struct rom_map rmap={0};
  if (rom_map_decode(&rmap,src,srcc)<0) {
    fprintf(stderr,"map:%d invalid! rom_map_decode of %d bytes failed.\n",rid,srcc);
    return -1;
  }
  struct map *map=maps_insert(p,rid);
  if (!map) return -1;
  return map_init(map,&rmap);
}

/* Add tilesheet to global store.
 * (EGG_TID_tilesheet>EGG_TID_map), so when this is called, all the maps are already loaded.
 * After each tilesheet load, we locate the impacted maps and update them.
 */
 
static int maps_load_tilesheet(int rid,const void *src,int srcc) {
  if (gmaps.tsc>=gmaps.tsa) {
    int na=gmaps.tsa+16;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(gmaps.tsv,sizeof(void*)*na);
    if (!nv) return -1;
    gmaps.tsv=nv;
    gmaps.tsa=na;
  }
  struct rom_tilesheet_reader reader;
  if (rom_tilesheet_reader_init(&reader,src,srcc)<0) return -1;
  struct rom_tilesheet_entry entry;
  uint8_t *nv=calloc(1,256);
  if (!nv) return -1;
  gmaps.tsv[gmaps.tsc++]=nv;
  while (rom_tilesheet_reader_next(&entry,&reader)>0) {
    if (entry.tableid==NS_tilesheet_physics) {
      memcpy(nv+entry.tileid,entry.v,entry.c);
    }
  }
  struct map *map=gmaps.v;
  int i=gmaps.c;
  for (;i-->0;map++) {
    if (map->imageid==rid) {
      map->physics=nv;
    }
  }
  return 0;
}

/* Initialize global store.
 */

int maps_init(const void *rom,int romc) {
  if (gmaps.c) return -1; // Once only, please.
  struct rom_reader reader;
  if (rom_reader_init(&reader,rom,romc)<0) return -1;
  struct rom_res *res;
  while (res=rom_reader_next(&reader)) {
    if (res->tid<EGG_TID_map) {
      continue;
    } else if (res->tid==EGG_TID_map) {
      if (maps_add(res->rid,res->v,res->c)<0) return -1;
    } else if (res->tid==EGG_TID_tilesheet) {
      if (maps_load_tilesheet(res->rid,res->v,res->c)<0) return -1;
    } else {
      break;
    }
  }
  //fprintf(stderr,"%s:%d: Loaded global map store, %d maps\n",__FILE__,__LINE__,gmaps.c);
  return 0;
}

/* Public access to store.
 */

struct map *map_by_rid(int rid) {
  int p=maps_search(rid);
  if (p<0) return 0;
  return gmaps.v+p;
}

struct map *map_by_index(int p) {
  if ((p<0)||(p>=gmaps.c)) return 0;
  return gmaps.v+p;
}

int maps_count() {
  return gmaps.c;
}
