# S3

The front of this console. One menu for the cartridges in this studio: pick a game, set the machine, read the totals, read what it is.

```bash
brew install sdl2   # macOS, once
make
./s3
./s3 --sim
./reel.sh --out /tmp/s3.mp4 --count 3 --seconds 8
```

`./s3` is the menu. A cartridge boots in its own window. Leave that game (Esc from its title, or close the window) and the menu is back.

The picture is still 320×224. The board under this menu is `~/dev/csys/s3rally`. A cartridge boots in its own window.

## Pages

Play is a cabinet. Each cartridge is a marquee, and the one in front is the one that boots. Arrows move through them. On the other pages, left and right change the page. Q and W change the page from anywhere except the name row. Enter or C starts the cartridge in front. E opens the MIT license. Esc steps back, and quits from Play.

| | Keyboard | Pad |
|---|---|---|
| Pick a game | arrows | d-pad or stick |
| Page | Q, W | shoulders |
| Play | Enter or C | Start or Cross |
| License | E | |
| Back | Esc | Back |

The bottom line is `(C) 2026 MACNCRASH`. The MIT page is the license: free to use, copy, and modify, and there is no warranty.

Setup is the whole machine, not one game.

- **Master**, 0 to 10. 10 leaves a cartridge at its own mix. Lower takes the level down in every cartridge, including ones you start without the menu.
- **Music**, 0 to 10. The tune on this screen. 0 is off. A game's own score stays, because the games mix music and effects on the same chips.
- **Scanlines**. F1 does the same thing for the screen you are on. The switch here is the one that sticks.
- **Full screen**. F11 still toggles the screen you are on.
- **Name**. Type it while that row is lit. Left, or Backspace, deletes. Twelve letters.

F12 saves a shot, same as the other cartridges.

Stats are sessions this menu has launched: how long, how often, and where the slate stands. About is the short version of this page.

Settings and the session log live in `~/Library/Application Support/s3/console/` (`console.cfg`, `sessions.log`).

## Reel

`reel.sh` needs ffmpeg. It films the menu, picks cartridges at random, builds each one, and lets its autopilot play. Picture and sound are taken from the console itself, so it does not need screen recording.

```bash
./reel.sh
./reel.sh --out ~/Desktop/s3.mp4 --count 4 --seconds 10
./reel.sh --only s3table --seconds 6 --seed 1
```

`--count` is at most 8. `--seconds` is 2 to 30. The mp4 is 960×672, nearest-neighbor, with the sound in the file.
