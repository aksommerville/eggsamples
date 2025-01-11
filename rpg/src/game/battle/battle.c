#include "game/rpg.h"
#include "game/world/world.h"
#include "game/ui/textbox.h"
#include "battle.h"
#include "opt/rom/rom.h"

/* Delete.
 */
 
void battle_del(struct battle *battle) {
  if (!battle) return;
  if (battle->fighterv) {
    free(battle->fighterv);
  }
  if (battle->priov) free(battle->priov);
  textbox_del(battle->menu0);
  textbox_del(battle->menu1);
  textbox_del(battle->prompt);
  textbox_del(battle->roster0);
  textbox_del(battle->roster1);
  free(battle);
}

/* Create textboxes and such.
 * Layout:
 *         /negotiable
 *   +-----+--------+-----+
 *   | ros | scene  | ros |
 *   | tr0 |        | tr1 |
 *   +-----+--------+-----+ -- half fb height
 *   | prompt             | -- one row of text, height comes from font
 *   +---------+----------+
 *   | menu0   | menu1    |
 *   +---------+----------+
 *             \half fb width
 */
 
static int battle_init_ui(struct battle *battle) {
  const int margin_edge=1;
  const int margin_between=2;
  battle->scenew=FBW>>1;
  
  /* First allocate all the textboxes with dummy dimensions, and alias for brevity's sake.
   */
  struct textbox *r0,*r1,*pr,*m0,*m1;
  if (!(r0=battle->roster0=textbox_new(0,0,0,0,1,8))) return -1;
  if (!(r1=battle->roster1=textbox_new(0,0,0,0,1,8))) return -1;
  if (!(pr=battle->prompt=textbox_new(0,0,0,0,1,1))) return -1;
  if (!(m0=battle->menu0=textbox_new(0,0,0,0,3,7))) return -1;
  if (!(m1=battle->menu1=textbox_new(0,0,0,0,3,7))) return -1;
  
  /* Top edge of prompt will be exactly halfway down the framebuffer.
   * And its height depends on the font.
   */
  pr->dstx=margin_edge;
  pr->dstw=FBW-margin_edge*2;
  pr->dsty=FBH>>1;
  pr->dsth=font_get_line_height(g.font)+7;
  
  /* Menus split the space below prompt in half.
   */
  m0->dsty=m1->dsty=pr->dsty+pr->dsth+margin_between;
  m0->dsth=m1->dsth=FBH-margin_edge-m0->dsty;
  m0->dstx=1;
  m0->dstw=(FBW-margin_between-margin_edge*2)>>1;
  m1->dstx=m0->dstx+m0->dstw+margin_between;
  m1->dstw=FBW-margin_edge-m1->dstx;
  
  /* Rosters equal size in the top left and top right corners, room for scene between them, and reaching down to prompt.
   */
  r0->dsty=r1->dsty=battle->sceney=margin_edge;
  r0->dsth=r1->dsth=battle->sceneh=pr->dsty-margin_between-r0->dsty;
  r0->dstx=margin_edge;
  r0->dstw=(FBW-battle->scenew-margin_edge*2-margin_between*2)>>1;
  battle->scenex=r0->dstx+r0->dstw+margin_between;
  r1->dstx=battle->scenex+battle->scenew+margin_edge;
  r1->dstw=FBW-margin_edge-r1->dstx;
  
  return 0;
}

/* New.
 */

struct battle *battle_new() {
  struct battle *battle=calloc(1,sizeof(struct battle));
  if (!battle) return 0;
  
  if (battle_init_ui(battle)<0) {
    battle_del(battle);
    return 0;
  }
  
  return battle;
}

/* Set (ready) and validate everything generically.
 * All "setup"s should call this at the end.
  */
  
