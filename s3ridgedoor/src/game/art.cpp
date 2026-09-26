#include "game/art.h"

#include <initializer_list>
#include <string>

namespace rdoor {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void legs(Bitmap& b, int x, int y, int step) {
    int liftL = step ? 0 : 3;
    int liftR = step ? 3 : 0;
    b.rect(float(x), float(y + liftL), 6, float(12 - liftL), 3);
    b.rect(float(x + 12), float(y + liftR), 6, float(12 - liftR), 3);
    b.rect(float(x - 1), float(y + 10 - liftL / 2), 8, 4, 4);
    b.rect(float(x + 11), float(y + 10 - liftR / 2), 8, 4, 4);
}

Bitmap keeperArt(int brace) {
    Bitmap b(46, 76);
    b.rect(16, 2, 14, 5, 8);
    b.rect(14, 5, 18, 3, 2);
    b.ellipse(23, 14, 7, 7, 6);
    b.rect(18, 12, 10, 3, 8);
    b.rect(20, 15, 2, 2, 5);
    b.rect(25, 15, 2, 2, 5);
    if (brace) {
        b.poly({{23, 20}, {8, 30}, {7, 54}, {39, 54}, {38, 30}}, 1);
        b.poly({{23, 26}, {16, 32}, {15, 52}, {24, 52}, {24, 32}}, 2);
        b.rect(4, 28, 8, 7, 1);
        b.rect(34, 28, 8, 7, 1);
        b.rect(2, 30, 6, 5, 6);
        b.rect(38, 30, 6, 5, 6);
    } else {
        b.poly({{23, 20}, {10, 28}, {8, 52}, {38, 52}, {36, 28}}, 1);
        b.poly({{23, 26}, {17, 32}, {16, 50}, {25, 50}, {24, 32}}, 2);
        b.rect(6, 30, 6, 14, 1);
        b.rect(34, 30, 6, 14, 1);
        b.rect(5, 42, 7, 5, 6);
        b.rect(34, 42, 7, 5, 6);
    }
    b.rect(12, 40, 22, 4, 4);
    b.rect(20, 39, 6, 5, 7);
    legs(b, 14, 52, brace ? 0 : 1);
    b.outline(5, false);
    return b;
}

Bitmap raiderArt(int step) {
    Bitmap b(44, 70);
    b.ellipse(22, 10, 8, 6, 7);
    b.rect(15, 12, 14, 5, 8);
    b.ellipse(22, 18, 6, 6, 6);
    b.rect(18, 18, 3, 2, 5);
    b.rect(24, 18, 3, 2, 5);
    b.rect(20, 22, 4, 2, 9);
    b.poly({{22, 24}, {10, 32}, {9, 50}, {35, 50}, {34, 32}}, 1);
    b.poly({{22, 28}, {16, 34}, {16, 48}, {24, 48}, {23, 34}}, 2);
    b.rect(6, 32, 28, 6, 3);
    b.rect(8, 33, 24, 2, 7);
    b.rect(4, 30, 6, 8, 6);
    b.rect(34, 30, 6, 8, 6);
    legs(b, 13, 50, step);
    b.outline(5, false);
    return b;
}

Bitmap leafArt() {
    Bitmap b(40, 96);
    b.rect(4, 4, 32, 88, 2);
    b.rect(6, 6, 8, 84, 1);
    b.rect(16, 6, 8, 84, 2);
    b.rect(26, 6, 8, 84, 3);
    b.rect(4, 22, 32, 6, 4);
    b.rect(4, 62, 32, 6, 4);
    b.rect(8, 24, 3, 3, 6);
    b.rect(18, 24, 3, 3, 6);
    b.rect(28, 24, 3, 3, 6);
    b.rect(8, 64, 3, 3, 6);
    b.rect(18, 64, 3, 3, 6);
    b.rect(28, 64, 3, 3, 6);
    b.ellipse(32, 44, 4, 5, 4);
    b.ellipse(32, 44, 2, 2, 5);
    b.rect(7, 40, 2, 18, 3);
    b.rect(17, 12, 2, 8, 3);
    b.rect(27, 74, 2, 10, 8);
    b.outline(5, false);
    return b;
}

Bitmap lintelArt() {
    Bitmap b(96, 28);
    b.rect(2, 8, 92, 16, 2);
    b.rect(0, 6, 96, 5, 1);
    b.rect(8, 14, 80, 6, 3);
    b.rect(20, 16, 10, 3, 4);
    b.rect(66, 17, 12, 3, 6);
    b.rect(40, 4, 16, 6, 1);
    b.outline(5, false);
    return b;
}

Bitmap jambArt() {
    Bitmap b(28, 96);
    b.rect(4, 2, 20, 92, 2);
    b.rect(6, 4, 16, 88, 1);
    for (int i = 0; i < 5; i++) b.rect(5, float(8 + i * 17), 18, 4, 3);
    b.rect(8, 30, 6, 3, 4);
    b.rect(14, 64, 7, 3, 6);
    b.outline(5, false);
    return b;
}

Bitmap sillArt() {
    Bitmap b(120, 22);
    b.rect(2, 4, 116, 14, 2);
    b.rect(0, 2, 120, 5, 1);
    b.rect(8, 10, 24, 4, 3);
    b.rect(70, 11, 30, 3, 3);
    b.rect(40, 8, 14, 3, 6);
    b.outline(5, false);
    return b;
}

Bitmap barArt() {
    Bitmap b(80, 14);
    b.rect(2, 4, 76, 6, 4);
    b.rect(4, 5, 72, 2, 6);
    b.rect(8, 3, 5, 8, 4);
    b.rect(66, 3, 5, 8, 4);
    b.outline(5, false);
    return b;
}

Bitmap wedgeArt() {
    Bitmap b(36, 18);
    b.poly({{2, 14}, {30, 14}, {30, 4}, {8, 14}}, 2);
    b.poly({{6, 13}, {28, 13}, {28, 7}}, 1);
    b.rect(4, 13, 26, 3, 3);
    b.outline(5, false);
    return b;
}

Bitmap pennantArt(int frame) {
    Bitmap b(36, 22);
    int dy = frame ? 3 : 0;
    b.rect(2, 2, 3, 18, 2);
    b.poly({{5, 4}, {32, 8 + dy}, {28, 16 + dy}, {5, 14}}, 1);
    b.poly({{8, 7}, {24, 10 + dy}, {22, 13 + dy}, {8, 12}}, 2);
    return b;
}

Bitmap rockArt() {
    Bitmap b(32, 22);
    b.poly({{3, 18}, {8, 8}, {18, 4}, {29, 12}, {24, 19}, {10, 20}}, 2);
    b.poly({{10, 12}, {16, 7}, {24, 12}, {18, 16}}, 1);
    b.rect(8, 15, 8, 3, 4);
    b.outline(5, false);
    return b;
}

Bitmap dustArt() {
    Bitmap b(24, 16);
    b.ellipse(12, 9, 10, 5, 3);
    b.ellipse(10, 8, 5, 3, 4);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(48, 14);
    b.ellipse(24, 7, 20, 4, 1);
    return b;
}

Bitmap sunArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 7, 7, 1);
    b.ellipse(14, 14, 4, 4, 5);
    b.rect(13, 1, 2, 4, 2);
    b.rect(13, 23, 2, 4, 2);
    b.rect(1, 13, 4, 2, 2);
    b.rect(23, 13, 4, 2, 2);
    b.line(5, 5, 8, 8, 2, 1);
    b.line(23, 5, 20, 8, 2, 1);
    b.line(5, 23, 8, 20, 2, 1);
    b.line(23, 23, 20, 20, 2, 1);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(72, 24);
    b.ellipse(22, 14, 14, 7, 3);
    b.ellipse(40, 11, 16, 8, 4);
    b.ellipse(56, 15, 12, 6, 3);
    return b;
}

