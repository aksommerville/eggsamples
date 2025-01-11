#include "game/rpg.h"
#include "game/ui/textbox.h"
#include "battle.h"

/* Render, main entry point.
 */
 
void battle_render(struct battle *battle) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
  textbox_render(battle->menu0);
  textbox_render(battle->menu1);
  textbox_render(battle->prompt);
  textbox_render(battle->roster0);
  textbox_render(battle->roster1);
  
  int texid=texcache_get_image(&g.texcache,RID_image_battlebg);
  int bgcol=0,bgrow=0;//TODO
  int16_t srcx=bgcol*battle->scenew;
  int16_t srcy=bgrow*battle->sceneh;
  graf_draw_decal(&g.graf,texid,battle->scenex,battle->sceney,srcx,srcy,battle->scenew,battle->sceneh,0);
  //TODO Sprites.
  textbox_render_border(battle->scenex,battle->sceney,battle->scenew,battle->sceneh);
}
