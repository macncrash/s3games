#include "game/art.h"

#include <cmath>
#include <initializer_list>

#include "console/gfx.h"

namespace mosaicgold {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void bevel(Bitmap& t) {
    for (int x = 0; x < t.w; x++) {
        t.set(x, 0, 1);
        t.set(x, t.h - 1, 2);
    }
    for (int y = 0; y < t.h; y++) {
        t.set(0, y, 1);
        t.set(t.w - 1, y, 2);
    }
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

Bitmap medallion(const Art& art) {
    Bitmap b(PIC, PIC);
    b.rect(0, 0, PIC, PIC, 3);
    b.ellipse(72, 72, 66, 66, 4);
    b.ellipse(72, 72, 58, 58, 5);
    b.ellipse(72, 72, 46, 46, 7);
    b.ellipse(72, 72, 34, 34, 6);
    b.ellipse(72, 72, 16, 16, 11);
    b.ellipse(66, 66, 5, 5, 13);
    for (int i = 0; i < 8; i++) {
        float a = i * 0.785398f;
        float x = 72 + 40 * std::cos(a);
        float y = 72 + 40 * std::sin(a);
        b.ellipse(x, y, 4, 4, 9);
    }
    for (int y = 3; y < PIC; y += 4)
        for (int x = 3; x < PIC; x += 4) b.set(x, y, 8);
    for (int i = 0; i < CELLS; i++) {
        int c = i % COLS, r = i / COLS;
        int x = c * TILE + 6;
        int y = r * TILE + 6;
        int ink = art.gold[i] ? 13 : 12;
        b.rect(x, y, 7, 7, ink);
        b.set(x + 3, y + 3, 1 + (i % 6));
        b.set(c * TILE + TILE - 4, r * TILE + 4, 10);
        b.set(c * TILE + 8 + (i % 5), r * TILE + TILE - 6, 11);
    }
    return b;
}

void slice(gs::VDP& vdp, Art& art, const Bitmap& src) {
    for (int i = 0; i < CELLS; i++) {
        int c = i % COLS, r = i / COLS;
        Bitmap tile(TILE, TILE);
        for (int y = 0; y < TILE; y++)
            for (int x = 0; x < TILE; x++) {
                int p = src.get(c * TILE + x, r * TILE + y);
                tile.set(x, y, p ? p : 3);
            }
        bevel(tile);
        art.tile[i] = gs::uploadImage(vdp, tile);
    }
    Bitmap small = src.resample(THUMB, THUMB);
    Bitmap box(THUMB + 8, THUMB + 8);
    box.rect(0, 0, box.w, box.h, 2);
    box.rect(2, 2, box.w - 4, box.h - 4, 1);
    box.blit(small, 4, 4);
    art.thumb = gs::uploadImage(vdp, box);
}

Bitmap makeFrame() {
    int s = BOARD + FRAME * 2;
    Bitmap b(s, s);
    b.rect(0, 0, s, s, 1);
    b.rect(3, 3, s - 6, s - 6, 2);
    b.rect(FRAME, FRAME, BOARD, BOARD, 0);
    return b;
}

Bitmap arrowUp() {
    Bitmap b(11, 9);
    b.rect(5, 0, 1, 9, 1);
    b.rect(1, 4, 9, 1, 1);
    b.rect(2, 3, 7, 1, 1);
    b.rect(3, 2, 5, 1, 1);
    b.set(4, 1, 1);
    b.set(6, 1, 1);
    return b;
}

Bitmap arrowDown() {
    Bitmap b(11, 9);
    b.rect(5, 0, 1, 9, 1);
    b.rect(1, 4, 9, 1, 1);
    b.rect(2, 5, 7, 1, 1);
    b.rect(3, 6, 5, 1, 1);
    b.set(4, 7, 1);
    b.set(6, 7, 1);
    return b;
}

Bitmap arrowLeft() {
    Bitmap b(9, 11);
    b.rect(0, 5, 9, 1, 1);
    b.rect(3, 2, 1, 7, 1);
    b.rect(4, 1, 1, 9, 1);
    b.set(1, 4, 1);
    b.set(1, 6, 1);
    return b;
}

Bitmap arrowRight() {
    Bitmap b(9, 11);
    b.rect(0, 5, 9, 1, 1);
    b.rect(4, 1, 1, 9, 1);
    b.rect(5, 2, 1, 7, 1);
    b.set(7, 4, 1);
    b.set(7, 6, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_CREAM, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(7, 15, 8));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 9, 11));
    setPal(vdp, PAL_MOSAIC,
           {0, gs::rgb4(15, 14, 11), gs::rgb4(4, 3, 2), gs::rgb4(12, 10, 7), gs::rgb4(9, 7, 5), gs::rgb4(14, 10, 2),
            gs::rgb4(15, 13, 4), gs::rgb4(10, 7, 1), gs::rgb4(3, 3, 3), gs::rgb4(13, 4, 3), gs::rgb4(4, 10, 11),
            gs::rgb4(15, 15, 14), gs::rgb4(14, 13, 10), gs::rgb4(15, 12, 3), gs::rgb4(6, 8, 12), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(13, 10, 6), gs::rgb4(7, 5, 3), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    loadFont(vdp, art);
    slice(vdp, art, medallion(art));
    art.frame = gs::uploadImage(vdp, makeFrame());
    art.arrow[0] = gs::uploadImage(vdp, arrowUp());
    art.arrow[1] = gs::uploadImage(vdp, arrowRight());
    art.arrow[2] = gs::uploadImage(vdp, arrowDown());
    art.arrow[3] = gs::uploadImage(vdp, arrowLeft());

    gs::TextStyle ink{1, 1, 0, 15, 1};
    art.title = gs::uploadImage(vdp, gs::textBitmap("S3 MOSAIC GOLD", {2, 1, 0, 15, 1}));
    art.sub = gs::uploadImage(vdp, gs::textBitmap("ONLY THE GOLD COUNTS DOUBLE", ink));
    art.tag = gs::uploadImage(vdp, gs::textBitmap("LEAVE WHEN THE LINE CLEARS", ink));
    art.prompt = gs::uploadImage(vdp, gs::textBitmap("PRESS START", {2, 1, 0, 15, 1}));
}

}  // namespace mosaicgold
