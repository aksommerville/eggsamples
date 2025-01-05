# Tetris

Requires [Egg](https://github.com/aksommerville/egg) to build.

## TODO

- [ ] Game model.
- - [x] What does it mean that we allow multiple players? Like, what happens when you drop above another falling piece?
- - - [x] One piece dropping onto another causes an infinite loop somewhere.
- - - [x] Rotation is rejected sometimes when player pieces are near each other. ...durp nope, i just need to configure my devices.
- - - ...ok seems good. But will need to test with >2 players in a field, and with actual humans at the wheel.
- - [ ] I think termination isn't triggering.
- - [ ] Scorekeeping.
- - [ ] Horz auto-motion, I think we need a separate initial interval.
- - [ ] Fanfare and a brief delay on scoring lines.
- - [ ] Will a line-score delay be disruptive to other players in the same field? Would it be less weird to let them keep playing during the animation?
- [ ] Menus.
- [x] Audio.
