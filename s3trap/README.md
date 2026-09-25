# S3 TRAP

Land the Hook. The first site is a carrier at dusk. Pitch holds angle of
attack: the green chevron means you are slow, the amber donut means you are on
speed, the red triangle means you are fast. Power holds the ball. High ball,
take a little off. Low ball, add power. The wires are numbered from the stern.
The 3-wire, on speed, in the middle, with a sane sink rate, is an `_OK_`.

Then the same jet has to get down somewhere worse.

| Site | What it asks |
|---|---|
| THE BOAT | Dusk carrier. Four wires. 3-wire is the aimpoint. |
| NIGHT BOAT | Same deck, pitching, with a crosswind. |
| CANYON LEDGE | Short dirt. No wire. Stop before the edge. |
| ROOFTOP | One net on a tower. |
| ICE SHELF | Long, and the brakes barely work. |
| FLATCAR | The pad is moving. One net. |
| CALDERA | Ash, updrafts, stop on the rim. |
| HIGHWAY | Two lanes. The overpass is the end of the runway. |
| THE RIG | A small pad on a leg that sways. |

Speedbrake (Space, or the left trigger) is how you stop when there is no wire.
On the boat, leave it alone and fly the ball into the deck. There is no flare.

The console code lives in `../engine` and is a fork of S3 RALLY (MIT, ©
macncrash). The jet, the boat, and the sites are new. Tour progress is a small
file in this title's own save folder. A pass unlocks the next site. Practice
can replay anything already unlocked.

```bash
brew install sdl2   # macOS, once
make
./s3trap
./s3trap --sim
./s3trap --sim --shots /tmp/trap
```

| | Keyboard | Gamepad |
|---|---|---|
| Pitch and roll | arrows (up noses up) | stick |
| Power up | C | right trigger, or A |
| Power down | X | left trigger, or B |
| Speedbrake | Space | while the left trigger is held, or X on the pad |
| Start, pause | Enter | Start |
| Back | Esc | Back |

On a pad with analog triggers, the right trigger is the throttle. The left
trigger is the speedbrake.

`--sim` flies every site. The run passes when the boat comes back `_OK_`, `OK`,
or `FAIR`. The autopilot's 3-wire is the check that the glideslope, the donut,
and the wires agree.
