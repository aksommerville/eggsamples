/* battle.h
 * Model for battle scenes.
 * These are always owned by layer_battle (which is a shallow pass-through to us).
 *
 * A Battle consists of two Teams, each containing one or more Fighters.
 * For now, we must have one CPU Team and one Man Team, but other combinations should be possible without too much drama.
 * I'm not going to have multiple fighters in the player's party for this demo.
 * If you want that, it shouldn't require much change here in battle. (world is a different story).
 */
 
#ifndef BATTLE_H
#define BATTLE_H

struct textbox;

#define FIGHTER_NAME_LIMIT 16 /* Arbitrary. */

// Team controllers. These will be locked to (MAN,CPU) in this demo.
// One could make a player-vs-player mode, probably want to split "MAN" into "P1" and "P2" in that case.
#define BATTLE_CONTROL_CPU 1
#define BATTLE_CONTROL_MAN 2

struct battle {
  int battleid;
  int ready; // True after successful setup.
  int finished; // Owner should poll this.
  int control0,control1; // Who controls each team?
  int gold,xp;
  
  struct fighter {
    int team; // 0,1. All team-zero fighters must be first in the list, followed by all team-one fighters. Must have at least one of each.
    int imageid;
    uint8_t tileid;
    int hp;
    int hpmax;
    char name[FIGHTER_NAME_LIMIT];
    int namec; // 0..FIGHTER_NAME_LIMIT, clamped at construction.
    //TODO Further stats: stength, defense, speed, etc.
    
    // Upcoming move:
    int input_state; // 0,1,2,3=need action,need qualifier,need target,ready. Only relevant under MAN control.
    char action; // 0,'a','m','i','d','r'=unselected,attack,magic,item,defend,run
    int qualifier; // Depends on (action). Spell or item index. Attack is always just the one equipped item, can't change during battle.
    int target; // Index of fighter to target, or <0 if not applicable per (action).
    
    int disphp; // Cache the currently displayed HP, so we can change (hp) freely and the view reacts.
  } *fighterv;
  int fighterc,fightera;
  
  // List of WEAK fighters, owned by (fighterv), in the order they are going to attack.
  // Rebuilt at the start of each action cycle.
  struct fighter **priov;
  int prioc;
  int priop;
  int priosub;
  
  struct fighter *fighterfocus; // WEAK. Null or one of (fighterv), which one we're configuring.
  struct textbox *tbfocus; // WEAK. Null or one of the real textboxes, which one has focus.
  struct textbox *menu0;
  struct textbox *menu1;
  struct textbox *prompt;
  struct textbox *roster0;
  struct textbox *roster1;
  int scenex,sceney,scenew,sceneh;
  
  // State flow flags.
  int introduced;
  int farewelled;
  int outcome_reported;
};

void battle_del(struct battle *battle);
struct battle *battle_new();

/* Owner must call one "setup" function after instantiating and before any other op.
 * "res": Man vs CPU, and the CPU team is configured from a "battle" resource. The Man team interacts with the global world.
 * If you want man-vs-man or cpu-vs-cpu, or for some reason you need to configure a battle programmatically, add a new "setup" for it.
 */
int battle_setup_res(struct battle *battle,int battleid);

void battle_suspend(struct battle *battle);
void battle_resume(struct battle *battle,int state);
void battle_input(struct battle *battle,int btnid,int value,int state);
void battle_update(struct battle *battle,double elapsed);
void battle_render(struct battle *battle);

int battle_advance(struct battle *battle);

/* 0 or 1 if such a team exists.
 * "single" means only return it if the other team is the other type.
 */
int battle_get_single_man_team(const struct battle *battle);
int battle_get_single_cpu_team(const struct battle *battle);

#endif
