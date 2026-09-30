#include "game/art.h"

#include <cmath>

namespace busturn {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap shear(const Bitmap& src, float topShift) {
    Bitmap d(src.w + 28, src.h);
    int ox = 14;
    for (int y = 0; y < src.h; y++) {
        float u = src.h <= 1 ? 0 : float(y) / float(src.h - 1);
        int sh = int(std::lround(topShift * (1.0f - u)));
        for (int x = 0; x < src.w; x++) {
            int c = src.get(x, y);
            if (c) d.set(x + ox + sh, y, c);
        }
    }
    return d.cropToContent(1);
}

Bitmap busRear() {
    Bitmap b(88, 112);
    b.rect(14, 18, 60, 78, 2);
    b.rect(18, 22, 52, 70, 1);
    b.poly({{14, 18}, {74, 18}, {68, 8}, {20, 8}}, 3);
    b.rect(22, 10, 44, 10, 8);
    b.rect(26, 12, 36, 6, 7);
    b.rect(20, 28, 22, 16, 4);
    b.rect(46, 28, 22, 16, 4);
    b.rect(22, 30, 8, 6, 9);
    b.rect(48, 30, 8, 6, 9);
    b.rect(30, 50, 28, 22, 5);
    b.rect(34, 54, 20, 14, 4);
    b.rect(16, 78, 12, 8, 6);
    b.rect(60, 78, 12, 8, 6);
    b.rect(18, 86, 16, 16, 5);
    b.rect(54, 86, 16, 16, 5);
    b.ellipse(26, 94, 6, 6, 8);
    b.ellipse(62, 94, 6, 6, 8);
    b.rect(38, 96, 12, 4, 5);
    b.rect(8, 40, 6, 28, 3);
    b.rect(74, 40, 6, 28, 3);
    b.outline(5, false);
    return b;
}

Bitmap crewRear() {
    Bitmap b(72, 96);
    b.rect(12, 16, 48, 66, 3);
    b.rect(16, 20, 40, 58, 2);
    b.poly({{12, 16}, {60, 16}, {54, 8}, {18, 8}}, 1);
    b.rect(18, 26, 16, 12, 4);
    b.rect(38, 26, 16, 12, 4);
    b.rect(24, 46, 24, 16, 5);
    b.rect(14, 68, 10, 6, 6);
    b.rect(48, 68, 10, 6, 6);
    b.rect(16, 76, 12, 12, 5);
    b.rect(44, 76, 12, 12, 5);
    b.outline(5, false);
    return b;
}

Bitmap blockArt(int kind) {
    Bitmap b(64, 96);
    int wall = kind == 0 ? 1 : kind == 1 ? 2 : 3;
    int roof = kind == 2 ? 6 : 4;
    b.rect(6, 28, 52, 64, wall);
    b.poly({{4, 30}, {32, 8}, {60, 30}}, roof);
    b.rect(10, 36, 12, 16, 5);
    b.rect(28, 36, 12, 16, 5);
    b.rect(42, 36, 12, 16, 7);
    b.rect(10, 58, 12, 16, 7);
    b.rect(28, 58, 12, 16, 5);
    b.rect(42, 58, 12, 16, 5);
    b.rect(26, 74, 14, 18, 8);
    b.outline(9, false);
    return b;
}

Bitmap treeArt() {
    Bitmap b(48, 72);
    b.rect(21, 40, 6, 28, 4);
    b.ellipse(24, 28, 18, 20, 2);
    b.ellipse(18, 24, 8, 10, 1);
    b.ellipse(30, 22, 7, 8, 3);
    b.outline(9, false);
    return b;
}

Bitmap arrowArt() {
    Bitmap b(64, 48);
    b.poly({{8, 24}, {34, 6}, {34, 16}, {56, 16}, {56, 32}, {34, 32}, {34, 42}}, 1);
    b.poly({{12, 24}, {32, 10}, {32, 18}, {52, 18}, {52, 30}, {32, 30}, {32, 38}}, 2);
    b.outline(3, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(24, 80);
    b.rect(10, 18, 4, 58, 3);
    b.rect(6, 6, 12, 14, 1);
    b.rect(8, 8, 8, 8, 2);
    b.outline(4, false);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t shadow = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 9, 8), gs::rgb4(15, 13, 4), gs::rgb4(15, 5, 3), gs::rgb4(4, 13, 6),
                          gs::rgb4(6, 10, 15), gs::rgb4(15, 10, 2), gs::rgb4(4, 4, 4), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(15, 14, 3), gs::rgb4(15, 10, 1), gs::rgb4(2, 2, 2), gs::rgb4(15, 15, 15), 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BUS, {0, gs::rgb4(15, 13, 2), gs::rgb4(13, 10, 1), gs::rgb4(8, 6, 1), gs::rgb4(2, 4, 8),
                          gs::rgb4(1, 1, 1), gs::rgb4(14, 2, 2), gs::rgb4(15, 15, 13), gs::rgb4(10, 10, 9),
                          gs::rgb4(12, 14, 15), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(4, 6, 12), gs::rgb4(8, 10, 15), gs::rgb4(3, 4, 8), gs::rgb4(2, 3, 6),
                           gs::rgb4(1, 1, 2), gs::rgb4(14, 3, 2), gs::rgb4(12, 12, 13), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TOWN, {0, gs::rgb4(11, 8, 6), gs::rgb4(6, 10, 4), gs::rgb4(8, 9, 10), gs::rgb4(5, 4, 3),
                           gs::rgb4(3, 5, 8), gs::rgb4(9, 4, 3), gs::rgb4(13, 12, 8), gs::rgb4(4, 3, 3),
                           gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(4, 9, 3), gs::rgb4(3, 7, 2), gs::rgb4(6, 10, 4), gs::rgb4(7, 7, 6), gs::rgb4(5, 5, 4),
            gs::rgb4(4, 4, 5), gs::rgb4(6, 6, 7), gs::rgb4(8, 8, 7), gs::rgb4(3, 3, 4), gs::rgb4(9, 9, 6),
            gs::rgb4(2, 5, 8), gs::rgb4(3, 6, 10), gs::rgb4(8, 12, 14), gs::rgb4(14, 13, 6), gs::rgb4(9, 9, 10)});

    Bitmap rear = busRear();
    const float banks[5] = {18, 9, 0, -9, -18};
    for (int i = 0; i < 5; i++) art.bus[i] = gs::uploadMipped(vdp, shear(rear, banks[i]));
    art.crew = gs::uploadMipped(vdp, crewRear());
    for (int i = 0; i < 3; i++) art.block[i] = gs::uploadMipped(vdp, blockArt(i));
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.arrow = gs::uploadMipped(vdp, arrowArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(8, 10, 13));
}

}  // namespace busturn
