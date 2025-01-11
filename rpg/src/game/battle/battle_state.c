/* battle_state.c
 * Most of the business logic for battles lives here.
 * And I've tried to put the more interesting bits higher up (scoring, decision making, etc).
 */

#include "game/rpg.h"
#include "game/ui/textbox.h"
#include "battle.h"

/* Select action for a CPU player.
 * This is triggered by the state-change logic but does not affect state, it gets resolved immediately here.
 */
 
static void battle_select_cpu_action(struct battle *battle,struct fighter *fighter) {
  //TODO CPU fighter AI. For now, we will always "Attack".
  fighter->action='a';
  fighter->qualifier=-1;
  fighter->target=-1;
  const struct fighter *other=battle->fighterv;
  int i=0;
  for (;i<battle->fighterc;i++,other++) {
    if (other->team==fighter->team) continue;
    if (other->hp<=0) continue;
    fighter->target=i;
    break;
  }
  if (fighter->target<0) {
    // No living fighters on the other team. This can't happen, but if it does, run.
    fighter->action='r';
  }
}

/* Speed for the upcoming player.
 * Higher values attack first.
 * <0 if the fighter shouldn't get an active stage. (dead or defending).
 */
 
static int battle_calculate_speed(const struct battle *battle,const struct fighter *fighter) {
  if (fighter->hp<=0) return -1; // No fighting while dead.
  int fspeed=50;//TODO Fighter's speed, 1..100
  switch (fighter->action) {
    case 'a': return fspeed; //TODO Adjust per weapon.
    case 'm': return fspeed; //TODO Adjust per selected spell.
    case 'i': return fspeed; // Fighter's speed only. Doesn't matter which item.
    case 'd': return -1; // No active phase, doesn't participate.
    case 'r': return fspeed; // Fighter's speed only.
  }
  return -1;
}

/* How much damage should (fighter)'s default attack do against (target)?
 * (target) may be null, in which case the attack will apply to each living member of the opposing team.
 */
 
static int battle_calculate_attack_damage(const struct battle *battle,const struct fighter *fighter,const struct fighter *target) {
  return 15;//TODO
}

/* Text for the introductory prompt.
 * Always returns in 1..dsta. (dsta must be >=1)
 * strings:battle has 3 options:
 *   3 Battle begins!    -- Generic fallback.
 *   4 % foes draw near! -- Man vs CPU, multiple fighters for CPU.
 *   5 % draws near!     -- Man vs CPU, single fighter for CPU.
 */
 
static int battle_generate_introduction(char *dst,int dsta,struct battle *battle) {
  if (dsta<1) return 0;
  // How many CPU fighters? And if just one, capture it.
  const struct fighter *cpufighter=0;
  int cpufighterc=0;
  int cputeam=battle_get_single_cpu_team(battle);
  if (cputeam>=0) {
    int i=battle->fighterc;
    const struct fighter *fighter=battle->fighterv;
    for (;i-->0;fighter++) {
      if (fighter->team!=cputeam) continue;
      cpufighter=fighter;
      cpufighterc++;
    }
  }
  // Ideal formatted message per CPU fighter count.
  struct strings_insertion ins={0};
  int strix;
  if ((cpufighterc==1)&&(cpufighter->namec>0)) {
    strix=5;
    ins.mode='s';
    ins.s.v=cpufighter->name;
    ins.s.c=cpufighter->namec;
  } else if (cpufighterc>1) {
    strix=4;
    ins.mode='i';
    ins.i=cpufighterc;
  } else {
    strix=3;
  }
  int dstc=strings_format(dst,dsta,RID_strings_battle,strix,&ins,1);
  if (dstc<=dsta) return dstc;
  // Try the generic message if it didn't fit.
  if (strix!=3) {
    dstc=strings_format(dst,dsta,RID_strings_battle,3,0,0);
    if (dstc<=dsta) return dstc;
  }
  // Call with a larger buffer! But we promise to produce something that fits, so do something stupid.
  dst[0]='!';
  return 1;
}

