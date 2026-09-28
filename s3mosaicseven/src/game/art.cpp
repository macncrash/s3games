#include "game/art.h"

#include <cmath>
#include <initializer_list>

#include "console/gfx.h"

namespace mosaicseven {
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

void paintPicture(Bitmap& b, int which) {
    b.rect(0, 0, PIC, PIC, 3);
    if (which == 0) {
        b.rect(0, 62, PIC, PIC - 62, 5);
        b.ellipse(48, 28, 16, 16, 13);
        b.rect(18, 40, 60, 8, 9);
        b.rect(28, 48, 8, 28, 8);
        b.rect(60, 48, 8, 28, 8);
        b.rect(28, 70, 40, 6, 7);
        b.ellipse(48, 78, 10, 6, 11);
    } else if (which == 1) {
        b.rect(0, 0, PIC, 40, 6);
        b.rect(0, 40, PIC, PIC - 40, 5);
        b.rect(36, 18, 24, 78, 8);
        b.ellipse(48, 18, 22, 16, 4);
        b.rect(40, 50, 16, 22, 10);
        b.ellipse(20, 70, 8, 14, 9);
        b.ellipse(76, 66, 10, 16, 9);
    } else {
        b.rect(0, 58, PIC, PIC - 58, 4);
        b.ellipse(48, 58, 36, 28, 12);
        b.rect(34, 58, 28, 38, 7);
        b.rect(42, 70, 12, 26, 2);
        b.ellipse(22, 30, 6, 6, 13);
        b.ellipse(74, 24, 4, 4, 13);
        b.rect(8, 78, 14, 10, 8);
        b.rect(74, 74, 12, 14, 8);
    }
    for (int i = 0; i < CELLS; i++) {
        int c = i % COLS, r = i / COLS;
        b.set(c * TILE + 6, r * TILE + 6, 14);
        b.set(c * TILE + TILE - 7, r * TILE + TILE - 7, 11);
    }
}

void slice(gs::VDP& vdp, Art& art, int which, const Bitmap& src) {
    for (int i = 0; i < CELLS; i++) {
        int c = i % COLS, r = i / COLS;
        Bitmap tile(TILE, TILE);
        for (int y = 0; y < TILE; y++)
            for (int x = 0; x < TILE; x++) {
                int p = src.get(c * TILE + x, r * TILE + y);
                tile.set(x, y, p ? p : 3);
            }
        bevel(tile);
        art.tile[which][i] = gs::uploadImage(vdp, tile);
    }
    Bitmap small = src.resample(THUMB, THUMB);
    Bitmap box(THUMB + 8, THUMB + 8);
    box.rect(0, 0, box.w, box.h, 2);
    box.rect(2, 2, box.w - 4, box.h - 4, 1);
    box.blit(small, 4, 4);
    art.thumb[which] = gs::uploadImage(vdp, box);
}

Bitmap makeFrame() {
    int s = BOARD + FRAME * 2;
    Bitmap b(s, s);
    b.rect(0, 0, s, s, 1);
    b.rect(2, 2, s - 4, s - 4, 2);
    b.rect(FRAME, FRAME, BOARD, BOARD, 0);
    return b;
}

Bitmap pip(int fill) {
    Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, fill);
    b.ellipse(4, 4, 2, 2, 1);
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

Bitmap rot(const Bitmap& s, int times) {
    Bitmap b = s;
    for (int n = 0; n < times; n++) {
        Bitmap d(b.h, b.w);
        for (int y = 0; y < b.h; y++)
            for (int x = 0; x < b.w; x++) d.set(b.h - 1 - y, x, b.get(x, y));
        b = d;
    }
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& a) {
    setPal(vdp, PAL_MOSAIC,
           {0,
            gs::rgb4(15, 14, 10),
            gs::rgb4(4, 3, 2),
            gs::rgb4(8, 7, 5),
            gs::rgb4(12, 4, 3),
            gs::rgb4(5, 9, 4),
            gs::rgb4(4, 7, 12),
            gs::rgb4(10, 8, 5),
            gs::rgb4(7, 5, 3),
            gs::rgb4(6, 10, 6),
            gs::rgb4(13, 11, 6),
            gs::rgb4(14, 12, 8),
            gs::rgb4(11, 5, 4),
            gs::rgb4(15, 13, 4),
            gs::rgb4(15, 15, 12),
            gs::rgb4(2, 2, 2)});
    textPal(vdp, PAL_CREAM, gs::rgb4(14, 13, 10));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 14, 7));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 8, 9));
    textPal(vdp, PAL_RIVAL, gs::rgb4(14, 5, 4));
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(12, 8, 4), gs::rgb4(5, 3, 2)});

    loadFont(vdp, a);
    for (int p = 0; p < PICTURES; p++) {
        Bitmap pic(PIC, PIC);
        paintPicture(pic, p);
        slice(vdp, a, p, pic);
    }
    a.frame = gs::uploadImage(vdp, makeFrame());
    a.pip = gs::uploadImage(vdp, pip(3));
    a.pipOn = gs::uploadImage(vdp, pip(13));
    a.pipThem = gs::uploadImage(vdp, pip(12));
    Bitmap up = arrowUp();
    a.arrow[0] = gs::uploadImage(vdp, up);
    a.arrow[1] = gs::uploadImage(vdp, rot(up, 1));
    a.arrow[2] = gs::uploadImage(vdp, rot(up, 2));
    a.arrow[3] = gs::uploadImage(vdp, rot(up, 3));

    gs::TextStyle title;
    title.scale = 2;
    title.color = 1;
    title.shadow = 15;
    title.spacing = 1;
    a.title = gs::uploadImage(vdp, gs::textBitmap("MOSAIC SEVEN", title));
    gs::TextStyle sub;
    sub.color = 1;
    sub.shadow = 15;
    a.sub = gs::uploadImage(vdp, gs::textBitmap("FIRST TO SEVEN", sub));
    gs::TextStyle go;
    go.color = 1;
    go.spacing = 1;
    a.prompt = gs::uploadImage(vdp, gs::textBitmap("START", go));
}

}  // namespace mosaicseven
