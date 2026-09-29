#include "game/art.h"

#include <initializer_list>
#include <string>

namespace orchard {
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

int solidTile(gs::TileAlloc& al, gs::VDP& vdp, int base, int speckle, int edge) {
    uint8_t px[64];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int v = base;
            if (y == 0 || x == 0) v = edge;
            else if (((x * 3 + y * 5) % 7) == 0) v = speckle;
            px[y * 8 + x] = uint8_t(v);
        }
    }
    int t = al.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

int leafTile(gs::TileAlloc& al, gs::VDP& vdp) {
    uint8_t px[64];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int v = ((x + y) & 1) ? 2 : 1;
            if ((x * y) % 11 == 0) v = 3;
            if ((x == 3 || x == 4) && y > 5) v = 4;
            px[y * 8 + x] = uint8_t(v);
        }
    }
    int t = al.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

void paintRows(gs::VDP& vdp, gs::TileAlloc& tiles) {
    int grass = solidTile(tiles, vdp, 2, 3, 1);
    int dirt = solidTile(tiles, vdp, 2, 3, 1);
    int leaves = leafTile(tiles, vdp);
    int trunk = solidTile(tiles, vdp, 2, 1, 3);
    vdp.B.resize(64, 32);
    vdp.A.resize(64, 32);
    vdp.B.clear();
    vdp.A.clear();
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            if (cy >= 10 && cy <= 16) {
                bool trunkCol = (cx % 5) == 2 && cy >= 14;
                vdp.B.set(cx, cy, gs::entry(trunkCol ? trunk : leaves, trunkCol ? PAL_BARK : PAL_LEAF));
            }
            if (cy >= 18 && cy <= 21) vdp.B.set(cx, cy, gs::entry(grass, PAL_GRASS));
            if (cy >= 22 && cy <= 26) vdp.A.set(cx, cy, gs::entry(dirt, PAL_DIRT));
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

Bitmap farmer(int pose) {
    Bitmap b(36, 60);
    const bool duck = pose == 3;
    const bool leap = pose == 4;
    const int y0 = duck ? 18 : 0;
    b.ellipse(18, y0 + 8, 9, 4, 3);
    b.rect(12, y0 + 10, 12, 3, 4);
    b.ellipse(18, y0 + 16, 6, 6, 1);
    b.set(21, y0 + 15, 7);
    b.set(22, y0 + 15, 7);
    b.rect(15, y0 + 19, 5, 1, 7);
    b.rect(11, y0 + 22, 14, 4, 6);
    b.rect(10, y0 + 26, 16, 12, 2);
    b.rect(12, y0 + 28, 5, 8, 6);
    if (pose == 1) b.rect(24, y0 + 26, 8, 4, 6);
    else if (pose == 2) b.rect(4, y0 + 26, 8, 4, 6);
    else b.rect(6, y0 + 26, 4, 8, 6);
    if (leap) {
        b.rect(10, y0 + 38, 6, 8, 2);
        b.rect(20, y0 + 38, 6, 8, 2);
        b.rect(8, y0 + 44, 8, 4, 5);
        b.rect(20, y0 + 44, 8, 4, 5);
    } else if (pose == 1) {
        b.rect(11, y0 + 38, 5, 14, 2);
        b.rect(20, y0 + 40, 5, 10, 2);
        b.rect(10, y0 + 50, 7, 4, 5);
        b.rect(19, y0 + 48, 7, 4, 5);
    } else if (pose == 2) {
        b.rect(20, y0 + 38, 5, 14, 2);
        b.rect(11, y0 + 40, 5, 10, 2);
        b.rect(19, y0 + 50, 7, 4, 5);
        b.rect(10, y0 + 48, 7, 4, 5);
    } else if (duck) {
        b.rect(8, y0 + 36, 8, 6, 2);
        b.rect(20, y0 + 36, 8, 6, 2);
        b.rect(6, y0 + 40, 8, 4, 5);
        b.rect(22, y0 + 40, 8, 4, 5);
    } else {
        b.rect(12, y0 + 38, 5, 14, 2);
        b.rect(19, y0 + 38, 5, 14, 2);
        b.rect(11, y0 + 50, 7, 4, 5);
        b.rect(18, y0 + 50, 7, 4, 5);
    }
    return b.cropToContent(1);
}

Bitmap satchel(int bob) {
    Bitmap b(28, 24);
    b.ellipse(14, 13, 11, 9, 1);
    b.ellipse(14, 12, 8, 6, 2);
    b.rect(6, 4, 16, 3, 3);
    b.ellipse(10, 11, 3, 3, 4);
    b.ellipse(17, 12, 2, 2, 5);
    b.rect(13, bob ? 2 : 1, 2, 6, 6);
    return b.cropToContent(1);
}

Bitmap appleTree() {
    Bitmap b(48, 86);
    b.rect(21, 40, 8, 42, 2);
    b.rect(19, 78, 12, 4, 1);
    b.ellipse(24, 28, 20, 22, 2);
    b.ellipse(16, 24, 10, 10, 1);
    b.ellipse(32, 22, 9, 9, 3);
    b.ellipse(18, 18, 3, 3, 4);
    b.ellipse(30, 30, 3, 3, 4);
    b.ellipse(26, 16, 2, 2, 5);
    b.ellipse(14, 32, 2, 2, 5);
    return b.cropToContent(1);
}

Bitmap apple() {
    Bitmap b(12, 12);
    b.ellipse(6, 7, 5, 4, 4);
    b.rect(5, 2, 2, 3, 1);
    return b.cropToContent(0);
}

Bitmap crate() {
    Bitmap b(28, 18);
    b.rect(1, 2, 26, 14, 2);
    b.rect(1, 2, 26, 3, 3);
    b.rect(3, 6, 22, 2, 1);
    b.rect(3, 11, 22, 2, 1);
    return b.cropToContent(1);
}

Bitmap limb() {
    Bitmap b(90, 10);
    b.rect(0, 3, 90, 4, 2);
    b.ellipse(12, 4, 4, 3, 1);
    b.ellipse(40, 3, 5, 3, 3);
    b.ellipse(70, 4, 4, 3, 1);
    b.ellipse(22, 2, 2, 2, 4);
    b.ellipse(58, 2, 2, 2, 4);
    return b.cropToContent(1);
}

Bitmap board() {
    Bitmap b(10, 22);
    b.rect(2, 0, 6, 22, 2);
    b.rect(3, 0, 2, 22, 3);
    return b;
}

Bitmap water() {
    Bitmap b(64, 28);
    b.rect(0, 6, 64, 22, 1);
    b.rect(0, 6, 64, 4, 2);
    b.ellipse(16, 14, 8, 3, 3);
    b.ellipse(42, 18, 10, 3, 4);
    return b;
}

Bitmap barn() {
    Bitmap b(52, 72);
    b.poly({{2, 28}, {26, 4}, {50, 28}}, 1);
    b.rect(6, 28, 40, 42, 2);
    b.rect(20, 44, 12, 26, 4);
    b.rect(10, 34, 8, 8, 3);
    b.rect(34, 34, 8, 8, 3);
    b.rect(24, 8, 4, 10, 3);
    return b.cropToContent(1);
}

Bitmap blob() {
    Bitmap b(28, 8);
    b.ellipse(14, 4, 12, 3, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 11), gs::rgb4(8, 7, 5), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(1, 5, 1), gs::rgb4(2, 8, 2), gs::rgb4(4, 11, 3), gs::rgb4(8, 12, 4)});
    setPal(vdp, PAL_DIRT, {0, gs::rgb4(5, 3, 1), gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3), gs::rgb4(13, 10, 5)});
    setPal(vdp, PAL_POUCH, {0, gs::rgb4(8, 4, 1), gs::rgb4(5, 2, 1), gs::rgb4(11, 8, 3), gs::rgb4(13, 2, 2),
                            gs::rgb4(2, 8, 2), gs::rgb4(14, 12, 4)});
    setPal(vdp, PAL_PLAYER, {0, gs::rgb4(13, 9, 6), gs::rgb4(2, 4, 10), gs::rgb4(13, 11, 4), gs::rgb4(10, 3, 2),
                             gs::rgb4(4, 3, 2), gs::rgb4(14, 13, 10), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_BARK, {0, gs::rgb4(4, 2, 1), gs::rgb4(7, 4, 2), gs::rgb4(10, 7, 3), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_LEAF, {0, gs::rgb4(1, 6, 1), gs::rgb4(2, 9, 2), gs::rgb4(5, 12, 3), gs::rgb4(12, 2, 2),
                           gs::rgb4(14, 12, 3)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(12, 8, 2), gs::rgb4(8, 5, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(10, 2, 2), gs::rgb4(15, 10, 4)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(1, 4, 8), gs::rgb4(2, 7, 11), gs::rgb4(6, 11, 13), gs::rgb4(12, 14, 14)});
    setPal(vdp, PAL_SHED, {0, gs::rgb4(12, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(14, 13, 11), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_GO, {0, gs::rgb4(4, 14, 5), gs::rgb4(2, 8, 3), gs::rgb4(10, 15, 8)});
    vdp.setFogColor(gs::rgb4(6, 8, 10));

    gs::TileAlloc tiles(vdp, 1);
    paintRows(vdp, tiles);
    loadFont(vdp, art, tiles);

    art.stand = gs::uploadMipped(vdp, farmer(0));
    art.runA = gs::uploadMipped(vdp, farmer(1));
    art.runB = gs::uploadMipped(vdp, farmer(2));
    art.duck = gs::uploadMipped(vdp, farmer(3));
    art.leap = gs::uploadMipped(vdp, farmer(4));
    art.pouch[0] = gs::uploadMipped(vdp, satchel(0));
    art.pouch[1] = gs::uploadMipped(vdp, satchel(1));
    art.tree = gs::uploadMipped(vdp, appleTree());
    art.apple = gs::uploadMipped(vdp, apple());
    art.crate = gs::uploadMipped(vdp, crate());
    art.bough = gs::uploadMipped(vdp, limb());
    art.plank = gs::uploadMipped(vdp, board());
    art.ditch = gs::uploadMipped(vdp, water());
    art.shed = gs::uploadMipped(vdp, barn());
    art.shadow = gs::uploadMipped(vdp, blob());
}

}  // namespace orchard