/* Enter a prompt state, with a few preselected format shapes.
 * All text comes from strings:battle.
 */
 
static int battle_prompt_static(struct battle *battle,int strix) {
  const char *msg=0;
  int msgc=strings_get(&msg,RID_strings_battle,strix);
  textbox_set_option(battle->prompt,0,0,msg,msgc,DIR_W,0);
  textbox_pack(battle->prompt);
  textbox_focus(battle->prompt,1);
  battle->tbfocus=battle->prompt;
  return 0;
}

static int battle_prompt_i(struct battle *battle,int strix,int v) {
  struct strings_insertion ins={'i',{.i=v}};
  char tmp[64];
  int tmpc=strings_format(tmp,sizeof(tmp),RID_strings_battle,strix,&ins,1);
  if ((tmpc<0)||(tmpc>sizeof(tmp))) tmpc=0;
  textbox_set_option(battle->prompt,0,0,tmp,tmpc,DIR_W,0);
  textbox_pack(battle->prompt);
  textbox_focus(battle->prompt,1);
  battle->tbfocus=battle->prompt;
  return 0;
}

static int battle_prompt_s(struct battle *battle,int strix,const char *a,int ac) {
  struct strings_insertion ins={'s',{.s={a,ac}}};
  char tmp[64];
  int tmpc=strings_format(tmp,sizeof(tmp),RID_strings_battle,strix,&ins,1);
  if ((tmpc<0)||(tmpc>sizeof(tmp))) tmpc=0;
  textbox_set_option(battle->prompt,0,0,tmp,tmpc,DIR_W,0);
  textbox_pack(battle->prompt);
  textbox_focus(battle->prompt,1);
  battle->tbfocus=battle->prompt;
  return 0;
}

static int battle_prompt_ss(struct battle *battle,int strix,const char *a,int ac,const char *b,int bc) {
  struct strings_insertion insv[]={
    {'s',.s={a,ac}},
    {'s',.s={b,bc}},
  };
  char tmp[64];
  int tmpc=strings_format(tmp,sizeof(tmp),RID_strings_battle,strix,insv,sizeof(insv)/sizeof(insv[0]));
  if ((tmpc<0)||(tmpc>sizeof(tmp))) tmpc=0;
  textbox_set_option(battle->prompt,0,0,tmp,tmpc,DIR_W,0);
  textbox_pack(battle->prompt);
  textbox_focus(battle->prompt,1);
  battle->tbfocus=battle->prompt;
  return 0;
}

/* Advance state, when at least one team's total HP is zero.
 */
 
static int battle_advance_finished(struct battle *battle,const int *hpv/*2*/) {
  if (!battle->farewelled) {
    battle->farewelled=1;
    if (!hpv[0]&&!hpv[1]) { // mutual defeat
      return battle_prompt_static(battle,6);
    }
    int manteam=battle_get_single_man_team(battle);
    if ((manteam>=0)&&(manteam<=1)) {
      if (!hpv[manteam]) { // lost
        return battle_prompt_static(battle,7);
      }
      if (!hpv[manteam^1]) { // won
        return battle_prompt_static(battle,8);
      }
    }
    return battle_prompt_static(battle,9);
  }
  if (!battle->outcome_reported) {
    int manteam=battle_get_single_man_team(battle);
    if ((manteam>=0)&&(manteam<=1)&&hpv[manteam]) { // Won. Report earnings.
      battle->outcome_reported=1;
      struct strings_insertion insv[]={
        {'i',{.i=battle->gold}},
        {'i',{.i=battle->xp}},
      };
      char tmp[64];
      int tmpc=strings_format(tmp,sizeof(tmp),RID_strings_battle,10,insv,sizeof(insv)/sizeof(insv[0]));
      if ((tmpc<0)||(tmpc>sizeof(tmp))) tmpc=0;
      textbox_set_option(battle->prompt,0,0,tmp,tmpc,DIR_W,0);
      textbox_pack(battle->prompt);
      textbox_focus(battle->prompt,1);
      battle->tbfocus=battle->prompt;
      return 0;
    }
  }
  battle->finished=1;
  return 0;
}

