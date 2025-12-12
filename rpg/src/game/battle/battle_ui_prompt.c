#include "battle_internal.h"

#define MARGIN_LEFT 4
#define MARGIN_RIGHT 4
#define MARGIN_TOP 4
#define MARGIN_BOTTOM 3

/* Object.
 */
 
struct prompt {
  int x,y,w,h;
  struct battle *battle;
  int awaiting_ack;
  int texid,texw,texh;
  int state;
};

/* Delete.
 */
 
void prompt_del(struct prompt *prompt) {
  if (!prompt) return;
  egg_texture_del(prompt->texid);
  free(prompt);
}

/* New.
 */
 
struct prompt *prompt_new(struct battle *battle) {
  struct prompt *prompt=calloc(1,sizeof(struct prompt));
  if (!prompt) return 0;
  prompt->battle=battle;
  return prompt;
}

/* Set bounds.
 */
 
void prompt_set_bounds(struct prompt *prompt,int x,int y,int w,int h) {
  prompt->x=x;
  prompt->y=y;
  prompt->w=w;
  prompt->h=h;
}

/* Acknowledgement.
 */
 
int prompt_awaiting_ack(const struct prompt *prompt) {
  return prompt->awaiting_ack;
}

void prompt_ack(struct prompt *prompt) {
  prompt->awaiting_ack=0;
  prompt->state=PROMPT_STATE_NONE;
}

/* State.
 */
 
void prompt_set_state(struct prompt *prompt,int state) {
  prompt->state=state;
}

int prompt_get_state(const struct prompt *prompt) {
  return prompt->state;
}

/* Render.
 */
 
void prompt_render(struct prompt *prompt) {
  if (prompt->awaiting_ack) {
    int16_t dstx=prompt->x+MARGIN_LEFT;
    int16_t dsty=prompt->y+MARGIN_TOP;
    // To be strictly polite, we should clip against our bounds. I'm going to assume it always fits.
    graf_set_input(&g.graf,prompt->texid);
    graf_decal(&g.graf,dstx,dsty,0,0,prompt->texw,prompt->texh);
  }
  textbox_render_border(prompt->x,prompt->y,prompt->w,prompt->h);
  if (prompt->awaiting_ack) {
    textbox_render_ack(prompt->x,prompt->y,prompt->w,prompt->h);
  }
}

/* Measurement assistance.
 */
 
int prompt_vertical_margins() {
  return MARGIN_TOP+MARGIN_BOTTOM;
}

/* Set message and require acknowledgement.
 */
 
void prompt_set_text(struct prompt *prompt,const char *src,int srcc) {
  egg_texture_del(prompt->texid);
  prompt->texid=font_render_to_texture(0,g.font,src,srcc,prompt->w-MARGIN_RIGHT-MARGIN_LEFT,font_get_line_height(g.font),0xffffffff);
  egg_texture_get_size(&prompt->texw,&prompt->texh,prompt->texid);
  prompt->awaiting_ack=1;
}

void prompt_set_res(struct prompt *prompt,int rid,int ix) {
  const char *src=0;
  int srcc=text_get_string(&src,rid,ix);
  prompt_set_text(prompt,src,srcc);
}

void prompt_set_res_iii(struct prompt *prompt,int rid,int ix,int a,int b,int c) {
  struct text_insertion insv[]={
    {'i',.i=a},
    {'i',.i=b},
    {'i',.i=c},
  };
  char tmp[64];
  int tmpc=text_format_res(tmp,sizeof(tmp),rid,ix,insv,sizeof(insv)/sizeof(insv[0]));
  if (tmpc<0) tmpc=0; else if (tmpc>sizeof(tmp)) tmpc=sizeof(tmp);
  prompt_set_text(prompt,tmp,tmpc);
}

void prompt_set_res_sss(struct prompt *prompt,int rid,int ix,const char *a,int ac,const char *b,int bc,const char *c,int cc) {
  struct text_insertion insv[]={
    {'s',.s={a,ac}},
    {'s',.s={b,bc}},
    {'s',.s={c,cc}},
  };
  char tmp[64];
  int tmpc=text_format_res(tmp,sizeof(tmp),rid,ix,insv,sizeof(insv)/sizeof(insv[0]));
  if (tmpc<0) tmpc=0; else if (tmpc>sizeof(tmp)) tmpc=sizeof(tmp);
  /**/
  prompt_set_text(prompt,tmp,tmpc);
}

void prompt_set_res_sis(struct prompt *prompt,int rid,int ix,const char *a,int ac,int b,const char *c,int cc) {
  struct text_insertion insv[]={
    {'s',.s={a,ac}},
    {'i',.i=b},
    {'s',.s={c,cc}},
  };
  char tmp[64];
  int tmpc=text_format_res(tmp,sizeof(tmp),rid,ix,insv,sizeof(insv)/sizeof(insv[0]));
  if (tmpc<0) tmpc=0; else if (tmpc>sizeof(tmp)) tmpc=sizeof(tmp);
  prompt_set_text(prompt,tmp,tmpc);
}
