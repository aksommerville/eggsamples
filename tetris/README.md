# Tetris

Requires [Egg](https://github.com/aksommerville/egg) to build.

## TODO

- [ ] Game model.
- - Must be encapsulated such that we can run two of the side-by-side.
- - [x] Tetromino shape analysis.
- - [x] Falling pieces. Mind that there can be multiple per field.
- - [x] Line detection and removal.
- - [ ] Incoming piece: Flush against the top, and search for a valid position. Failing at that is the terminal condition.
- - [ ] Termination at the top.
- - [ ] What does it mean that we allow multiple players? Like, what happens when you drop above another falling piece?
- - [ ] Scorekeeping.
- - [ ] Rejecting rotation due to collision: Try moving horizontally to allow it.
- - [ ] Horz auto-motion, I think we need a separate initial interval.
- - [x] Match the initial orientations of NES Tetris. I got a few wrong and I think it's throwing me off while playing.
- - [ ] Fanfare and a brief delay on scoring lines.
- - [ ] Will a line-score delay be disruptive to other players in the same field? Would it be less weird to let them keep playing during the animation?
- [ ] Menus.
- [ ] Audio.

## Music

1-tcha_nutover_71a1.mid | Kunstderfuge | 3:14 | Very nice. Fast and distinctive.
2-tcha_swan_7_8.mid | Kunstderfuge | 7:24 | Great tune but too sparse early on.
3-tcha_swan_13.mid | Kunstderfuge | 16:46 | Too rambly for video game music. Plus, there's a theme here that was used in NES GB Tetris.
4-tcha_quart_11.mid | Kunstderfuge | 32:18 | Much too long, and also too rambly.
...i think i'd rather write something new
