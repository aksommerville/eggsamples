/* menu.h
 * A menu is a set of buttons. Normally owned by a layer.
 * Buttons are elements on screen that can be focussed and activated.
 * We're not a full nested GUI -- just these two things.
 */
 
#ifndef MENU_H
#define MENU_H

struct menu;
struct button;

struct menu {
  struct button **buttonv;
  int buttonc,buttona;
  
  void *userdata; // Set directly.
  
  /* Owner may set directly.
   * Render the focus highlight at position (t), 0..1,  between (a) and (b).
   * If (t) is not strictly between 0 and 1, (a) and (b) are the same button.
   * If you're not doing animation, ignore (a) and just highlight (b).
   * Set (render_focus_before_buttons) to do it first, then render the buttons on top of the highlight.
   */
  void (*cb_render_focus)(struct menu *menu,struct button *a,struct button *b,double t);
  int render_focus_before_buttons;
  
  void (*cb_activate)(struct menu *menu,struct button *button);
  
  struct button *focus_from,*focus_to; // (from) is optional, only present while animating.
  double focus_clock; // Counts up.
  double focus_anim_time;
};

struct button {
  struct menu *menu; // WEAK
  int id;
  int enable;
  int x,y,w,h;
  
  char rendermode; // [0ctdx]=none,color,tile,decal,custom
  uint32_t rgba; // rendermode=='c'
  int imageid; // rendermode=='t' or rendermode=='d'
  int texid; // rendermode=='d', if we own the texture (ie from text)
  uint8_t tileid; // rendermode=='t'
  uint8_t xform; // rendermode=='t' or rendermode=='d'
  int16_t srcx,srcy,srcw,srch; // rendermode=='d'
  void (*render_cb)(struct button *button,void *userdata); // rendermode=='x'
  void *render_userdata; // rendermode=='x'
  int xalign,yalign; // rendermode=='t' or rendermode=='d'
};

void menu_del(struct menu *menu);
struct menu *menu_new();

/* Digested input events.
 * Not necessary if you call menu_input().
 */
void menu_motion(struct menu *menu,int dx,int dy);
void menu_activate(struct menu *menu);
void menu_cancel(struct menu *menu);

/* Main hooks, matching the layer interface.
 */
void menu_input(struct menu *menu,int btnid,int value,int state);
void menu_update(struct menu *menu,double elapsed);
void menu_render(struct menu *menu);

/* Quickie button ops, accessed by id.
 */
void menu_enable_button(struct menu *menu,int id,int enable);

/* Add a button to the menu and return a WEAK reference.
 */
struct button *button_spawn(struct menu *menu);

/* After spawning a button, call one of these to tell us how to render it.
 * These will replace (w,h).
 */
void button_setup_color(struct button *button,int w,int h,uint32_t rgba);
void button_setup_tile(struct button *button,int imageid,uint8_t tileid,uint8_t xform);
void button_setup_decal(struct button *button,int imageid,int16_t srcx,int16_t srcy,int16_t w,int16_t h,uint8_t xform);
void button_setup_text(struct button *button,const char *src,int srcc,uint32_t rgba);
void button_setup_string(struct button *button,int rid,int index,uint32_t rgba);
void button_setup_custom(struct button *button,void (*render)(struct button *button,void *userdata),void *userdata);

/* With everything spawned, either size and position them yourself, or use one of these conveniences.
 */
void menu_pack_col(struct menu *menu,int x,int y,int w,int h,int xalign,int yalign,int uniform_widths);
void menu_pack_row(struct menu *menu,int x,int y,int w,int h,int xalign,int yalign,int uniform_heights);

struct button *menu_get_initial_focus(struct menu *menu,int end);
struct button *menu_get_next_focus(struct menu *menu,struct button *from,int dx,int dy);

/* For menu's use only.
 */
void button_del(struct button *button);
struct button *button_new();
void button_render(struct button *button);

#endif
