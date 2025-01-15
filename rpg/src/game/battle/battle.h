/* battle.h
 * Manages a single encounter.
 * We connect to the broader app via layer_battle, which is a shallow pass-thru.
 */
 
#ifndef BATTLE_H
#define BATTLE_H

struct battle;
struct world;

void battle_del(struct battle *battle);

/* After new, you must call one "setup" function.
 */
struct battle *battle_new();
int battle_setup_res(struct battle *battle,struct world *world,int battleid);
int battle_setup_adhoc(struct battle *battle,struct world *world);//TODO Params.
int battle_setup_pvp(struct battle *battle);//TODO Params.

/* Events from layer.
 * Render overwrites the entire framebuffer.
 * Note that we take (playerid) (1,2) for input, but the rest of the app is not providing that.
 * If you want a 2-player mode, you'll have to change the layer framework, but you won't need much change here in battle.
 */
void battle_suspend(struct battle *battle);
void battle_resume(struct battle *battle,int input1,int input2);
void battle_input(struct battle *battle,int playerid,int btnid,int value,int state);
void battle_update(struct battle *battle,double elapsed);
void battle_render(struct battle *battle);

/* Nonzero to dismiss, either because we've finished or we are not in a valid state.
 */
int battle_is_finished(const struct battle *battle);

#endif