Bitmap peakNear() {
    Bitmap b(100, 64);
    b.poly({{0, 63}, {0, 40}, {18, 32}, {36, 8}, {54, 28}, {74, 16}, {100, 36}, {100, 63}}, 2);
    b.poly({{0, 63}, {16, 46}, {34, 24}, {52, 36}, {76, 26}, {100, 44}, {100, 63}}, 3);
    b.poly({{30, 16}, {36, 8}, {44, 18}}, 1);
    b.poly({{68, 22}, {74, 16}, {82, 24}}, 1);
    return b;
}

Bitmap peakFar() {
    Bitmap b(110, 48);
    b.poly({{0, 47}, {0, 28}, {24, 18}, {48, 6}, {70, 20}, {92, 12}, {110, 26}, {110, 47}}, 2);
    b.poly({{0, 47}, {20, 32}, {46, 16}, {72, 26}, {94, 18}, {110, 30}, {110, 47}}, 4);
    b.poly({{42, 14}, {48, 6}, {56, 16}}, 1);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(9, 10, 12), gs::rgb4(5, 6, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 4), gs::rgb4(10, 7, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 8), gs::rgb4(3, 8, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           gs::rgb4(1, 3, 1)});

    setPal(vdp, PAL_YOU, {0, gs::rgb4(12, 8, 3), gs::rgb4(7, 4, 2), gs::rgb4(3, 2, 1), gs::rgb4(2, 2, 2), ink,
                          gs::rgb4(13, 9, 6), gs::rgb4(14, 12, 6), gs::rgb4(8, 3, 2), gs::rgb4(10, 4, 3), 0, 0, 0, 0,
                          0, ink});
    setPal(vdp, PAL_FOE, {0, gs::rgb4(5, 7, 5), gs::rgb4(3, 4, 3), gs::rgb4(8, 6, 3), gs::rgb4(2, 2, 2), ink,
                          gs::rgb4(12, 9, 7), gs::rgb4(8, 9, 10), gs::rgb4(4, 5, 6), gs::rgb4(10, 3, 3), 0, 0, 0, 0, 0,
                          ink});
    setPal(vdp, PAL_DOOR, {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(6, 7, 8), ink,
                           gs::rgb4(13, 13, 12), gs::rgb4(4, 6, 3), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(11, 10, 8), gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 4), gs::rgb4(5, 6, 3), ink,
                            gs::rgb4(12, 12, 9), gs::rgb4(9, 8, 6), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 13, 6), gs::rgb4(15, 10, 4), gs::rgb4(12, 10, 7), gs::rgb4(15, 14, 11),
                         gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MOUNT, {0, gs::rgb4(14, 13, 12), gs::rgb4(5, 6, 8), gs::rgb4(3, 3, 5), gs::rgb4(4, 4, 6), 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t field[16] = {
        0,
        gs::rgb4(5, 5, 3),
        gs::rgb4(3, 3, 2),
        gs::rgb4(6, 6, 4),
        gs::rgb4(7, 6, 4),
        gs::rgb4(4, 4, 3),
        gs::rgb4(8, 7, 5),
        gs::rgb4(5, 4, 3),
        gs::rgb4(11, 10, 8),
        gs::rgb4(4, 4, 3),
        gs::rgb4(9, 8, 6),
        gs::rgb4(3, 4, 6),
        gs::rgb4(2, 3, 5),
        gs::rgb4(4, 5, 6),
        gs::rgb4(10, 9, 6),
        gs::rgb4(12, 11, 9),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_FIELD * 16 + i, field[i]);

    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.raider[0] = gs::uploadMipped(vdp, raiderArt(0));
    art.raider[1] = gs::uploadMipped(vdp, raiderArt(1));
    art.leaf = gs::uploadMipped(vdp, leafArt());
    art.lintel = gs::uploadMipped(vdp, lintelArt());
    art.jamb = gs::uploadMipped(vdp, jambArt());
    art.sill = gs::uploadMipped(vdp, sillArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.wedge = gs::uploadMipped(vdp, wedgeArt());
    art.pennant[0] = gs::uploadMipped(vdp, pennantArt(0));
    art.pennant[1] = gs::uploadMipped(vdp, pennantArt(1));
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.peak[0] = gs::uploadMipped(vdp, peakNear());
    art.peak[1] = gs::uploadMipped(vdp, peakFar());
    loadFont(vdp, art);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
}

}  // namespace rdoor
