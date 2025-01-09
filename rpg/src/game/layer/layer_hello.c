/* layer_hello.c
 * The main menu.
 * This is deliberately sparse, and if you're copying this project, you'll definitely want to flesh it out with more color and animation.
 */

#include "game/rpg.h"
#include "game/ui/menu.h"

/* Button IDs match index in strings:1, as a convenience.
 */
#define MENUITEM_CONTINUE 3
#define MENUITEM_NEWGAME 4
#define MENUITEM_QUIT 5

/* Object definition.
 */
 
struct layer_hello {
  struct layer hdr;
  struct menu *menu;
  char *save;
  int savec;
};

#define LAYER ((struct layer_hello*)layer)

/* Cleanup.
 */
 
static void _hello_del(struct layer *layer) {
  menu_del(LAYER->menu);
  if (LAYER->save) free(LAYER->save);
}

/* Load the saved game from storage, replace whatever we have.
 */
 
static int hello_reacquire_save(struct layer *layer) {
  char *nv=0;
  int nc=egg_store_get(0,0,"savedGame",9);
  if (nc>0) {
    if (!(nv=malloc(nc+1))) return -1;
    if (egg_store_get(nv,nc,"savedGame",9)!=nc) {
      free(nv);
      return -1;
    }
    nv[nc]=0;
  } else nc=0;
  if (LAYER->save) free(LAYER->save);
  LAYER->save=nv;
  LAYER->savec=nc;
  return 0;
}

/* Continue.
 */
 
static void hello_continue(struct layer *layer) {
  layer_spawn_world(LAYER->save,LAYER->savec);
}

/* New game.
 */
 
static void hello_newgame(struct layer *layer) {
  layer_spawn_world(0,0);
}

/* Quit.
 */
 
static void hello_quit(struct layer *layer) {
  layer->defunct=1;
}

/* Input.
 */
 
static void _hello_input(struct layer *layer,int btnid,int value,int state) {
  if (value) switch (btnid) {
    case EGG_BTN_FOCUS: {
        egg_play_song(RID_song_hello,0,1);
        hello_reacquire_save(layer);
        menu_enable_button(LAYER->menu,MENUITEM_CONTINUE,LAYER->savec);
        if (!LAYER->menu->focus_to) menu_motion(LAYER->menu,0,1); // autofocus, if not yet established.
      } break;
    case EGG_BTN_WEST: hello_quit(layer); return;
  }
  menu_input(LAYER->menu,btnid,value,state);
}

/* Update.
 */
 
static void _hello_update(struct layer *layer,double elapsed) {
  menu_update(LAYER->menu,elapsed);
}

/* Render.
 */
 
static void _hello_render(struct layer *layer) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
  menu_render(LAYER->menu);
}

/* Activate menu item.
 */
 
static void hello_cb_activate(struct menu *menu,struct button *button) {
  struct layer *layer=menu->userdata;
  switch (button->id) {
    case MENUITEM_CONTINUE: hello_continue(layer); break;
    case MENUITEM_NEWGAME: hello_newgame(layer); break;
    case MENUITEM_QUIT: hello_quit(layer); break;
  }
}

/* New.
 */
 
struct layer *layer_spawn_hello() {
  struct layer *layer=layer_spawn(sizeof(struct layer_hello));
  if (!layer) return 0;
  
  layer->opaque=1;
  layer->magic=layer_spawn_hello;
  layer->del=_hello_del;
  layer->input=_hello_input;
  layer->update=_hello_update;
  //layer->update_bg=_hello_update;
  //layer->update_hidden=_hello_update;
  layer->render=_hello_render;
  
  if (!(LAYER->menu=menu_new())) {
    layer->defunct=1;
    return 0;
  }
  LAYER->menu->cb_activate=hello_cb_activate;
  LAYER->menu->userdata=layer;
  
  struct button *button;
  if (button=button_spawn(LAYER->menu)) {
    button_setup_string(button,1,button->id=MENUITEM_CONTINUE,0xffffffff);
    button->enable=0;
  }
  if (button=button_spawn(LAYER->menu)) {
    button_setup_string(button,1,button->id=MENUITEM_NEWGAME,0xffffffff);
  }
  if (button=button_spawn(LAYER->menu)) {
    button_setup_string(button,1,button->id=MENUITEM_QUIT,0xffffffff);
  }
  menu_pack_col(LAYER->menu,FBW-60,FBH-40,60,40,-1,-1,0);
  
  return layer;
}
