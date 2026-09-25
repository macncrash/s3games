# S3 ASH

Ash Year. The mountain has already burned. One terrace, one year, three seats.

This is the terrace we wrote, rebuilt on the S3-16 in this studio. It is not the living world it used to sit inside. The sky, the shelves, the people, and the herd are drawn when the cartridge boots.

Soot, Rain, and Cinder each spend one edict in ash, dry, and fire: sow a grove, bank a cistern, cut a firebreak, or pass. Rain scores. The same seed and the same edicts always end the same way. One player can sit all three seats, or the keyboard can be passed. The keepers are a search inside the cartridge. They can spend the edict in front of them, or sit the rest of the year.

Hold a camp, a herd, and something green. Ash feeds the herds. A cistern on the hearth keeps the camp through the dry season. Fire walks west from the east, and a break stops it. The wind row, `seed % 3`, spends that break.

`ash.0.s4s3b4s4k5k8k2pp` holds. Nine passes leave the camp empty.

```bash
brew install sdl2   # macOS, once
make
./s3ash
./s3ash --sim
./s3ash --sim --shots /tmp/s3ash
```

| | Keyboard | Gamepad |
|---|---|---|
| Move | arrows | stick or d-pad |
| Sow | `C` | Cross, or right trigger |
| Bank | `X` | Circle, or left trigger |
| Break | `Z` or `Space` | Square |
| Pass | `Q` | L |
| Keeper spends this edict | `W` | R |
| Keepers finish the year | `E` | Triangle |
| Start, pause | `Enter` | Start |
| Back | `Esc` | Back |

On the menu, left and right change the seed for the first three lines. The wind row changes with it.

`./s3ash --sim` prints the known lines, then lets the keepers sit seeds 0, 1, 2, and 4. A held terrace is a pass.
