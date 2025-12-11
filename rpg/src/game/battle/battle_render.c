#include "battle_internal.h"

/* Render, main entry point.
 */
 
void battle_render(struct battle *battle) {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
  scene_render(battle->scene);
  prompt_render(battle->prompt);
  roster_render(battle->rosterv[0]);
  roster_render(battle->rosterv[1]);
  bmenu_render(battle->bmenuv[0]);
  bmenu_render(battle->bmenuv[1]);
}
