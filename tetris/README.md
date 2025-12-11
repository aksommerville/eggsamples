# Tetris

Requires [Egg v2](https://github.com/aksommerville/egg2) to build.

Songs and sound effects were not revoiced in the v2 conversion.
So they all sound kind of plain, would need some attention if we wanted to do things right.

## Differences from NES Tetris

- Different graphics and sound, obviously.
- Multiplayer.
- No "B" mode.
- Drop scoring is different, I just didn't bother figuring out the formula.
- Levels stop at 19.
- Level increases when lines reach (level*10), regardless of which you start at.
- I get more lines and higher scores here than on the NES, marginally. Not sure how to account for that.
- - I'm able to play somewhat reliably on level 19 here. That's not the case on NES, over there getting a line on 19 is usually dumb luck.
- Incoming pieces appear wherever there's room on the top, and might be rotated.
- PRNG might not be exactly the same, I haven't rigorously tested the NES's and there's a few arbitrary constants involved.

## Things to fix, if we wanted to do it right

This is just a demo that I got a little carried away with.
If we wanted to get _fully_ carried away...

- Menu is weird and hacky, needs cleaned up all around.
- Let the two fields start at different levels.
- Specific selectable rules for multifield games:
- - Terminate both at the first loss?
- - Throw penalty blocks on multi-line plays?
- - Play to a given line count?
- More songs, and let the user pick. "None" should be an option.
- Detailed stats. We're already tracking but not using line counts per multiplier. Could do count per tetromino like NES.
- Enter name for high score list.
- Options to pause and abort during play.
- Higher speed limit, I guess implement levels 20..29? I'm able to play at 19, which means it's not fast enough.
- Show current level in score display, or somewhere.
- Test multiplayer and fix the inevitable bugs. As I'm committing this, I haven't yet played with another human.
