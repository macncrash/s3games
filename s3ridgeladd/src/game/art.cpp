#include "game/art.h"

#include <initializer_list>
#include <string>

namespace rladd {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void boot(Bitmap& b, float x, float y, int step) {
    int liftL = step ? 0 : 4;
    int liftR = step ? 4 : 0;
    b.rect(x, y + liftL, 6, 14 - liftL, 4);
    b.rect(x + 10, y + liftR, 6, 14 - liftR, 4);
    b.rect(x - 1, y + 12, 8, 4, 10);
    b.rect(x + 9, y + 12, 8, 4, 10);
    b.rect(x - 1, y + 12, 8, 2, 11);
    b.rect(x + 9, y + 12, 8, 2, 11);
}

Bitmap walker(int step) {
    Bitmap b(46, 84);
    b.ellipse(23, 11, 9, 7, 7);
    b.rect(15, 13, 16, 5, 7);
    b.rect(15, 16, 16, 2, 13);
    b.ellipse(23, 22, 6, 7, 6);
    b.rect(19, 20, 3, 2, 5);
    b.rect(25, 20, 3, 2, 5);
    b.rect(21, 25, 4, 2, 12);
    b.poly({{23, 28}, {9, 40}, {11, 60}, {35, 60}, {37, 40}}, 2);
    b.poly({{23, 31}, {15, 40}, {16, 58}, {24, 58}}, 1);
    b.line(14, 34, 31, 56, 8, 2.4f);
    b.ellipse(30, 48, 5, 4, 8);
    b.ellipse(30, 48, 2, 2, 2);
    b.line(14, 36, 4, 52, 9, 2.2f);
    b.poly({{1, 48}, {8, 46}, {7, 56}, {2, 54}}, 9);
    b.rect(16, 44, 12, 3, 11);
    boot(b, 14, 58, step);
    b.outline(5, false);
    return b;
}

Bitmap jumper() {
    Bitmap b(50, 68);
    b.ellipse(26, 10, 8, 6, 7);
    b.rect(18, 12, 16, 4, 7);
    b.ellipse(26, 20, 6, 6, 6);
    b.rect(22, 18, 3, 2, 5);
    b.rect(28, 18, 3, 2, 5);
    b.poly({{26, 26}, {8, 34}, {12, 48}, {40, 48}, {44, 32}}, 2);
    b.poly({{26, 28}, {16, 34}, {18, 46}, {28, 46}}, 1);
    b.line(16, 30, 34, 46, 8, 2.2f);
    b.poly({{12, 30}, {2, 24}, {6, 20}, {16, 32}}, 2);
    b.poly({{38, 30}, {48, 22}, {44, 18}, {34, 32}}, 2);
    b.rect(12, 46, 8, 6, 4);
    b.rect(28, 44, 8, 6, 4);
    b.rect(10, 50, 10, 4, 10);
    b.rect(26, 48, 12, 4, 10);
    b.ellipse(32, 40, 4, 3, 8);
    b.outline(5, false);
    return b;
}

Bitmap climber() {
    Bitmap b(40, 90);
    b.line(14, 36, 10, 10, 2, 3.2f);
    b.line(26, 36, 30, 10, 2, 3.2f);
    b.ellipse(9, 8, 4, 4, 6);
    b.ellipse(31, 8, 4, 4, 6);
    b.ellipse(20, 22, 8, 6, 7);
    b.rect(13, 24, 14, 4, 7);
    b.ellipse(20, 32, 6, 6, 6);
    b.rect(16, 30, 3, 2, 5);
    b.rect(22, 30, 3, 2, 5);
    b.poly({{20, 38}, {10, 48}, {12, 70}, {28, 70}, {30, 46}}, 2);
    b.poly({{20, 40}, {14, 48}, {15, 68}, {22, 68}}, 1);
    b.line(14, 46, 26, 66, 8, 2.2f);
    b.rect(14, 68, 5, 14, 4);
    b.rect(22, 66, 5, 16, 4);
    b.rect(12, 80, 8, 4, 10);
    b.rect(20, 80, 8, 4, 10);
    b.outline(5, false);
    return b;
}

Bitmap ladderArt() {
    Bitmap b(30, 108);
    b.rect(3, 2, 5, 104, 2);
    b.rect(22, 2, 5, 104, 2);
    b.rect(4, 2, 2, 104, 1);
    b.rect(23, 2, 2, 104, 1);
    for (int i = 0; i < 9; ++i) {
        float y = 8.f + float(i) * 11.f;
        b.rect(6, y, 18, 3, 4);
        b.rect(6, y, 18, 1, 1);
        b.set(8, int(y) + 1, 5);
        b.set(20, int(y) + 1, 5);
    }
    b.rect(2, 0, 26, 4, 3);
    b.outline(7, false);
    return b;
}

Bitmap ragArt() {
    Bitmap b(18, 16);
    b.poly({{2, 2}, {16, 5}, {12, 14}, {3, 10}}, 1);
    b.poly({{4, 4}, {12, 6}, {9, 11}}, 2);
    b.outline(3, false);
    return b;
}

Bitmap cliffArt() {
    Bitmap b(84, 120);
    b.poly({{6, 118}, {2, 62}, {16, 28}, {30, 8}, {46, 2}, {62, 22}, {78, 58}, {74, 118}}, 2);
    b.poly({{18, 118}, {14, 70}, {28, 36}, {42, 22}, {52, 40}, {48, 118}}, 3);
    b.poly({{30, 18}, {40, 4}, {52, 16}, {44, 28}}, 7);
    b.poly({{34, 24}, {42, 12}, {48, 22}}, 1);
    b.rect(36, 78, 14, 28, 3);
    b.rect(40, 84, 6, 16, 5);
    return b;
}

Bitmap postArt() {
    Bitmap b(24, 52);
    b.rect(9, 8, 5, 42, 2);
    b.rect(10, 10, 2, 36, 1);
    b.rect(7, 46, 9, 4, 3);
    b.poly({{14, 8}, {22, 12}, {20, 20}, {14, 16}}, 6);
    b.poly({{15, 10}, {20, 13}, {18, 17}}, 1);
    b.outline(5, false);
    return b;
}

Bitmap toothArt() {
    Bitmap b(28, 36);
    b.poly({{2, 34}, {6, 18}, {12, 8}, {16, 20}, {22, 6}, {26, 34}}, 2);
    b.poly({{8, 34}, {12, 16}, {16, 22}, {18, 34}}, 3);
    b.poly({{18, 14}, {22, 6}, {24, 16}}, 1);
    b.outline(5, false);
    return b;
}

Bitmap stoneArt(int squat) {
    Bitmap b(26, 26);
    float ry = squat ? 8.f : 10.f;
    b.ellipse(13, 14, 11, ry, 2);
    b.ellipse(12, 13, 7, ry * 0.62f, 1);
    b.poly({{7, 14}, {12, 6}, {16, 13}}, 3);
    b.ellipse(16, 16, 3, 2, 7);
    b.outline(5, false);
    return b;
}

Bitmap dustArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 11, 8, 3);
    b.ellipse(12, 13, 6, 4, 5);
    b.ellipse(11, 12, 2, 2, 4);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(44, 14);
    b.ellipse(22, 7, 18, 5, 1);
    return b;
}

