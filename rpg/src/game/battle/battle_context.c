#include "battle_internal.h"

/* Delete.
 */
 
static void fighter_cleanup(struct fighter *fighter) {
}
 
static void team_cleanup(struct team *team) {
  if (team->fighterv) {
    while (team->fighterc-->0) fighter_cleanup(team->fighterv+team->fighterc);
    free(team->fighterv);
  }
}
 
void battle_del(struct battle *battle) {
  if (!battle) return;
  team_cleanup(battle->teamv+0);
  team_cleanup(battle->teamv+1);
  scene_del(battle->scene);
  prompt_del(battle->prompt);
  roster_del(battle->rosterv[0]);
  roster_del(battle->rosterv[1]);
  bmenu_del(battle->bmenuv[0]);
  bmenu_del(battle->bmenuv[1]);
  if (battle->priov) free(battle->priov);
  free(battle);
}

/* New.
 */

struct battle *battle_new() {
  struct battle *battle=calloc(1,sizeof(struct battle));
  if (!battle) return 0;
  
  battle->idnext=1;
  
  if (
    !(battle->scene=scene_new(battle))||
    !(battle->prompt=prompt_new(battle))||
    !(battle->rosterv[0]=roster_new(battle,0))||
    !(battle->rosterv[1]=roster_new(battle,1))||
    !(battle->bmenuv[0]=bmenu_new(battle,0))||
    !(battle->bmenuv[1]=bmenu_new(battle,1))
  ) {
    battle_del(battle);
    return 0;
  }
  
  return battle;
}

/* Validate team, after setup.
 */
 
static int team_validate(const struct team *team) {
  switch (team->control) {
    case TEAM_CONTROL_MAN:
    case TEAM_CONTROL_CPU:
      break;
    default: return -1;
  }
  if (team->fighterc<1) return -1;
  return 0;
}

/* Pack UI.
 */
 
static void battle_pack_ui(struct battle *battle) {

  /* These constants are entirely negotiable, season to taste.
   * The remainder is built around them in a pretty predictable fashion that I don't think will need adjustment.
   * But of course, you are free to tear all of this out and position elements wherever it suits you.
   */
  const int margin_edge=1;
  const int margin_between=2;
  const int scenew=BATTLE_SCENE_W;
  const int sceneh=BATTLE_SCENE_H;
  const int prompty=margin_edge+sceneh+margin_between;
  
  int prompth=font_get_line_height(g.font);
  prompth+=prompt_vertical_margins();
  prompt_set_bounds(battle->prompt,margin_edge,prompty,FBW-margin_edge*2,prompth);
  
  int menuy=prompty+prompth+margin_between;
  int menuh=FBH-margin_edge-menuy;
  int menulw=(FBW-margin_edge*2-margin_between)>>1;
  int menurx=margin_edge+menulw+margin_between;
  int menurw=FBW-margin_edge-menurx;
  bmenu_set_bounds(battle->bmenuv[0],margin_edge,menuy,menulw,menuh);
  bmenu_set_bounds(battle->bmenuv[1],menurx,menuy,menurw,menuh);
  
  int scenex=(FBW>>1)-(scenew>>1);
  int rosterrx=scenex+scenew+margin_between;
  scene_set_bounds(battle->scene,scenex,margin_edge,scenew,sceneh);
  roster_set_bounds(battle->rosterv[0],margin_edge,margin_edge,scenex-margin_between-margin_edge,sceneh);
  roster_set_bounds(battle->rosterv[1],rosterrx,margin_edge,FBW-margin_edge-rosterrx,sceneh);
}

/* End of setup.
 */
 
static int battle_ready(struct battle *battle) {
  int err;
  if ((err=team_validate(battle->teamv+0))<0) return err;
  if ((err=team_validate(battle->teamv+1))<0) return err;
  battle_pack_ui(battle);
  rpg_song(battle->songid);
  battle->ready=1;
  battle_advance(battle);
  return 0;
}

/* Add an unconfigured fighter.
 */
 
