#include "game/art.h"

#include <initializer_list>
#include <string>

namespace cistern {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
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

Bitmap truckArt() {
    Bitmap b(48, 64);
    b.rect(8, 2, 32, 12, 3);
    b.rect(12, 4, 10, 6, 6);
    b.rect(26, 4, 10, 6, 6);
    b.rect(6, 14, 36, 24, 2);
    b.rect(10, 18, 28, 6, 4);
    b.rect(4, 36, 40, 12, 1);
    b.rect(8, 40, 32, 3, 5);
    b.ellipse(14, 52, 6, 6, 7);
    b.ellipse(34, 52, 6, 6, 7);
    b.ellipse(14, 52, 2, 2, 8);
    b.ellipse(34, 52, 2, 2, 8);
    b.outline(8, false);
    return b;
}

Bitmap carArt() {
    Bitmap b(40, 32);
    b.rect(8, 6, 24, 10, 2);
    b.rect(12, 8, 7, 5, 6);
    b.rect(22, 8, 7, 5, 6);
    b.rect(4, 16, 32, 8, 1);
    b.ellipse(11, 26, 5, 5, 7);
    b.ellipse(29, 26, 5, 5, 7);
    b.outline(8, false);
    return b;
}

Bitmap cisternArt() {
    Bitmap b(64, 80);
    b.ellipse(32, 28, 22, 16, 2);
    b.ellipse(32, 26, 14, 9, 3);
    b.rect(22, 36, 20, 28, 1);
    b.rect(18, 60, 28, 8, 4);
    b.rect(28, 8, 8, 16, 5);
    b.rect(26, 4, 12, 6, 6);
    b.rect(46, 48, 12, 6, 4);
    b.outline(8, false);
    return b;
}

Bitmap gateArt() {
    Bitmap b(18, 28);
    b.rect(2, 2, 14, 24, 1);
    b.rect(4, 6, 10, 3, 2);
    b.rect(4, 14, 10, 3, 2);
    b.rect(4, 20, 10, 3, 3);
    b.outline(8, false);
    return b;
}

Bitmap spoutArt() {
    Bitmap b(36, 16);
    b.ellipse(18, 8, 16, 6, 1);
    b.ellipse(20, 8, 8, 3, 2);
    b.ellipse(10, 9, 4, 2, 3);
    return b;
}

Bitmap reedArt() {
    Bitmap b(20, 36);
    b.line(6, 34, 8, 4, 1, 2);
    b.line(10, 34, 14, 8, 2, 2);
    b.line(14, 34, 12, 12, 1, 2);
    b.ellipse(8, 6, 3, 4, 3);
    return b;
}

Bitmap treeArt() {
    Bitmap b(28, 48);
    b.ellipse(14, 14, 12, 12, 2);
    b.ellipse(14, 16, 6, 6, 3);
    b.rect(12, 24, 4, 22, 4);
    return b;
}

Bitmap dustArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 5, 1);
    b.ellipse(8, 8, 3, 2, 2);
    return b;
}

Bitmap wheelArt() {
    Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(13, 14, 15), gs::rgb4(5, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 4), gs::rgb4(8, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(7, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 14, 10), gs::rgb4(2, 6, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TRUCK, {0, gs::rgb4(4, 6, 4), gs::rgb4(6, 8, 5), gs::rgb4(8, 9, 6), gs::rgb4(3, 3, 2), gs::rgb4(10, 9, 5),
                            gs::rgb4(12, 13, 11), gs::rgb4(1, 1, 1), gs::rgb4(7, 7, 6), ink});
    setPal(vdp, PAL_CAR, {0, gs::rgb4(9, 8, 6), gs::rgb4(12, 11, 8), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 2), 0, gs::rgb4(11, 13, 14),
                          gs::rgb4(1, 1, 1), gs::rgb4(6, 6, 5), ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(6, 6, 6), gs::rgb4(8, 8, 7), gs::rgb4(5, 7, 8), gs::rgb4(4, 4, 4), gs::rgb4(3, 3, 3),
                            gs::rgb4(10, 10, 9), gs::rgb4(2, 2, 2), gs::rgb4(7, 7, 6), ink});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(3, 8, 12), gs::rgb4(6, 12, 14), gs::rgb4(2, 5, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(3, 7, 3), gs::rgb4(2, 5, 2), gs::rgb4(5, 8, 3), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(10, 10, 9), gs::rgb4(6, 6, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(5, 5, 4), gs::rgb4(11, 9, 3), gs::rgb4(3, 6, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(3, 5, 2),
        gs::rgb4(2, 4, 2),
        gs::rgb4(5, 6, 3),
        gs::rgb4(4, 4, 3),
        gs::rgb4(3, 3, 2),
        gs::rgb4(5, 5, 5),
        gs::rgb4(3, 3, 4),
        gs::rgb4(6, 6, 5),
        gs::rgb4(4, 4, 4),
        gs::rgb4(7, 7, 6),
        gs::rgb4(2, 4, 7),
        gs::rgb4(3, 6, 9),
        gs::rgb4(4, 8, 11),
        gs::rgb4(14, 12, 4),
        gs::rgb4(7, 7, 6),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.car = gs::uploadMipped(vdp, carArt());
    art.cistern = gs::uploadMipped(vdp, cisternArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.spout = gs::uploadMipped(vdp, spoutArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
}

}  // namespace cistern
