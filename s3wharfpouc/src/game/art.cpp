#include "game/art.h"

#include <initializer_list>
#include <string>

namespace wharf {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    if (i == 15) vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

int plankTile(gs::TileAlloc& al, gs::VDP& vdp) {
    uint8_t px[64];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int v = 2;
            if (y == 0 || y == 7) v = 5;
            else if (x == 3) v = 4;
            else if ((x + y) % 6 == 0) v = 3;
            px[y * 8 + x] = uint8_t(v);
        }
    }
    int t = al.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

int skyTile(gs::TileAlloc& al, gs::VDP& vdp, int shade) {
    uint8_t px[64];
    for (int i = 0; i < 64; i++) px[i] = uint8_t(shade);
    int t = al.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

void paintWharf(gs::VDP& vdp, gs::TileAlloc& tiles) {
    int hi = skyTile(tiles, vdp, 1);
    int mid = skyTile(tiles, vdp, 2);
    int lo = skyTile(tiles, vdp, 3);
    int plank = plankTile(tiles, vdp);
    vdp.B.resize(64, 32);
    vdp.A.resize(64, 32);
    vdp.B.clear();
    vdp.A.clear();
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int sky = cy < 6 ? hi : (cy < 14 ? mid : lo);
            vdp.B.set(cx, cy, gs::entry(sky, PAL_SKY));
            if (cy >= 20 && cy <= 22) vdp.A.set(cx, cy, gs::entry(plank, PAL_PLANK));
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

Bitmap docker(int pose) {
    Bitmap b(36, 60);
    const bool duck = pose == 3;
    const bool leap = pose == 4;
    const int y0 = duck ? 18 : 0;
    b.rect(11, y0 + 1, 14, 4, 8);
    b.rect(13, y0 + 4, 10, 2, 8);
    b.ellipse(18, y0 + 13, 6, 6, 4);
    b.set(21, y0 + 12, 1);
    b.set(22, y0 + 12, 1);
    b.rect(15, y0 + 16, 5, 2, 9);
    b.rect(8, y0 + 20, 20, 16, 2);
    b.rect(12, y0 + 22, 8, 8, 6);
    b.rect(22, y0 + 24, 5, 8, 7);
    if (pose == 1) b.rect(26, y0 + 22, 6, 8, 2);
    else if (pose == 2) b.rect(4, y0 + 22, 6, 8, 2);
    else b.rect(6, y0 + 22, 4, 10, 2);
    if (leap) {
        b.rect(9, y0 + 36, 7, 8, 5);
        b.rect(20, y0 + 36, 7, 8, 5);
        b.rect(8, y0 + 42, 8, 3, 7);
        b.rect(20, y0 + 42, 8, 3, 7);
    } else if (duck) {
        b.rect(8, y0 + 34, 20, 6, 5);
        b.rect(8, y0 + 38, 8, 3, 7);
        b.rect(20, y0 + 38, 8, 3, 7);
    } else if (pose == 1) {
        b.rect(11, y0 + 36, 5, 16, 5);
        b.rect(20, y0 + 38, 5, 12, 5);
        b.rect(10, y0 + 50, 7, 4, 7);
        b.rect(19, y0 + 48, 7, 4, 7);
    } else if (pose == 2) {
        b.rect(20, y0 + 36, 5, 16, 5);
        b.rect(11, y0 + 38, 5, 12, 5);
        b.rect(19, y0 + 50, 7, 4, 7);
        b.rect(10, y0 + 48, 7, 4, 7);
    } else {
        b.rect(12, y0 + 36, 5, 16, 5);
        b.rect(19, y0 + 36, 5, 16, 5);
        b.rect(11, y0 + 50, 7, 4, 7);
        b.rect(18, y0 + 50, 7, 4, 7);
    }
    return b;
}

Bitmap satchel(int bob) {
    Bitmap b(22, 18);
    b.rect(3, 4 + bob, 16, 12, 3);
    b.rect(5, 6 + bob, 12, 8, 2);
    b.rect(9, 1, 4, 5, 6);
    b.rect(7, 8 + bob, 8, 2, 8);
    b.set(8, 9 + bob, 9);
    return b;
}

Bitmap cask() {
    Bitmap b(28, 26);
    b.ellipse(14, 14, 12, 11, 3);
    b.ellipse(14, 14, 8, 9, 2);
    b.rect(4, 6, 20, 2, 6);
    b.rect(4, 18, 20, 2, 6);
    b.rect(12, 4, 3, 18, 5);
    return b;
}

Bitmap hook() {
    Bitmap b(18, 28);
    b.rect(8, 0, 3, 16, 2);
    b.rect(8, 14, 8, 3, 3);
    b.rect(13, 14, 3, 8, 3);
    b.rect(8, 20, 8, 3, 4);
    return b;
}

Bitmap mast() {
    Bitmap b(16, 80);
    b.rect(6, 0, 4, 78, 2);
    b.rect(2, 8, 12, 3, 3);
    b.rect(4, 40, 8, 3, 5);
    return b;
}

Bitmap shed() {
    Bitmap b(48, 64);
    b.poly({{2, 18}, {24, 2}, {46, 18}}, 4);
    b.rect(4, 18, 40, 44, 2);
    b.rect(18, 36, 14, 26, 1);
    b.rect(8, 26, 8, 8, 6);
    b.rect(32, 26, 8, 8, 6);
    b.rect(20, 40, 10, 16, 8);
    return b;
}

Bitmap piling() {
    Bitmap b(14, 48);
    b.rect(3, 0, 8, 46, 3);
    b.rect(2, 0, 10, 4, 5);
    b.rect(4, 16, 6, 2, 2);
    b.rect(4, 32, 6, 2, 2);
    return b;
}

Bitmap gull(int flap) {
    Bitmap b(28, 12);
    int y = flap ? 2 : 5;
    b.line(2, y + 4, 12, 6, 1, 2);
    b.line(26, y + 4, 16, 6, 1, 2);
    b.ellipse(14, 7, 3, 2, 2);
    return b;
}

Bitmap shadowBlob() {
    Bitmap b(28, 8);
    b.ellipse(14, 4, 12, 3, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    vdp.reset();
    setPal(vdp, PAL_HUD, {0, rgb4(15, 15, 14), rgb4(8, 8, 7), rgb4(3, 3, 3)});
    setPal(vdp, PAL_SKY, {0, rgb4(6, 9, 14), rgb4(4, 7, 12), rgb4(3, 5, 9)});
    setPal(vdp, PAL_PLANK, {0, rgb4(10, 8, 4), rgb4(8, 6, 3), rgb4(12, 10, 6), rgb4(6, 5, 3), rgb4(4, 3, 2)});
    setPal(vdp, PAL_POUCH, {0, rgb4(6, 3, 1), rgb4(10, 6, 2), rgb4(13, 9, 3), rgb4(4, 2, 1), rgb4(8, 7, 4), rgb4(14, 12, 6), rgb4(2, 1, 0)});
    setPal(vdp, PAL_PLAYER, {0, rgb4(2, 2, 3), rgb4(2, 5, 9), rgb4(4, 8, 13), rgb4(13, 9, 6), rgb4(4, 3, 2),
                             rgb4(12, 10, 3), rgb4(1, 1, 2), rgb4(15, 14, 10), rgb4(9, 3, 3)});
    setPal(vdp, PAL_IRON, {0, rgb4(5, 6, 7), rgb4(8, 9, 10), rgb4(11, 12, 12), rgb4(14, 14, 13), rgb4(3, 3, 4)});
    setPal(vdp, PAL_WOOD, {0, rgb4(5, 3, 1), rgb4(8, 5, 2), rgb4(11, 7, 3), rgb4(7, 4, 2), rgb4(14, 11, 5), rgb4(3, 2, 1)});
    setPal(vdp, PAL_LAMP, {0, rgb4(15, 14, 6), rgb4(14, 9, 2), rgb4(10, 5, 1)});
    setPal(vdp, PAL_ALERT, {0, rgb4(15, 4, 3), rgb4(12, 8, 2), rgb4(8, 2, 2)});
    setPal(vdp, PAL_WATER, {0, rgb4(1, 3, 6), rgb4(2, 5, 9), rgb4(8, 12, 14)});
    setPal(vdp, PAL_SHED, {0, rgb4(3, 3, 4), rgb4(8, 7, 6), rgb4(5, 4, 4), rgb4(11, 4, 3), rgb4(6, 8, 9), rgb4(14, 12, 8)});
    setPal(vdp, PAL_GO, {0, rgb4(6, 14, 7), rgb4(3, 8, 4), rgb4(12, 15, 10)});
    setPal(vdp, PAL_ROAD, {0, rgb4(1, 2, 4), rgb4(2, 4, 7), rgb4(3, 6, 9), rgb4(4, 7, 10), rgb4(2, 3, 5), rgb4(5, 8, 11),
                           rgb4(1, 3, 6), rgb4(6, 9, 12), rgb4(8, 11, 13), rgb4(1, 4, 8), rgb4(2, 6, 11), rgb4(10, 14, 15),
                           rgb4(3, 5, 8), rgb4(4, 8, 12), rgb4(7, 10, 12)});
    vdp.setFogColor(rgb4(4, 6, 9));

    gs::TileAlloc tiles(vdp, 1);
    paintWharf(vdp, tiles);
    loadFont(vdp, art, tiles);

    art.stand = gs::uploadMipped(vdp, docker(0));
    art.runA = gs::uploadMipped(vdp, docker(1));
    art.runB = gs::uploadMipped(vdp, docker(2));
    art.duck = gs::uploadMipped(vdp, docker(3));
    art.leap = gs::uploadMipped(vdp, docker(4));
    art.pouch[0] = gs::uploadMipped(vdp, satchel(0));
    art.pouch[1] = gs::uploadMipped(vdp, satchel(1));
    art.barrel = gs::uploadMipped(vdp, cask());
    art.hook = gs::uploadMipped(vdp, hook());
    art.mast = gs::uploadMipped(vdp, mast());
    art.shed = gs::uploadMipped(vdp, shed());
    art.piling = gs::uploadMipped(vdp, piling());
    art.gull[0] = gs::uploadMipped(vdp, gull(0));
    art.gull[1] = gs::uploadMipped(vdp, gull(1));
    art.shadow = gs::uploadMipped(vdp, shadowBlob());
}

}  // namespace wharf
