/* textbox.h
 * Menu of options laid out in a grid, where each option is a text label.
 * Most of the battle UI will use this, one for each outlined zone.
 */
 
#ifndef TEXTBOX_H
#define TEXTBOX_H

#define TEXTBOX_SCROLL_TIME 0.250
#define TEXTBOX_CURSOR_W 8

/* Don't touch a struct textbox directly.
 * Accessor functions are provided for everything you're allowed to modify.
 */
struct textbox {
  int16_t dstx,dsty,dstw,dsth;
  int colc,rowc;
  int colw,rowh;
  int selp; // LRTB; divide by (colc).
  int scrollx,scrolly; // By column and row. Updates immediately, and animation catches up.
  uint8_t scrolldir;
  double scrollclock;
  int focus; // (0,1,2)=(none,prompt,option)
  int clock; // Based on render frames. We don't take a continuous update.
  
  // (optionv) is indexed by (selp), so normally (optionc==colc*rowc).
  // If it's short, the remaining cells are empty and disabled.
  struct tb_option {
    int enable;
    char *text;
    int textc;
    int texid;
    int texw,texh;
    uint8_t align; // Zero or one of (DIR_W,DIR_E), and zero or one of (DIR_N,DIR_S).
    int comment;
  } *optionv;
  int optionc,optiona;
};

void textbox_del(struct textbox *textbox);

/* (dstx,dsty,dstw,dsth) is the total space we may touch, including the border.
 * We immediately expand right and down to the smallest legal size.
 * (colc,rowc) are the size of the logical content. We scroll as needed.
 */
struct textbox *textbox_new(
  int dstx,int dsty,int dstw,int dsth,
  int colc,int rowc
);

/* Fails if (col,row) OOB.
 * All cells are initially disabled.
 * Preferred alignment is DIR_W. DIR_E and zero are also sensible.
 * DIR_N and DIR_S are available but I don't expect to use, since row heights should be constant and governed by the font.
 */
int textbox_set_option(struct textbox *textbox,int col,int row,const char *text,int textc,uint8_t align,int enable);
int textbox_set_option_string(struct textbox *textbox,int col,int row,int rid,int ix,uint8_t align,int enable);

// You can attach an arbitrary comment to any cell with an option present.
void textbox_set_comment(struct textbox *textbox,int col,int row,int comment);
int textbox_get_comment(const struct textbox *textbox,int col,int row);

int textbox_find_option_by_comment(int *col,int *row,const struct textbox *textbox,int comment);

void textbox_clear(struct textbox *textbox);

/* Establish cell size based on current labels.
 * You must call this after all initial textbox_set_option() are done.
 * It's wise to call after modifying labels too, in case columns need to get wider.
 */
void textbox_pack(struct textbox *textbox);

/* A focused textbox shows a cursor if at least one option is enabled.
 * No options enabled, we show a blinking caret at the bottom.
 */
void textbox_focus(struct textbox *textbox,int focus);

void textbox_move(struct textbox *textbox,int dx,int dy);

void textbox_get_selection(int *x,int *y,const struct textbox *textbox);

void textbox_render(struct textbox *textbox);

/* Backdrop and border for a textbox are available for others to use.
 * (x,y,w,h) are the outer bounds of the box, including a 2-pixel border.
 * Try not to touch the border.
 * "ack" is a blinking caret at the lower right of the border.
 */
void textbox_render_backdrop(int16_t x,int16_t y,int16_t w,int16_t h);
void textbox_render_border(int16_t x,int16_t y,int16_t w,int16_t h);
void textbox_render_ack(int16_t x,int16_t y,int16_t w,int16_t h);
void textbox_render_more(int16_t x,int16_t y,int16_t w,int16_t h,uint8_t mask); // Scroll indicators. (DIR_N|DIR_S|DIR_W|DIR_E)

#endif
