#include "game/art.h"

#include <initializer_list>
#include <string>

namespace rdawn {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap wardenArt(int step) {
    Bitmap b(40, 72);
    b.ellipse(20, 13, 11, 12, 2);
    b.ellipse(20, 15, 7, 7, 3);
    b.rect(15, 18, 10, 5, 1);
    b.poly({{20, 22}, {6, 32}, {5, 50}, {35, 50}, {34, 32}}, 2);
    b.poly({{20, 26}, {12, 34}, {12, 48}, {22, 48}}, 3);
    b.rect(10, 46, 20, 4, 8);
    int a = step ? 0 : 4;
    int d = step ? 4 : 0;
    b.rect(11, 50 + a, 7, 14 - a, 2);
    b.rect(22, 50 + d, 7, 14 - d, 2);
    b.rect(10, 62, 9, 5, 4);
    b.rect(21, 62, 9, 5, 4);
    b.rect(27, 34, 9, 12, 6);
    b.rect(28, 35, 4, 5, 7);
    b.rect(29, 30, 5, 5, 6);
    b.rect(8, 34, 4, 8, 9);
    b.outline(5, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(30, 68);
    b.ellipse(15, 58, 12, 5, 4);
    b.ellipse(15, 57, 7, 2, 3);
    b.rect(13, 20, 4, 38, 1);
    b.rect(15, 20, 2, 38, 2);
    b.line(15, 48, 4, 64, 1, 2);
    b.line(15, 48, 26, 64, 1, 2);
    b.ellipse(15, 18, 12, 7, 2);
    b.ellipse(15, 17, 8, 4, 6);
    b.rect(4, 16, 4, 3, 3);
    b.rect(22, 16, 4, 3, 3);
    b.outline(5, false);
    return b;
}

Bitmap flameArt(int frame) {
    Bitmap b(22, 36);
    if (frame == 0) {
        b.ellipse(11, 24, 8, 10, 1);
        b.ellipse(11, 21, 6, 9, 2);
        b.ellipse(11, 18, 4, 7, 3);
        b.ellipse(11, 15, 2, 4, 4);
        b.set(11, 12, 5);
        b.set(10, 14, 5);
    } else {
        b.ellipse(12, 25, 7, 9, 1);
        b.ellipse(11, 22, 5, 8, 2);
        b.ellipse(10, 18, 3, 6, 3);
        b.ellipse(11, 14, 2, 3, 4);
        b.set(11, 12, 5);
        b.ellipse(13, 20, 2, 3, 2);
    }
    return b;
}

Bitmap glowArt() {
    Bitmap b(30, 30);
    b.ellipse(15, 16, 13, 11, 1);
    b.ellipse(15, 15, 7, 6, 2);
    return b;
}

Bitmap smokeArt(int frame) {
    Bitmap b(26, 26);
    b.ellipse(13, 16, 9, 6, 1);
    b.ellipse(12 + frame * 2, 12, 6, 5, 2);
    b.ellipse(15, 9, 3, 3, 3);
    return b;
}

Bitmap gustArt() {
    Bitmap b(52, 20);
    b.line(2, 5, 40, 4, 1, 2);
    b.line(6, 10, 48, 9, 2, 2);
    b.line(2, 15, 36, 15, 3, 2);
    b.poly({{40, 1}, {50, 5}, {40, 8}}, 2);
    b.poly({{34, 12}, {46, 16}, {34, 19}}, 1);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(7, 7);
    b.rect(3, 0, 1, 7, 4);
    b.rect(0, 3, 7, 1, 3);
    b.set(3, 3, 5);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(40, 12);
    b.ellipse(20, 6, 16, 4, 1);
    return b;
}

Bitmap cairnArt() {
    Bitmap b(30, 40);
    b.ellipse(15, 34, 13, 5, 2);
    b.rect(6, 24, 18, 10, 1);
    b.rect(8, 16, 14, 10, 3);
    b.rect(11, 8, 8, 9, 1);
    b.rect(13, 2, 4, 7, 4);
    b.outline(5, false);
    return b;
}

Bitmap stakeArt() {
    Bitmap b(16, 44);
    b.rect(7, 8, 3, 28, 1);
    b.rect(8, 8, 1, 28, 2);
    b.poly({{4, 8}, {8, 1}, {12, 8}}, 3);
    b.rect(4, 36, 8, 4, 4);
    b.poly({{8, 10}, {15, 16}, {8, 18}}, 6);
    b.outline(5, false);
    return b;
}

Bitmap moonArt() {
    Bitmap b(28, 28);
    b.ellipse(13, 14, 11, 11, 1);
    b.ellipse(19, 12, 9, 9, 0);
    b.ellipse(9, 12, 2, 2, 2);
    b.ellipse(12, 18, 1, 1, 3);
    b.set(8, 16, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(34, 34);
    b.line(4, 4, 10, 10, 3, 2);
    b.line(30, 4, 24, 10, 3, 2);
    b.line(4, 30, 10, 24, 3, 2);
    b.line(30, 30, 24, 24, 3, 2);
    b.rect(16, 1, 2, 7, 3);
    b.rect(16, 26, 2, 7, 3);
    b.rect(1, 16, 7, 2, 3);
    b.rect(26, 16, 7, 2, 3);
    b.ellipse(17, 17, 8, 8, 2);
    b.ellipse(17, 17, 5, 5, 1);
    b.ellipse(15, 15, 2, 2, 4);
    return b;
}

Bitmap starArt() {
    Bitmap b(9, 9);
    b.rect(4, 0, 1, 9, 1);
    b.rect(0, 4, 9, 1, 1);
    b.set(2, 2, 2);
    b.set(6, 2, 2);
    b.set(2, 6, 2);
    b.set(6, 6, 2);
    b.set(4, 4, 1);
    return b;
}

Bitmap peakArt(int which) {
    Bitmap b(which == 0 ? 100 : 84, which == 2 ? 36 : 50);
    if (which == 0) {
        b.poly({{0, 49}, {0, 30}, {18, 26}, {36, 8}, {54, 22}, {74, 14}, {100, 28}, {100, 49}}, 2);
        b.poly({{0, 49}, {16, 36}, {34, 22}, {52, 30}, {78, 20}, {100, 34}, {100, 49}}, 3);
        b.poly({{30, 16}, {36, 8}, {44, 18}}, 1);
    } else if (which == 1) {
        b.poly({{0, 49}, {0, 28}, {22, 18}, {40, 6}, {58, 20}, {84, 16}, {84, 49}}, 2);
        b.poly({{0, 49}, {18, 32}, {40, 18}, {66, 26}, {84, 22}, {84, 49}}, 3);
        b.poly({{34, 14}, {40, 6}, {48, 16}}, 1);
    } else {
        b.poly({{0, 35}, {0, 20}, {16, 14}, {32, 4}, {50, 16}, {68, 10}, {84, 18}, {84, 35}}, 2);
        b.poly({{0, 35}, {14, 24}, {32, 14}, {54, 20}, {84, 16}, {84, 35}}, 3);
        b.poly({{26, 10}, {32, 4}, {40, 12}}, 1);
    }
    b.outline(5, false);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 13), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 5), gs::rgb4(10, 7, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 8), gs::rgb4(3, 8, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(13, 9, 6), gs::rgb4(2, 3, 6), gs::rgb4(5, 7, 11), gs::rgb4(1, 1, 2), ink,
                          gs::rgb4(13, 10, 3), gs::rgb4(15, 14, 7), gs::rgb4(7, 5, 3), gs::rgb4(4, 4, 5), 0, 0, 0, 0,
                          0, 0});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(10, 2, 1), gs::rgb4(14, 5, 1), gs::rgb4(15, 9, 2), gs::rgb4(15, 13, 4),
                           gs::rgb4(15, 15, 12), gs::rgb4(6, 1, 0), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_EMBER, {0, gs::rgb4(6, 1, 1), gs::rgb4(10, 3, 1), gs::rgb4(12, 5, 2), gs::rgb4(10, 6, 2),
                            gs::rgb4(8, 4, 2), gs::rgb4(3, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(3, 3, 4), gs::rgb4(7, 7, 8), gs::rgb4(10, 10, 11), gs::rgb4(4, 3, 3), ink,
                           gs::rgb4(2, 2, 3), gs::rgb4(12, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 8), gs::rgb4(10, 10, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0});
    setPal(vdp, PAL_WIND, {0, gs::rgb4(8, 10, 13), gs::rgb4(12, 14, 15), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(14, 14, 12), gs::rgb4(8, 8, 9), gs::rgb4(5, 5, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 12, 4), gs::rgb4(15, 8, 2), gs::rgb4(15, 14, 8), 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 4), gs::rgb4(10, 9, 8), gs::rgb4(6, 5, 4), ink,
                            gs::rgb4(12, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_MOUNT, {0, gs::rgb4(13, 13, 14), gs::rgb4(4, 5, 7), gs::rgb4(2, 3, 5), 0, ink, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0});

    const uint16_t road[16] = {
        0,
        gs::rgb4(2, 2, 3),
        gs::rgb4(1, 1, 2),
        gs::rgb4(3, 3, 4),
        gs::rgb4(5, 5, 4),
        gs::rgb4(3, 3, 2),
        gs::rgb4(4, 4, 5),
        gs::rgb4(3, 3, 4),
        gs::rgb4(8, 7, 6),
        gs::rgb4(5, 5, 6),
        gs::rgb4(6, 6, 5),
        gs::rgb4(2, 3, 5),
        gs::rgb4(1, 2, 4),
        gs::rgb4(3, 4, 6),
        gs::rgb4(9, 8, 7),
        gs::rgb4(7, 6, 5),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    art.warden[0] = gs::uploadMipped(vdp, wardenArt(0));
    art.warden[1] = gs::uploadMipped(vdp, wardenArt(1));
    art.post = gs::uploadMipped(vdp, postArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.smoke[0] = gs::uploadMipped(vdp, smokeArt(0));
    art.smoke[1] = gs::uploadMipped(vdp, smokeArt(1));
    art.gust = gs::uploadMipped(vdp, gustArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cairn = gs::uploadMipped(vdp, cairnArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.peak[0] = gs::uploadMipped(vdp, peakArt(0));
    art.peak[1] = gs::uploadMipped(vdp, peakArt(1));
    art.peak[2] = gs::uploadMipped(vdp, peakArt(2));
    loadFont(vdp, art);
    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace rdawn