/* Prep one team's roster and menu for an interactive stage.
 */
 
static int battle_advance_interactive(struct battle *battle,int team,struct textbox *roster,struct textbox *menu,struct fighter *fighter) {

  //TODO Locate this fighter in the roster and highlight that row.
  
  switch (fighter->input_state) {
  
    case 0: { // Need action.
        textbox_set_option_string(menu,0,0,RID_strings_battle,12,DIR_W,1); // attack
        textbox_set_option_string(menu,0,1,RID_strings_battle,13,DIR_W,1); // magic
        textbox_set_option_string(menu,0,2,RID_strings_battle,14,DIR_W,1); // item
        textbox_set_option_string(menu,0,3,RID_strings_battle,15,DIR_W,1); // defend
        textbox_set_option_string(menu,0,4,RID_strings_battle,16,DIR_W,1); // run
      } break;
  
    case 1: { // Need qualifier.
        textbox_set_option(menu,0,0,"TODO: Qualifier for selected action",-1,DIR_W,1);
      } break;
      
    case 2: { // Need target.
        int row=0;
        const struct fighter *other=battle->fighterv;
        int i=battle->fighterc,p=0;
        for (;i-->0;other++,p++) {
          if (other->team==fighter->team) continue;//TODO What about things like Heal, where the target is on my team?
          textbox_set_option(menu,0,row,other->name,other->namec,DIR_W,1);
          textbox_set_comment(menu,0,row,p);
          row++;
        }
      } break;
  }
  // To support man-vs-man, we would have to abandon the single (tbfocus) in battle, would be a little tricky.
  // Also, for selecting target, we might want focus in the enemy roster or scene instead.
  textbox_pack(menu);
  textbox_focus(menu,1);
  battle->tbfocus=menu;
  battle->fighterfocus=fighter;
  return 0;
}

/* Select insertion point in priov for this fighter.
 * Returns <0 to skip.
 */
 
static int battle_decide_priority(struct battle *battle,const struct fighter *fighter,struct fighter **priov,int prioc) {
  int fspeed=battle_calculate_speed(battle,fighter);
  if (fspeed<0) return -1;
  int i=0; for (;i<prioc;i++) {
    int pspeed=battle_calculate_speed(battle,priov[i]);
    if (pspeed<fspeed) return i;
    if (pspeed>fspeed) continue;
    if (rand()&1) return i; // Equal speed, it's 50/50. Don't let it default, it must be random.
  }
  return prioc;
}

/* Commit attack.
 * (target) may be null for attacks against the entire opposing team.
 */
 
static int battle_commit_attack(struct battle *battle,struct fighter *fighter,struct fighter *target) {
  int damage=battle_calculate_attack_damage(battle,fighter,target);
  if (target) {
    if ((target->hp-=damage)<0) target->hp=0;
  } else {
    struct fighter *other=battle->fighterv;
    int i=battle->fighterc;
    for (;i-->0;other++) {
      if (other->team==fighter->team) continue;
      if (other->hp<=0) continue;
      if ((other->hp-=damage)<0) other->hp=0;
    }
  }
  return battle_prompt_i(battle,21,damage);
}

/* Cast spell.
 * (target) may be null for attacks against the entire opposing team. (or entire same team, for beneficent spells).
 */
 
static int battle_commit_magic(struct battle *battle,struct fighter *fighter,struct fighter *target) {
  fprintf(stderr,"TODO %.*s casts spell %d upon %.*s\n",fighter->namec,fighter->name,fighter->qualifier,target?target->namec:3,target?target->name:"all");
  return battle_prompt_static(battle,11);//"TODO"
}

/* Use item.
 * (target) may be null if irrelevant (applies to self only, or globally, or the entire team).
 */
 
