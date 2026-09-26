#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace pinsbell {
namespace {

constexpr float TAU = 6.2831853f;

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void shadow(gs::VDP& vdp, int pal) { vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2)); }

Bitmap pinArt() {
    Bitmap b(40, 88);
    b.ellipse(20, 62, 13, 18, 1);
    b.ellipse(16, 54, 6, 10, 2);
    b.ellipse(20, 36, 6, 14, 1);
    b.rect(12, 32, 16, 5, 3);
    b.rect(14, 40, 12, 3, 4);
    b.ellipse(20, 16, 9, 10, 1);
    b.ellipse(16, 13, 3, 3, 6);
    b.ellipse(20, 70, 7, 5, 2);
    b.outline(5);
    return b;
}

Bitmap pinFlat() {
    Bitmap b(88, 32);
    b.ellipse(40, 16, 24, 11, 1);
    b.ellipse(34, 13, 10, 5, 6);
    b.ellipse(64, 16, 10, 8, 1);
    b.rect(58, 11, 6, 10, 3);
    b.ellipse(78, 16, 7, 7, 1);
    b.ellipse(76, 14, 2, 2, 6);
    b.outline(5);
    return b;
}

Bitmap ballArt(int frame) {
    Bitmap b(40, 40);
    b.ellipse(20, 20, 16, 16, 1);
    b.ellipse(14, 14, 7, 6, 2);
    b.ellipse(13, 13, 3, 2, 6);
    float ang = frame * TAU / 3.0f;
    for (int i = 0; i < 3; i++) {
        float a = ang + i * TAU / 3.0f;
        b.ellipse(20 + std::cos(a) * 5.5f, 21 + std::sin(a) * 4.5f, 2.2f, 2.2f, 3);
    }
    b.outline(5);
    return b;
}

Bitmap bowlerArt(int pose) {
    Bitmap b(64, 96);
    b.ellipse(32, 16, 12, 11, 4);
    b.ellipse(22, 20, 3, 4, 3);
    b.ellipse(42, 20, 3, 4, 3);
    b.rect(28, 26, 8, 6, 3);
    b.poly({{12, 34}, {52, 34}, {48, 64}, {16, 64}}, 1);
    b.rect(22, 34, 20, 5, 8);
    b.ellipse(32, 48, 5, 6, 7);
    b.ellipse(32, 46, 2, 3, 6);
    b.rect(16, 62, 32, 4, 2);
    int step = pose == 3 ? 4 : 0;
    b.rect(20 - step, 66, 9, 20, 5);
    b.rect(35 + step, 66, 9, 20, 5);
    b.ellipse(24 - step, 88, 8, 4, 9);
    b.ellipse(40 + step, 88, 8, 4, 9);
    if (pose == 0) {
        b.rect(8, 36, 6, 22, 1);
        b.rect(50, 36, 6, 22, 1);
        b.ellipse(11, 60, 4, 4, 3);
        b.ellipse(53, 60, 4, 4, 3);
    } else if (pose == 1) {
        b.rect(8, 36, 6, 18, 1);
        b.ellipse(11, 56, 4, 4, 3);
        b.poly({{48, 40}, {60, 58}, {54, 62}, {44, 46}}, 1);
        b.ellipse(58, 62, 4, 4, 3);
    } else if (pose == 2) {
        b.rect(8, 38, 6, 16, 1);
        b.ellipse(11, 56, 3, 3, 3);
        b.poly({{44, 36}, {56, 12}, {60, 16}, {48, 40}}, 1);
        b.ellipse(58, 14, 4, 4, 3);
    } else {
        b.rect(8, 38, 6, 16, 1);
        b.rect(50, 40, 6, 16, 1);
        b.ellipse(11, 56, 3, 3, 3);
        b.ellipse(53, 58, 3, 3, 3);
    }
    b.outline(10);
    return b;
}

Bitmap bellArt() {
    Bitmap b(40, 48);
    b.rect(18, 2, 4, 8, 2);
    b.ellipse(20, 26, 15, 16, 1);
    b.ellipse(16, 20, 5, 6, 6);
    b.ellipse(20, 30, 8, 9, 4);
    b.ellipse(20, 38, 14, 4, 3);
    b.rect(17, 0, 6, 4, 2);
    b.outline(5);
    return b;
}

Bitmap clapperArt() {
    Bitmap b(10, 14);
    b.rect(4, 0, 2, 5, 2);
    b.ellipse(5, 9, 4, 4, 3);
    return b;
}

Bitmap yokeArt() {
    Bitmap b(52, 12);
    b.rect(2, 4, 48, 4, 2);
    b.rect(8, 8, 2, 4, 1);
    b.rect(42, 8, 2, 4, 1);
    b.rect(24, 2, 4, 4, 3);
    return b;
}

Bitmap arrowArt() {
    Bitmap b(14, 20);
    b.poly({{7, 1}, {13, 18}, {1, 18}}, 1);
    b.poly({{7, 6}, {10, 16}, {4, 16}}, 2);
    return b;
}

Bitmap dotArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

