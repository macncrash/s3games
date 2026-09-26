#include "game/art.h"

#include <initializer_list>
#include <string>

namespace rwell {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void legs(Bitmap& b, int x, int y, int step, int c) {
    int a = step ? 0 : 4;
    int d = step ? 4 : 0;
    b.rect(float(x), float(y + a), 6, float(14 - a), c);
    b.rect(float(x + 10), float(y + d), 6, float(14 - d), c);
    b.rect(float(x - 1), float(y + 12), 8, 3, 5);
    b.rect(float(x + 9), float(y + 12), 8, 3, 5);
}

Bitmap wardenArt(int thrust) {
    Bitmap b(40, 76);
    b.ellipse(20, 18, 9, 10, 9);
    b.ellipse(20, 20, 5, 5, 1);
    b.poly({{20, 24}, {8, 34}, {7, 52}, {33, 52}, {32, 34}}, 2);
    b.poly({{20, 28}, {12, 36}, {12, 50}, {21, 50}}, 3);
    b.rect(12, 48, 16, 3, 8);
    b.rect(11, 51, 7, 16, 3);
    b.rect(22, 51, 7, 16, 3);
    b.rect(10, 65, 9, 4, 4);
    b.rect(21, 65, 9, 4, 4);
    if (thrust) {
        b.rect(18, 1, 3, 36, 6);
        b.poly({{15, 10}, {20, 0}, {25, 10}}, 7);
        b.rect(8, 34, 12, 4, 2);
    } else {
        b.rect(28, 16, 3, 34, 6);
        b.poly({{26, 18}, {30, 10}, {34, 18}}, 7);
        b.rect(24, 38, 8, 4, 2);
    }
    b.outline(5, false);
    return b;
}

Bitmap runnerArt(int step) {
    Bitmap b(34, 64);
    b.ellipse(17, 12, 7, 7, 1);
    b.rect(11, 6, 12, 5, 3);
    b.poly({{17, 18}, {7, 26}, {6, 40}, {28, 40}, {27, 26}}, 2);
    b.rect(9, 26, 16, 9, 6);
    b.ellipse(17, 26, 7, 3, 7);
    b.rect(7, 24, 3, 8, 8);
    b.rect(24, 24, 3, 8, 8);
    legs(b, 9, 40, step, 4);
    b.outline(5, false);
    return b;
}

Bitmap clubArt(int step) {
    Bitmap b(42, 68);
    b.ellipse(18, 13, 7, 7, 1);
    b.rect(12, 7, 12, 5, 3);
    b.poly({{18, 18}, {8, 28}, {7, 42}, {30, 42}, {28, 26}}, 2);
    if (step) {
        b.rect(26, 6, 5, 20, 8);
        b.ellipse(28, 6, 6, 6, 4);
    } else {
        b.rect(28, 22, 5, 20, 8);
        b.ellipse(30, 20, 6, 6, 4);
    }
    legs(b, 10, 42, step, 4);
    b.outline(5, false);
    return b;
}

Bitmap ramArt(int cracked) {
    Bitmap b(78, 54);
    b.ellipse(16, 12, 6, 6, 1);
    b.ellipse(62, 12, 6, 6, 1);
    b.rect(12, 16, 10, 8, 8);
    b.rect(56, 16, 10, 8, 8);
    b.rect(16, 18, 46, 14, 2);
    b.rect(16, 18, 46, 4, 3);
    b.ellipse(40, 36, 16, 12, 6);
    b.ellipse(40, 36, 9, 6, 4);
    if (!cracked) b.ellipse(40, 35, 4, 3, 7);
    else b.line(30, 30, 50, 44, 5, 2);
    b.ellipse(22, 46, 6, 6, 5);
    b.ellipse(56, 46, 6, 6, 5);
    b.ellipse(22, 46, 2, 2, 7);
    b.ellipse(56, 46, 2, 2, 7);
    b.outline(5, false);
    return b;
}

Bitmap wellArt() {
    Bitmap b(64, 74);
    b.rect(8, 8, 5, 30, 8);
    b.rect(51, 8, 5, 30, 8);
    b.rect(8, 4, 48, 6, 9);
    b.rect(30, 10, 3, 12, 10);
    b.ellipse(31, 22, 6, 4, 9);
    b.ellipse(31, 21, 3, 2, 7);
    b.ellipse(32, 32, 22, 10, 1);
    b.ellipse(32, 32, 14, 6, 6);
    b.ellipse(28, 30, 4, 2, 7);
    b.rect(12, 34, 40, 24, 2);
    b.rect(12, 34, 9, 24, 3);
    b.rect(14, 40, 36, 3, 1);
    b.rect(14, 48, 36, 3, 1);
    b.rect(16, 44, 8, 3, 4);
    b.ellipse(32, 60, 24, 8, 2);
    b.ellipse(32, 62, 24, 5, 3);
    b.outline(5, false);
    return b;
}

Bitmap rubbleArt() {
    Bitmap b(68, 40);
    b.ellipse(22, 26, 14, 8, 2);
    b.ellipse(44, 28, 16, 9, 3);
    b.ellipse(32, 16, 9, 7, 1);
    b.rect(14, 8, 6, 10, 8);
    b.ellipse(36, 22, 7, 3, 6);
    b.outline(5, false);
    return b;
}

Bitmap crackArt() {
    Bitmap b(22, 30);
    b.line(4, 2, 11, 14, 3, 2);
    b.line(11, 14, 6, 28, 3, 2);
    b.line(11, 14, 20, 20, 5, 2);
    return b;
}

Bitmap bucketArt() {
    Bitmap b(16, 16);
    b.rect(3, 5, 10, 8, 9);
    b.ellipse(8, 5, 5, 3, 6);
    b.ellipse(7, 5, 2, 1, 7);
    b.rect(3, 2, 2, 5, 10);
    b.rect(11, 2, 2, 5, 10);
    b.outline(5, false);
    return b;
}

Bitmap stakeArt() {
    Bitmap b(12, 28);
    b.rect(5, 4, 3, 18, 4);
    b.poly({{3, 4}, {6, 0}, {9, 4}}, 2);
    b.rect(2, 22, 8, 4, 3);
    b.outline(5, false);
    return b;
}

Bitmap cairnArt() {
    Bitmap b(28, 42);
    b.ellipse(14, 34, 12, 6, 2);
    b.rect(6, 24, 16, 10, 1);
    b.rect(8, 16, 12, 10, 2);
    b.rect(10, 9, 8, 8, 3);
    b.rect(12, 3, 4, 7, 1);
    b.outline(5, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 58);
    b.rect(6, 12, 4, 40, 3);
    b.rect(4, 50, 8, 4, 4);
    b.ellipse(8, 8, 6, 6, 1);
    b.ellipse(8, 8, 3, 3, 2);
    b.outline(5, false);
    return b;
}

Bitmap pennantArt() {
    Bitmap b(22, 40);
    b.rect(3, 2, 2, 36, 4);
    b.poly({{5, 6}, {19, 13}, {5, 20}}, 2);
    b.poly({{5, 9}, {14, 13}, {5, 17}}, 3);
    b.outline(5, false);
    return b;
}

Bitmap dustArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 11, 8, 3);
    b.ellipse(13, 13, 6, 4, 1);
    return b;
}

