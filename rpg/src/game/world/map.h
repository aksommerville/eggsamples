/* map.h
 * Map resources are decoded at init and kept in a global cache.
 * The map object itself is mostly immutable: The cells can be changed, but only by privileged parties.
 * If you change map cells willy-nilly, the changes will eventually get reverted.
 */
 
#ifndef MAP_H
#define MAP_H

struct map {
  int rid;
  int w,h;
  uint8_t *v; // mutable-ish
  const uint8_t *ov; // strictly read-only, owned by rom.
  const void *cmdv; // ''
  int cmdc;
  const uint8_t *physics; // 256 NS_physics_* values, indexed by tileid. Never null.
  // Other properties we need often can go here, we can pull them from (cmdv) at init.
  uint16_t imageid;
  uint16_t songid;
};

/* (map) must be initially zero except (rid).
 * (src,srcc) is an entire "map" resource.
 * We initially set (physics) to an all-zeroes dummy.
 */
int map_init(struct map *map,const void *src,int srcc);

#endif
