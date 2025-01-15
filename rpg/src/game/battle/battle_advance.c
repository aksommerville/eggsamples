#include "battle_internal.h"

/* Drop menu content and roster highlights.
 */
 
static void battle_non_interactive(struct battle *battle) {
  roster_clear_highlights(battle->rosterv[0]);
  roster_clear_highlights(battle->rosterv[1]);
  bmenu_clear(battle->bmenuv[0]);
  bmenu_clear(battle->bmenuv[1]);
  battle->teamv[0].focus_fighter=0;
  battle->teamv[1].focus_fighter=0;
}

/* Welcome prompt, first thing the player sees.
 */
 
static void battle_set_welcome_prompt(struct battle *battle) {
  battle_non_interactive(battle);
  if (prompt_get_state(battle->prompt)==PROMPT_STATE_WELCOME) return;
  prompt_set_state(battle->prompt,PROMPT_STATE_WELCOME);
  //TODO Different messages for single-foe, multi-foe, multi-player. And must be sourced from strings resource.
  prompt_set_text(battle->prompt,"Battle begins!",-1);
}

/* Farewell prompt, report battle's outcome.
 */
 
/*
12 Both sides are annihilated!
13 End of battle.
14 Victory is yours!
15 You have fallen.
*/
 
static void battle_set_farewell_prompt(struct battle *battle) {
  battle_non_interactive(battle);
  if (prompt_get_state(battle->prompt)==PROMPT_STATE_FAREWELL) return;
  prompt_set_state(battle->prompt,PROMPT_STATE_FAREWELL);
  //TODO If outcome<0 and gold or XP was awarded, say so. If not, "win" or "lose" if we have one human.
  prompt_set_text(battle->prompt,"End of battle.",-1);
}

/* Human team is ready, clear their menu.
 * This may fire redundantly.
 */
 
static void battle_ready_team(struct battle *battle,struct team *team,int teamp) {
  if ((teamp<0)||(teamp>1)) return;
  bmenu_clear(battle->bmenuv[teamp]);
}

/* Populate bmenu for gathering action.
 */
 
static void battle_gather_action(struct battle *battle,struct team *team,struct fighter *fighter,int teamp) {
  if ((teamp<0)||(teamp>1)) return;
  struct bmenu *bmenu=battle->bmenuv[teamp];
  if (bmenu_get_state(bmenu)==BMENU_STATE_ACTIONS) return;
  bmenu_clear(bmenu);
  bmenu_set_state(bmenu,BMENU_STATE_ACTIONS);
  bmenu_add_option(battle->bmenuv[0],"Attack",-1,'a');//XXX use strings resource
  bmenu_add_option(battle->bmenuv[0],"Magic",-1,'m');
  bmenu_add_option(battle->bmenuv[0],"Item",-1,'i');
  bmenu_add_option(battle->bmenuv[0],"Defend",-1,'d');
  bmenu_add_option(battle->bmenuv[0],"Run",-1,'r');
  bmenu_pack(battle->bmenuv[0]);
}

/* Populate bmenu for gathering qualifier. Either spell or item. (or any other action if we add things).
 */
 
static int cb_add_bmenu_option(int qualifier,const char *name,int namec,void *userdata) {
  struct bmenu *bmenu=userdata;
  bmenu_add_option(bmenu,name,namec,qualifier);
  return 0;
}
 
static void battle_gather_qualifier(struct battle *battle,struct team *team,struct fighter *fighter,int teamp) {
  if ((teamp<0)||(teamp>1)) return;
  struct bmenu *bmenu=battle->bmenuv[teamp];
  switch (fighter->action) {
    case 'm': {
        if (bmenu_get_state(bmenu)==BMENU_STATE_SPELLS) return;
        bmenu_clear(bmenu);
        bmenu_set_state(bmenu,BMENU_STATE_SPELLS);
        battle_for_each_spell(battle,fighter,cb_add_bmenu_option,bmenu);
        bmenu_pack(bmenu);
      } break;
    case 'i': {
        if (bmenu_get_state(bmenu)==BMENU_STATE_ITEMS) return;
        bmenu_clear(bmenu);
        bmenu_set_state(bmenu,BMENU_STATE_ITEMS);
        battle_for_each_item(battle,fighter,cb_add_bmenu_option,bmenu);
        bmenu_pack(bmenu);
      } break;
  }
}

/* Populate bmenu for gathering target.
 * May include fighters from this team, the other team, or any combination.
 */
 
