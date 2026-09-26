#include "game/art.h"

#include <initializer_list>
#include <string>

namespace rcol {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void legs(Bitmap& b, int x, int y, int step) {
    int a = step ? 0 : 3;
    int d = step ? 3 : 0;
    b.rect(float(x), float(y + a), 7, float(14 - a), 3);
    b.rect(float(x + 12), float(y + d), 7, float(14 - d), 3);
    b.rect(float(x - 1), float(y + 12), 9, 4, 4);
    b.rect(float(x + 11), float(y + 12), 9, 4, 4);
}

Bitmap warden(int step) {
    Bitmap b(48, 80);
    b.ellipse(24, 14, 10, 11, 2);
    b.ellipse(24, 13, 6, 7, 3);
    b.poly({{24, 20}, {12, 30}, {8, 58}, {40, 58}, {36, 30}}, 2);
    b.poly({{24, 24}, {16, 32}, {15, 56}, {27, 56}, {26, 32}}, 3);
    b.rect(12, 40, 24, 4, 7);
    b.rect(21, 39, 6, 6, 1);
    b.ellipse(36, 48, 5, 4, 4);
    b.ellipse(36, 48, 2, 2, 1);
    legs(b, 14, 56, step);
    b.outline(5, false);
    return b;
}

Bitmap braceArt() {
    Bitmap b(64, 80);
    b.ellipse(32, 14, 10, 11, 2);
    b.ellipse(32, 13, 6, 7, 3);
    b.poly({{32, 20}, {20, 30}, {16, 58}, {48, 58}, {44, 30}}, 2);
    b.poly({{32, 24}, {24, 32}, {23, 56}, {35, 56}, {34, 32}}, 3);
    b.rect(4, 34, 16, 5, 2);
    b.rect(44, 34, 16, 5, 2);
    b.rect(2, 32, 6, 6, 6);
    b.rect(56, 32, 6, 6, 6);
    b.rect(20, 40, 24, 4, 7);
    b.rect(29, 39, 6, 6, 1);
    b.rect(22, 56, 7, 14, 3);
    b.rect(35, 56, 7, 14, 3);
    b.rect(20, 68, 10, 4, 4);
    b.rect(34, 68, 10, 4, 4);
    b.outline(5, false);
    return b;
}

Bitmap scoutArt() {
    Bitmap b(52, 56);
    b.rect(10, 14, 32, 8, 3);
    b.rect(14, 16, 10, 4, 1);
    b.rect(28, 16, 10, 4, 6);
    b.rect(8, 22, 36, 14, 2);
    b.rect(18, 24, 16, 4, 7);
    b.ellipse(16, 34, 4, 3, 6);
    b.ellipse(36, 34, 4, 3, 6);
    b.rect(6, 36, 40, 5, 4);
    b.ellipse(12, 46, 7, 7, 5);
    b.ellipse(40, 46, 7, 7, 5);
    b.ellipse(12, 46, 3, 3, 3);
    b.ellipse(40, 46, 3, 3, 3);
    b.outline(5, false);
    return b;
}

Bitmap truckArt() {
    Bitmap b(64, 76);
    b.poly({{14, 8}, {50, 8}, {46, 2}, {18, 2}}, 3);
    b.rect(12, 8, 40, 20, 2);
    b.rect(16, 10, 32, 8, 1);
    b.rect(14, 28, 36, 18, 4);
    b.rect(18, 30, 28, 10, 3);
    b.rect(20, 32, 10, 6, 1);
    b.rect(34, 32, 10, 6, 6);
    b.ellipse(20, 50, 4, 3, 6);
    b.ellipse(44, 50, 4, 3, 6);
    b.rect(8, 50, 48, 6, 3);
    b.ellipse(14, 64, 8, 8, 5);
    b.ellipse(50, 64, 8, 8, 5);
    b.ellipse(14, 64, 3, 3, 1);
    b.ellipse(50, 64, 3, 3, 1);
    b.outline(5, false);
    return b;
}

Bitmap pennantArt() {
    Bitmap b(22, 30);
    b.rect(3, 2, 2, 26, 4);
    b.poly({{5, 4}, {18, 10}, {5, 16}}, 2);
    b.poly({{5, 6}, {14, 10}, {5, 14}}, 6);
    b.outline(5, false);
    return b;
}

Bitmap chainArt() {
    Bitmap b(104, 16);
    for (int i = 0; i < 8; ++i) {
        float x = 8.f + float(i) * 12.5f;
        b.ellipse(x, 8, 6, 5, 2);
        b.ellipse(x, 8, 3.4f, 2.6f, 1);
        b.ellipse(x, 8, 1.7f, 1.2f, 0);
    }
    return b;
}

Bitmap postArt() {
    Bitmap b(28, 72);
    b.rect(8, 14, 12, 54, 2);
    b.rect(10, 16, 4, 48, 1);
    b.rect(4, 8, 20, 10, 3);
    b.rect(6, 6, 16, 4, 4);
    b.ellipse(14, 8, 3, 3, 6);
    b.outline(5, false);
    return b;
}

Bitmap cairnArt() {
    Bitmap b(40, 48);
    b.ellipse(20, 38, 16, 7, 3);
    b.ellipse(20, 30, 12, 7, 2);
    b.ellipse(20, 22, 8, 6, 1);
    b.ellipse(20, 14, 5, 5, 4);
    b.outline(5, false);
    return b;
}

Bitmap bannerArt() {
    Bitmap b(36, 70);
    b.rect(6, 2, 3, 66, 4);
    b.rect(9, 8, 22, 18, 2);
    b.rect(11, 10, 18, 14, 3);
    b.poly({{16, 12}, {26, 17}, {16, 22}}, 6);
    b.rect(8, 26, 5, 3, 7);
    b.outline(5, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 2);
    b.ellipse(8, 8, 3, 3, 1);
    return b;
}

Bitmap dustArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 12, 9, 3);
    b.ellipse(15, 15, 7, 5, 2);
    b.ellipse(14, 14, 3, 2, 1);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(64, 16);
    b.ellipse(32, 8, 26, 5, 1);
    return b;
}

