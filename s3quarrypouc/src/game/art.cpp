#include "game/art.h"

#include <initializer_list>
#include <string>

namespace quarry {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    if (i == 15) vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 1));
}

int rockTile(gs::TileAlloc& al, gs::VDP& vdp, int shade) {
    uint8_t px[64];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int v = shade;
            if ((x * 3 + y * 5) % 11 == 0) v = shade + 2;
            else if ((x + y) % 7 == 0) v = shade + 1;
            if (y == 7) v = 1;
            px[y * 8 + x] = uint8_t(v);
        }
    }
    int t = al.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

int benchTile(gs::TileAlloc& al, gs::VDP& vdp) {
    uint8_t px[64];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int v = 2;
            if (y < 2) v = 5;
            else if ((x + y) % 5 == 0) v = 3;
            else if (y > 5) v = 1;
            px[y * 8 + x] = uint8_t(v);
        }
    }
    int t = al.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

void paintQuarry(gs::VDP& vdp, gs::TileAlloc& tiles) {
    int a = rockTile(tiles, vdp, 2);
    int b = rockTile(tiles, vdp, 3);
    int bench = benchTile(tiles, vdp);
    vdp.B.resize(64, 32);
    vdp.A.resize(64, 32);
    vdp.B.clear();
    vdp.A.clear();
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            if (cy >= 4 && cy < 20) vdp.B.set(cx, cy, gs::entry((cx + cy) & 1 ? a : b, PAL_ROCK));
            if (cy >= 22 && cy <= 27) vdp.A.set(cx, cy, gs::entry(bench, PAL_BENCH));
        }
    }
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    gs::TextStyle big{2, 1, 0, 15, 1};
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

using gs::Bitmap;

Bitmap worker(int pose) {
    Bitmap b(36, 60);
    const bool duck = pose == 3;
    const bool leap = pose == 4;
    const int y0 = duck ? 22 : 0;
    b.rect(12, y0 + 2, 14, 5, 7);          // hard hat
    b.rect(11, y0 + 6, 16, 2, 6);
    b.ellipse(18, y0 + 14, 6, 6, 4);       // face
    b.set(21, y0 + 13, 1);
    b.set(22, y0 + 13, 1);
    b.rect(14, y0 + 19, 10, 14, 8);        // vest
    b.rect(12, y0 + 21, 3, 8, 5);
    b.rect(23, y0 + 21, 3, 8, 5);
    b.rect(16, y0 + 22, 6, 8, 9);          // shirt
    if (!duck) {
        int leg = leap ? 8 : (pose == 1 ? 4 : pose == 2 ? -2 : 0);
        b.rect(14, y0 + 33, 4, 16 + (leap ? -4 : 0), 3);
        b.rect(20, y0 + 33 + (leap ? 2 : 0), 4, 16 - leg, 3);
        b.rect(13, y0 + 48, 6, 3, 2);
        b.rect(19, y0 + 48 - (leap ? 6 : 0), 6, 3, 2);
        if (pose == 1) b.rect(8, y0 + 22, 5, 3, 5);
        if (pose == 2) b.rect(25, y0 + 24, 5, 3, 5);
    } else {
        b.rect(8, y0 + 24, 8, 4, 5);
        b.rect(14, y0 + 32, 12, 5, 3);
        b.rect(12, y0 + 36, 16, 3, 2);
    }
    return b;
}

Bitmap pouchBmp(int bob) {
    Bitmap b(22, 18);
    b.ellipse(11, 10 + bob, 8, 6, 4);
    b.rect(6, 4, 10, 3, 6);
    b.rect(8, 2, 6, 3, 5);
    b.rect(9, 7, 4, 5, 7);
    b.set(7, 9, 3);
    b.set(15, 9, 3);
    return b;
}

Bitmap faceBmp() {
    Bitmap b(48, 36);
    b.poly({{4, 34}, {14, 8}, {28, 2}, {44, 18}, {40, 34}}, 3);
    b.poly({{10, 34}, {18, 14}, {30, 10}, {36, 34}}, 5);
    b.rect(0, 30, 48, 6, 2);
    return b;
}

Bitmap skipBmp() {
    Bitmap b(28, 36);
    b.rect(12, 0, 4, 8, 6);
    b.poly({{4, 10}, {24, 10}, {22, 32}, {6, 32}}, 4);
    b.rect(6, 12, 16, 3, 7);
    b.rect(8, 20, 12, 8, 2);
    b.line(4, 10, 24, 10, 8, 2);
    return b;
}

Bitmap cableBmp() {
    Bitmap b(8, 8);
    b.rect(3, 0, 2, 8, 6);
    return b;
}

Bitmap beamBmp() {
    Bitmap b(64, 10);
    b.rect(0, 2, 64, 6, 5);
    b.rect(0, 2, 64, 2, 8);
    for (int x = 4; x < 64; x += 10) b.rect(x, 3, 2, 4, 2);
    return b;
}

Bitmap lipBmp() {
    Bitmap b(10, 28);
    b.rect(2, 0, 6, 28, 4);
    b.rect(0, 0, 10, 4, 7);
    return b;
}

Bitmap rampBmp() {
    Bitmap b(40, 22);
    b.poly({{0, 20}, {36, 4}, {40, 8}, {6, 22}}, 5);
    b.rect(0, 18, 40, 4, 3);
    return b;
}

