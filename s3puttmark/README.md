# S3 PUTTMARK

Putt: a finished mark ends it. That is the whole cartridge.

Play it in the browser: https://macncrash.github.io/s3games/play/s3puttmark/

## Design

One green. The coin is the mark. Set it on the ball, then putt. The hole opens the mark, and it stays open until the coin is lifted. Lifting it finishes the mark. The rest of a card is not the job.

The green is uphill to the cup and breaks right. Four putts without a hole leaves the mark open.

```bash
make
./s3puttmark
./s3puttmark --sim
```

| | Keyboard | Gamepad |
|---|---|---|
| Move the coin, aim, walk | arrows | stick |
| Mark, putt, lift | Z or C, hold to putt | A |
| Start, pause | Enter | Start |
| Back | Esc | Back |

Esc on the title quits. From a pause it returns to the title.