static int battle_commit_item(struct battle *battle,struct fighter *fighter,struct fighter *target) {
  fprintf(stderr,"TODO %.*s uses item %d upon %.*s\n",fighter->namec,fighter->name,fighter->qualifier,target?target->namec:3,target?target->name:"all");
  return battle_prompt_static(battle,11);//"TODO"
}

/* Run away.
 */
 
static int battle_commit_run(struct battle *battle,struct fighter *fighter) {
  fprintf(stderr,"TODO %.*s tries to run\n",fighter->namec,fighter->name);
  return battle_prompt_static(battle,11);//"TODO"
}

/* Start the next step of the actions phase.
 * If we're done with all actions, return <0.
 * The first time per cycle that this gets called, it must not be done.
 */
 
static int battle_commit_next_action(struct battle *battle) {
  while (battle->priop<battle->prioc) {
    struct fighter *fighter=battle->priov[battle->priop];
  
    /* At priosub==0, it's a static message: "Somebody does something!"
     */
    if (battle->priosub==0) {
      battle->priosub=1;
      switch (fighter->action) {
        case 'a': return battle_prompt_s(battle,17,fighter->name,fighter->namec);
        case 'm': {
            const char *spellname="TODO";
            int spellnamec=4;
            return battle_prompt_ss(battle,18,fighter->name,fighter->namec,spellname,spellnamec);
          }
        case 'i': {
            const char *itemname="TODO";
            int itemnamec=4;
            return battle_prompt_ss(battle,19,fighter->name,fighter->namec,itemname,itemnamec);
          }
        case 'r': return battle_prompt_s(battle,20,fighter->name,fighter->namec);
        default: return -1;
      }
    }
    
    /* At priosub==1, begin animation, if the action warrants.
     */
    if (battle->priosub==1) {
      battle->priosub=2;
      switch (fighter->action) {
        case 'a': break;//TODO attack animation
        case 'm': break;//TODO magic animation
        case 'i': break;//TODO item animation
        // Nothing for 'r', just proceed.
      }
    }
    
    /* At priosub==2, the action takes effect for real.
     */
    if (battle->priosub==2) {
      battle->priosub=3;
      struct fighter *target=0;
      if ((fighter->target>=0)&&(fighter->target<battle->fighterc)) target=battle->fighterv+fighter->target;
      switch (fighter->action) {
        case 'a': return battle_commit_attack(battle,fighter,target);
        case 'm': return battle_commit_magic(battle,fighter,target);
        case 'i': return battle_commit_item(battle,fighter,target);
        case 'r': return battle_commit_run(battle,fighter);
      }
    }
    
    /* OK, on to the next fighter.
     */
    battle->priop++;
    battle->priosub=0;
  }
  return -1;
}

/* Advance state, entering action cycle.
 * By this point, every living fighter must have an action selected.
 */
 
static int battle_advance_begin_action(struct battle *battle) {
  
  if (!battle->priov) {
    if (!(battle->priov=calloc(1,sizeof(void*)*battle->fighterc))) return -1;
  }
  battle->prioc=0;
  battle->priop=0;
  battle->priosub=0;
  
  /* Build up the priority list as a naive insertion sort.
   * We're calculating the attack's speed at each comparison.
   * For a larger set of fighters, it might be wise to cache those speeds, and use quicksort.
   * But I don't expect more than 5 fighters per battle so whatever.
   */
  struct fighter *fighter=battle->fighterv;
  int fi=battle->fighterc;
  for (;fi-->0;fighter++) {
    int p=battle_decide_priority(battle,fighter,battle->priov,battle->prioc);
    if ((p<0)||(p>battle->prioc)) continue;
    memmove(battle->priov+p+1,battle->priov+p,sizeof(void*)*(battle->prioc-p));
    battle->priov[p]=fighter;
    battle->prioc++;
  }
  if (battle->prioc<1) {
    fprintf(stderr,"%s:%d: No qualifying fighters at %s. This isn't supposed to be possible.\n",__FILE__,__LINE__,__func__);
    battle->finished=1;
    return -1;
  }
  
  return battle_commit_next_action(battle);
}

/* Clear all state associated with the action phase.
 */
 
