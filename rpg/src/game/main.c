#include "rpg.h"
#include "game/world/map.h"
#include "game/sprite/sprite.h"

struct g g={0};

/* Egg entry points.
 */

void egg_client_quit(int status) {
}

void egg_client_notify(int k,int v) {
}

int egg_client_init() {

  int fbw=0,fbh=0;
  egg_texture_get_size(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) {
    fprintf(stderr,"Expected %dx%d framebuffer (rpg.h) but got %dx%d (metadata)\n",FBW,FBH,fbw,fbh);
    return -1;
  }
  
  if (!(g.font=font_new())) return -1;
  if (font_add_image(g.font,RID_image_font9_0020,0x0020)<0) return -1;
  
  srand_auto();
  
  int err=rpg_res_init();
  if (err<0) {
    if (err!=-2) fprintf(stderr,"Unspecified error loading resources.\n");
    return -2;
  }
  
  if (!layer_spawn_hello()) return -1;
  
  return 0;
}

static void send_input_event(int btnid,int value) {
  int i=g.layerc;
  while (i-->0) {
    struct layer *layer=g.layerv[i];
    if (layer->defunct) continue;
    if (layer->input) {
      layer->input(layer,btnid,value,g.pvinput);
      return;
    }
  }
}

void egg_client_update(double elapsed) {

  /* Receive and deliver input.
   */
  int input=egg_input_get_one(0);
  if (input!=g.pvinput) {
    int bit=0x8000;
    for (;bit;bit>>=1) {
      int prev=g.pvinput&bit,next=input&bit;
      if (prev==next) continue;
      if (next) {
        g.pvinput|=bit;
        send_input_event(bit,1);
      } else {
        g.pvinput&=~bit;
        send_input_event(bit,0);
      }
    }
  }

  /* Trigger update hooks.
   */
  int gotinput=0,gotopaque=0;
  int i=g.layerc;
  while (i-->0) {
    struct layer *layer=g.layerv[i];
    if (layer->defunct) continue;
    if (!gotinput) {
      if (layer->update) {
        layer->update(layer,elapsed);
        if (layer->defunct) continue;
      }
    } else if (!gotopaque) {
      if (layer->update_bg) {
        layer->update_bg(layer,elapsed);
        if (layer->defunct) continue;
      }
    } else {
      if (layer->update_hidden) {
        layer->update_hidden(layer,elapsed);
        if (layer->defunct) continue;
      }
    }
    if (layer->input) gotinput=1;
    if (layer->opaque) gotopaque=1;
  }
  
  /* Drop defunct layers.
   */
  for (i=g.layerc;i-->0;) {
    struct layer *layer=g.layerv[i];
    if (!layer->defunct) continue;
    g.layerc--;
    memmove(g.layerv+i,g.layerv+i+1,sizeof(void*)*(g.layerc-i));
    layer_del(layer);
    g.layerdirty=1;
  }
  
  /* If the layer stack is empty, terminate.
   */
  if (!g.layerc) {
    egg_terminate(0);
    return;
  }
  
  /* Check for focus changes.
   */
  if (g.layerdirty) {
    g.layerdirty=0;
    struct layer *nfocus=0;
    for (i=g.layerc;i-->0;) {
      struct layer *layer=g.layerv[i];
      if (layer->defunct) continue; // how?
      if (!layer->input) continue;
      nfocus=layer;
      break;
    }
    if (nfocus!=g.pvfocus) {
      if (layer_is_resident(g.pvfocus)) {
        g.pvfocus->input(g.pvfocus,EGG_BTN_FOCUS,0,g.pvinput);
      }
      g.pvfocus=nfocus;
      nfocus->input(nfocus,EGG_BTN_FOCUS,1,g.pvinput);
    }
  }
}

void egg_client_render() {
  g.framec++;
  graf_reset(&g.graf);
  
  /* Find the topmost opaque layer.
   * Nothing should be defunct at this point, but we'll allow that they could be.
   */
  int i=g.layerc,opaquep=-1;
  while (i-->0) {
    struct layer *layer=g.layerv[i];
    if (layer->defunct) continue;
    if (layer->opaque) {
      opaquep=i;
      break;
    }
  }
  
  /* If there isn't an opaque layer, black out the framebuffer.
   */
  if (opaquep<0) {
    graf_fill_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
    opaquep=0;
  }
  
  /* Render in order from (opaquep).
   */
  for (;opaquep<g.layerc;opaquep++) {
    struct layer *layer=g.layerv[opaquep];
    if (layer->defunct) continue;
    if (!layer->render) continue;
    layer->render(layer);
  }
  
  graf_flush(&g.graf);
}

/* Layers and layer stack.
 */
 
void layer_del(struct layer *layer) {
  if (!layer) return;
  if (layer->del) layer->del(layer);
  free(layer);
}

struct layer *layer_new(int len) {
  if (len<(int)sizeof(struct layer)) return 0;
  struct layer *layer=calloc(1,len);
  if (!layer) return 0;
  return layer;
}

int layer_is_resident(const struct layer *layer) {
  if (!layer) return 0;
  int i=g.layerc;
  while (i-->0) {
    if (g.layerv[i]==layer) return 1;
  }
  return 0;
}

struct layer *layer_get_focus() {
  int i=g.layerc;
  while (i-->0) {
    struct layer *layer=g.layerv[i];
    if (layer->defunct) continue;
    if (layer->input) return layer;
  }
  return 0;
}

struct layer *layer_spawn(int len) {
  if (g.layerc>=g.layera) {
    int na=g.layera+16;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(g.layerv,sizeof(void*)*na);
    if (!nv) return 0;
    g.layerv=nv;
    g.layera=na;
  }
  struct layer *layer=layer_new(len);
  if (!layer) return 0;
  g.layerv[g.layerc++]=layer;
  g.layerdirty=1;
  return layer;
}

/* Audio.
 */
 
void rpg_song(int rid) {
  if (rid==g.song_playing) return;
  g.song_playing=rid;
  egg_play_song(1,rid,1,0.5f,1.0f);
}

void rpg_sound(int rid) {
  egg_play_sound(rid,1.0f,0.0f);
}
