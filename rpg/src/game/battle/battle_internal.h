#ifndef BATTLE_INTERNAL_H
#define BATTLE_INTERNAL_H

#include "game/rpg.h"
#include "game/world/world.h"
#include "game/ui/textbox.h"
#include "battle.h"
#include "battle_ui.h"

#define FIGHTER_NAME_LIMIT 16

#define TEAM_CONTROL_MAN 1
#define TEAM_CONTROL_CPU 2

struct fighter {
  int id; // Unique nonzero id assigned at construction.
  int fighterid; // Resource ID or zero. Not unique.
  int hp;
  int hpmax;
  char name[FIGHTER_NAME_LIMIT];
  int namec;
  int imageid;
  uint8_t tileid;
  
  // Selections for next move:
  char action; // 0,'a','m','i','d','r'
  int qualifier; // Spell or item id.
  int target; // Fighter id.
};

struct team {
  int control;
  
  /* The set of fighters is stable after init, but not during init.
   */
  struct fighter *fighterv;
  int fighterc,fightera;
  
  int focus_fighter; // id of some fighter we're passively highlighting as a target.
};

struct battle {
  int ready; // Nonzero after successful setup.
  int finished;
  int battleid;
  struct world *world; // WEAK, OPTIONAL. Required for normal resource battles.
  int idnext;
  
  int gold;
  int xp;
  int songid;
  
  // Volatile state, tells us the next step.
  int welcomed;
  int outcome; // -2,-1,0,1=run,left,pending,right -- NB -1 typically means player wins.
  int farewelled;
  struct fighter **priov; // Fighters, in the order they will act. Populated at the start of each commission phase.
  int prioc,prioa,priop;
  int priosubp; // Progress within (priov[priop])'s attack.
  
  // We're loose about how many fighters are allowed, but 2 teams is a firm rule.
  struct team teamv[2];
  
  // UI elements.
  struct roster *rosterv[2];
  struct bmenu *bmenuv[2];
  struct scene *scene;
  struct prompt *prompt;
};

struct fighter *battle_fighter_by_id(struct battle *battle,int id);

/* Catch-all for moving the the next state.
 */
void battle_advance(struct battle *battle);
void battle_receive_bmenu_input(struct battle *battle,struct bmenu *bmenu,struct team *team,int optionid,int teamp);
void battle_retreat(struct battle *battle,struct team *team,int teamp);

int battle_for_each_spell(struct battle *battle,struct fighter *fighter,int (*cb)(int qualifier,const char *name,int namec,void *userdata),void *userdata);
int battle_for_each_item(struct battle *battle,struct fighter *fighter,int (*cb)(int qualifier,const char *name,int namec,void *userdata),void *userdata);
int battle_spell_name_by_qualifier(char *dst,int dsta,struct battle *battle,struct fighter *fighter,int qualifier);
int battle_item_name_by_qualifier(char *dst,int dsta,struct battle *battle,struct fighter *fighter,int qualifier);
int battle_fighter_is_ready(struct battle *battle,struct team *team,struct fighter *fighter);
int battle_fighter_requires_qualifier(struct battle *battle,struct fighter *fighter);
int battle_fighter_requires_target(struct battle *battle,struct fighter *fighter);
int battle_fighter_is_target(struct battle *battle,struct team *ateam,struct fighter *afighter,struct team *dteam,struct fighter *dfighter);
void battle_ready_cpu_fighter(struct battle *battle,struct team *team,struct fighter *fighter);
int battle_prioritize_fighters(struct fighter **dstv,int dsta,struct battle *battle);
int battle_calculate_attack_damage(struct battle *battle,struct fighter *attacker,struct fighter *defender);
int battle_try_run(struct battle *battle,struct fighter *coward); // => nonzero to escape
int battle_cast_spell(struct battle *battle,struct fighter *fighter,int qualifier,struct fighter *target); // Applies effect and cost. Nonzero if effective at all.
int battle_use_item(struct battle *battle,struct fighter *fighter,int qualifier,struct fighter *target); // Applies effect and cost. Nonzero if effective at all.

#endif