static int battle_ready(struct battle *battle) {

  // Both control schemes must be initialized sanely.
  switch (battle->control0) {
    case BATTLE_CONTROL_MAN:
    case BATTLE_CONTROL_CPU:
      break;
    default: return -1;
  }
  switch (battle->control1) {
    case BATTLE_CONTROL_MAN:
    case BATTLE_CONTROL_CPU:
      break;
    default: return -1;
  }

  // Must have at least one fighter on each team.
  if (battle->fighterc<2) return -1;
  if (battle->fighterv[0].team!=0) return -1;
  if (battle->fighterv[battle->fighterc-1].team!=1) return -1;
  
  // Other validation?
  
  // Populate rosters.
  struct fighter *fighter=battle->fighterv;
  int i=battle->fighterc;
  int row=0;
  struct textbox *tb=battle->roster0;
  for (;i-->0;fighter++) {
    if (fighter->team&&(tb==battle->roster0)) {
      row=0;
      tb=battle->roster1;
    }
    fighter->disphp=fighter->hp;
    textbox_set_option(tb,0,row,fighter->name,fighter->namec,DIR_W,1);
    row++;
    char tmp[32];
    int tmpc=snprintf(tmp,sizeof(tmp),"%d/%d",fighter->hp,fighter->hpmax);
    if ((tmpc<1)||(tmpc>=sizeof(tmp))) tmpc=0;
    textbox_set_option(tb,0,row,tmp,tmpc,DIR_E,0);
    textbox_set_comment(tb,0,row,1000+fighter-battle->fighterv);
    row++;
  }
  textbox_pack(battle->roster0);
  textbox_pack(battle->roster1);
  
  battle->ready=1;
  return battle_advance(battle);
}

/* Add unconfigured fighter.
 */
 
static struct fighter *battle_add_fighter(struct battle *battle,int team) {
  if ((team<0)||(team>1)) return 0;
  if (!team&&battle->fighterc&&battle->fighterv[battle->fighterc-1].team) return 0; // Team zero then team one.
  if (battle->fighterc>=battle->fightera) {
    int na=battle->fightera+8;
    if (na>INT_MAX/sizeof(struct fighter)) return 0;
    void *nv=realloc(battle->fighterv,sizeof(struct fighter)*na);
    if (!nv) return 0;
    battle->fighterv=nv;
    battle->fightera=na;
  }
  struct fighter *fighter=battle->fighterv+battle->fighterc++;
  memset(fighter,0,sizeof(struct fighter));
  fighter->team=team;
  fighter->qualifier=-1;
  fighter->target=-1;
  return fighter;
}

/* Set fighter name.
 */
 
static void fighter_set_name_string(struct fighter *fighter,int rid,int ix) {
  const char *src;
  int srcc=strings_get(&src,rid,ix);
  if ((srcc>0)&&(srcc<=sizeof(fighter->name))) {
    memcpy(fighter->name,src,srcc);
    fighter->namec=srcc;
  } else {
    fighter->name[0]='?';
    fighter->namec=1;
  }
}

/* Add a fighter for the player.
 */
 
static int battle_add_fighter_global(struct battle *battle,int team) {
  struct fighter *fighter=battle_add_fighter(battle,team);
  if (!fighter) return -1;
  //TODO Where does image come from, for the hero fighter?
  fighter->imageid=RID_image_sprites1;//TODO
  fighter->tileid=0x00;
  fighter->hp=g.world->state.hp;
  fighter->hpmax=g.world->state.hpmax;
  fighter_set_name_string(fighter,RID_strings_battle,1);
  return 0;
}

/* Add a fighter by decoding resource.
 */
 
static int battle_add_fighter_res(struct battle *battle,int team,int fighterid,int c) {
  const void *serial;
  int serialc=rpg_res_get(&serial,EGG_TID_fighter,fighterid);
  if (serialc<1) {
    fprintf(stderr,"fighter:%d not found, required by battle:%d\n",fighterid,battle->battleid);
    return -2;
  }
  while (c-->0) {
    struct fighter *fighter=battle_add_fighter(battle,team);
    if (!fighter) return -1;
    struct rom_command_reader reader={.v=serial,.c=serialc};
    struct rom_command cmd;
    while (rom_command_reader_next(&cmd,&reader)>0) {
      switch (cmd.opcode) {
        case CMD_fighter_hp: fighter->hp=fighter->hpmax=(cmd.argv[0]<<8)|cmd.argv[1]; break;
        case CMD_fighter_name: {
            int rid=(cmd.argv[0]<<8)|cmd.argv[1];
            int ix=(cmd.argv[2]<<8)|cmd.argv[3];
            fighter_set_name_string(fighter,rid,ix);
          } break;
        case CMD_fighter_image: {
            fighter->imageid=(cmd.argv[0]<<8)|cmd.argv[1];
            fighter->tileid=cmd.argv[2];
          } break;
        default: fprintf(stderr,"fighter:%d:WARNING: Ignoring unexpected command 0x%02x with %d-byte payload.\n",fighterid,cmd.opcode,cmd.argc);
      }
    }
  }
  return 0;
}