Bitmap foulArt() {
    Bitmap b(32, 4);
    b.rect(0, 0, 32, 4, 1);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 1);
    return b;
}

Bitmap curtainArt() {
    Bitmap b(96, 36);
    b.rect(0, 0, 96, 6, 2);
    b.rect(0, 6, 96, 30, 1);
    b.rect(6, 12, 84, 16, 4);
    for (int i = 0; i < 4; i++) b.rect(10 + i * 22, 14, 8, 10, 1);
    b.outline(3);
    return b;
}

Bitmap lampArt() {
    Bitmap b(20, 28);
    b.rect(9, 0, 2, 6, 3);
    b.poly({{3, 6}, {17, 6}, {19, 12}, {1, 12}}, 3);
    b.ellipse(10, 18, 8, 7, 1);
    b.ellipse(10, 17, 4, 3, 2);
    return b;
}

Bitmap postArt() {
    Bitmap b(18, 48);
    b.rect(6, 4, 6, 40, 1);
    b.rect(4, 0, 10, 6, 2);
    b.rect(7, 16, 4, 3, 3);
    b.ellipse(9, 44, 7, 3, 4);
    b.outline(5);
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
    const uint16_t ink = gs::rgb4(15, 15, 15);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(9, 9, 11), gs::rgb4(15, 13, 8), gs::rgb4(15, 4, 4), gs::rgb4(4, 4, 6)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 2), gs::rgb4(15, 8, 1), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_PIN, {0, gs::rgb4(15, 14, 12), gs::rgb4(11, 9, 7), gs::rgb4(2, 3, 8), gs::rgb4(1, 1, 4),
                          gs::rgb4(2, 1, 2), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(1, 6, 3), gs::rgb4(0, 3, 1), gs::rgb4(1, 1, 1), 0, gs::rgb4(0, 2, 1),
                           gs::rgb4(8, 13, 8)});
    setPal(vdp, PAL_BOWLER,
           {0, gs::rgb4(2, 3, 8), gs::rgb4(1, 2, 4), gs::rgb4(14, 10, 7), gs::rgb4(2, 1, 1), gs::rgb4(4, 4, 6), 0,
            gs::rgb4(15, 12, 3), gs::rgb4(14, 14, 15), gs::rgb4(8, 8, 9), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(14, 10, 2), gs::rgb4(9, 6, 1), gs::rgb4(15, 14, 6), gs::rgb4(4, 3, 2),
                           gs::rgb4(3, 2, 1), gs::rgb4(15, 15, 11)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(4, 14, 5), gs::rgb4(12, 15, 10)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_DEAD, {0, gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 3), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_CURTAIN, {0, gs::rgb4(5, 1, 2), gs::rgb4(12, 9, 3), gs::rgb4(2, 0, 1), gs::rgb4(3, 1, 2)});
    setPal(vdp, PAL_HEAD, {0, gs::rgb4(15, 14, 12), gs::rgb4(11, 9, 7), gs::rgb4(15, 12, 2), gs::rgb4(10, 7, 1),
                           gs::rgb4(2, 1, 2), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 12, 8)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(3, 2, 4), gs::rgb4(14, 11, 3), gs::rgb4(15, 13, 6), gs::rgb4(2, 2, 3),
                           gs::rgb4(1, 1, 2)});
    for (int p : {PAL_HUD, PAL_AMBER, PAL_GOLD, PAL_GREEN, PAL_ALERT}) shadow(vdp, p);

    const uint16_t lane[16] = {
        0,
        gs::rgb4(3, 1, 2), gs::rgb4(2, 1, 1), gs::rgb4(4, 2, 2),
        gs::rgb4(1, 2, 4), gs::rgb4(2, 3, 5),
        gs::rgb4(13, 9, 4), gs::rgb4(11, 7, 3),
        gs::rgb4(8, 5, 2), gs::rgb4(9, 6, 3), gs::rgb4(14, 11, 6),
        gs::rgb4(1, 1, 3), gs::rgb4(2, 2, 4), gs::rgb4(3, 3, 6),
        gs::rgb4(2, 1, 3), gs::rgb4(14, 12, 8),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_LANE * 16 + i, lane[i]);
    vdp.setFogColor(gs::rgb4(1, 1, 2));

    loadFont(vdp, art);
    art.pin = gs::uploadMipped(vdp, pinArt());
    art.pinFlat = gs::uploadMipped(vdp, pinFlat());
    for (int i = 0; i < 3; i++) art.ball[i] = gs::uploadMipped(vdp, ballArt(i));
    for (int i = 0; i < 4; i++) art.bowler[i] = gs::uploadMipped(vdp, bowlerArt(i));
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.clapper = gs::uploadMipped(vdp, clapperArt());
    art.yoke = gs::uploadMipped(vdp, yokeArt());
    art.arrow = gs::uploadMipped(vdp, arrowArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.foul = gs::uploadMipped(vdp, foulArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.curtain = gs::uploadMipped(vdp, curtainArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.post = gs::uploadMipped(vdp, postArt());
}

}  // namespace pinsbell
