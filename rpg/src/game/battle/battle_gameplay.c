#include "battle_internal.h"

/* Iterate spells and items.
 */
 
int battle_for_each_spell(struct battle *battle,struct fighter *fighter,int (*cb)(int qualifier,const char *name,int namec,void *userdata),void *userdata) {
  int err;
  //TODO fake data for now
  if (err=cb(1,"Curse",5,userdata)) return err;
  if (err=cb(2,"Heal",4,userdata)) return err;
  if (err=cb(3,"Revive",6,userdata)) return err;
  return 0;
}

int battle_for_each_item(struct battle *battle,struct fighter *fighter,int (*cb)(int qualifier,const char *name,int namec,void *userdata),void *userdata) {
  int err;
  //TODO fake data for now
  if (err=cb(1,"Herb",4,userdata)) return err;
  if (err=cb(2,"Torch",5,userdata)) return err;
  if (err=cb(3,"Bell",4,userdata)) return err;
  return 0;
}

/* Fetch name of spell or item name -- automatic, uses the iterators above.
 */
 
struct battle_match_iter_string {
  int qualifier;
  char *dst;
  int dsta;
};
 
static int battle_cb_match_iter_string(int qualifier,const char *name,int namec,void *userdata) {
  struct battle_match_iter_string *ctx=userdata;
  if (ctx->qualifier!=qualifier) return 0;
  if (namec<=ctx->dsta) memcpy(ctx->dst,name,namec);
  return namec;
}
 
int battle_spell_name_by_qualifier(char *dst,int dsta,struct battle *battle,struct fighter *fighter,int qualifier) {
  struct battle_match_iter_string ctx={.qualifier=qualifier,.dst=dst,.dsta=dsta};
  return battle_for_each_spell(battle,fighter,battle_cb_match_iter_string,&ctx);
}

int battle_item_name_by_qualifier(char *dst,int dsta,struct battle *battle,struct fighter *fighter,int qualifier) {
  struct battle_match_iter_string ctx={.qualifier=qualifier,.dst=dst,.dsta=dsta};
  return battle_for_each_item(battle,fighter,battle_cb_match_iter_string,&ctx);
}

/* Nonzero if this fighter's next move is recorded.
 */

int battle_fighter_is_ready(struct battle *battle,struct team *team,struct fighter *fighter) {
  if (fighter->hp<=0) return 0;
  switch (fighter->action) {
    case 'a': {
        if (!fighter->target) return 0;
        return 1;
      }
    case 'm': {
        if (!fighter->qualifier) return 0;
        if (!fighter->target) return 0;
        return 1;
      }
    case 'i': {
        if (!fighter->qualifier) return 0;
        if (!fighter->target) return 0;
        return 1;
      }
    case 'd': {
        return 1;
      }
    case 'r': {
        return 1;
      }
  }
  return 0;
}

/* Nonzero if a qualifier is required.
 * (fighter->action) is set.
 */
 
int battle_fighter_requires_qualifier(struct battle *battle,struct fighter *fighter) {
  switch (fighter->action) {
    case 'm':
    case 'i':
      return 1;
  }
  return 0;
}

/* Nonzero if a target is required.
 * (fighter->action) and (fighter->qualifier) are set.
 */

int battle_fighter_requires_target(struct battle *battle,struct fighter *fighter) {
  switch (fighter->action) {
    case 'a':
      return 1;
    case 'm': {
        //TODO depends on the spell
        return 1;
      }
    case 'i': {
        //TODO depends on the item
        return 1;
      }
  }
  return 0;
}

/* Nonzero if (dfighter) should be included among the targets for (afighter).
 * "a"=attacker, "d"=defender.
 * (afighter) has (action,qualifier) set already.
 * This is only called if you responded true to battle_fighter_requires_target, and is called for all fighters, even yourself.
 */
 
int battle_fighter_is_target(struct battle *battle,struct team *ateam,struct fighter *afighter,struct team *dteam,struct fighter *dfighter) {
  switch (afighter->action) {
    case 'a': {
        if (ateam==dteam) return 0;
        if (dfighter->hp<=0) return 0;
        return 1;
      }
    case 'm': {
        //TODO Healing etc should only propose same team, hurting etc only the other.
        if (ateam==dteam) return 0;
        if (dfighter->hp<=0) return 0;
        return 1;
      }
    case 'i': {
        //TODO Decide per item, is it for the same team, only for the living, etc.
        return 1;
      }
  }
  return 0;
}

/* Pick the next move for a fighter on a CPU team.
 */
 
void battle_ready_cpu_fighter(struct battle *battle,struct team *team,struct fighter *fighter) {
  //TODO sensible fight logic. For now, we attack any living fighter on the other team, and if there's none we run.
  struct team *oteam=(team==battle->teamv)?(battle->teamv+1):battle->teamv;
  struct fighter *ofighter=oteam->fighterv;
  int fi=oteam->fighterc;
  for (;fi-->0;ofighter++) {
    if (ofighter->hp<=0) continue;
    fighter->action='a';
    fighter->target=ofighter->id;
    return;
  }
  fighter->action='r';
}

/* Populate (dstv) with WEAK references to (battle)'s players, in the order of their next attack.
 * Caller ensures that (dsta) is at least the total count of (battle)'s players.
 * If a fighter sits out or is dead or whatever, don't include him. Defending counts as sitting out.
 */
 
int battle_prioritize_fighters(struct fighter **dstv,int dsta,struct battle *battle) {
  int dstc=0;
  struct team *team=battle->teamv;
  int ti=2;
  for (;ti-->0;team++) {
    struct fighter *fighter=team->fighterv;
    int fi=team->fighterc;
    for (;fi-->0;fighter++) {
      if (fighter->hp<=0) continue;
      if (fighter->action=='d') continue;
      if (dstc>=dsta) return -1;
      //TODO Calculate speed of fighter's scheduled move.
      //TODO Compare to fighters already included in (dstv) and insert at the appropriate place.
      dstv[dstc++]=fighter;
    }
  }
  return dstc;
}

/* How much damage does (attacker)'s default attack do upon (defender)?
 * Negative is allowed, for attacks that can backfire. But we can't hurt both of them.
 */
 
int battle_calculate_attack_damage(struct battle *battle,struct fighter *attacker,struct fighter *defender) {
  return 10;//TODO
}

/* (coward) wants to run. Return nonzero if he makes it.
 */
 
int battle_try_run(struct battle *battle,struct fighter *coward) {
  return 1;//TODO
}

/* Cast spell.
 * Apply all costs and effects.
 * Return zero if it failed to cast (eg insufficient MP), nonzero if it worked (even if no actual effect).
 */

int battle_cast_spell(struct battle *battle,struct fighter *fighter,int qualifier,struct fighter *target) {
  return 1;//TODO
}

/* Use item.
 * Apply all costs and effects (eg inventory).
 * Return zero if we failed to use it (eg insufficient inventory), nonzero if it was used even if no actual effect.
 */
 
int battle_use_item(struct battle *battle,struct fighter *fighter,int qualifier,struct fighter *target) {
  return 1;//TODO
}
