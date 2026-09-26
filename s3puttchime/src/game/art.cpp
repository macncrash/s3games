#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace puttchime {
namespace {

constexpr int kDial = 48;
constexpr float kTau = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void shaft(gs::Bitmap& b, float theta, float len, float tail, float w, int col) {
    const float cx = kDial * 0.5f, cy = kDial * 0.5f;
    float dx = std::sin(theta), dy = -std::cos(theta);
    float px = -dy, py = dx;
    float x0 = cx - dx * tail, y0 = cy - dy * tail;
    float x1 = cx + dx * len, y1 = cy + dy * len;
    b.poly({{x0 + px * w * 0.35f, y0 + py * w * 0.35f},
            {x1 + px * w, y1 + py * w},
            {x1 - px * w, y1 - py * w},
            {x0 - px * w * 0.35f, y0 - py * w * 0.35f}},
           col);
}

gs::Bitmap handArt(int kind, int step) {
    gs::Bitmap b(kDial, kDial);
    float theta = step * (kTau / 60.f);
    if (kind == 0) {
        shaft(b, theta, 11.f, 3.5f, 2.3f, 1);
        shaft(b, theta, 10.f, 2.5f, 1.1f, 2);
    } else if (kind == 1) {
        shaft(b, theta, 16.f, 4.f, 1.15f, 3);
    } else {
        shaft(b, theta, 18.5f, 5.f, 0.55f, 4);
    }
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(kDial, kDial);
    b.ellipse(24, 24, 23, 23, 1);
    b.ellipse(24, 24, 20.5f, 20.5f, 2);
    b.ellipse(24, 24, 18.2f, 18.2f, 3);
    b.ellipse(22, 21, 12, 9, 4);
    b.ellipse(24, 24, 18.2f, 18.2f, 3);
    for (int i = 0; i < 60; i += 5) {
        float a = i * (kTau / 60.f);
        float s = std::sin(a), c = std::cos(a);
        int col = i == 0 ? 6 : 5;
        b.line(24 + s * 13.5f, 24 - c * 13.5f, 24 + s * 16.6f, 24 - c * 16.6f, col, i % 15 == 0 ? 1.7f : 1.f);
    }
    auto stamp = [&](const char* t, int x, int y) {
        gs::Bitmap num = gs::textBitmap(t, gs::TextStyle{1, 7, 0, 0, 1});
        b.blit(num, x - num.w / 2, y);
    };
    stamp("12", 24, 5);
    stamp("3", 36, 20);
    stamp("6", 24, 34);
    stamp("9", 8, 20);
    b.ellipse(24, 24, 1.6f, 1.6f, 8);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(kDial, kDial);
    b.ellipse(24, 24, 23.2f, 23.2f, 1);
    b.ellipse(24, 24, 20.2f, 20.2f, 2);
    b.ellipse(24, 24, 17.6f, 17.6f, 0);
    return b;
}

gs::Bitmap capArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 5.2f, 5.2f, 1);
    b.ellipse(7, 7, 2.3f, 2.3f, 2);
    return b;
}

void tileFill(uint8_t* px, int base, int alt, int speck) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int c = ((x + y * 2) & 3) == 0 ? alt : base;
            if (((x * 5 + y * 3 + speck) & 7) == 0) c = speck;
            px[y * 8 + x] = uint8_t(c);
        }
    }
}

gs::Bitmap ballArt(int frame) {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6.4f, 6.4f, 1);
    b.ellipse(6.2f, 5.6f, 2.2f, 1.6f, 3);
    const float spin = frame ? 0.7f : 0.f;
    const float dots[6][2] = {{5, 8}, {8, 7}, {11, 8}, {6.5f, 10.5f}, {10, 10.5f}, {8, 12}};
    for (int i = 0; i < 6; i++) {
        float a = spin + i;
        b.set(int(dots[i][0] + std::sin(a) * 0.4f), int(dots[i][1]), 2);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 8);
    b.ellipse(8, 4, 6.5f, 2.6f, 1);
    return b;
}

gs::Bitmap cupArt() {
    gs::Bitmap b(30, 18);
    b.ellipse(15, 9, 14, 8, 2);
    b.ellipse(15, 9, 11.2f, 6.4f, 3);
    b.ellipse(15, 9, 7.4f, 4.3f, 1);
    b.ellipse(15, 9, 4.2f, 2.5f, 4);
    b.ellipse(12.4f, 7.2f, 2.2f, 1.1f, 5);
    return b;
}

