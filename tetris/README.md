# Tetris

Requires [Egg](https://github.com/aksommerville/egg) to build.

## Differences from NES Tetris

- Different graphics and sound, obviously.
- Multiplayer.
- No "B" mode.
- Drop scoring is different, I just didn't bother figuring out the formula.
- Levels stop at 19.
- Level increases when lines reach (level*10), regardless of which you start at.
- I get more lines and higher scores here than on the NES, marginally. Not sure how to account for that.
- Incoming pieces appear wherever there's room on the top, and might be rotated.

## TODO

- [ ] Menus.
- [x] Observed a game-over when the next tetromino was O and there were four cells available at the far right. Should have continued there.
- - random seed 89079556 exposes it without input. In fact, in that case, the failing tetromino is a S, and there's 3x2 available, shouldn't even need to transform.
- - ...was discarding possibilities that touch the right edge due to an off-by-one error in choose_position_for_tiles().

## Scores

It really does feel like NES Tetris now.
The missing line-score delay becomes apparent to me around level 10 or 13.
Play seriously a few times, let's see if I score in my usual range.
A typical round for me on the NES is 170-180 lines and 250-300k points.
With an all-time high around 477k -- If I beat that here, something is broken.

161 : 281124
151 : 239272
167 : 196265

^ duh oops i'd been starting at level zero

 33 :  49442
204 : 315772
161 : 359158
117 : 224143
191 : 323703

It's definitely not identical. But close enough that I can live with it.
Added line removal delay...

207 : 391526
139 : 194157
200 : 416274
165 : 302561