/* Setup from resource.
 */
 
int battle_setup_res(struct battle *battle,int battleid) {
  int err;
  if (!battle||battle->ready) return -1;
  if (!g.world) return -1; // Resource-configured battles read and write globals.
  
  battle->battleid=battleid;
  battle->control0=BATTLE_CONTROL_MAN;
  battle->control1=BATTLE_CONTROL_CPU;
  battle->fighterc=0;
  
  if (battle_add_fighter_global(battle,0)<0) return -1;
  
  const void *serial=0;
  int serialc=rpg_res_get(&serial,EGG_TID_battle,battleid);
  if (serialc<1) return -1;
  struct rom_command_reader reader={.v=serial,.c=serialc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_battle_gold: battle->gold=(cmd.argv[0]<<8)|cmd.argv[1]; break;
      case CMD_battle_xp: battle->xp=(cmd.argv[0]<<8)|cmd.argv[1]; break;
      case CMD_battle_song: egg_play_song((cmd.argv[0]<<8)|cmd.argv[1],0,1); break;
      case CMD_battle_fighter: {
          int fighterid=(cmd.argv[0]<<8)|cmd.argv[1];
          int c=(cmd.argv[2]<<8)|cmd.argv[3];
          if (!c) {
            fprintf(stderr,"battle:%d:WARNING: fighter:%d with count zero. Ignoring.\n",battleid,fighterid);
            continue;
          }
          if ((err=battle_add_fighter_res(battle,1,fighterid,c))<0) return err;
        } break;
      default: fprintf(stderr,"battle:%d:WARNING: Ignoring unknown command 0x%02x with %d-byte payload.\n",battleid,cmd.opcode,cmd.argc);
    }
  }
  
  return battle_ready(battle);
}

/* Select menu items per fighter.
 */
 
static void battle_select_action(struct battle *battle,struct fighter *fighter,struct textbox *textbox) {
  int col,row;
  textbox_get_selection(&col,&row,textbox);
  if (col) return;
  switch (row) {
    case 0: {
        fighter->action='a';
        fighter->input_state=2;
      } break;
    case 1: {
        fighter->action='m';
        fighter->input_state=1;
      } break;
    case 2: {
        fighter->action='i';
        fighter->input_state=1;
      } break;
    case 3: {
        fighter->action='d';
        fighter->input_state=3;
      } break;
    case 4: {
        fighter->action='r';
        fighter->input_state=3;
      } break;
    default: return;
  }
  battle_advance(battle);
}
 
static void battle_select_qualifier(struct battle *battle,struct fighter *fighter,struct textbox *textbox) {
  fprintf(stderr,"%s\n",__func__);//TODO qualifiers
  fighter->input_state=2; //TODO 3 if the selected spell or item doesn't need a target. Also, if there's just one qualifying target, pick it automatically.
  battle_advance(battle);
}
 
static void battle_select_target(struct battle *battle,struct fighter *fighter,struct textbox *textbox) {
  int col,row;
  textbox_get_selection(&col,&row,textbox);
  if (col) return;
  int fighterp=textbox_get_comment(textbox,col,row);
  if ((fighterp<0)||(fighterp>=battle->fighterc)) return;
  if (fighterp<0) return;
  fighter->target=fighterp;
  fighter->input_state=3;
  battle_advance(battle);
}

/* Digested input events.
 */
 
static void battle_move(struct battle *battle,int dx,int dy) {
  if (!battle->tbfocus) return;
  textbox_move(battle->tbfocus,dx,dy);
}

