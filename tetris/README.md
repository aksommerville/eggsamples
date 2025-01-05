# Tetris

Requires [Egg](https://github.com/aksommerville/egg) to build.

## Differences from NES Tetris

- Different graphics and sound, obviously.
- Multiplayer.
- No "B" mode.
- Drop scoring is different, I just didn't bother figuring out the formula.
- Levels stop at 19.
- Level increases when lines reach (level*10), regardless of which you start at.

## TODO

- [x] Game model.
- - [x] What does it mean that we allow multiple players? Like, what happens when you drop above another falling piece?
- - - [x] One piece dropping onto another causes an infinite loop somewhere.
- - - [x] Rotation is rejected sometimes when player pieces are near each other. ...durp nope, i just need to configure my devices.
- - - ...ok seems good. But will need to test with >2 players in a field, and with actual humans at the wheel.
- - [x] I think termination isn't triggering.
- - [x] Per-player colors in a multiplayer field, instead of per-shape.
- - [x] Scorekeeping.
- - [x] Horz auto-motion, I think we need a separate initial interval. ...nah it's fine
- - [x] Fanfare and a brief delay on scoring lines.
- - [x] Will a line-score delay be disruptive to other players in the same field? Would it be less weird to let them keep playing during the animation?
- - - Play proceeds during the 375-ms removal animation. No implementation I've seen before does this, but I think it's appropriate owing to multiplayer.
- - [x] Should we do the drop bonus like NES Tetris? It doesn't add much, and it's a little complicated. ...no it's not, and yes.
- - [x] Levels. Speed up and increase scores.
- [x] Made an effort to match NES Tetris, but we still feel fast. What gives?
- - ...ran side-by-side and nope, the drop timing is spot-on.
- - [x] Horz timing is probably way off; I haven't assessed it carefully.
- - - Do the same thing I did for vert timing: Automated framebuffer monitor in FCEU. Identify pieces leaving the left column and arriving at the right.
- - - 52 frames for 8 steps, including the longer initial time. Initial feels about double subsequent.
- - - 10 then 6 adds up exactly.
- [ ] Fanfare on level change.
- [ ] Persist high score.
- [ ] Menus.
- [x] Audio.

## NES Tetris Timing and Scoring

No doubt I could look these up somewhere, but where's the fun in that?

"Speed" is the count of beats between a 2-row piece appearing and hitting the floor.
The three songs have identical tempo.

I'm not able to play beyond level 19. Done it once or twice, but definitely not reliably :(
We'll have to infer the pattern for levels beyond that.

NES Tetris includes an initial delay, which we don't. Roughly 2 beats.

| Level | Speed | Single | Double | Triple | Tetris |
|-------|-------|--------|--------|--------|--------|
|     0 | 38    |     40 |    100 |    300 |   1200 |
|     1 | 35    |     80 |    200 |    600 |   2400 |
|     2 | 32    |    120 |    300 |    900 |   3600 |
|     3 | 27    |    160 |    400 |   1200 |   4800 |
|     4 | 23    |
|     5 | 19    |
|     6 | 15    |
|     7 | 11    |
|     8 |  8    |
|     9 |  6    |
|    10 |  5    |    440 |   1100 |   3300 |  13200 |
|    11 | 
|    12 | 
|    13 | 
|    14 | 
|    15 | 
|    16 | 
|    17 | 
|    18 | 
|    19 |  2    |

This is enough to infer the scoring:
- Single = (n+1)*40
- Double = (n+1)*100
- Triple = (n+1)*300
- Tetris = (n+1)*1200

Counting beats does not seem adequate to assess timing.
Modify FCEU to include a stopwatch I can tap.
...even better, have it read the framebuffer and count frames between top rows going full black.
...these numbers report in lockstep, awesome.
Ensure that you're dropping height-two pieces and hitting the floor, and don't rotate into the top rows.
So the pieces are travelling 19 rows. (they hold at the bottom).

| Level | Frames | F/row |
|-------|--------|-------|
|     0 | 922    |    48 |
|     1 | 827    |    43 |
|     2 | 732    |    38 |
|     3 | 637    |    33 |
|     4 | 542    |    28 |
|     5 | 447    |    23 |
|     6 | 352    |    18 |
|     7 | 257    |    13 |
|     8 | 162    |     8 |
|     9 | 124    |     6 |
|    10 | 105    |     5 |
|    11 | 105    |     5 |
|    12 | 105    |     5 |
|    13 |  86    |     4 |
|    14 |  86    |     4 |
|    15 |  86    |     4 |
|    16 |  67    |     3 |
|    17 |  67    |     3 |
|    18 |  67    |     3 |
|    19 |  48    |     2 |

(10,11,12), (13,14,15), (16,17,18) have the same timing. I'd never noticed that before!

These times include the arrival delay.
So I expect them to work out to some integer (a*19)+b.
The counts above are all exactly 10 greater than a multiple of 19.
So, the initial delay is 10 frames.

I would infer that levels 20 and 21 use 2 F/row, and 22 and above 1 F/row.

Assume 16666 us/frame, and the numbers above give us the final timing.

## Scores, without line-score delay

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
