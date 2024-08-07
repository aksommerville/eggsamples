#include "hero_internal.h"

/* Motion.
 */
 
static void hero_begin_motion(struct sprite *sprite,int dx,int dy) {
  if (!SPRITE->indx&&!SPRITE->indy) {
    SPRITE->animframe=0;
    SPRITE->animclock=HERO_WALK_ANIM_TIME;
  }
  if (dx) {
    SPRITE->indx=dx;
    if (dx<0) {
      SPRITE->facedir=DIR_W;
    } else {
      SPRITE->facedir=DIR_E;
    }
  } else if (dy) {
    SPRITE->indy=dy;
    if (dy<0) {
      SPRITE->facedir=DIR_N;
    } else {
      SPRITE->facedir=DIR_S;
    }
  }
}

static void hero_end_motion(struct sprite *sprite,int dx,int dy) {
  if (dx) {
    SPRITE->indx=0;
    if (SPRITE->indy<0) {
      SPRITE->facedir=DIR_N;
    } else if (SPRITE->indy>0) {
      SPRITE->facedir=DIR_S;
    }
  } else if (dy) {
    SPRITE->indy=0;
    if (SPRITE->indx<0) {
      SPRITE->facedir=DIR_W;
    } else if (SPRITE->indx>0) {
      SPRITE->facedir=DIR_E;
    }
  }
  SPRITE->animframe=0;
}

/* Update.
 */
 
void hero_update(struct sprite *sprite,double elapsed) {

  if (SPRITE->hurtclock>0.0) {
    if ((SPRITE->hurtclock-=elapsed)<=0.0) {
      SPRITE->hurtclock=0.0;
    }
  }

  if (SPRITE->pushsprite&&!SPRITE->pushsprite_again) {
    SPRITE->pushsprite=0;
  } else if (SPRITE->pushsprite) {
    SPRITE->pushsprite_time+=elapsed;
    if (SPRITE->pushsprite_time>=HERO_PUSH_ACTIVATION_TIME) {
      SPRITE->pushsprite_time-=HERO_PUSH_ACTIVATION_TIME;
      if (sprite_pushtrigger_activate(sprite_if_alive(SPRITE->pushsprite),sprite,SPRITE->facedir)) {
        SPRITE->indx=0;
        SPRITE->indy=0;
        SPRITE->animframe=0;
        SPRITE->pushing=0;
      }
    }
  }
  SPRITE->pushsprite_again=0;

  if (g.instate!=SPRITE->pvinput) {
    #define PRESS(tag) ((g.instate&INKEEP_BTNID_##tag)&&!(SPRITE->pvinput&INKEEP_BTNID_##tag))
    #define RELEASE(tag) (!(g.instate&INKEEP_BTNID_##tag)&&(SPRITE->pvinput&INKEEP_BTNID_##tag))
    if (PRESS(LEFT)) hero_begin_motion(sprite,-1,0); else if (RELEASE(LEFT)&&(SPRITE->indx<0)) hero_end_motion(sprite,-1,0);
    if (PRESS(RIGHT)) hero_begin_motion(sprite,1,0); else if (RELEASE(RIGHT)&&(SPRITE->indx>0)) hero_end_motion(sprite,1,0);
    if (PRESS(UP)) hero_begin_motion(sprite,0,-1); else if (RELEASE(UP)&&(SPRITE->indy<0)) hero_end_motion(sprite,0,-1);
    if (PRESS(DOWN)) hero_begin_motion(sprite,0,1); else if (RELEASE(DOWN)&&(SPRITE->indy>0)) hero_end_motion(sprite,0,1);
    if (PRESS(SOUTH)) hero_item_begin(sprite,g.aitem,0); else if (RELEASE(SOUTH)) hero_item_end(sprite,SPRITE->aitem);
    if (PRESS(WEST)) hero_item_begin(sprite,g.bitem,1); else if (RELEASE(WEST)) hero_item_end(sprite,SPRITE->bitem);
    #undef PRESS
    #undef RELEASE
    SPRITE->pvinput=g.instate;
  }
  
  if (SPRITE->aitem) hero_item_update(sprite,SPRITE->aitem,elapsed);
  if (SPRITE->bitem) hero_item_update(sprite,SPRITE->bitem,elapsed);

  if (SPRITE->motion_blackout>0.0) {
    SPRITE->motion_blackout-=elapsed;
  } else if ((SPRITE->indx||SPRITE->indy)&&!hero_walk_inhibited_by_item(sprite)) {
    const double speed=HERO_WALK_SPEED;
    sprite->x+=SPRITE->indx*elapsed*speed;
    sprite->y+=SPRITE->indy*elapsed*speed;
    if ((SPRITE->animclock-=elapsed)<=0.0) {
      SPRITE->animclock+=HERO_WALK_ANIM_TIME;
      if (++(SPRITE->animframe)>=4) {
        SPRITE->animframe=0;
      }
    }
  } else {
    SPRITE->animframe=1;
  }

  SPRITE->pushing=0; // until physics tells us otherwise, each frame.
}
