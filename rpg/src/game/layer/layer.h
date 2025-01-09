/* layer.h
 * At the top level of the app, everything is organized into a layer stack.
 */
 
#ifndef LAYER_H
#define LAYER_H

#define EGG_BTN_FOCUS 0x8001

struct layer {
  void *magic;
  int opaque; // Nonzero if we can skip drawing any layers below this one.
  int defunct; // Nonzero to remove and delete at the end of the current cycle. When set, this layer does not update or render.
  
  void (*del)(struct layer *layer);
  void (*render)(struct layer *layer);
  
  /* Called for each input state change, on the topmost layer that implements this.
   * (btnid) is one of EGG_BTN_*.
   * (value) is 0 or 1.
   * (state) is multiple EGG_BTN_*, including this event, and not including other events from this cycle if not reported yet.
   * It is safe to modify the layer stack during this call, including defuncting yourself -- main reassesses focus state for each event.
   * The special btnid EGG_BTNID_FOCUS is set and cleared as the stack changes.
   * When you receive focus, (state) might not be what you expect, since you've been blacked out.
   * It is possible to receive other events before the first FOCUS.
   */
  void (*input)(struct layer *layer,int btnid,int value,int state);
  
  /* Implement the 'update' functions appropriate to your layer.
   * At most, one of them will be called per cycle.
   * Regular update is called on the topmost layer that implements (input), and any above it that don't implement (input).
   * "_bg" is called for layers below the focus if they are being rendered.
   * "_hidden" for those behind some other opaque layer.
   */
  void (*update)(struct layer *layer,double elapsed);
  void (*update_bg)(struct layer *layer,double elapsed);
  void (*update_hidden)(struct layer *layer,double elapsed);
};

/* Only generic layer api should call these.
 * Implemented in main.c.
 */
void layer_del(struct layer *layer);
struct layer *layer_new(int len);

int layer_is_resident(const struct layer *layer);
struct layer *layer_get_focus();
struct layer *layer_spawn(int len); // => WEAK, installed on top

/* Create a layer by calling one of these ctors.
 * The layer will be installed, and we return a WEAK reference.
 * If you need to abort the addition, set (layer->defunct).
 * Layer implementations should aim to be lean: Typically you're just forwarding hooks to some specific business object.
 */
struct layer *layer_spawn_hello();
struct layer *layer_spawn_world(const char *save,int savec);
struct layer *layer_spawn_battle();
struct layer *layer_spawn_cutscene();
struct layer *layer_spawn_pause();

#endif
