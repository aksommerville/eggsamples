#include "tetris.h"

struct g g={0};

void egg_client_quit(int status) {
}

int egg_client_init() {
  
  int fbw=0,fbh=0;
  egg_texture_get_status(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) {
    fprintf(stderr,"Framebuffer mismatch! header=%dx%d metadata=%dx%d\n",FBW,FBH,fbw,fbh);
    return -1;
  }
  
  if ((g.romc=egg_get_rom(0,0))<=0) return -1;
  if (!(g.rom=malloc(g.romc))) return -1;
  if (egg_get_rom(g.rom,g.romc)!=g.romc) return -1;
  strings_set_rom(g.rom,g.romc);
  
  if (!(g.font=font_new())) return -1;
  //if (font_add_image_resource(g.font,0x0020,RID_image_font9_0020)<0) return -1; // A bit too small.
  if (font_add_image_resource(g.font,0x0020,RID_image_font_0020)<0) return -1;
  
  if (egg_texture_load_image(g.texid_tiles=egg_texture_new(),RID_image_tiles)<0) return -1;
  
  srand_auto();
  
  g.fieldc=2;
  int seed=(int)(egg_time_real()*500.0);
  if (bag_reset(g.fieldc,seed))<0) return -1;
  
  return 0;
}

static int pvinput=0;//XXX

void egg_client_update(double elapsed) {
  int input=egg_input_get_one(0);//XXX we're very multiplayer, try not to use the aggregate
  if (input!=pvinput) {
    if ((input&EGG_BTN_NORTH)&&!(pvinput&EGG_BTN_NORTH)) {
      if (g.fieldc==1) g.fieldc=2;
      else g.fieldc=1;
    }
    pvinput=input;
  }
}

void egg_client_render() {
  graf_reset(&g.graf);
  render();
  graf_flush(&g.graf);
}
