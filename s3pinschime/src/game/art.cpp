#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace pinschime {
namespace {

constexpr int kDial = 96;
constexpr float kTau = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void shaft(gs::Bitmap& b, float theta, float len, float tail, float w, int col) {
    const float cx = kDial * 0.5f, cy = kDial * 0.5f;
    float dx = std::sin(theta), dy = -std::cos(theta);
    float px = -dy, py = dx;
    float x0 = cx - dx * tail, y0 = cy - dy * tail;
    float x1 = cx + dx * len, y1 = cy + dy * len;
    b.poly({{x0 + px * w * 0.4f, y0 + py * w * 0.4f},
            {x1 + px * w, y1 + py * w},
            {x1 - px * w, y1 - py * w},
            {x0 - px * w * 0.4f, y0 - py * w * 0.4f}},
           col);
}

void paintHand(gs::Bitmap& b, int kind, int step) {
    float theta = step * (kTau / 60.f);
    if (kind == 0) {
        shaft(b, theta, 26, 8, 5.2f, 1);
        shaft(b, theta, 24, 6, 2.6f, 2);
    } else if (kind == 1) {
        shaft(b, theta, 36, 10, 2.4f, 2);
        shaft(b, theta, 34, 8, 1.1f, 3);
    } else {
        shaft(b, theta, 40, 12, 1.15f, 4);
        float dx = std::sin(theta), dy = -std::cos(theta);
        b.ellipse(48 - dx * 10.f, 48 - dy * 10.f, 2.4f, 2.4f, 4);
    }
}

gs::Bitmap faceArt() {
    gs::Bitmap b(kDial, kDial);
    b.ellipse(48, 48, 46, 46, 1);
    b.ellipse(48, 48, 42, 42, 2);
    b.ellipse(48, 48, 38, 38, 3);
    b.ellipse(48, 47, 36, 36, 4);
    b.ellipse(46, 44, 28, 22, 5);
    b.ellipse(48, 48, 36, 36, 4);
    for (int i = 0; i < 60; i++) {
        float a = i * (kTau / 60.f);
        float s = std::sin(a), c = std::cos(a);
        bool hour = i % 5 == 0;
        float r0 = hour ? 28.f : 32.f;
        int col = (i == 0) ? 7 : 6;
        b.line(48 + s * r0, 48 - c * r0, 48 + s * 35.f, 48 - c * 35.f, col, hour ? 2.4f : 1.f);
    }
    gs::Bitmap num = gs::textBitmap("12", gs::TextStyle{2, 7, 0, 0, 1});
    b.blit(num, 48 - num.w / 2, 16);
    b.ellipse(48, 48, 3.2f, 3.2f, 8);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(kDial, kDial);
    b.ellipse(48, 48, 46, 46, 1);
    b.ellipse(48, 48, 40, 40, 2);
    b.ellipse(48, 48, 36, 36, 0);
    return b;
}

gs::Bitmap capArt() {
    gs::Bitmap b(24, 24);
    b.ellipse(12, 12, 8, 8, 1);
    b.ellipse(12, 12, 4, 4, 2);
    return b;
}

gs::Bitmap pinArt() {
    gs::Bitmap b(32, 72);
    b.ellipse(16, 60, 10, 8, 1);
    b.ellipse(16, 62, 6, 3, 5);
    b.ellipse(16, 44, 11, 13, 1);
    b.ellipse(13, 40, 4, 6, 2);
    b.rect(10, 34, 12, 4, 3);
    b.rect(10, 30, 12, 3, 4);
    b.ellipse(16, 22, 4, 8, 1);
    b.ellipse(16, 12, 6, 6, 1);
    b.ellipse(14, 10, 2, 2, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap pinFlatArt() {
    gs::Bitmap b(68, 24);
    b.ellipse(28, 13, 16, 8, 1);
    b.ellipse(24, 11, 5, 3, 2);
    b.rect(22, 8, 12, 4, 3);
    b.ellipse(48, 13, 8, 7, 1);
    b.ellipse(46, 11, 2, 2, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap bellPinArt() {
    gs::Bitmap b(36, 72);
    b.poly({{18, 8}, {28, 22}, {32, 58}, {4, 58}, {8, 22}}, 1);
    b.poly({{18, 14}, {24, 24}, {26, 52}, {18, 52}}, 2);
    b.ellipse(18, 8, 6, 5, 3);
    b.rect(12, 56, 12, 5, 5);
    b.ellipse(18, 40, 3, 5, 4);
    b.ellipse(18, 48, 2, 2, 6);
    b.rect(16, 4, 4, 6, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap bellFlatArt() {
    gs::Bitmap b(70, 26);
    b.ellipse(30, 14, 18, 9, 1);
    b.ellipse(26, 12, 6, 4, 2);
    b.ellipse(48, 14, 9, 8, 3);
    b.ellipse(18, 14, 5, 4, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap ballArt(int frame) {
    gs::Bitmap b(40, 40);
    b.ellipse(20, 20, 16, 16, 1);
    b.ellipse(15, 14, 6, 5, 2);
    b.ellipse(14, 13, 2, 2, 6);
    const float ang = frame * kTau / 4.f;
    for (int i = 0; i < 3; i++) {
        float a = ang + i * kTau / 3.f;
        b.ellipse(20 + std::cos(a) * 6.f, 19 + std::sin(a) * 5.f, 2.1f, 2.1f, 3);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap bowlerArt(int pose) {
    gs::Bitmap b(52, 80);
    int y = pose == 1 ? 4 : 0;
    b.ellipse(26, 10 + y, 11, 5, 4);
    b.rect(18, 12 + y, 16, 3, 4);
    b.ellipse(26, 18 + y, 7, 7, 3);
    b.set(23, 17 + y, 5);
    b.set(29, 17 + y, 5);
    b.poly({{14, 26 + y}, {38, 26 + y}, {42, 50 + y}, {10, 50 + y}}, 1);
    b.poly({{22, 28 + y}, {30, 28 + y}, {29, 46 + y}, {23, 46 + y}}, 2);
    b.rect(16, 48 + y, 20, 3, 6);
    int s = pose == 3 ? 2 : 0;
    b.rect(16 - s, 51 + y, 8, 16, 7);
    b.rect(28 + s, 51 + y, 8, 16, 7);
    b.ellipse(20 - s, 68 + y, 6, 3, 8);
    b.ellipse(32 + s, 68 + y, 6, 3, 8);
    if (pose == 0) {
        b.rect(6, 30 + y, 7, 14, 1);
        b.ellipse(9, 46 + y, 4, 4, 3);
        b.rect(39, 28 + y, 6, 12, 1);
        b.ellipse(42, 42 + y, 4, 4, 3);
    } else if (pose == 1) {
        b.rect(6, 34 + y, 7, 12, 1);
        b.ellipse(9, 47 + y, 4, 4, 3);
        b.poly({{36, 36 + y}, {48, 50 + y}, {44, 54 + y}, {32, 40 + y}}, 1);
        b.ellipse(46, 52 + y, 4, 4, 3);
    } else if (pose == 2) {
        b.rect(8, 32 + y, 6, 12, 1);
        b.ellipse(10, 45 + y, 4, 4, 3);
        b.poly({{34, 28 + y}, {48, 12 + y}, {52, 18 + y}, {38, 32 + y}}, 1);
        b.ellipse(50, 14 + y, 4, 4, 3);
    } else {
        b.rect(10, 30 + y, 6, 10, 1);
        b.poly({{30, 24 + y}, {46, 6 + y}, {50, 12 + y}, {34, 28 + y}}, 1);
        b.ellipse(48, 8 + y, 4, 3, 3);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap arrowArt() {
    gs::Bitmap b(16, 18);
    b.poly({{8, 1}, {15, 16}, {1, 16}}, 1);
    b.poly({{8, 6}, {12, 14}, {4, 14}}, 2);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

gs::Bitmap foulArt() {
    gs::Bitmap b(8, 4);
    b.rect(0, 0, 8, 4, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(20, 36);
    b.rect(9, 0, 2, 8, 3);
    b.poly({{2, 8}, {18, 8}, {19, 14}, {1, 14}}, 3);
    b.ellipse(10, 22, 8, 8, 1);
    b.ellipse(10, 20, 3, 3, 2);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(88, 120);
    b.rect(16, 18, 56, 100, 1);
    b.poly({{12, 22}, {44, 2}, {76, 22}}, 2);
    b.rect(20, 28, 48, 36, 3);
    b.ellipse(44, 46, 16, 16, 4);
    b.rect(38, 78, 12, 40, 5);
    b.rect(22, 96, 44, 4, 6);
    for (int i = 0; i < 5; i++) b.rect(24 + i * 10, 40, 2, 70, 7);
    b.outline(15, false);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(28, 32);
    b.rect(12, 0, 4, 5, 3);
    b.poly({{4, 8}, {24, 8}, {26, 24}, {2, 24}}, 1);
    b.ellipse(14, 24, 12, 5, 1);
    b.ellipse(14, 16, 3, 4, 2);
    b.ellipse(14, 22, 2, 2, 4);
    b.outline(15, false);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
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
    setPal(vdp, PAL_INK, {0, gs::rgb4(14, 14, 15), gs::rgb4(8, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(12, 8, 1), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, shadow});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 14, 12), gs::rgb4(11, 10, 8), gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1),
                            gs::rgb4(7, 6, 5), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(12, 8, 2), gs::rgb4(7, 4, 1), gs::rgb4(15, 13, 6), gs::rgb4(3, 2, 1),
                           gs::rgb4(5, 3, 1), gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(7, 1, 2), gs::rgb4(12, 3, 3), gs::rgb4(2, 1, 1), gs::rgb4(15, 12, 8), 0,
                           gs::rgb4(15, 13, 12), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOWLER,
           {0, gs::rgb4(2, 3, 8), gs::rgb4(14, 11, 5), gs::rgb4(14, 11, 8), gs::rgb4(3, 2, 1), gs::rgb4(1, 1, 1),
            gs::rgb4(10, 7, 3), gs::rgb4(1, 1, 3), gs::rgb4(4, 2, 1), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3), gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(5, 15, 7), gs::rgb4(12, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(5, 3, 2), gs::rgb4(13, 9, 2), gs::rgb4(8, 5, 2), gs::rgb4(14, 12, 8),
                           gs::rgb4(8, 7, 5), gs::rgb4(2, 2, 2), gs::rgb4(15, 14, 8), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0,
                           0, shadow});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(6, 4, 1), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 12), gs::rgb4(15, 3, 2),
                           gs::rgb4(12, 9, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TOWER, {0, gs::rgb4(4, 3, 5), gs::rgb4(6, 5, 7), gs::rgb4(3, 2, 4), gs::rgb4(8, 7, 9),
                            gs::rgb4(2, 2, 3), gs::rgb4(10, 8, 4), gs::rgb4(3, 2, 3), 0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t lane[16] = {
        0,
        gs::rgb4(2, 2, 4),
        gs::rgb4(1, 1, 3),
        gs::rgb4(4, 3, 6),
        gs::rgb4(2, 1, 2),
        gs::rgb4(4, 2, 2),
        gs::rgb4(13, 9, 4),
        gs::rgb4(10, 7, 3),
        gs::rgb4(8, 5, 2),
        gs::rgb4(7, 4, 2),
        gs::rgb4(14, 11, 6),
        0,
        0,
        0,
        gs::rgb4(2, 1, 1),
        gs::rgb4(15, 12, 7),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_LANE * 16 + i, lane[i]);
    vdp.setFogColor(gs::rgb4(1, 1, 3));

    loadFont(vdp, art);
    art.face = gs::uploadImage(vdp, faceArt());
    art.ring = gs::uploadImage(vdp, ringArt());
    art.cap = gs::uploadImage(vdp, capArt());
    for (int kind = 0; kind < 3; kind++) {
        for (int step = 0; step < 60; step++) {
            gs::Bitmap b(kDial, kDial);
            paintHand(b, kind, step);
            art.hand[kind][step] = gs::uploadImage(vdp, b);
        }
    }
    art.pin = gs::uploadMipped(vdp, pinArt());
    art.pinFlat = gs::uploadMipped(vdp, pinFlatArt());
    art.bellPin = gs::uploadMipped(vdp, bellPinArt());
    art.bellFlat = gs::uploadMipped(vdp, bellFlatArt());
    for (int i = 0; i < 4; i++) {
        art.ball[i] = gs::uploadMipped(vdp, ballArt(i));
        art.bowler[i] = gs::uploadMipped(vdp, bowlerArt(i));
    }
    art.arrow = gs::uploadMipped(vdp, arrowArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.foul = gs::uploadMipped(vdp, foulArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
}

}  // namespace pinschime
