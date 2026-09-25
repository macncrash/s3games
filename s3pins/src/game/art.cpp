#include "game/art.h"

#include <cmath>
#include <string>

namespace pins {
namespace {

constexpr float TAU = 6.2831853f;

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap pinArt() {
    Bitmap b(40, 96);
    b.ellipse(20, 64, 14, 22, 1);
    b.ellipse(15, 56, 6, 12, 2);
    b.ellipse(20, 34, 6, 16, 1);
    b.rect(13, 28, 14, 4, 3);
    b.rect(14, 36, 12, 3, 4);
    b.ellipse(20, 16, 8, 9, 1);
    b.ellipse(17, 13, 3, 3, 6);
    b.ellipse(20, 70, 8, 6, 2);
    b.outline(5, false);
    return b;
}

Bitmap pinFlat() {
    Bitmap b(96, 36);
    b.ellipse(46, 18, 26, 12, 1);
    b.ellipse(40, 14, 10, 6, 6);
    b.ellipse(74, 18, 12, 9, 1);
    b.rect(66, 12, 6, 12, 3);
    b.rect(60, 14, 5, 8, 4);
    b.ellipse(86, 18, 7, 7, 1);
    b.ellipse(84, 16, 2, 2, 6);
    b.outline(5, false);
    return b;
}

Bitmap ballArt(int frame) {
    Bitmap b(48, 48);
    b.ellipse(24, 24, 20, 20, 1);
    b.ellipse(18, 18, 9, 8, 2);
    b.ellipse(16, 16, 4, 3, 4);
    float ang = frame * TAU / 4.0f;
    for (int i = 0; i < 3; i++) {
        float a = ang + i * TAU / 3.0f;
        b.ellipse(24 + std::cos(a) * 7.0f, 24 + std::sin(a) * 6.0f, 2.6f, 2.6f, 3);
    }
    b.outline(5, false);
    return b;
}

Bitmap bowlerArt(int pose) {
    Bitmap b(64, 88);
    int y = pose == 1 ? 5 : 0;
    b.ellipse(32, 14 + y, 9, 10, 4);
    b.ellipse(32, 16 + y, 7, 8, 3);
    b.ellipse(24, 17 + y, 2, 3, 3);
    b.ellipse(40, 17 + y, 2, 3, 3);
    b.rect(29, 22 + y, 6, 5, 3);
    b.poly({{18, 28 + y}, {46, 28 + y}, {50, 52 + y}, {14, 52 + y}}, 1);
    b.poly({{26, 30 + y}, {38, 30 + y}, {36, 46 + y}, {28, 46 + y}}, 2);
    b.rect(18, 50 + y, 28, 4, 8);
    int spread = pose == 2 ? 3 : 0;
    b.rect(22 - spread, 54 + y, 8, 22, 5);
    b.rect(34 + spread, 54 + y, 8, 22, 5);
    b.ellipse(24 - spread, 78 + y, 8, 4, 6);
    b.ellipse(40 + spread, 78 + y, 8, 4, 7);
    if (pose == 0) {
        b.rect(10, 30 + y, 6, 22, 1);
        b.rect(48, 30 + y, 6, 22, 1);
        b.ellipse(13, 54 + y, 4, 4, 3);
        b.ellipse(51, 54 + y, 4, 4, 3);
    } else if (pose == 1) {
        b.rect(10, 32 + y, 6, 16, 1);
        b.ellipse(13, 50 + y, 3, 3, 3);
        b.poly({{46, 30 + y}, {60, 42 + y}, {54, 50 + y}, {42, 36 + y}}, 1);
        b.ellipse(58, 48 + y, 4, 4, 3);
    } else if (pose == 2) {
        b.rect(10, 30 + y, 6, 16, 1);
        b.ellipse(13, 48 + y, 3, 3, 3);
        b.poly({{40, 28 + y}, {54, 12 + y}, {58, 18 + y}, {44, 34 + y}}, 1);
        b.ellipse(56, 14 + y, 4, 4, 3);
    } else {
        b.rect(10, 32 + y, 6, 14, 1);
        b.poly({{34, 26 + y}, {48, 6 + y}, {54, 12 + y}, {40, 30 + y}}, 1);
        b.ellipse(50, 8 + y, 4, 3, 3);
    }
    b.outline(9, false);
    return b;
}

Bitmap arrowArt() {
    Bitmap b(16, 22);
    b.poly({{8, 1}, {15, 20}, {1, 20}}, 1);
    b.poly({{8, 7}, {12, 18}, {4, 18}}, 2);
    return b;
}

Bitmap dotArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

Bitmap foulArt() {
    Bitmap b(16, 4);
    b.rect(0, 0, 16, 4, 1);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 5, 1);
    return b;
}

Bitmap glowArt() {
    Bitmap b(64, 32);
    b.ellipse(32, 18, 30, 12, 1);
    b.ellipse(32, 18, 16, 7, 2);
    return b;
}

Bitmap lampArt() {
    Bitmap b(24, 36);
    b.rect(11, 0, 2, 8, 3);
    b.poly({{3, 8}, {21, 8}, {23, 16}, {1, 16}}, 3);
    b.ellipse(12, 22, 10, 8, 1);
    b.ellipse(12, 21, 5, 4, 2);
    return b;
}

Bitmap returnArt() {
    Bitmap b(28, 56);
    b.rect(4, 2, 20, 50, 1);
    b.rect(6, 4, 16, 46, 2);
    b.ellipse(14, 16, 6, 6, 3);
    b.rect(8, 28, 12, 3, 4);
    b.rect(8, 36, 12, 3, 4);
    b.outline(5, false);
    return b;
}

Bitmap curtainArt() {
    Bitmap b(96, 40);
    b.rect(0, 0, 96, 8, 2);
    b.rect(0, 8, 96, 32, 1);
    b.rect(4, 12, 88, 24, 4);
    for (int i = 0; i < 5; i++) b.rect(8 + i * 18, 16, 8, 14, 1);
    b.outline(3, false);
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
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t ink = gs::rgb4(15, 15, 15);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(10, 10, 12), gs::rgb4(15, 14, 8), gs::rgb4(15, 4, 4), gs::rgb4(5, 5, 7), 0, 0,
                          0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 8, 1), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, shadow});
    setPal(vdp, PAL_PIN, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 11, 9), gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1),
                          gs::rgb4(3, 3, 4), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(8, 1, 3), gs::rgb4(4, 0, 2), gs::rgb4(1, 1, 1), gs::rgb4(15, 12, 12),
                           gs::rgb4(2, 0, 1), gs::rgb4(12, 4, 6), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOWLER,
           {0, gs::rgb4(13, 10, 2), gs::rgb4(8, 6, 1), gs::rgb4(15, 11, 8), gs::rgb4(2, 1, 1), gs::rgb4(2, 2, 6),
            gs::rgb4(14, 14, 15), gs::rgb4(12, 2, 2), gs::rgb4(4, 3, 2), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 3), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(6, 15, 6), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 15, 13), gs::rgb4(3, 3, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, shadow});
    setPal(vdp, PAL_RETURN, {0, gs::rgb4(4, 4, 6), gs::rgb4(2, 2, 3), gs::rgb4(8, 1, 3), gs::rgb4(10, 8, 3),
                             gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GLOW, {0, gs::rgb4(10, 7, 3), gs::rgb4(15, 13, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CURTAIN, {0, gs::rgb4(5, 1, 2), gs::rgb4(14, 12, 8), gs::rgb4(2, 1, 2), gs::rgb4(3, 0, 1), 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t lane[16] = {
        0,
        gs::rgb4(6, 1, 2), gs::rgb4(3, 0, 1), gs::rgb4(9, 2, 3),
        gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 6),
        gs::rgb4(12, 8, 3), gs::rgb4(9, 6, 2),
        gs::rgb4(7, 5, 2), gs::rgb4(8, 5, 2), gs::rgb4(11, 8, 4),
        gs::rgb4(2, 4, 8), gs::rgb4(3, 6, 10), gs::rgb4(6, 9, 12),
        gs::rgb4(15, 14, 11), gs::rgb4(14, 10, 5),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_LANE * 16 + i, lane[i]);
    vdp.setFogColor(gs::rgb4(2, 1, 2));

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
    art.ret = gs::uploadMipped(vdp, returnArt());
    art.curtain = gs::uploadMipped(vdp, curtainArt());
}

}  // namespace pins
