#include "game/art.h"

#include <cmath>
#include <string>

namespace pinsgold {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

Bitmap pinArt() {
    Bitmap b(34, 76);
    b.ellipse(17, 62, 11, 10, 1);
    b.ellipse(17, 64, 7, 4, 5);
    b.ellipse(17, 46, 12, 14, 1);
    b.ellipse(13, 42, 4, 7, 2);
    b.poly({{17, 36}, {22, 42}, {17, 48}, {12, 42}}, 3);
    b.rect(11, 34, 12, 3, 4);
    b.ellipse(17, 24, 4, 9, 1);
    b.ellipse(16, 22, 2, 4, 2);
    b.ellipse(17, 12, 6, 7, 1);
    b.ellipse(15, 10, 2, 2, 6);
    b.outline(15, false);
    return b;
}

Bitmap pinFlat() {
    Bitmap b(72, 26);
    b.ellipse(30, 14, 18, 9, 1);
    b.ellipse(26, 12, 6, 4, 2);
    b.poly({{30, 10}, {34, 14}, {30, 18}, {26, 14}}, 3);
    b.ellipse(50, 14, 8, 8, 1);
    b.ellipse(48, 12, 2, 2, 6);
    b.ellipse(14, 14, 7, 6, 5);
    b.outline(15, false);
    return b;
}

Bitmap ballArt(int frame) {
    Bitmap b(40, 40);
    b.ellipse(20, 20, 16, 16, 1);
    b.ellipse(15, 15, 6, 5, 2);
    b.ellipse(14, 14, 2, 2, 5);
    b.ellipse(20, 22, 4, 4, 3);
    b.ellipse(20, 22, 2, 2, 1);
    const float TAU = 6.2831853f;
    float ang = frame * TAU / 4.0f;
    for (int i = 0; i < 3; i++) {
        float a = ang + i * TAU / 3.0f;
        b.ellipse(20 + std::cos(a) * 6.0f, 18 + std::sin(a) * 5.0f, 2.2f, 2.2f, 4);
    }
    b.outline(15, false);
    return b;
}

Bitmap bowlerArt(int pose) {
    Bitmap b(56, 84);
    int y = pose == 1 ? 6 : 0;
    b.ellipse(28, 12 + y, 10, 7, 4);
    b.rect(18, 14 + y, 20, 4, 4);
    b.ellipse(28, 20 + y, 7, 8, 3);
    b.set(25, 18 + y, 4);
    b.set(31, 18 + y, 4);
    b.poly({{16, 28 + y}, {40, 28 + y}, {44, 52 + y}, {12, 52 + y}}, 1);
    b.poly({{24, 30 + y}, {32, 30 + y}, {31, 48 + y}, {25, 48 + y}}, 2);
    b.rect(16, 50 + y, 24, 3, 4);
    int s = pose == 3 ? 2 : 0;
    b.rect(18 - s, 53 + y, 8, 18, 5);
    b.rect(30 + s, 53 + y, 8, 18, 5);
    b.ellipse(22 - s, 72 + y, 7, 3, 6);
    b.ellipse(34 + s, 72 + y, 7, 3, 6);
    if (pose == 0) {
        b.rect(8, 30 + y, 7, 16, 1);
        b.rect(41, 30 + y, 7, 14, 1);
        b.ellipse(11, 47 + y, 4, 4, 3);
        b.ellipse(45, 45 + y, 4, 4, 3);
    } else if (pose == 1) {
        b.rect(8, 34 + y, 7, 12, 1);
        b.ellipse(11, 47 + y, 4, 4, 3);
        b.poly({{40, 36 + y}, {52, 48 + y}, {48, 54 + y}, {36, 42 + y}}, 1);
        b.ellipse(50, 52 + y, 4, 4, 3);
    } else if (pose == 2) {
        b.rect(8, 32 + y, 7, 12, 1);
        b.ellipse(11, 45 + y, 4, 4, 3);
        b.poly({{36, 30 + y}, {52, 14 + y}, {56, 20 + y}, {40, 36 + y}}, 1);
        b.ellipse(54, 16 + y, 4, 4, 3);
    } else {
        b.rect(10, 30 + y, 6, 12, 1);
        b.poly({{30, 26 + y}, {46, 6 + y}, {50, 12 + y}, {34, 30 + y}}, 1);
        b.ellipse(48, 8 + y, 4, 3, 3);
    }
    b.outline(9, false);
    return b;
}

Bitmap arrowArt() {
    Bitmap b(16, 20);
    b.poly({{8, 1}, {15, 18}, {1, 18}}, 1);
    b.poly({{8, 7}, {12, 16}, {4, 16}}, 2);
    return b;
}

Bitmap dotArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

Bitmap foulArt() {
    Bitmap b(8, 4);
    b.rect(0, 0, 8, 4, 1);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 1);
    return b;
}

