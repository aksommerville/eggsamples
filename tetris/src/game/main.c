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
  
  g.fieldc=1;
  int seed=(int)(fmod(egg_time_real()*500.0,2000000000.0));
  fprintf(stderr,"egg_time_real: %f, seed=0x%08x\n",egg_time_real(),seed);
  if (bag_reset(g.fieldc,seed)<0) return -1;
  if (field_init(&g.l,0,1)<0) return -1;
  g.l.playerv[0].playerid=1;
  //g.l.playerv[1].playerid=2;
  if (g.fieldc==2) {
    if (field_init(&g.r,1,1)<0) return -1;
    g.r.playerv[0].playerid=2;
  }
  g.running=1;
  
  //TODO Let the user pick a song. And I guess make a few more. And None should be an option too.
  //egg_play_song(RID_song_in_thru_the_window,0,1);
  egg_play_song(RID_song_bakers_dozen,0,1);
  
  return 0;
}

void egg_client_update(double elapsed) {
  if (g.running) {
    field_update(&g.l,elapsed);
    if (g.fieldc==2) {
      field_update(&g.r,elapsed);
    }
  }
}

void egg_client_render() {
  graf_reset(&g.graf);
  render();
  graf_flush(&g.graf);
}