static void battle_activate(struct battle *battle) {
  if (!battle->tbfocus) return;
  
  /* When the prompt is focused, we're just waiting for acknowledgement.
   * State is already updated for it, so just advance.
   */
  if (battle->tbfocus==battle->prompt) {
    battle_advance(battle);
    return;
  }
  
  /* When it's a menu, find the first unconfigured fighter on the corresponding team.
   */
  struct fighter *fighter=0;
  if (battle->tbfocus==battle->menu0) {
    struct fighter *q=battle->fighterv;
    int i=battle->fighterc;
    for (;i-->0;q++) {
      if (q->team!=0) continue;
      if (q->input_state>=3) continue;
      fighter=q;
      break;
    }
  } else if (battle->tbfocus==battle->menu1) {
    struct fighter *q=battle->fighterv;
    int i=battle->fighterc;
    for (;i-->0;q++) {
      if (q->team!=1) continue;
      if (q->input_state>=3) continue;
      fighter=q;
      break;
    }
  }
  if (fighter) switch (fighter->input_state) {
    case 0: battle_select_action(battle,fighter,battle->tbfocus); return;
    case 1: battle_select_qualifier(battle,fighter,battle->tbfocus); return;
    case 2: battle_select_target(battle,fighter,battle->tbfocus); return;
  }
}

static void battle_cancel(struct battle *battle) {
  // Cancel is a real thing when a menu is focussed and the associated fighter is above input_state 0.
  if (!battle->tbfocus) return;
  if (battle->tbfocus==battle->prompt) return;
  struct fighter *fighter=0;
  if (battle->tbfocus==battle->menu0) {
    struct fighter *q=battle->fighterv;
    int i=battle->fighterc;
    for (;i-->0;q++) {
      if (q->team!=0) continue;
      if (q->input_state>=3) continue;
      fighter=q;
      break;
    }
  } else if (battle->tbfocus==battle->menu1) {
    struct fighter *q=battle->fighterv;
    int i=battle->fighterc;
    for (;i-->0;q++) {
      if (q->team!=1) continue;
      if (q->input_state>=3) continue;
      fighter=q;
      break;
    }
  }
  if (!fighter||!fighter->input_state) return;
  fighter->action=0;
  fighter->qualifier=-1;
  fighter->target=-1;
  fighter->input_state=0;
  battle_advance(battle);
}

/* Event reception.
 */

void battle_suspend(struct battle *battle) {
}

void battle_resume(struct battle *battle,int state) {
}

void battle_input(struct battle *battle,int btnid,int value,int state) {
  if (!battle->ready) return;
  if (value) switch (btnid) {
    case EGG_BTN_LEFT: battle_move(battle,-1,0); break;
    case EGG_BTN_RIGHT: battle_move(battle,1,0); break;
    case EGG_BTN_UP: battle_move(battle,0,-1); break;
    case EGG_BTN_DOWN: battle_move(battle,0,1); break;
    case EGG_BTN_SOUTH: battle_activate(battle); break;
    case EGG_BTN_WEST: battle_cancel(battle); break;
  }
}

void battle_update(struct battle *battle,double elapsed) {
  if (!battle->ready) return;
  
  /* Update roster fields if any HP changed.
   */
  struct fighter *fighter=battle->fighterv;
  int i=0; for (;i<battle->fighterc;i++,fighter++) {
    if (fighter->hp==fighter->disphp) continue;
    fighter->disphp=fighter->hp;
    char tmp[32];
    int tmpc=snprintf(tmp,sizeof(tmp),"%d/%d",fighter->hp,fighter->hpmax);
    if ((tmpc<1)||(tmpc>=sizeof(tmp))) tmpc=0;
    int col,row;
    if (textbox_find_option_by_comment(&col,&row,battle->roster0,1000+i)>=0) {
      textbox_set_option(battle->roster0,col,row,tmp,tmpc,DIR_E,0);
    } else if (textbox_find_option_by_comment(&col,&row,battle->roster1,1000+i)>=0) {
      textbox_set_option(battle->roster1,col,row,tmp,tmpc,DIR_E,0);
    }
  }
}

/* Team composition conveniences.
 */
 
int battle_get_single_man_team(const struct battle *battle) {
  if (battle->control0==BATTLE_CONTROL_MAN) {
    if (battle->control1==BATTLE_CONTROL_CPU) return 0;
  } else if (battle->control0==BATTLE_CONTROL_CPU) {
    if (battle->control1==BATTLE_CONTROL_MAN) return 1;
  }
  return -1;
}

int battle_get_single_cpu_team(const struct battle *battle) {
  if (battle->control0==BATTLE_CONTROL_MAN) {
    if (battle->control1==BATTLE_CONTROL_CPU) return 1;
  } else if (battle->control0==BATTLE_CONTROL_CPU) {
    if (battle->control1==BATTLE_CONTROL_MAN) return 0;
  }
  return -1;
}
