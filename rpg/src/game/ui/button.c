#include "game/rpg.h"
#include "menu.h"

/* Delete.
 */
 
void button_del(struct button *button) {
  if (!button) return;
  if (button->texid) egg_texture_del(button->texid);
  free(button);
}

/* New.
 */
 
struct button *button_new() {
  struct button *button=calloc(1,sizeof(struct button));
  if (!button) return 0;
  button->enable=1;
  return button;
}

/* Render.
 */
 
void button_render(struct button *button) {
  if ((button->w<1)||(button->h<1)) return;
  if (!button->enable) graf_set_alpha(&g.graf,0x80);
  switch (button->rendermode) {
    case 'c': graf_fill_rect(&g.graf,button->x,button->y,button->w,button->h,button->rgba); break;
    case 't': {
        int16_t dstx,dsty;
        if (button->xalign<0) dstx=button->x+(NS_sys_tilesize>>1);
        else if (button->xalign>0) dstx=button->x+button->w-(NS_sys_tilesize>>1);
        else dstx=button->x+(button->w>>1);
        if (button->yalign<0) dsty=button->y+(NS_sys_tilesize>>1);
        else if (button->yalign>0) dsty=button->y+button->h-(NS_sys_tilesize>>1);
        else dsty=button->y+(button->h>>1);
        graf_set_image(&g.graf,button->imageid);
        graf_tile(&g.graf,dstx,dsty,button->tileid,button->xform);
      } break;
    case 'd': {
        int16_t dstx,dsty;
        if (button->xalign<0) dstx=button->x;
        else if (button->xalign>0) dstx=button->x+button->w-button->srcw;
        else dstx=button->x+(button->w>>1)-(button->srcw>>1);
        if (button->yalign<0) dsty=button->y;
        else if (button->yalign>0) dsty=button->y+button->h-button->srch;
        else dsty=button->y+(button->h>>1)-(button->srch>>1);
        int texid=button->texid?button->texid:graf_tex(&g.graf,button->imageid);
        graf_set_input(&g.graf,texid);
        graf_decal(&g.graf,dstx,dsty,button->srcx,button->srcy,button->srcw,button->srch); //TODO ,button->xform); does it get used? v2 graf doesn't transform decals
      } break;
    case 'x': button->render_cb(button,button->render_userdata); break;
  }
  if (!button->enable) graf_set_alpha(&g.graf,0xff);
}

/* Setup rendermode=='c'
 */
 
void button_setup_color(struct button *button,int w,int h,uint32_t rgba) {
  if (!button) return;
  button->rendermode='c';
  button->w=w;
  button->h=h;
  button->rgba=rgba;
}

/* Setup rendermode=='t'
 */
 
void button_setup_tile(struct button *button,int imageid,uint8_t tileid,uint8_t xform) {
  if (!button) return;
  button->rendermode='t';
  button->imageid=imageid;
  button->tileid=tileid;
  button->xform=xform;
  button->w=NS_sys_tilesize;
  button->h=NS_sys_tilesize;
}

/* Setup rendermode=='d'
 */
 
void button_setup_decal(struct button *button,int imageid,int16_t srcx,int16_t srcy,int16_t w,int16_t h,uint8_t xform) {
  if (!button) return;
  button->rendermode='d';
  button->imageid=imageid;
  button->srcx=srcx;
  button->srcy=srcy;
  button->w=button->srcw=w;
  button->h=button->srch=h;
  button->xform=xform;
  if (button->texid) {
    egg_texture_del(button->texid);
    button->texid=0;
  }
}

/* Setup rendermode=='d' from text.
 */
 
void button_setup_text(struct button *button,const char *src,int srcc,uint32_t rgba) {
  if (!button) return;
  int texid=font_render_to_texture(0,g.font,src,srcc,FBW,font_get_line_height(g.font),rgba);
  if (texid<1) return;
  button->rendermode='d';
  button->imageid=0;
  button->texid=texid;
  button->srcx=0;
  button->srcy=0;
  egg_texture_get_size(&button->w,&button->h,texid);
  button->srcw=button->w;
  button->srch=button->h;
  button->xform=0;
}

/* Setup rendermode=='d' from string resource.
 */
 
void button_setup_string(struct button *button,int rid,int index,uint32_t rgba) {
  if (!button) return;
  const char *src=0;
  int srcc=strings_get(&src,rid,index);
  int texid=font_render_to_texture(0,g.font,src,srcc,FBW,font_get_line_height(g.font),rgba);
  if (texid<1) return;
  button->rendermode='d';
  button->imageid=0;
  button->texid=texid;
  button->srcx=0;
  button->srcy=0;
  egg_texture_get_size(&button->w,&button->h,texid);
  button->srcw=button->w;
  button->srch=button->h;
  button->xform=0;
}

/* Setup rendermode=='x'
 */
 
void button_setup_custom(struct button *button,void (*render)(struct button *button,void *userdata),void *userdata) {
  if (!button||!render) return;
  button->rendermode='x';
  button->render_cb=render;
  button->render_userdata=userdata;
}