static void battle_clear_action_phase(struct battle *battle) {
  battle->prioc=0;
  struct fighter *fighter=battle->fighterv;
  int i=battle->fighterc;
  for (;i-->0;fighter++) {
    fighter->input_state=0;
    fighter->action=0;
    fighter->qualifier=-1;
    fighter->target=-1;
  }
}

/* Advance state.
 */
 
int battle_advance(struct battle *battle) {
  
  /* Unfocus all textboxes and drop content from the volatile ones.
   */
  textbox_focus(battle->roster0,0);
  textbox_focus(battle->roster1,0);
  textbox_focus(battle->prompt,0);
  textbox_focus(battle->menu0,0);
  textbox_focus(battle->menu1,0);
  battle->tbfocus=0;
  textbox_clear(battle->prompt);
  textbox_clear(battle->menu0);
  textbox_clear(battle->menu1);
  battle->fighterfocus=0;
  
  /* If we haven't said hello yet, do that.
   */
  if (!battle->introduced) {
    battle->introduced=1;
    char tmp[64];
    int tmpc=battle_generate_introduction(tmp,sizeof(tmp),battle);
    textbox_set_option(battle->prompt,0,0,tmp,tmpc,DIR_W,0);
    textbox_pack(battle->prompt);
    textbox_focus(battle->prompt,1);
    battle->tbfocus=battle->prompt;
    return 0;
  }
  
  /* Is one side defeated?
   * I'm not sure whether mutual defeat will be possible. Allow that it is.
   */
  int hpv[2]={0,0};
  struct fighter *fighter=battle->fighterv;
  int i=battle->fighterc;
  for (;i-->0;fighter++) {
    if (fighter->hp>0) switch (fighter->team) {
      case 0: hpv[0]+=fighter->hp; break;
      case 1: hpv[1]+=fighter->hp; break;
    }
  }
  if (!hpv[0]||!hpv[1]) return battle_advance_finished(battle,hpv);
  
  /* If we're in the action phase, do the next thing.
   * But if that turns out not to be possible, fall thru.
   */
  if (battle->prioc) {
    if (battle_commit_next_action(battle)>=0) return 0;
    battle_clear_action_phase(battle);
  }
  
  /* Select action for CPU fighters that don't have one yet.
   * Man fighters, take the first unselected one per team, and go interactive as needed.
   */
  struct fighter *interactivev[2]={0,0}; // Which fighter needs updated, per team.
  for (fighter=battle->fighterv,i=battle->fighterc;i-->0;fighter++) {
    if ((fighter->team<0)||(fighter->team>1)) { // Invalid fighter, get out.
      battle->finished=1;
      return 0;
    }
    int ctl=fighter->team?battle->control1:battle->control0;
    if (ctl==BATTLE_CONTROL_CPU) {
      // CPU fighters are simple, we can select their action immediately.
      // So they are never "interactive" and always "ready".
      if (fighter->action) continue;
      battle_select_cpu_action(battle,fighter);
      
    } else if (ctl==BATTLE_CONTROL_MAN) {
      // Man fighters also work out pretty simple from here: They're all ready, or note the first one that isn't.
      if (interactivev[fighter->team]) continue;
      if (fighter->input_state<3) interactivev[fighter->team]=fighter;
      
    } else { // Invalid controller, get out.
      battle->finished=1;
      return 0;
    }
  }
  
  /* If neither team is gathering input, begin the action cycle.
   */
  if (!interactivev[0]&&!interactivev[1]) {
    return battle_advance_begin_action(battle);
  }
  
  /* Populate roster and menu according to the interactive fighters.
   * If we ever do support man-vs-man, this will break: Your opponent's UI will obnoxiously rebuild when you select something.
   */
  if (interactivev[0]) battle_advance_interactive(battle,0,battle->roster0,battle->menu0,interactivev[0]);
  if (interactivev[1]) battle_advance_interactive(battle,1,battle->roster1,battle->menu1,interactivev[1]);
  return 0;
}
