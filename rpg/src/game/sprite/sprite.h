/* sprite.h
 * Compared to most games, our sprite object is pretty lean.
 * Sprites are owned by world.
 * There's basically nothing in the way of physics. Hero manages what little there is.
 */
 
#ifndef SPRITE_H
#define SPRITE_H

struct sprite;
struct sprite_type;

struct sprite {
  const struct sprite_type *type;
  uint16_t rid; // Nonzero if we instantiated from a resource.
  const void *cmdv; // Content of that resource (command list).
  int cmdc;
  uint32_t arg; // From the map's spawn point.
  int defunct; // Set nonzero to delete at the end of the update cycle.
  
  /* We generically take a few things from the resource for rendering.
   * If you're OK with a single tile, use these and do not implement render().
   */
  int imageid;
  uint8_t tileid;
  uint8_t xform;
  
  double x,y; // Center of the sprite in map cells.
  int layer; // Render order. We default to 0x80 and suggest putting the hero there.
  int always_render; // We assume that sprites have a half-tile radius and cull OOBs. Set this nonzero to render even if OOB.
  int solid; // Hero can't pass thru me. If set, the sprite must either be stationary, or carefully avoid moving when the hero is near.
};

struct sprite_type {
  const char *name;
  int objlen;
  void (*del)(struct sprite *sprite);
  int (*init)(struct sprite *sprite);
  
  void (*update)(struct sprite *sprite,double elapsed);
  
  /* Implement only if the sprite's rendering is more complex than a single tile.
   * (x,y) is the sprite's position projected to the main framebuffer.
   */
  void (*render)(struct sprite *sprite,int16_t x,int16_t y);
  
  /* If (sprite->solid), this triggers when the hero bumps into me.
   * Otherwise, when the hero's center comes within half a tile of mine.
   */
  void (*bump)(struct sprite *sprite,struct sprite *hero);
};

/* We manage a global store of sprite resources, similar to map.
 * Callers don't need to care.
 */
int sprites_init(const void *rom,int romc);
int sprite_res_get(const struct sprite_type **type,void *dstpp,int rid);

/* Sprite type IDs are defined in shared_symbols.h.
 */
const struct sprite_type *sprite_type_by_id(int id);
#define _(tag) extern const struct sprite_type sprite_type_##tag;
FOR_EACH_spritetype
#undef _

/* Only world may call this.
 * Anyone else wanting to delete a sprite should set (sprite->defunct) nonzero and wait.
 */
void sprite_del(struct sprite *sprite);

/* Create a sprite programmatically.
 * You may supply anything for (type,rid,cmdv,cmdc), they don't have to agree and don't have to be a real resource.
 * (cmdv) will be borrowed weakly. It's assumed to come from the ROM. (but any permanent immutable source is fine).
 * Should be a command list, ie the sprite resource with its 4-byte signature stripped.
 * Returns WEAK.
 */
struct sprite *sprite_new(const struct sprite_type *type,int rid,const void *cmdv,int cmdc,double x,double y,uint32_t arg);

/* Create a sprite from a resource, at a given map tile.
 * Returns WEAK.
 */
struct sprite *sprite_new_res(int rid,uint8_t x,uint8_t y,uint32_t arg);

/* API for specific sprite types.
 ********************************************************************************/
 
void hero_motion(struct sprite *sprite,int dx,int dy,int value);

#endif