Bitmap shockArt() {
    Bitmap b(26, 26);
    b.poly({{13, 1}, {16, 10}, {25, 11}, {17, 15}, {21, 25}, {13, 18}, {5, 25}, {9, 15}, {1, 11}, {10, 10}}, 2);
    b.poly({{13, 7}, {15, 11}, {19, 12}, {15, 14}, {17, 20}, {13, 16}, {9, 20}, {11, 14}, {7, 12}, {11, 11}}, 1);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(48, 14);
    b.ellipse(24, 7, 20, 5, 1);
    return b;
}

Bitmap sunArt() {
    Bitmap b(26, 26);
    b.ellipse(13, 13, 10, 10, 2);
    b.ellipse(13, 13, 6, 6, 6);
    b.ellipse(10, 10, 2, 2, 1);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(70, 24);
    b.ellipse(20, 14, 16, 8, 3);
    b.ellipse(40, 12, 18, 9, 1);
    b.ellipse(56, 15, 12, 6, 3);
    return b;
}

Bitmap peakNear() {
    Bitmap b(90, 52);
    b.poly({{0, 51}, {0, 34}, {16, 28}, {34, 10}, {52, 26}, {70, 16}, {90, 32}, {90, 51}}, 2);
    b.poly({{0, 51}, {14, 38}, {32, 24}, {50, 32}, {72, 22}, {90, 36}, {90, 51}}, 3);
    b.poly({{28, 18}, {34, 10}, {42, 20}}, 1);
    b.outline(5, false);
    return b;
}

