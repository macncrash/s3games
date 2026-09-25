# S3 BARON

A Western Front dogfight on the S3-16. You fly a scout from a chase view,
put the sight on whatever is coming at you, and clear five waves. The last one
is painted red.

The console code lives in `../engine` and is a fork of S3 RALLY (MIT, ©
macncrash). This cartridge is new.

```bash
brew install sdl2   # macOS, once
make
./s3baron
./s3baron --sim
./s3baron --sim --shots /tmp/baron
```

| | Keyboard | Gamepad |
|---|---|---|
| Fly | arrows | stick |
| Fire | C or Z | A / right trigger |
| Barrel roll | Space | X / stick click |
| Start, pause | Enter | Start |
| Back | Esc | Back |

Esc on the title quits. From a pause it returns to the menu.

Sortie is the five waves. Practice is endless scouts. A barrel roll is brief
invulnerability, then it has to cool down. Two-seaters that reach your altitude
and pass cost hull. Scouts that get past come around again until somebody is
shot down. Three lives, and a hull bar inside each life.

`--sim` flies the sortie with a simple pilot. Exit status is 0 when the circus
breaks.