static void battle_gather_target(struct battle *battle,struct team *team,struct fighter *fighter,int teamp) {
  if ((teamp<0)||(teamp>1)) return;
  if (team->focus_fighter) {
    roster_highlight_id(battle->rosterv[0],team->focus_fighter,0);
    roster_highlight_id(battle->rosterv[1],team->focus_fighter,0);
    team->focus_fighter=0;
  }
  struct bmenu *bmenu=battle->bmenuv[teamp];
  if (bmenu_get_state(bmenu)==BMENU_STATE_FIGHTERS) return;
  bmenu_clear(bmenu);
  bmenu_set_state(bmenu,BMENU_STATE_FIGHTERS);
  struct team *oteam=battle->teamv;
  int ti=2;
  for (;ti-->0;oteam++) {
    struct fighter *ofighter=oteam->fighterv;
    int fi=oteam->fighterc;
    for (;fi-->0;ofighter++) {
      if (!battle_fighter_is_target(battle,team,fighter,oteam,ofighter)) continue;
      if (!team->focus_fighter) team->focus_fighter=ofighter->id;
      bmenu_add_option(bmenu,ofighter->name,ofighter->namec,ofighter->id);
    }
  }
  bmenu_pack(bmenu);
  if (team->focus_fighter) {
    roster_highlight_id(battle->rosterv[0],team->focus_fighter,1);
    roster_highlight_id(battle->rosterv[1],team->focus_fighter,1);
  }
}

/* Fighter needs input. Prep the team's roster and menu.
 */
 
static void battle_setup_interactive(struct battle *battle,struct team *team,struct fighter *fighter,int teamp) {
  if ((teamp<0)||(teamp>1)) return;
  roster_clear_highlights(battle->rosterv[teamp]); // TODO This nixes any transient highlight created by the other side. Would be tricky to do it right.
  roster_highlight_id(battle->rosterv[teamp],fighter->id,1);
  if (!fighter->action) battle_gather_action(battle,team,fighter,teamp);
  else if (!fighter->qualifier&&battle_fighter_requires_qualifier(battle,fighter)) battle_gather_qualifier(battle,team,fighter,teamp);
  else if (!fighter->target&&battle_fighter_requires_target(battle,fighter)) battle_gather_target(battle,team,fighter,teamp);
}

/* Highlight participants of an attack, magic, or item play.
 * Resets highlight of both rosters.
 */
 
static void battle_highlight_participants(struct battle *battle,struct fighter *fighter,struct fighter *target) {
  roster_clear_highlights(battle->rosterv[0]);
  roster_clear_highlights(battle->rosterv[1]);
  if (fighter) {
    roster_highlight_id(battle->rosterv[0],fighter->id,1);
    roster_highlight_id(battle->rosterv[1],fighter->id,1);
  }
  if (target) {
    roster_highlight_id(battle->rosterv[0],target->id,1);
    roster_highlight_id(battle->rosterv[1],target->id,1);
  }
}

/* Commit an attack.
 * Both fighters must be non-null.
 */
 
static void battle_commit_attack(struct battle *battle,struct fighter *attacker,struct fighter *defender) {
  char msg[64];
  int msgc=0;
  int damage=battle_calculate_attack_damage(battle,attacker,defender);
  battle_highlight_participants(battle,attacker,defender);
  //TODO Begin animation in scene.
  
  if (damage<0) { // Backfire.
    if ((attacker->hp+=damage)<=0) attacker->hp=0;
    prompt_set_res_sis(battle->prompt,RID_strings_battle,1,attacker->name,attacker->namec,-damage,0,0);
    
  } else if (damage>0) { // Regular attack.
    if ((defender->hp-=damage)<=0) defender->hp=0;
    prompt_set_res_sis(battle->prompt,RID_strings_battle,2,attacker->name,attacker->namec,damage,defender->name,defender->namec);
    
  } else { // No effect.
    prompt_set_res_sss(battle->prompt,RID_strings_battle,3,attacker->name,attacker->namec,defender->name,defender->namec,0,0);
  }
}

/* Cast spell.
 */
 
static void battle_commit_magic(struct battle *battle,struct fighter *fighter) {
  struct fighter *target=battle_fighter_by_id(battle,fighter->target);
  battle_highlight_participants(battle,fighter,target);
  //TODO Begin animation in scene.
  char name[64];
  int namec=battle_spell_name_by_qualifier(name,sizeof(name),battle,fighter,fighter->qualifier);
  if (!battle_cast_spell(battle,fighter,fighter->qualifier,target)) {
    prompt_set_res_sss(battle->prompt,RID_strings_battle,16,fighter->name,fighter->namec,name,namec,0,0);
  } else if (target) {
    prompt_set_res_sss(battle->prompt,RID_strings_battle,7,fighter->name,fighter->namec,name,namec,target->name,target->namec);
  } else {
    prompt_set_res_sss(battle->prompt,RID_strings_battle,6,fighter->name,fighter->namec,name,namec,0,0);
  }
}