static struct fighter *battle_add_fighter(struct battle *battle,struct team *team) {
  if (!battle||!team) return 0;
  if (team->fighterc>=team->fightera) {
    int na=team->fightera+4;
    if (na>INT_MAX/sizeof(struct fighter)) return 0;
    void *nv=realloc(team->fighterv,sizeof(struct fighter)*na);
    if (!nv) return 0;
    team->fighterv=nv;
    team->fightera=na;
  }
  struct fighter *fighter=team->fighterv+team->fighterc++;
  memset(fighter,0,sizeof(struct fighter));
  
  fighter->id=battle->idnext++;
  
  return fighter;
}

/* Add fighters from a resource.
 */
 
static int battle_add_fighter_res(struct battle *battle,struct team *team,int fighterid,int c) {
  if ((c<1)||(c>8)) {
    fprintf(stderr,"Request to add %d of fighter:%d to battle. This count is invalid.\n",c,fighterid);
    return -1;
  }
  const void *src=0;
  int srcc=rpg_res_get(&src,EGG_TID_fighter,fighterid);
  if (srcc<1) {
    fprintf(stderr,"fighter:%d not found, required for battle:%d\n",fighterid,battle->battleid);
    return -1;
  }
  int hp=0,namec=0,imageid=0,tileid=0;
  const char *name=0;
  struct cmdlist_reader reader={.v=src,.c=srcc};
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_fighter_hp: hp=(cmd.arg[0]<<8)|cmd.arg[1]; break;
      case CMD_fighter_name: {
          int rid=(cmd.arg[0]<<8)|cmd.arg[1];
          int ix=(cmd.arg[2]<<8)|cmd.arg[3];
          namec=strings_get(&name,rid,ix);
        } break;
      case CMD_fighter_image: {
          imageid=(cmd.arg[0]<<8)|cmd.arg[1];
          tileid=cmd.arg[2];
        } break;
    }
  }
  if (hp<1) {
    fprintf(stderr,"fighter:%d must specify hp\n",fighterid);
    return -1;
  }
  while (c-->0) {
    struct fighter *fighter=battle_add_fighter(battle,team);
    if (!fighter) return -1;
    fighter->fighterid=fighterid;
    fighter->hp=fighter->hpmax=hp;
    if (namec>sizeof(fighter->name)) namec=sizeof(fighter->name);
    memcpy(fighter->name,name,namec);
    fighter->namec=namec;
    fighter->imageid=imageid;
    fighter->tileid=tileid;
  }
  return 0;
}

/* Add a fighter for the player, from the world state.
 */
 
static int battle_add_global_fighter(struct battle *battle,struct team *team,struct world *world) {
  struct fighter *fighter=battle_add_fighter(battle,team);
  if (!fighter) return -1;
  fighter->hp=world->state.hp;
  fighter->hpmax=world->state.hpmax;
  //TODO Stats, inventory... Copy from world.
  const char *name=0;
  int namec=strings_get(&name,RID_strings_battle,11);
  if (namec>sizeof(fighter->name)) namec=sizeof(fighter->name);
  memcpy(fighter->name,name,namec);
  fighter->namec=namec;
  fighter->imageid=RID_image_sprites1;
  fighter->tileid=0x02;
  return 0;
}

/* Setup from resource.
 */
 
int battle_setup_res(struct battle *battle,struct world *world,int battleid) {
  if (!battle) return -1;
  if (battle->ready) return -1;
  if (!world) return -1;
  
  battle->battleid=battleid;
  battle->world=world;
  battle->teamv[0].control=TEAM_CONTROL_MAN;
  battle->teamv[1].control=TEAM_CONTROL_CPU;
  if (battle_add_global_fighter(battle,battle->teamv+0,world)<0) return -1;
  
  //XXX What does a large party look like? I don't think we'll actually be doing this, but at least battle supports it.
  if (0) {
    if (battle_add_global_fighter(battle,battle->teamv+0,world)<0) return -1;
    if (battle_add_global_fighter(battle,battle->teamv+0,world)<0) return -1;
    if (battle_add_global_fighter(battle,battle->teamv+0,world)<0) return -1;
  }
  
  const void *src=0;
  int srcc=rpg_res_get(&src,EGG_TID_battle,battleid);
  if (srcc<1) {
    fprintf(stderr,"battle:%d not found\n",battleid);
    return -1;
  }
  struct cmdlist_reader reader={.v=src,.c=srcc};
  struct cmdlist_entry cmd;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_battle_gold: battle->gold=(cmd.arg[0]<<8)|cmd.arg[1]; break;
      case CMD_battle_xp: battle->xp=(cmd.arg[0]<<8)|cmd.arg[1]; break;
      case CMD_battle_song: battle->songid=(cmd.arg[0]<<8)|cmd.arg[1]; break;
      case CMD_battle_fighter: {
          int fighterid=(cmd.arg[0]<<8)|cmd.arg[1];
          int c=(cmd.arg[2]<<8)|cmd.arg[3];
          if (battle_add_fighter_res(battle,battle->teamv+1,fighterid,c)<0) return -1;
        } break;
    }
  }
  
  return battle_ready(battle);
}