Bitmap gateBmp() {
    Bitmap b(28, 64);
    b.rect(2, 8, 6, 56, 4);
    b.rect(20, 8, 6, 56, 4);
    b.rect(2, 4, 24, 8, 6);
    b.rect(8, 20, 12, 4, 8);
    b.rect(8, 36, 12, 4, 8);
    b.rect(10, 0, 8, 6, 7);
    return b;
}

Bitmap lampBmp() {
    Bitmap b(10, 48);
    b.rect(4, 10, 2, 38, 3);
    b.rect(1, 4, 8, 8, 5);
    b.rect(3, 6, 4, 4, 7);
    return b;
}

Bitmap flameBmp(int f) {
    Bitmap b(8, 10);
    b.ellipse(4, 6, 3, 4, f ? 5 : 4);
    b.ellipse(4, 5, 2, 2, 7);
    return b;
}

Bitmap shadowBmp() {
    Bitmap b(28, 6);
    b.ellipse(14, 3, 12, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    setPal(vdp, PAL_HUD, {0, rgb4(14, 13, 10), rgb4(8, 7, 6), rgb4(4, 3, 3)});
    setPal(vdp, PAL_ROCK, {0, rgb4(5, 3, 2), rgb4(8, 5, 3), rgb4(11, 7, 4), rgb4(13, 9, 5), rgb4(6, 4, 3)});
    setPal(vdp, PAL_BENCH, {0, rgb4(4, 4, 4), rgb4(7, 7, 6), rgb4(9, 8, 7), rgb4(12, 11, 9), rgb4(14, 13, 10)});
    setPal(vdp, PAL_POUCH, {0, rgb4(3, 2, 1), rgb4(6, 4, 2), rgb4(9, 6, 3), rgb4(12, 8, 4), rgb4(14, 11, 6), rgb4(8, 3, 2), rgb4(15, 13, 8)});
    setPal(vdp, PAL_PLAYER, {0, rgb4(2, 2, 2), rgb4(4, 3, 2), rgb4(6, 5, 4), rgb4(12, 8, 6), rgb4(3, 4, 6), rgb4(14, 12, 3),
                             rgb4(15, 14, 6), rgb4(9, 10, 12), rgb4(13, 6, 3)});
    setPal(vdp, PAL_IRON, {0, rgb4(2, 2, 3), rgb4(4, 4, 5), rgb4(6, 6, 7), rgb4(8, 8, 9), rgb4(10, 10, 11), rgb4(12, 12, 13),
                           rgb4(14, 10, 4), rgb4(15, 14, 12)});
    setPal(vdp, PAL_DUST, {0, rgb4(8, 6, 4), rgb4(10, 8, 5), rgb4(12, 10, 7), rgb4(14, 12, 8)});
    setPal(vdp, PAL_LAMP, {0, rgb4(6, 3, 1), rgb4(10, 5, 1), rgb4(13, 8, 2), rgb4(15, 12, 3), rgb4(15, 15, 8), rgb4(15, 10, 4),
                           rgb4(14, 14, 10)});
    setPal(vdp, PAL_ALERT, {0, rgb4(8, 2, 2), rgb4(12, 3, 2), rgb4(15, 6, 3), rgb4(15, 12, 6)});
    setPal(vdp, PAL_PIT, {0, rgb4(1, 1, 1), rgb4(2, 2, 2), rgb4(3, 2, 2), rgb4(5, 4, 3), rgb4(7, 6, 5)});
    setPal(vdp, PAL_GATE, {0, rgb4(1, 3, 2), rgb4(2, 5, 3), rgb4(3, 7, 4), rgb4(6, 10, 5), rgb4(10, 13, 7), rgb4(13, 15, 9),
                           rgb4(8, 12, 6)});
    setPal(vdp, PAL_GO, {0, rgb4(2, 6, 3), rgb4(4, 10, 5), rgb4(8, 14, 7), rgb4(12, 15, 10)});

    vdp.setFogColor(rgb4(6, 5, 4));
    gs::TileAlloc tiles(vdp);
    paintQuarry(vdp, tiles);
    loadFont(vdp, art, tiles);

    art.stand = gs::uploadMipped(vdp, worker(0));
    art.runA = gs::uploadMipped(vdp, worker(1));
    art.runB = gs::uploadMipped(vdp, worker(2));
    art.duck = gs::uploadMipped(vdp, worker(3));
    art.leap = gs::uploadMipped(vdp, worker(4));
    art.pouch[0] = gs::uploadMipped(vdp, pouchBmp(0));
    art.pouch[1] = gs::uploadMipped(vdp, pouchBmp(1));
    art.face = gs::uploadMipped(vdp, faceBmp());
    art.skip = gs::uploadMipped(vdp, skipBmp());
    art.cable = gs::uploadMipped(vdp, cableBmp());
    art.beam = gs::uploadMipped(vdp, beamBmp());
    art.lip = gs::uploadMipped(vdp, lipBmp());
    art.ramp = gs::uploadMipped(vdp, rampBmp());
    art.gate = gs::uploadMipped(vdp, gateBmp());
    art.lamp = gs::uploadMipped(vdp, lampBmp());
    art.flame[0] = gs::uploadMipped(vdp, flameBmp(0));
    art.flame[1] = gs::uploadMipped(vdp, flameBmp(1));
    art.shadow = gs::uploadMipped(vdp, shadowBmp());
}

}  // namespace quarry