/* Use item.
 */
 
static void battle_commit_item(struct battle *battle,struct fighter *fighter) {
  struct fighter *target=battle_fighter_by_id(battle,fighter->target);
  battle_highlight_participants(battle,fighter,target);
  //TODO Begin animation in scene.
  char name[64];
  int namec=battle_item_name_by_qualifier(name,sizeof(name),battle,fighter,fighter->qualifier);
  if (!battle_use_item(battle,fighter,fighter->qualifier,target)) {
    prompt_set_res_sss(battle->prompt,RID_strings_battle,17,fighter->name,fighter->namec,name,namec,0,0);
  } else if (target) {
    prompt_set_res_sss(battle->prompt,RID_strings_battle,9,fighter->name,fighter->namec,name,namec,target->name,target->namec);
  } else {
    prompt_set_res_sss(battle->prompt,RID_strings_battle,8,fighter->name,fighter->namec,name,namec,0,0);
  }
}

/* End commission cycle.
 */
 
static void battle_end_commission(struct battle *battle) {
  battle->prioc=0;
  struct team *team=battle->teamv;
  int ti=2;
  for (;ti-->0;team++) {
    struct fighter *fighter=team->fighterv;
    int fi=team->fighterc;
    for (;fi-->0;fighter++) {
      fighter->action=0;
      fighter->qualifier=0;
      fighter->target=0;
    }
  }
}

/* Nonzero if everybody is dead.
 */
 
static int battle_team_finished(const struct team *team) {
  const struct fighter *fighter=team->fighterv;
  int fi=team->fighterc;
  for (;fi-->0;fighter++) if (fighter->hp>0) return 0;
  return 1;
}

/* Advance, in commission cycle. ie (prioc!=0).
 * Returns >0 if committed, or 0 if the cycle is over and we should enter an interactive stage instead.
 */
 
static int battle_advance_commission(struct battle *battle) {
  battle_non_interactive(battle);
  for (;;) {
    if (battle->outcome) {
      battle->finished=1;
      return 1;
    }
    if (battle_team_finished(battle->teamv+0)||battle_team_finished(battle->teamv+1)) {
      battle_end_commission(battle);
      return 0;
    }
    if ((battle->priop<0)||(battle->priop>=battle->prioc)) {
      battle_end_commission(battle);
      return 0;
    }
    struct fighter *fighter=battle->priov[battle->priop];
    if (fighter->hp>0) switch (fighter->action) {
    
      case 'a': {
          if (battle->priosubp<=0) {
            battle->priosubp=1;
            struct fighter *target=battle_fighter_by_id(battle,fighter->target);
            if (target) {
              battle_commit_attack(battle,fighter,target);
              return 1;
            }
          }
        } break;

      case 'm': {
          if (battle->priosubp<=0) {
            battle->priosubp=1;
            battle_commit_magic(battle,fighter);
            return 1;
          }
        } break;
        
      case 'i': {
          if (battle->priosubp<=0) {
            battle->priosubp=1;
            battle_commit_item(battle,fighter);
            return 1;
          }
        } break;
        
      case 'r': {
          if (battle->priosubp<=0) {
            battle->priosubp=1;
            if (battle_try_run(battle,fighter)) {
              prompt_set_res_sss(battle->prompt,RID_strings_battle,4,fighter->name,fighter->namec,0,0,0,0);
              battle->outcome=-2;
            } else {
              prompt_set_res_sss(battle->prompt,RID_strings_battle,5,fighter->name,fighter->namec,0,0,0,0);
            }
            return 1;
          }
        } break;
    }
    battle->priop++;
    battle->priosubp=0;
  }
}

/* End gather cycle, begin commission cycle.
 */
 
static int battle_begin_commission_cycle(struct battle *battle) {

  int fighterc=battle->teamv[0].fighterc+battle->teamv[1].fighterc;
  if (fighterc>battle->prioa) {
    void *nv=realloc(battle->priov,sizeof(void*)*fighterc);
    if (!nv) return -1;
    battle->priov=nv;
    battle->prioa=fighterc;
  }
  battle->prioc=battle_prioritize_fighters(battle->priov,battle->prioa,battle);
  if ((battle->prioc<1)||(battle->prioc>battle->prioa)) return -1;
  
  battle->priop=0;
  battle->priosubp=0;
  if (!battle_advance_commission(battle)) return -1;
  return 0;
}

/* Advance, main entry point.
 */
 
