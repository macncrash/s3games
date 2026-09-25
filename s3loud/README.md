# S3 LOUD

A yellow cab on the coast road. The meter is the enemy.

Pick someone up by stopping beside them. The arrow follows the highway. The inside of a bend is shorter than the arrow's line, and the striped gate on the crest can be jumped. Drop them where they asked before the clock dies. Only delivered fares get paid.

This is the cab from the three.js prototype, rebuilt on the S3-16 in this studio. The road, the cab, and the tires are drawn when the cartridge boots.

```bash
brew install sdl2   # macOS, once
make
./s3loud
./s3loud --sim
./s3loud --sim --shots /tmp/loud
```

| | Keyboard | Gamepad |
|---|---|---|
| Gas | `↑` or `W` | stick up, or right trigger |
| Brake | `↓` | stick down, or left trigger |
| Steer | `←` `→` | stick |
| Howl | `C` | C / right face |
| Hop | `Space` | |
| Slide | `Z` | A |
| Start, pause | `Enter` | Start |
| Back | `Esc` | Back |

Slow down next to a fare. Driving through them does not count. Howl is a burst. Hop and a fast crest clear the gate. Slide in the hairpin and the crazy meter climbs.

`--sim` drives the loop, banks a fare, and checks that the wheel lug climbs the tire instead of falling down it.
