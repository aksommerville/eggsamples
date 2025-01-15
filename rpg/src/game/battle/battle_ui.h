/* battle_ui.h
 * Private to battle.
 * There are four UI elements in battle:
 *  - scene: Animated graphics showing the current action.
 *  - prompt: Text describing current action.
 *  - roster(2): List of fighters on one team, with their HP.
 *  - bmenu(2): Input zone per team.
 * UI elements are allowed to interact directly with the battle object.
 */
 
#ifndef BATTLE_UI_H
#define BATTLE_UI_H

struct scene;
struct prompt;
struct roster;
struct bmenu;
struct battle;

void scene_del(struct scene *scene);
struct scene *scene_new(struct battle *battle);
void scene_set_bounds(struct scene *scene,int x,int y,int w,int h);
void scene_update(struct scene *scene,double elapsed);
void scene_render(struct scene *scene);

void prompt_del(struct prompt *prompt);
struct prompt *prompt_new(struct battle *battle);
void prompt_set_bounds(struct prompt *prompt,int x,int y,int w,int h);
int prompt_awaiting_ack(const struct prompt *prompt);
void prompt_ack(struct prompt *prompt);
void prompt_render(struct prompt *prompt);
int prompt_vertical_margins(); // Sum of upper and lower.
void prompt_set_text(struct prompt *prompt,const char *src,int srcc);
void prompt_set_res(struct prompt *prompt,int rid,int ix);
void prompt_set_res_iii(struct prompt *prompt,int rid,int ix,int a,int b,int c);
void prompt_set_res_sss(struct prompt *prompt,int rid,int ix,const char *a,int ac,const char *b,int bc,const char *c,int cc);
void prompt_set_res_sis(struct prompt *prompt,int rid,int ix,const char *a,int ac,int b,const char *c,int cc);
// Add other parameter shapes as the need arises. It's safe on the calling side to provide more parameters than needed.
// Setting a message also starts requiring acknowledgement -- For now at least, there are no passive messages.
// We don't know what these states mean, we just hold on to it for you:
void prompt_set_state(struct prompt *prompt,int state);
int prompt_get_state(const struct prompt *prompt);
#define PROMPT_STATE_NONE 0
#define PROMPT_STATE_WELCOME 1
#define PROMPT_STATE_FAREWELL 2

void roster_del(struct roster *roster);
struct roster *roster_new(struct battle *battle,int teamp);
void roster_set_bounds(struct roster *roster,int x,int y,int w,int h);
void roster_update(struct roster *roster,double elapsed);
void roster_render(struct roster *roster);
void roster_clear_highlights(struct roster *roster);
void roster_highlight_id(struct roster *roster,int id,int highlight);

void bmenu_del(struct bmenu *bmenu);
struct bmenu *bmenu_new(struct battle *battle,int teamp);
void bmenu_set_bounds(struct bmenu *bmenu,int x,int y,int w,int h);
void bmenu_input(struct bmenu *bmenu,int btnid,int value,int state);
void bmenu_update(struct bmenu *bmenu,double elapsed);
void bmenu_render(struct bmenu *bmenu);
void bmenu_clear(struct bmenu *bmenu); // Drop all options
void bmenu_add_option(struct bmenu *bmenu,const char *text,int textc,int id); // (id) must be >0 and should be unique
void bmenu_pack(struct bmenu *bmenu); // Must call after adding options.
// Same as prompt, we let you tag a state, but we don't know what it means:
void bmenu_set_state(struct bmenu *bmenu,int state);
int bmenu_get_state(const struct bmenu *bmenu);
#define BMENU_STATE_NONE 0
#define BMENU_STATE_ACTIONS 1
#define BMENU_STATE_SPELLS 2
#define BMENU_STATE_ITEMS 3
#define BMENU_STATE_FIGHTERS 4
int bmenu_get_chosen(const struct bmenu *bmenu); // => option id, zero, or <0 if cancelled. Check after any input.
int bmenu_get_focus(const struct bmenu *bmenu); // => option id or zero

#endif
