# Eggsamples

Little games for [Egg](https://github.com/aksommerville/egg2), for demonstration.

Everything here was made by AK Sommerville and is explicitly placed in the public domain.
Feel free to eggstract any bits you might find useful.

With Egg installed, a simple `make` here will build all the games.
If that fails, confirm that `../egg2/out/eggdev` exists, and check your `EGG_SDK` environment variable.

`make playables` to build everything and also copy out all the web builds, for serving out of this repo.
`make serve` to serve the playables locally.

## The Games

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

### rpg

Skeleton for a formal RPG a la Dragon Warrior.
Incomplete, but surely completable.

## v1 => v2 migration notes

These are the first things that I'm migrating from Egg v1 to Egg v2.
So really not sure yet exactly what the migration entails.

...after completing: Migration isn't that bad. Mostly it's a bunch of rephrasing `graf` calls.
We don't have a equivalent for the v1 `strings` unit. (rpg uses it but I stubbed that out).
That's going to bite us at Spelling Bee. Possibly others.
Leaving these notes in place until all the v1 games are migrated.

- Makefile are substantially different. Copy from any v2 boilerplate project. In v2 we normally keep the stock Makefile.
- `shared_symbols.h`: `#define EGGDEV_importUtil "res,font,graf,stdlib"`
- `egg_rom_toc.h` => `egg_res_toc.h`. duh oops.
- "include opt..." => "include util..." as needed.
- `egg_play_song(rid,force,repeat)` => `egg_play_song(songid,rid,repeat,trim,pan)`. Add a global client-side wrapper to avoid restarting.
- `egg_play_sound(rid)` => `egg_play_sound(rid,trim,pan)`.
- strings: We don't have an equivalent in v2; implement per game if needed.
- font: Changes all around.
- `egg_texture_get_status(&w,&h,texid)` => `egg_texture_get_size(&w,&h,texid)`.
- All Egg render calls: New vertex types, and a struct of uniforms.
- `graf_draw_tile_buffer`: No equivalent in the new graf; call for each tile individually.
- Eliminate `texcache`; in v2, `graf` does that job.
- Add `egg_client_notify(int,int)`.
- Revoice songs and sound effects.

## TODO

- Migrate the existing v1 eggsamples to v2, in place.
- [x] hello_world. ELIMINATED. What was this even for?
- [x] snake
- [x] eggsweeper
- [x] rpg
- [x] tetris
- [x] Then update docs and such.

- Implement some well-known styles, building out specific framework as needed:
- - [x] Formal RPG
- - [ ] Adventure
- - [ ] Platformer
- - [ ] Rhythm
- - [ ] Visual novel
- - [ ] Fighting
- - [ ] Orthographic Racing
- Clone some familiar games, ones that aren't too involved:
- - [x] Snake
- - [x] Tetris
- - [x] Minesweeper
- - [ ] Sudoku
- - - See https://github.com/aksommerville/tiny-sudoku -- I've written Sudoku before, but its generator is not perfect.
- - [ ] Picross
- - [ ] Space Invaders
- Technical demos, proving out questions:
- - [x] Build without libc (snake)
- - [ ] Something in WAT instead of C.
- - [ ] Beat-em-up with Paper Mario style graphics.
