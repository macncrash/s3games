# s3games

S3-16 cartridges. Each one builds against the console in [s3rally](https://github.com/macncrash/s3rally).

```bash
git clone https://github.com/macncrash/s3rally.git
git clone https://github.com/macncrash/s3games.git
cd s3games/s3pins
S3_ENGINE="$PWD/../../s3rally/src" make
./s3pins
```

`S3_ENGINE` defaults to a `csys/s3rally` checkout two directories up. Point it at `s3rally/src` if your folders sit somewhere else.

## Play in the browser

The index is [web/index.html](web/index.html). On GitHub Pages that page is [macncrash.github.io/s3games](https://macncrash.github.io/s3games/).

Each game has its own page. The page says what the game is and why it is shaped that way, and it links straight at that game's browser build. For pins, the page is [s3pins](s3pins/README.md) and the build is [play/s3pins](https://macncrash.github.io/s3games/play/s3pins/).

The build is one file: script, wasm, and shell packed together. Build one with Emscripten:

```bash
./tools/make-web-asset.sh s3pins
```

`./tools/make-web-asset.sh --all` packs every cartridge and rewrites the index. A push to `main` runs that pack and publishes the pages site.

## A few of them

![s3pins](web/shots/s3pins.png)
![s3baron](web/shots/s3baron.png)
![s3harbor](web/shots/s3harbor.png)
![s3bike](web/shots/s3bike.png)
![s3market](web/shots/s3market.png)