gs::Bitmap flagArt(int frame) {
    gs::Bitmap b(22, 32);
    b.rect(4, 6, 2, 24, 3);
    b.ellipse(5, 30, 3.2f, 1.4f, 4);
    float dip = frame ? 3.f : 0.f;
    b.poly({{6, 6}, {19, 9 + dip}, {6, 15}}, 1);
    b.poly({{6, 7}, {16, 9.5f + dip * 0.6f}, {6, 13}}, 2);
    return b;
}

gs::Bitmap golferArt(int pose) {
    gs::Bitmap b(32, 40);
    b.ellipse(13, 36, 3.4f, 2.f, 6);
    b.ellipse(20, 36, 3.4f, 2.f, 6);
    b.rect(12, 24, 3, 12, 4);
    b.rect(18, 24, 3, 12, 4);
    b.rect(10, 16, 12, 10, 2);
    b.rect(11, 17, 5, 7, 3);
    b.ellipse(16, 11, 4.6f, 4.4f, 1);
    b.ellipse(16, 8, 5.6f, 2.4f, 5);
    b.rect(11, 7, 10, 2, 5);
    b.set(14, 11, 7);
    b.set(18, 11, 7);
    if (pose == 0) {
        b.rect(6, 18, 4, 3, 1);
        b.line(22, 20, 30, 28, 8, 1.4f);
        b.rect(28, 26, 3, 3, 8);
    } else if (pose == 1) {
        b.rect(5, 20, 4, 3, 1);
        b.line(18, 18, 30, 14, 8, 1.4f);
        b.rect(28, 12, 3, 3, 8);
    } else {
        b.rect(7, 14, 3, 6, 1);
        b.line(20, 16, 24, 4, 8, 1.4f);
        b.rect(22, 2, 4, 3, 8);
        b.rect(22, 18, 3, 6, 1);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(72, 96);
    b.poly({{36, 2}, {66, 26}, {6, 26}}, 2);
    b.rect(14, 24, 44, 68, 1);
    b.rect(18, 28, 36, 22, 3);
    b.ellipse(36, 40, 12, 12, 4);
    b.rect(30, 62, 12, 28, 5);
    b.poly({{30, 62}, {42, 62}, {36, 52}}, 6);
    for (int i = 0; i < 4; i++) b.rect(16 + i * 12, 22, 6, 4, 2);
    b.rect(12, 90, 48, 4, 7);
    b.outline(15, false);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(16, 18);
    b.rect(7, 0, 2, 3, 3);
    b.poly({{3, 4}, {13, 4}, {14, 13}, {2, 13}}, 1);
    b.ellipse(8, 13, 6.2f, 2.6f, 1);
    b.ellipse(8, 9, 1.6f, 2.2f, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 40);
    b.rect(12, 22, 4, 14, 3);
    b.rect(8, 34, 12, 3, 3);
    b.ellipse(14, 16, 12, 11, 2);
    b.ellipse(9, 15, 6, 5, 1);
    b.ellipse(18, 12, 6, 5, 1);
    return b;
}

gs::Bitmap flowerArt() {
    gs::Bitmap b(12, 14);
    b.line(6, 13, 6, 6, 3, 1);
    b.ellipse(6, 5, 3.1f, 3.1f, 1);
    b.ellipse(6, 5, 1.3f, 1.3f, 2);
    b.ellipse(3, 7, 1.5f, 1.5f, 1);
    b.ellipse(9, 7, 1.5f, 1.5f, 1);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(32, 14);
    b.ellipse(10, 8, 7, 4, 1);
    b.ellipse(18, 6, 8, 5, 1);
    b.ellipse(25, 8, 5, 3, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 4.6f, 4.6f, 1);
    b.ellipse(7.6f, 7.4f, 1.6f, 1.2f, 2);
    b.line(9, 1, 9, 3, 1, 1);
    b.line(9, 15, 9, 17, 1, 1);
    b.line(1, 9, 3, 9, 1, 1);
    b.line(15, 9, 17, 9, 1, 1);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(6, 6);
    b.ellipse(3, 3, 2.1f, 2.1f, 1);
    return b;
}

gs::Bitmap borrowArt() {
    gs::Bitmap b(12, 10);
    b.poly({{1, 2}, {8, 5}, {1, 8}}, 1);
    b.poly({{4, 2}, {11, 5}, {4, 8}}, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    }
    uint8_t px[64];
    tileFill(px, 1, 2, 3);
    a.grassA = tiles.alloc(1);
    vdp.loadTile(a.grassA, px);
    tileFill(px, 2, 1, 3);
    a.grassB = tiles.alloc(1);
    vdp.loadTile(a.grassB, px);
    tileFill(px, 4, 5, 5);
    a.rough = tiles.alloc(1);
    vdp.loadTile(a.rough, px);
    tileFill(px, 5, 4, 1);
    a.hedge = tiles.alloc(1);
    vdp.loadTile(a.hedge, px);
    for (int y = 8; y <= 26; y++) {
        for (int x = 0; x < 40; x++) {
            bool edge = x < 3 || x > 36;
            int tile = y == 8 ? a.hedge : edge ? a.rough : (y & 1) ? a.grassA : a.grassB;
            vdp.B.set(x, y, gs::entry(tile, PAL_GRASS));
        }
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(6, 8, 10)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 11, 4), gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 8), gs::rgb4(1, 6, 2)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 9, 10), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(14, 2, 2), gs::rgb4(15, 8, 5), gs::rgb4(12, 12, 13), gs::rgb4(6, 4, 2),
                          gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_TOWER, {0, gs::rgb4(12, 12, 11), gs::rgb4(7, 7, 8), gs::rgb4(5, 6, 8), gs::rgb4(2, 2, 3),
                            gs::rgb4(4, 4, 5), gs::rgb4(9, 8, 7), gs::rgb4(6, 6, 5)});
    setPal(vdp, PAL_CUP, {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 10, 4), gs::rgb4(8, 6, 2), gs::rgb4(0, 0, 1),
                          gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(7, 13, 4), gs::rgb4(2, 8, 3), gs::rgb4(7, 4, 2)});
    setPal(vdp, PAL_AIM, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 10, 3)});
    setPal(vdp, PAL_PLAYER, {0, gs::rgb4(14, 11, 8), gs::rgb4(2, 5, 12), gs::rgb4(4, 8, 14), gs::rgb4(10, 8, 5),
                             gs::rgb4(13, 3, 3), gs::rgb4(3, 2, 2), gs::rgb4(1, 1, 1), gs::rgb4(9, 9, 10)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(2, 8, 3), gs::rgb4(4, 12, 4), gs::rgb4(7, 14, 6), gs::rgb4(3, 7, 2),
                            gs::rgb4(1, 5, 2), gs::rgb4(8, 6, 3)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(12, 9, 3), gs::rgb4(6, 4, 1), gs::rgb4(15, 14, 11), gs::rgb4(13, 12, 9),
                           gs::rgb4(4, 3, 2), gs::rgb4(14, 2, 2), gs::rgb4(2, 2, 2), gs::rgb4(10, 8, 3)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 7), gs::rgb4(1, 1, 2), gs::rgb4(14, 2, 2)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(15, 12, 3), gs::rgb4(8, 6, 2), gs::rgb4(12, 9, 4), gs::rgb4(4, 3, 1)});

    loadFont(vdp, art);
    for (int k = 0; k < 3; k++)
        for (int s = 0; s < 60; s++) art.hand[k][s] = gs::uploadImage(vdp, handArt(k, s));
    art.face = gs::uploadImage(vdp, faceArt());
    art.ring = gs::uploadImage(vdp, ringArt());
    art.cap = gs::uploadImage(vdp, capArt());
    art.ball[0] = gs::uploadMipped(vdp, ballArt(0));
    art.ball[1] = gs::uploadMipped(vdp, ballArt(1));
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cup = gs::uploadMipped(vdp, cupArt());
    art.flag[0] = gs::uploadMipped(vdp, flagArt(0));
    art.flag[1] = gs::uploadMipped(vdp, flagArt(1));
    art.golfer[0] = gs::uploadMipped(vdp, golferArt(0));
    art.golfer[1] = gs::uploadMipped(vdp, golferArt(1));
    art.golfer[2] = gs::uploadMipped(vdp, golferArt(2));
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.flower = gs::uploadMipped(vdp, flowerArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.borrow = gs::uploadMipped(vdp, borrowArt());
}

}  // namespace puttchime
