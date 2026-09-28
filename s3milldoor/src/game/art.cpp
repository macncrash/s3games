#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace mill {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
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

gs::Bitmap leafArt() {
    gs::Bitmap b(48, 112);
    b.rect(3, 2, 42, 108, 2);
    b.rect(3, 2, 8, 108, 1);
    b.rect(37, 2, 8, 108, 3);
    for (int y = 12; y < 100; y += 16) b.rect(8, float(y), 32, 3, 4);
    b.line(10, 8, 38, 104, 5, 3.f);
    b.line(38, 8, 10, 104, 5, 2.f);
    b.rect(20, 48, 8, 14, 6);
    b.rect(22, 52, 4, 6, 8);
    b.ellipse(24, 28, 6, 8, 7);
    b.rect(6, 100, 36, 5, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(18, 124);
    b.rect(2, 2, 14, 120, 2);
    b.rect(2, 2, 5, 120, 1);
    b.rect(11, 2, 5, 120, 3);
    for (int y = 10; y < 114; y += 14) b.rect(3, float(y), 12, 3, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(132, 18);
    b.rect(1, 3, 130, 12, 2);
    b.rect(1, 3, 130, 4, 1);
    b.rect(1, 11, 130, 4, 3);
    for (int i = 0; i < 8; i++) b.rect(float(8 + i * 15), 6, 4, 6, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap millerArt(int pose) {
    gs::Bitmap b(30, 52);
    b.ellipse(15, 7, 6, 4, 4);
    b.ellipse(15, 13, 5, 5, 3);
    b.set(13, 13, 8);
    b.set(17, 13, 8);
    b.rect(11, 17, 8, 10, 1);
    b.rect(10, 26, 10, 4, 6);
    if (pose == 0) {
        b.line(11, 22, 4, 36, 1, 3.f);
        b.rect(6, 34, 6, 12, 2);
        b.rect(17, 34, 6, 12, 1);
    } else {
        b.line(12, 22, 2, 28, 1, 3.f);
        b.line(18, 22, 26, 30, 1, 3.f);
        b.rect(8, 34, 5, 12, 2);
        b.rect(17, 34, 5, 12, 1);
    }
    b.rect(7, 44, 6, 5, 5);
    b.rect(17, 44, 6, 5, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap wheelArt(int tick) {
    gs::Bitmap b(64, 64);
    b.ellipse(32, 32, 28, 28, 2);
    b.ellipse(32, 32, 20, 20, 1);
    b.ellipse(32, 32, 6, 6, 4);
    for (int i = 0; i < 8; i++) {
        float a = (float(i) + (tick ? 0.5f : 0.f)) * 0.785f;
        float c = std::cos(a), s = std::sin(a);
        b.line(32 + c * 8, 32 + s * 8, 32 + c * 26, 32 + s * 26, (i & 1) ? 3 : 5, 2.f);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(22, 28);
    b.ellipse(11, 16, 9, 10, 2);
    b.rect(6, 4, 10, 8, 1);
    b.line(6, 8, 16, 8, 4, 1.f);
    b.rect(8, 14, 6, 3, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap chockArt() {
    gs::Bitmap b(20, 12);
    b.poly({{1, 10}, {18, 10}, {14, 2}, {4, 2}}, 2);
    b.rect(6, 5, 8, 3, 1);
    b.outline(15, false);
    return b;
}

gs::Bitmap splashArt() {
    gs::Bitmap b(16, 12);
    b.ellipse(8, 8, 6, 3, 2);
    b.line(8, 8, 4, 2, 1, 1.5f);
    b.line(8, 8, 12, 1, 3, 1.5f);
    b.line(8, 8, 8, 2, 1, 1.5f);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(6, 6);
    b.ellipse(3, 3, 2, 2, 1);
    return b;
}

gs::Bitmap vaneArt() {
    gs::Bitmap b(10, 36);
    b.rect(3, 2, 4, 32, 2);
    b.rect(2, 2, 6, 4, 1);
    b.outline(15, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    auto C = gs::rgb4;
    setPal(vdp, PAL_TEXT, {0, C(14, 13, 11), C(8, 7, 6), C(4, 3, 3), C(15, 15, 14), C(6, 5, 4), C(10, 9, 7), C(2, 2, 2),
                           C(12, 11, 9), C(7, 6, 5), 0, 0, 0, 0, 0, C(1, 1, 1)});
    setPal(vdp, PAL_LAMP, {0, C(15, 12, 4), C(12, 8, 2), C(8, 5, 1), C(15, 15, 8), C(6, 4, 1), C(14, 10, 3), C(3, 2, 1),
                           C(15, 14, 6), C(9, 6, 2), 0, 0, 0, 0, 0, C(2, 1, 0)});
    setPal(vdp, PAL_ALERT, {0, C(15, 4, 2), C(10, 2, 1), C(6, 1, 1), C(15, 10, 6), C(8, 3, 2), C(14, 6, 3), C(3, 1, 1),
                            C(15, 14, 10), C(9, 2, 1), 0, 0, 0, 0, 0, C(2, 0, 0)});
    setPal(vdp, PAL_GOOD, {0, C(6, 14, 6), C(3, 9, 3), C(2, 5, 2), C(12, 15, 8), C(4, 8, 3), C(8, 12, 5), C(1, 3, 1),
                           C(14, 15, 12), C(5, 10, 4), 0, 0, 0, 0, 0, C(0, 2, 0)});
    setPal(vdp, PAL_STONE, {0, C(9, 9, 8), C(6, 6, 6), C(4, 4, 4), C(12, 11, 10), C(7, 6, 5), C(11, 10, 9), C(3, 3, 3),
                            C(14, 13, 12), C(5, 5, 5), 0, 0, 0, 0, 0, C(1, 1, 1)});
    setPal(vdp, PAL_OAK, {0, C(10, 6, 2), C(7, 4, 1), C(4, 2, 1), C(13, 8, 3), C(8, 5, 2), C(12, 7, 3), C(3, 2, 1),
                          C(14, 11, 6), C(6, 3, 1), 0, 0, 0, 0, 0, C(1, 1, 0)});
    setPal(vdp, PAL_IRON, {0, C(8, 8, 9), C(5, 5, 6), C(3, 3, 4), C(12, 12, 13), C(6, 6, 7), C(10, 10, 11), C(2, 2, 3),
                           C(14, 14, 15), C(4, 4, 5), 0, 0, 0, 0, 0, C(1, 1, 1)});
    setPal(vdp, PAL_WATER, {0, C(4, 8, 12), C(2, 5, 8), C(1, 3, 5), C(8, 12, 15), C(3, 6, 9), C(6, 10, 13), C(1, 2, 3),
                            C(12, 14, 15), C(3, 7, 10), 0, 0, 0, 0, 0, C(0, 1, 2)});
    setPal(vdp, PAL_FLOUR, {0, C(14, 13, 11), C(10, 9, 7), C(7, 6, 5), C(15, 15, 13), C(9, 8, 6), C(12, 11, 9), C(4, 3, 2),
                            C(15, 14, 12), C(8, 7, 6), 0, 0, 0, 0, 0, C(2, 2, 1)});
    setPal(vdp, PAL_SKY, {0, C(6, 8, 12), C(4, 6, 9), C(2, 3, 6), C(10, 12, 14), C(5, 6, 8), C(8, 10, 13), C(1, 2, 3),
                          C(13, 14, 15), C(3, 5, 7), 0, 0, 0, 0, 0, C(1, 1, 2)});
    setPal(vdp, PAL_WHEEL, {0, C(6, 8, 4), C(4, 5, 2), C(2, 3, 1), C(9, 11, 6), C(5, 6, 3), C(8, 9, 5), C(1, 2, 1),
                            C(12, 13, 8), C(3, 4, 2), 0, 0, 0, 0, 0, C(1, 1, 0)});
    setPal(vdp, PAL_APRON, {0, C(12, 10, 6), C(8, 6, 3), C(5, 4, 2), C(14, 12, 8), C(9, 7, 4), C(11, 9, 5), C(3, 2, 1),
                            C(15, 14, 10), C(7, 5, 3), 0, 0, 0, 0, 0, C(2, 1, 0)});
    setPal(vdp, PAL_RACE, {0, C(3, 7, 10), C(2, 4, 6), C(1, 2, 4), C(7, 11, 13), C(2, 5, 7), C(5, 9, 11), C(1, 1, 2),
                           C(10, 13, 14), C(2, 6, 8), 0, 0, 0, 0, 0, C(0, 1, 1)});

    art.leaf = gs::uploadMipped(vdp, leafArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.miller[0] = gs::uploadMipped(vdp, millerArt(0));
    art.miller[1] = gs::uploadMipped(vdp, millerArt(1));
    art.wheel[0] = gs::uploadMipped(vdp, wheelArt(0));
    art.wheel[1] = gs::uploadMipped(vdp, wheelArt(1));
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.chock = gs::uploadMipped(vdp, chockArt());
    art.splash = gs::uploadMipped(vdp, splashArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.vane = gs::uploadMipped(vdp, vaneArt());
    loadFont(vdp, art);
}

}  // namespace mill
