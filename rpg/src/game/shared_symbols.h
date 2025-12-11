/* shared_symbols.h
 * Consumed by both the game and the tools.
 */

#ifndef SHARED_SYMBOLS_H
#define SHARED_SYMBOLS_H

#define EGGDEV_importUtil "res,font,graf,stdlib"

#define NS_sys_tilesize 16
#define NS_sys_mapw 20
#define NS_sys_maph 15

#define CMD_map_image     0x20 /* u16:imageid */
#define CMD_map_song      0x21 /* u16:songid */
#define CMD_map_hero      0x22 /* u16:pos */
#define CMD_map_flagtile  0x40 /* u16:pos u16:flag ; Increment tileid by 1 if (flag) set. */
#define CMD_map_toggle    0x41 /* u16:pos u16:flag ; Toggle flag when bumped or stepped on. Usually accompanied by (flagtile). */
#define CMD_map_neighbors 0x60 /* u16:left u16:right u16:up u16:down */
#define CMD_map_sprite    0x61 /* u16:pos u16:spriteid u32:arg */
#define CMD_map_door      0x62 /* u16:pos u16:mapid u16:dstpos u16:reserved */

#define CMD_sprite_solid  0x01 /* */
#define CMD_sprite_image  0x20 /* u16:imageid */
#define CMD_sprite_tile   0x21 /* u8:tileid u8:xform */
#define CMD_sprite_type   0x22 /* u16:spritetype */
#define CMD_sprite_layer  0x23 /* u8:layer u8:reserved */

#define CMD_battle_gold    0x20 /* u16:gold */
#define CMD_battle_xp      0x21 /* u16:xp */
#define CMD_battle_song    0x22 /* u16:songid */
#define CMD_battle_fighter 0x40 /* u16:fighterid u16:count */

#define CMD_fighter_hp     0x20 /* u16:hp */
#define CMD_fighter_name   0x40 /* u16:stringsid u16:index */
#define CMD_fighter_image  0x41 /* u16:imageid u8:tileid u8:reserved */

#define NS_tilesheet_physics     1
#define NS_tilesheet_neighbors   0
#define NS_tilesheet_family      0
#define NS_tilesheet_weight      0

#define NS_physics_vacant     0
#define NS_physics_solid      1
#define NS_physics_water      2
#define NS_physics_dock       3

#define NS_spritetype_dummy 0
#define NS_spritetype_hero 1
#define NS_spritetype_battle 2
#define FOR_EACH_spritetype \
  _(dummy) \
  _(hero) \
  _(battle)

#define NS_flag_false 0
#define NS_flag_true 1
#define NS_flag_bridge1 2
#define NS_flag_bridge2 3
#define NS_flag_bridge3 4
#define NS_flag_bridge4 5
#define FLAG_COUNT 6

#endif