Bitmap sunArt() {
    Bitmap b(30, 30);
    b.ellipse(15, 15, 12, 12, 2);
    b.ellipse(15, 15, 8, 8, 1);
    b.ellipse(12, 12, 3, 3, 6);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(72, 28);
    b.ellipse(20, 16, 16, 8, 4);
    b.ellipse(38, 13, 18, 10, 5);
    b.ellipse(56, 17, 13, 7, 4);
    return b;
}

Bitmap peakSpire() {
    Bitmap b(80, 52);
    b.poly({{0, 51}, {0, 34}, {18, 28}, {34, 8}, {42, 22}, {60, 16}, {80, 30}, {80, 51}}, 3);
    b.poly({{0, 51}, {10, 40}, {28, 32}, {40, 18}, {58, 28}, {80, 36}, {80, 51}}, 4);
    b.poly({{30, 16}, {34, 8}, {40, 16}}, 1);
    return b;
}

Bitmap peakMesa() {
    Bitmap b(96, 40);
    b.poly({{0, 39}, {0, 24}, {16, 20}, {28, 12}, {58, 12}, {74, 20}, {96, 16}, {96, 39}}, 3);
    b.poly({{0, 39}, {12, 28}, {30, 20}, {60, 18}, {80, 24}, {96, 22}, {96, 39}}, 4);
    b.poly({{28, 14}, {32, 8}, {54, 8}, {58, 14}}, 1);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 9, 11), gs::rgb4(4, 5, 7), 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 6), gs::rgb4(10, 8, 3), 0, 0, gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 4), gs::rgb4(10, 3, 2), gs::rgb4(4, 1, 1), 0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 8), gs::rgb4(3, 8, 4), 0, 0, gs::rgb4(1, 3, 1)});
    vdp.setColor(PAL_HUD * 16 + 15, gs::rgb4(2, 2, 4));
    vdp.setColor(PAL_AMBER * 16 + 15, gs::rgb4(4, 2, 1));
    vdp.setColor(PAL_ALERT * 16 + 15, gs::rgb4(3, 0, 0));
    vdp.setColor(PAL_GOOD * 16 + 15, gs::rgb4(1, 3, 1));

    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(14, 6, 3), gs::rgb4(11, 3, 2), gs::rgb4(6, 2, 2), gs::rgb4(3, 3, 6), ink, gs::rgb4(14, 10, 7),
            gs::rgb4(3, 4, 6), gs::rgb4(13, 11, 6), gs::rgb4(12, 13, 14), gs::rgb4(2, 2, 3), gs::rgb4(13, 12, 11),
            gs::rgb4(12, 7, 6), gs::rgb4(8, 7, 5)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(14, 10, 5), gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(12, 9, 4), gs::rgb4(8, 9, 10),
            gs::rgb4(13, 3, 2), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(12, 12, 13), gs::rgb4(8, 8, 9), gs::rgb4(5, 5, 6), gs::rgb4(6, 7, 5), ink, gs::rgb4(11, 6, 3),
            gs::rgb4(14, 14, 15)});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 10, 5), gs::rgb4(11, 10, 8), gs::rgb4(13, 13, 14), gs::rgb4(9, 9, 11),
            gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_MOUNT, {0, gs::rgb4(14, 14, 15), gs::rgb4(9, 10, 12), gs::rgb4(5, 6, 8), gs::rgb4(3, 4, 6)});

    const uint16_t field[16] = {
        0,
        gs::rgb4(6, 7, 8),
        gs::rgb4(4, 4, 6),
        gs::rgb4(5, 6, 6),
        gs::rgb4(5, 6, 5),
        gs::rgb4(3, 4, 4),
        gs::rgb4(8, 8, 9),
        gs::rgb4(6, 6, 7),
        gs::rgb4(10, 9, 8),
        gs::rgb4(5, 5, 6),
        gs::rgb4(11, 10, 9),
        gs::rgb4(2, 3, 6),
        gs::rgb4(2, 2, 5),
        gs::rgb4(3, 4, 7),
        gs::rgb4(12, 10, 7),
        gs::rgb4(12, 12, 13),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_FIELD * 16 + i, field[i]);

    art.walk[0] = gs::uploadMipped(vdp, walker(0));
    art.walk[1] = gs::uploadMipped(vdp, walker(1));
    art.jump = gs::uploadMipped(vdp, jumper());
    art.climb = gs::uploadMipped(vdp, climber());
    art.ladder = gs::uploadMipped(vdp, ladderArt());
    art.rag = gs::uploadMipped(vdp, ragArt());
    art.cliff = gs::uploadMipped(vdp, cliffArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.tooth = gs::uploadMipped(vdp, toothArt());
    art.stone[0] = gs::uploadMipped(vdp, stoneArt(0));
    art.stone[1] = gs::uploadMipped(vdp, stoneArt(1));
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.peak[0] = gs::uploadMipped(vdp, peakSpire());
    art.peak[1] = gs::uploadMipped(vdp, peakMesa());
    loadFont(vdp, art);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace rladd