/* Setup ad-hoc.
 */
 
int battle_setup_adhoc(struct battle *battle,struct world *world) {
  if (!battle) return -1;
  if (battle->ready) return -1;
  fprintf(stderr,"TODO %s\n",__func__);
  return battle_ready(battle);
}

/* Setup 2-player.
 */
 
int battle_setup_pvp(struct battle *battle) {
  if (!battle) return -1;
  if (battle->ready) return -1;
  fprintf(stderr,"TODO %s\n",__func__);
  return battle_ready(battle);
}

/* Suspend.
 */
 
void battle_suspend(struct battle *battle) {
}

/* Resume.
 */
 
void battle_resume(struct battle *battle,int input1,int input2) {
  rpg_song(battle->songid);
}

/* General input.
 */
 
void battle_input(struct battle *battle,int playerid,int btnid,int value,int state) {

  /* When the prompt is awaiting acknowledgement, SOUTH on any device satisfies it.
   */
  if (prompt_awaiting_ack(battle->prompt)) {
    if ((btnid==EGG_BTN_SOUTH)&&value) {
      int fromstate=prompt_get_state(battle->prompt);
      switch (fromstate) {
        case PROMPT_STATE_WELCOME: battle->welcomed=1; break;
        case PROMPT_STATE_FAREWELL: battle->farewelled=1; break;
      }
      rpg_sound(RID_sound_ui_activate);
      prompt_ack(battle->prompt);
      battle_advance(battle);
    }
    return;
  }

  /* With no prompt in play, all input is directed to the appropriate menu.
   */
  if ((playerid>=1)&&(playerid<=2)) {
    struct team *team=battle->teamv+playerid-1;
    if (team->control!=TEAM_CONTROL_MAN) return;
    struct bmenu *bmenu=battle->bmenuv[playerid-1];
    bmenu_input(bmenu,btnid,value,state);
    if (value) {
      int choice=bmenu_get_chosen(bmenu);
      if (choice<0) {
        battle_retreat(battle,team,playerid-1);
      } else if (choice>0) {
        battle_receive_bmenu_input(battle,bmenu,team,choice,playerid-1);
      } else if (bmenu_get_state(bmenu)==BMENU_STATE_FIGHTERS) {
        int id=bmenu_get_focus(bmenu);
        if (id!=team->focus_fighter) {
          roster_highlight_id(battle->rosterv[0],team->focus_fighter,0);
          roster_highlight_id(battle->rosterv[1],team->focus_fighter,0);
          roster_highlight_id(battle->rosterv[0],id,1);
          roster_highlight_id(battle->rosterv[1],id,1);
          team->focus_fighter=id;
        }
      }
    }
  }
}

/* Update.
 */
 
void battle_update(struct battle *battle,double elapsed) {
  scene_update(battle->scene,elapsed);
  roster_update(battle->rosterv[0],elapsed);
  roster_update(battle->rosterv[1],elapsed);
  bmenu_update(battle->bmenuv[0],elapsed);
  bmenu_update(battle->bmenuv[1],elapsed);
}

/* Test completion.
 */

int battle_is_finished(const struct battle *battle) {
  if (!battle) return 1;
  if (!battle->ready) return 1;
  return battle->finished;
}

/* Fighter list.
 */
 
struct fighter *battle_fighter_by_id(struct battle *battle,int id) {
  struct team *team=battle->teamv;
  int ti=2;
  for (;ti-->0;team++) {
    struct fighter *fighter=team->fighterv;
    int fi=team->fighterc;
    for (;fi-->0;fighter++) {
      if (fighter->id==id) return fighter;
    }
  }
  return 0;
}