Bitmap glowArt() {
    Bitmap b(48, 20);
    b.ellipse(24, 12, 22, 8, 1);
    b.ellipse(24, 12, 10, 4, 2);
    return b;
}

Bitmap lampArt() {
    Bitmap b(22, 32);
    b.rect(10, 0, 2, 8, 3);
    b.poly({{2, 8}, {20, 8}, {21, 14}, {1, 14}}, 3);
    b.ellipse(11, 20, 9, 7, 1);
    b.ellipse(11, 19, 4, 3, 2);
    return b;
}

Bitmap curtainArt() {
    Bitmap b(96, 44);
    b.rect(0, 0, 96, 8, 3);
    b.rect(0, 8, 96, 36, 1);
    for (int i = 0; i < 6; i++) b.rect(4 + i * 16, 12, 8, 28, 2);
    b.poly({{48, 16}, {58, 26}, {48, 36}, {38, 26}}, 3);
    b.outline(4, false);
    return b;
}

Bitmap machineArt() {
    Bitmap b(22, 48);
    b.rect(2, 2, 18, 44, 6);
    b.rect(5, 6, 12, 36, 7);
    b.rect(7, 14, 8, 3, 3);
    b.rect(7, 24, 8, 3, 3);
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
    const uint16_t shadow = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 2), gs::rgb4(11, 8, 1), gs::rgb4(15, 15, 11), gs::rgb4(13, 9, 1),
                           gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 15, 13), gs::rgb4(12, 11, 9), gs::rgb4(13, 2, 3), gs::rgb4(8, 1, 2),
                            gs::rgb4(6, 5, 4), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6), gs::rgb4(15, 12, 3), gs::rgb4(1, 1, 1),
                           gs::rgb4(14, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOWLER,
           {0, gs::rgb4(2, 8, 9), gs::rgb4(15, 12, 3), gs::rgb4(15, 12, 9), gs::rgb4(2, 1, 1), gs::rgb4(2, 2, 6),
            gs::rgb4(14, 14, 12), 0, 0, gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3), gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(5, 15, 7), gs::rgb4(12, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 13), gs::rgb4(4, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, shadow});
    setPal(vdp, PAL_DECK, {0, gs::rgb4(6, 1, 2), gs::rgb4(3, 0, 1), gs::rgb4(15, 12, 3), gs::rgb4(1, 0, 1),
                           gs::rgb4(8, 4, 1), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GLOW, {0, gs::rgb4(12, 8, 2), gs::rgb4(15, 13, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t lane[16] = {
        0,
        gs::rgb4(2, 2, 3),
        gs::rgb4(1, 1, 2),
        gs::rgb4(3, 2, 3),
        gs::rgb4(4, 1, 1),
        gs::rgb4(6, 2, 2),
        gs::rgb4(13, 9, 4),
        gs::rgb4(11, 7, 3),
        gs::rgb4(8, 5, 2),
        gs::rgb4(9, 6, 2),
        gs::rgb4(10, 7, 3),
        0,
        0,
        0,
        gs::rgb4(15, 14, 11),
        gs::rgb4(8, 5, 2),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_LANE * 16 + i, lane[i]);
    vdp.setFogColor(gs::rgb4(2, 1, 1));

    loadFont(vdp, art);
    art.pin = gs::uploadMipped(vdp, pinArt());
    art.pinFlat = gs::uploadMipped(vdp, pinFlat());
    for (int i = 0; i < 4; i++) {
        art.ball[i] = gs::uploadMipped(vdp, ballArt(i));
        art.bowler[i] = gs::uploadMipped(vdp, bowlerArt(i));
    }
    art.arrow = gs::uploadMipped(vdp, arrowArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.foul = gs::uploadMipped(vdp, foulArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.curtain = gs::uploadMipped(vdp, curtainArt());
    art.machine = gs::uploadMipped(vdp, machineArt());
}

}  // namespace pinsgold
