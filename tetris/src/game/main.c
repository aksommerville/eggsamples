#include "tetris.h"

struct g g={0};

void egg_client_quit(int status) {
}

void egg_client_notify(int k,int v) {
}

int egg_client_init() {
  
  int fbw=0,fbh=0;
  egg_texture_get_size(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) {
    fprintf(stderr,"Framebuffer mismatch! header=%dx%d metadata=%dx%d\n",FBW,FBH,fbw,fbh);
    return -1;
  }
  
  if ((g.romc=egg_rom_get(0,0))<=0) return -1;
  if (!(g.rom=malloc(g.romc))) return -1;
  if (egg_rom_get(g.rom,g.romc)!=g.romc) return -1;
  
  if (!(g.font=font_new())) return -1;
  //if (font_add_image_resource(g.font,0x0020,RID_image_font9_0020)<0) return -1; // A bit too small.
  if (font_add_image(g.font,RID_image_font_0020,0x0020)<0) return -1;
  
  if (egg_texture_load_image(g.texid_tiles=egg_texture_new(),RID_image_tiles)<0) return -1;
  
  srand_auto(); // Probably not necessary... Where it counts, we use a private PRNG. See bag.c.
  
  db_init();
  
  g.cursorv[0].color=0x2063dbff;
  g.cursorv[1].color=0xc70041ff;
  g.cursorv[2].color=0x00b400ff;
  g.cursorv[3].color=0xd49100ff;
  g.cursorv[4].color=0x00d49bff;
  g.cursorv[5].color=0xd8280cff;
  g.cursorv[6].color=0x697751ff;
  g.cursorv[7].color=0x6e6d75ff;
  
  g.fieldc=1;
  g.running=0;
  tetris_song(RID_song_bakers_dozen);
  
  return 0;
}

static void begin_game(int level) {
  
  // Who's playing where?
  int lpv[8],rpv[8],lpc=0,rpc=0,i=8,playerid=1;
  const struct cursor *cursor=g.cursorv;
  for (;i-->0;cursor++,playerid++) {
    if (cursor->decline) continue;
    if (!cursor->pvinput) continue;
    if (cursor->p<0) continue;
    if (cursor->p<10) lpv[lpc++]=playerid;
    else if (cursor->p<20) rpv[rpc++]=playerid;
    else if (cursor->p==20) lpv[lpc++]=playerid;
    else if (cursor->p==21) rpv[rpc++]=playerid;
  }
  if ((lpc<1)&&(rpc<1)) return;
  
  int seed=(int)(fmod(egg_time_real()*500.0,2000000000.0));
  //fprintf(stderr,"%s level=%d, seed=%d\n",__func__,level,seed);
  if (bag_reset(g.fieldc,seed)<0) return;
  
  field_init(&g.l,0,lpc,level);
  for (i=0;i<lpc;i++) g.l.playerv[i].playerid=lpv[i];
  if (g.fieldc==2) {
    field_init(&g.r,1,rpc,level);
    for (i=0;i<rpc;i++) g.r.playerv[i].playerid=rpv[i];
  } else {
    field_init(&g.r,1,0,0);
  }
  
  g.running=1;
  tetris_song(RID_song_in_thru_the_window);
}

static void cursor_activate(struct cursor *cursor) {
  if (cursor->decline) {
    cursor->decline=0;
  } else if ((cursor->p>=0)&&(cursor->p<=9)) {
    begin_game(cursor->p);
  } else if ((cursor->p>=10)&&(cursor->p<=19)&&(g.fieldc==2)) {
    begin_game(cursor->p-10);
  }
}

