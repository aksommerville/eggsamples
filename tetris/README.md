# Tetris

Requires [Egg](https://github.com/aksommerville/egg) to build.

## TODO

- [ ] Game model.
- - Must be encapsulated such that we can run two of the side-by-side.
- - [x] Tetromino shape analysis.
- - [x] Falling pieces. Mind that there can be multiple per field.
- - [x] Line detection and removal.
- - [x] Incoming piece: Flush against the top, and search for a valid position. Failing at that is the terminal condition.
- - [x] Termination at the top.
- - [ ] What does it mean that we allow multiple players? Like, what happens when you drop above another falling piece?
- - [ ] Scorekeeping.
- - [x] Rejecting rotation due to collision: Try moving horizontally to allow it.
- - [ ] Horz auto-motion, I think we need a separate initial interval.
- - [x] Match the initial orientations of NES Tetris. I got a few wrong and I think it's throwing me off while playing.
- - [ ] Fanfare and a brief delay on scoring lines.
- - [ ] Will a line-score delay be disruptive to other players in the same field? Would it be less weird to let them keep playing during the animation?
- [ ] Menus.
- [ ] Audio.
- [x] !!! Make a 2-column tunnel on the edge, drop in a J and try to rotate it. The nub disappears offscreen, it becomes just 3 tiles. And it hit some infinite loop last time.

