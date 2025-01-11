#include "game/rpg.h"
#include "map.h"
#include "opt/rom/rom.h"

/* Size limit is more or less arbitrary.
 * Technically it could go up to 0xffff.
 * But I've assumed in several places -- both here and in Egg itself -- that map coords can be 8 bits per axis.
 */
#define MAP_SIZE_LIMIT 256

static const uint8_t default_physics[256]={0};

/* Initialize one map, already split by rom.
 * (dst->rid) should already be populated, and the rest zero. (ie how they arrive from maps_insert())
 */
 
int map_init(struct map *dst,const void *src,int srcc) {
  struct rom_map rmap;
  if (rom_map_decode(&rmap,src,srcc)<0) return -1;
  if ((rmap.w<1)||(rmap.h<1)||(rmap.w>MAP_SIZE_LIMIT)||(rmap.h>MAP_SIZE_LIMIT)) return -1;
  int len=rmap.w*rmap.h;
  if (!(dst->v=malloc(len))) return -1;
  memcpy(dst->v,rmap.v,len);
  dst->w=rmap.w;
  dst->h=rmap.h;
  dst->ov=rmap.v;
  dst->cmdv=rmap.cmdv;
  dst->cmdc=rmap.cmdc;
  dst->physics=default_physics;
  struct rom_command_reader reader={.v=rmap.cmdv,.c=rmap.cmdc};
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