void battle_advance(struct battle *battle) {
  
  /* Welcome and farewell prompts, and termination.
   */
  if (!battle->welcomed) {
    battle_set_welcome_prompt(battle);
    return;
  }
  if (battle->outcome) {
    if (!battle->farewelled) {
      battle_set_farewell_prompt(battle);
      return;
    }
    battle->finished=1;
    return;
  }
  
  /* Within commission cycle? See above.
   */
  if (battle->prioc) {
    if (battle_advance_commission(battle)) return;
  }
  
  /* Check for completion.
   */
  int dead0=battle_team_finished(battle->teamv+0);
  int dead1=battle_team_finished(battle->teamv+1);
  if (dead0||dead1) {
    if (dead0&&dead1) {
      battle->outcome=-2;
      prompt_set_res(battle->prompt,RID_strings_battle,12);
    } else if (dead0) {
      battle->outcome=1;
    } else if (dead1) {
      battle->outcome=-1;
    }
    battle_set_farewell_prompt(battle);
    return;
  }
  
  /* We are collecting the next move.
   * First, ready any CPU-controlled teams.
   */
  struct team *team=battle->teamv;
  int ti=2;
  for (;ti-->0;team++) {
    if (team->control!=TEAM_CONTROL_CPU) continue;
    struct fighter *fighter=team->fighterv;
    int fi=team->fighterc;
    for (;fi-->0;fighter++) {
      if (battle_fighter_is_ready(battle,team,fighter)) continue;
      battle_ready_cpu_fighter(battle,team,fighter);
    }
  }
  
  /* Examine each fighter on each human-controlled team.
   * The first one incomplete per team, set up to collect the info we need.
   */
  int interactivec=0;
  for (team=battle->teamv,ti=2;ti-->0;team++) {
    if (team->control!=TEAM_CONTROL_MAN) continue;
    int collecting=0;
    struct fighter *fighter=team->fighterv;
    int fi=team->fighterc;
    for (;fi-->0;fighter++) {
      if (battle_fighter_is_ready(battle,team,fighter)) continue;
      battle_setup_interactive(battle,team,fighter,team-battle->teamv);
      collecting=1;
      interactivec++;
      break;
    }
    if (!collecting) {
      battle_ready_team(battle,team,team-battle->teamv);
    }
  }
  if (interactivec) return;
  
  /* All human players are ready. Enter the commission cycle.
   */
  if (battle_begin_commission_cycle(battle)<0) {
    battle->finished=1;
  }
}

/* Which fighter receives input?
 */
 
struct fighter *battle_pending_fighter(struct battle *battle,struct team *team) {
  if (team->control!=TEAM_CONTROL_MAN) return 0;
  struct fighter *fighter=team->fighterv;
  int i=team->fighterc;
  for (;i-->0;fighter++) {
    if (battle_fighter_is_ready(battle,team,fighter)) continue;
    return fighter;
  }
  return 0;
}

/* Receive input from a bmenu.
 */
 
void battle_receive_bmenu_input(struct battle *battle,struct bmenu *bmenu,struct team *team,int optionid,int teamp) {
  struct fighter *fighter=battle_pending_fighter(battle,team);
  if (!fighter) return;
  switch (bmenu_get_state(bmenu)) {
    case BMENU_STATE_ACTIONS: {
        fighter->action=optionid;
        battle_advance(battle);
      } break;
    case BMENU_STATE_SPELLS:
    case BMENU_STATE_ITEMS: {
        fighter->qualifier=optionid;
        battle_advance(battle);
      } break;
    case BMENU_STATE_FIGHTERS: {
        fighter->target=optionid;
        battle_advance(battle);
      } break;
  }
}

/* Retreat: Return to previous input state for one team.
 */
 
void battle_retreat(struct battle *battle,struct team *team,int teamp) {
  struct fighter *fighter=battle_pending_fighter(battle,team);
  if (!fighter) return;
  
  if (team->focus_fighter) {
    roster_highlight_id(battle->rosterv[0],team->focus_fighter,0);
    roster_highlight_id(battle->rosterv[1],team->focus_fighter,0);
    team->focus_fighter=0;
  }
  
  if (!fighter->action) {
    struct fighter *prev=0,*q=team->fighterv;
    int i=team->fighterc;
    for (;i-->0;q++) {
      if (q==fighter) break;
      if (q->hp<=0) continue;
      prev=q;
    }
    if (!prev) return;
    if (prev->target) prev->target=0;
    else if (prev->qualifier) prev->qualifier=0;
    else prev->action=0;
    battle_advance(battle);
    return;
  }
  
  if (!fighter->qualifier) {
    fighter->action=0;
    battle_advance(battle);
    return;
  }
  
  fighter->qualifier=0;
  battle_advance(battle);
}