Bitmap peakFar() {
    Bitmap b(110, 40);
    b.poly({{0, 39}, {0, 24}, {22, 16}, {44, 6}, {66, 18}, {88, 10}, {110, 22}, {110, 39}}, 2);
    b.poly({{0, 39}, {18, 26}, {42, 16}, {70, 22}, {96, 14}, {110, 26}, {110, 39}}, 3);
    b.poly({{38, 12}, {44, 6}, {52, 14}}, 1);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(9, 9, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 5), gs::rgb4(10, 7, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(13, 8, 5), gs::rgb4(6, 7, 9), gs::rgb4(3, 4, 6), gs::rgb4(2, 2, 2), ink,
                          gs::rgb4(11, 8, 4), gs::rgb4(14, 14, 15), gs::rgb4(13, 11, 5), gs::rgb4(4, 5, 7), 0, 0, 0, 0,
                          0, 0});
    setPal(vdp, PAL_FOE, {0, gs::rgb4(13, 8, 5), gs::rgb4(13, 3, 2), gs::rgb4(4, 2, 1), gs::rgb4(6, 4, 3), ink,
                          gs::rgb4(10, 7, 3), gs::rgb4(3, 9, 12), gs::rgb4(7, 7, 8), 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_RAM, {0, gs::rgb4(13, 8, 5), gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(6, 6, 7), ink,
                          gs::rgb4(12, 12, 13), gs::rgb4(15, 15, 15), gs::rgb4(11, 3, 2), 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(12, 11, 9), gs::rgb4(8, 8, 7), gs::rgb4(4, 4, 4), gs::rgb4(3, 6, 3), ink,
                            gs::rgb4(2, 6, 10), gs::rgb4(5, 12, 14), gs::rgb4(8, 5, 2), gs::rgb4(11, 8, 4),
                            gs::rgb4(12, 10, 7), 0, 0, 0, 0, 0});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(15, 14, 6), gs::rgb4(11, 8, 5), gs::rgb4(7, 5, 3), gs::rgb4(4, 3, 2), ink,
                           gs::rgb4(4, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FX, {0, gs::rgb4(14, 13, 10), gs::rgb4(15, 12, 5), gs::rgb4(8, 7, 5), gs::rgb4(5, 4, 3), ink,
                         gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_MOUNT, {0, gs::rgb4(13, 13, 14), gs::rgb4(5, 6, 8), gs::rgb4(3, 3, 5), 0, ink, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(14, 12, 8), gs::rgb4(11, 2, 2), gs::rgb4(6, 1, 1), gs::rgb4(5, 4, 3), ink, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0});

    const uint16_t field[16] = {
        0,
        gs::rgb4(6, 6, 5),
        gs::rgb4(3, 3, 2),
        gs::rgb4(5, 5, 4),
        gs::rgb4(7, 6, 4),
        gs::rgb4(4, 4, 3),
        gs::rgb4(8, 7, 6),
        gs::rgb4(5, 5, 4),
        gs::rgb4(11, 10, 8),
        gs::rgb4(6, 5, 4),
        gs::rgb4(9, 8, 7),
        gs::rgb4(3, 4, 6),
        gs::rgb4(2, 3, 5),
        gs::rgb4(4, 5, 7),
        gs::rgb4(12, 11, 8),
        gs::rgb4(13, 12, 10),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_FIELD * 16 + i, field[i]);

    art.warden[0] = gs::uploadMipped(vdp, wardenArt(0));
    art.warden[1] = gs::uploadMipped(vdp, wardenArt(1));
    art.runner[0] = gs::uploadMipped(vdp, runnerArt(0));
    art.runner[1] = gs::uploadMipped(vdp, runnerArt(1));
    art.club[0] = gs::uploadMipped(vdp, clubArt(0));
    art.club[1] = gs::uploadMipped(vdp, clubArt(1));
    art.ram[0] = gs::uploadMipped(vdp, ramArt(0));
    art.ram[1] = gs::uploadMipped(vdp, ramArt(1));
    art.well = gs::uploadMipped(vdp, wellArt());
    art.rubble = gs::uploadMipped(vdp, rubbleArt());
    art.crack = gs::uploadMipped(vdp, crackArt());
    art.bucket = gs::uploadMipped(vdp, bucketArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.cairn = gs::uploadMipped(vdp, cairnArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shock = gs::uploadMipped(vdp, shockArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.peak[0] = gs::uploadMipped(vdp, peakNear());
    art.peak[1] = gs::uploadMipped(vdp, peakFar());
    loadFont(vdp, art);
    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace rwell
