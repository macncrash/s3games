#include "game/art.h"

#include <initializer_list>
#include <string>

namespace culvert {
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
    Bitmap b(44, 58);
    b.rect(10, 2, 24, 10, 3);
    b.rect(14, 4, 7, 5, 6);
    b.rect(6, 12, 32, 18, 2);
    b.rect(8, 16, 28, 4, 4);
    b.rect(4, 30, 36, 14, 1);
    b.rect(6, 34, 32, 4, 5);
    b.ellipse(13, 48, 6, 6, 7);
    b.ellipse(31, 48, 6, 6, 7);
    b.ellipse(13, 48, 2, 2, 8);
    b.ellipse(31, 48, 2, 2, 8);
    b.outline(8, false);
    return b;
}

Bitmap carArt() {
    Bitmap b(36, 28);
    b.rect(6, 4, 24, 9, 2);
    b.rect(10, 6, 6, 4, 6);
    b.rect(19, 6, 6, 4, 6);
    b.rect(3, 13, 30, 7, 1);
    b.ellipse(10, 22, 4, 4, 7);
    b.ellipse(26, 22, 4, 4, 7);
    b.outline(8, false);
    return b;
}

Bitmap pipeArt() {
    Bitmap b(48, 40);
    b.ellipse(24, 20, 20, 16, 1);
    b.ellipse(24, 20, 12, 9, 2);
    b.ellipse(24, 20, 6, 4, 3);
    b.rect(4, 18, 8, 16, 4);
    b.rect(36, 18, 8, 16, 4);
    b.outline(8, false);
    return b;
}

Bitmap beamArt() {
    Bitmap b(72, 16);
    b.rect(2, 3, 68, 10, 1);
    b.rect(6, 5, 60, 3, 2);
    b.rect(8, 9, 8, 4, 3);
    b.rect(32, 9, 8, 4, 3);
    b.rect(56, 9, 8, 4, 3);
    b.outline(8, false);
    return b;
}

Bitmap wingArt() {
    Bitmap b(22, 36);
    b.poly({{2, 34}, {18, 34}, {14, 4}, {6, 4}}, 1);
    b.rect(8, 10, 6, 16, 2);
    b.outline(8, false);
    return b;
}

Bitmap reedArt() {
    Bitmap b(16, 32);
    b.line(4, 30, 6, 4, 1, 2);
    b.line(8, 30, 12, 8, 2, 2);
    b.line(12, 30, 9, 12, 1, 2);
    b.ellipse(7, 5, 3, 3, 3);
    return b;
}

Bitmap treeArt() {
    Bitmap b(26, 44);
    b.ellipse(13, 12, 11, 11, 2);
    b.ellipse(13, 14, 5, 5, 3);
    b.rect(11, 22, 4, 18, 4);
    return b;
}

Bitmap dustArt() {
    Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 4, 1);
    b.ellipse(7, 7, 2, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 14, 13), gs::rgb4(6, 6, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3), gs::rgb4(8, 5, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 2), gs::rgb4(8, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(7, 14, 8), gs::rgb4(2, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TRUCK, {0, gs::rgb4(8, 4, 2), gs::rgb4(11, 6, 3), gs::rgb4(13, 8, 4), gs::rgb4(4, 3, 2), gs::rgb4(6, 5, 3),
                            gs::rgb4(14, 13, 10), gs::rgb4(1, 1, 1), gs::rgb4(9, 8, 6), ink});
    setPal(vdp, PAL_CAR, {0, gs::rgb4(12, 12, 10), gs::rgb4(8, 9, 10), gs::rgb4(4, 4, 4), gs::rgb4(2, 2, 2), 0, gs::rgb4(10, 13, 14),
                          gs::rgb4(1, 1, 1), gs::rgb4(6, 6, 5), ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(7, 7, 6), gs::rgb4(9, 9, 8), gs::rgb4(5, 5, 5), gs::rgb4(4, 4, 3), gs::rgb4(3, 3, 3),
                            gs::rgb4(11, 11, 9), gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 7), ink});
    setPal(vdp, PAL_PIPE, {0, gs::rgb4(5, 6, 6), gs::rgb4(2, 3, 4), gs::rgb4(8, 9, 8), gs::rgb4(4, 4, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(3, 6, 2), gs::rgb4(2, 4, 2), gs::rgb4(5, 7, 3), gs::rgb4(5, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(10, 9, 7), gs::rgb4(6, 5, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BEAM, {0, gs::rgb4(8, 8, 7), gs::rgb4(13, 11, 4), gs::rgb4(4, 4, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(4, 5, 3),
        gs::rgb4(3, 4, 2),
        gs::rgb4(6, 6, 4),
        gs::rgb4(5, 5, 4),
        gs::rgb4(3, 3, 2),
        gs::rgb4(6, 6, 5),
        gs::rgb4(4, 4, 4),
        gs::rgb4(7, 7, 6),
        gs::rgb4(5, 5, 5),
        gs::rgb4(8, 8, 7),
        gs::rgb4(2, 3, 5),
        gs::rgb4(3, 4, 6),
        gs::rgb4(4, 5, 7),
        gs::rgb4(13, 11, 4),
        gs::rgb4(8, 7, 6),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.car = gs::uploadMipped(vdp, carArt());
    art.pipe = gs::uploadMipped(vdp, pipeArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.wing = gs::uploadMipped(vdp, wingArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
}

}  // namespace culvert