static void cursor_move(struct cursor *cursor,int dx,int dy) {
  // There is some order to it but it's largely ad-hoc. Just handle each case separately.
  switch (cursor->p) {
    case  0: if (dx<0) cursor->p=14; else if (dx>0) cursor->p= 1; else if (dy<0) cursor->p=20; else cursor->p= 5; break;
    case  1: if (dx<0) cursor->p= 0; else if (dx>0) cursor->p= 2; else if (dy<0) cursor->p=20; else cursor->p= 6; break;
    case  2: if (dx<0) cursor->p= 1; else if (dx>0) cursor->p= 3; else if (dy<0) cursor->p=20; else cursor->p= 7; break;
    case  3: if (dx<0) cursor->p= 2; else if (dx>0) cursor->p= 4; else if (dy<0) cursor->p=20; else cursor->p= 8; break;
    case  4: if (dx<0) cursor->p= 3; else if (dx>0) cursor->p=10; else if (dy<0) cursor->p=20; else cursor->p= 9; break;
    case  5: if (dx<0) cursor->p=19; else if (dx>0) cursor->p= 6; else if (dy<0) cursor->p= 0; else cursor->p=20; break;
    case  6: if (dx<0) cursor->p= 5; else if (dx>0) cursor->p= 7; else if (dy<0) cursor->p= 1; else cursor->p=20; break;
    case  7: if (dx<0) cursor->p= 6; else if (dx>0) cursor->p= 8; else if (dy<0) cursor->p= 2; else cursor->p=20; break;
    case  8: if (dx<0) cursor->p= 7; else if (dx>0) cursor->p= 9; else if (dy<0) cursor->p= 3; else cursor->p=20; break;
    case  9: if (dx<0) cursor->p= 8; else if (dx>0) cursor->p=15; else if (dy<0) cursor->p= 4; else cursor->p=20; break;
    case 10: if (dx<0) cursor->p= 4; else if (dx>0) cursor->p=11; else if (dy<0) cursor->p=21; else cursor->p=15; break;
    case 11: if (dx<0) cursor->p=10; else if (dx>0) cursor->p=12; else if (dy<0) cursor->p=21; else cursor->p=16; break;
    case 12: if (dx<0) cursor->p=11; else if (dx>0) cursor->p=13; else if (dy<0) cursor->p=21; else cursor->p=17; break;
    case 13: if (dx<0) cursor->p=12; else if (dx>0) cursor->p=14; else if (dy<0) cursor->p=21; else cursor->p=18; break;
    case 14: if (dx<0) cursor->p=13; else if (dx>0) cursor->p= 0; else if (dy<0) cursor->p=21; else cursor->p=19; break;
    case 15: if (dx<0) cursor->p= 9; else if (dx>0) cursor->p=16; else if (dy<0) cursor->p=10; else cursor->p=21; break;
    case 16: if (dx<0) cursor->p=15; else if (dx>0) cursor->p=17; else if (dy<0) cursor->p=11; else cursor->p=21; break;
    case 17: if (dx<0) cursor->p=16; else if (dx>0) cursor->p=18; else if (dy<0) cursor->p=12; else cursor->p=21; break;
    case 18: if (dx<0) cursor->p=17; else if (dx>0) cursor->p=19; else if (dy<0) cursor->p=13; else cursor->p=21; break;
    case 19: if (dx<0) cursor->p=18; else if (dx>0) cursor->p= 5; else if (dy<0) cursor->p=14; else cursor->p=21; break;
    case 20: if (dx<0) g.fieldc=1; else if (dx>0) g.fieldc=2; else if (dy<0) cursor->p= 9; else cursor->p= 0; break;
    case 21: if (dx<0) { g.fieldc=1; cursor->p=20; } else if (dx>0) g.fieldc=2; else if (dy<0) cursor->p=19; else cursor->p=10; break;
    default: cursor->p=0;
  }
}

static void cursor_decline(struct cursor *cursor) {
  cursor->decline=1;
}

static void cursor_input(struct cursor *cursor,int input) {
  if ((input&EGG_BTN_LEFT)&&!(cursor->pvinput&EGG_BTN_LEFT)) cursor_move(cursor,-1,0);
  if ((input&EGG_BTN_RIGHT)&&!(cursor->pvinput&EGG_BTN_RIGHT)) cursor_move(cursor,1,0);
  if ((input&EGG_BTN_UP)&&!(cursor->pvinput&EGG_BTN_UP)) cursor_move(cursor,0,-1);
  if ((input&EGG_BTN_DOWN)&&!(cursor->pvinput&EGG_BTN_DOWN)) cursor_move(cursor,0,1);
  if ((input&EGG_BTN_SOUTH)&&!(cursor->pvinput&EGG_BTN_SOUTH)) cursor_activate(cursor);
  if ((input&EGG_BTN_WEST)&&!(cursor->pvinput&EGG_BTN_WEST)) cursor_decline(cursor);
}

void egg_client_update(double elapsed) {

  // At any time, AUX3 on any input is instant-quit.
  if (egg_input_get_one(0)&EGG_BTN_AUX3) egg_terminate(0);

  // Field manages pretty much everything, when active.
  if (g.running) {
    field_update(&g.l,elapsed);
    if (g.fieldc==2) {
      field_update(&g.r,elapsed);
    }
    if (g.l.end_ack&&g.r.end_ack) {
      g.input_blackout=0.500;
      tetris_song(RID_song_bakers_dozen);
      g.running=0;
    }
    
  // Otherwise it's the menu, and we manage it all here.
  } else if (g.input_blackout>0.0) {
    g.input_blackout-=elapsed;
  } else {
    int inputv[9]={0};
    egg_input_get_all(inputv,9);
    const int *input=inputv+1; // Skip the aggregate.
    struct cursor *cursor=g.cursorv;
    int i=8;
    for (;i-->0;input++,cursor++) {
      if (*input!=cursor->pvinput) {
        cursor_input(cursor,*input);
        cursor->pvinput=*input;
      }
    }
  }
}

void egg_client_render() {
  graf_reset(&g.graf);
  render();
  graf_flush(&g.graf);
}

void tetris_song(int rid) {
  if (rid==g.song_playing) return;
  g.song_playing=rid;
  egg_play_song(1,rid,1,0.5f,0.0f);
}

void tetris_sound(int rid) {
  egg_play_sound(rid,1.0f,0.0f);
}
