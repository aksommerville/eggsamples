# Eggsamples

Little games for [Egg](https://github.com/aksommerville/egg), for demonstration.

Everything here is public domain. Feel free to eggstract any bits you might find useful.

With Egg installed, a simple `make` here will build all the games.
If that fails, confirm that `...egg/out/eggdev` exists, and check your `EGG_SDK` environment variable.

`make playables` to build everything and also copy out all the HTML, for serving out of this repo.

## The Games

### hello_world

Nothing interesting, basically just what `eggdev project` produces.
A fair starting point, but you might as well call eggdev yourself for a fresh one.

### snake

The familiar Snake game.
Implemented without Egg's client runtime helpers, not even stdlib.
So this demonstrates a bare-minimum C game.

### eggsweeper

Minesweeper clone.
Nothing special technically.

### tetris

Massively Local Multiplayer Tetris.
1..8 players can play in one field, or two fields competitively.

## TODO

- Implement some well-known styles, building out specific framework as needed:
- - [ ] Formal RPG
- - [ ] Adventure
- - [ ] Platformer
- - [ ] Rhythm
- - [ ] Visual novel
- - [ ] Fighting
- - [ ] Orthographic Racing
- Clone some familiar games, ones that aren't too involved:
- - [x] Snake
- - [ ] Tetris
- - [x] Minesweeper
- - [ ] Sudoku
- - [ ] Picross
- - [ ] Space Invaders
- Technical demos, proving out questions:
- - [x] Build without libc (snake)
- - [ ] Something in WAT instead of C.
- - [ ] Beat-em-up with Paper Mario style graphics.