Bitmap sunArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 10, 10, 2);
    b.ellipse(14, 14, 6, 6, 1);
    b.ellipse(11, 11, 2, 2, 6);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(72, 26);
    b.ellipse(22, 15, 16, 8, 2);
    b.ellipse(40, 13, 18, 9, 1);
    b.ellipse(56, 16, 12, 6, 2);
    return b;
}

Bitmap peakSharp() {
    Bitmap b(84, 50);
    b.poly({{0, 49}, {0, 32}, {18, 28}, {36, 8}, {54, 26}, {70, 18}, {84, 30}, {84, 49}}, 2);
    b.poly({{0, 49}, {12, 38}, {30, 24}, {48, 30}, {66, 22}, {84, 34}, {84, 49}}, 3);
    b.poly({{30, 16}, {36, 8}, {44, 18}}, 1);
    return b;
}

Bitmap peakLong() {
    Bitmap b(100, 42);
    b.poly({{0, 41}, {0, 24}, {20, 18}, {40, 8}, {62, 20}, {82, 12}, {100, 22}, {100, 41}}, 2);
    b.poly({{0, 41}, {16, 28}, {38, 18}, {60, 22}, {84, 16}, {100, 26}, {100, 41}}, 3);
    b.poly({{34, 14}, {40, 8}, {48, 16}}, 1);
    return b;
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(9, 9, 10), gs::rgb4(5, 5, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 5), gs::rgb4(10, 7, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           gs::rgb4(1, 3, 1)});

    setPal(vdp, PAL_YOU, {0, gs::rgb4(14, 12, 8), gs::rgb4(9, 4, 2), gs::rgb4(5, 2, 1), gs::rgb4(3, 2, 2),
                          gs::rgb4(1, 1, 1), gs::rgb4(13, 9, 6), gs::rgb4(12, 11, 8), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SCOUT, {0, gs::rgb4(14, 13, 9), gs::rgb4(11, 9, 4), gs::rgb4(6, 5, 3), gs::rgb4(4, 3, 2),
                            gs::rgb4(1, 1, 1), gs::rgb4(15, 14, 10), gs::rgb4(8, 10, 12), gs::rgb4(10, 3, 2), 0, 0, 0,
                            0, 0, 0, ink});
    setPal(vdp, PAL_TRUCK, {0, gs::rgb4(10, 12, 7), gs::rgb4(6, 8, 4), gs::rgb4(3, 4, 2), gs::rgb4(5, 6, 5),
                            gs::rgb4(1, 1, 1), gs::rgb4(15, 15, 12), gs::rgb4(12, 11, 8), gs::rgb4(8, 7, 4), 0, 0, 0, 0,
                            0, 0, ink});
    setPal(vdp, PAL_CHAIN, {0, gs::rgb4(13, 13, 14), gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 5), gs::rgb4(10, 6, 3),
                            gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 10), gs::rgb4(15, 11, 4), gs::rgb4(8, 7, 5), gs::rgb4(5, 4, 3),
                         gs::rgb4(15, 8, 3), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(12, 11, 9), gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 3), gs::rgb4(9, 8, 6),
                            gs::rgb4(1, 1, 1), gs::rgb4(14, 13, 11), gs::rgb4(15, 12, 5), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(14, 12, 8), gs::rgb4(8, 2, 2), gs::rgb4(4, 1, 1), gs::rgb4(6, 5, 4),
                             gs::rgb4(1, 1, 1), gs::rgb4(13, 10, 4), gs::rgb4(15, 14, 10), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MOUNT, {0, gs::rgb4(12, 12, 13), gs::rgb4(5, 6, 8), gs::rgb4(3, 3, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, ink});

    const uint16_t field[16] = {
        0,
        gs::rgb4(5, 6, 4),
        gs::rgb4(3, 3, 2),
        gs::rgb4(6, 6, 4),
        gs::rgb4(8, 7, 4),
        gs::rgb4(5, 4, 2),
        gs::rgb4(9, 8, 7),
        gs::rgb4(6, 6, 5),
        gs::rgb4(12, 11, 9),
        gs::rgb4(4, 4, 3),
        gs::rgb4(10, 9, 8),
        gs::rgb4(3, 4, 6),
        gs::rgb4(2, 3, 5),
        gs::rgb4(4, 5, 6),
        gs::rgb4(13, 12, 9),
        gs::rgb4(7, 6, 5),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_FIELD * 16 + i, field[i]);

    art.warden[0] = gs::uploadMipped(vdp, warden(0));
    art.warden[1] = gs::uploadMipped(vdp, warden(1));
    art.brace = gs::uploadMipped(vdp, braceArt());
    art.scout = gs::uploadMipped(vdp, scoutArt());
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.chain = gs::uploadMipped(vdp, chainArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.cairn = gs::uploadMipped(vdp, cairnArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.peak[0] = gs::uploadMipped(vdp, peakSharp());
    art.peak[1] = gs::uploadMipped(vdp, peakLong());
    loadFont(vdp, art);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace rcol
