#include "game/art.h"

#include <cmath>
#include <string>

namespace clk {
namespace {

constexpr float kTau = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void shadow(gs::VDP& vdp, int pal, uint16_t c) { vdp.setColor(pal * 16 + 15, c); }

void shaft(gs::Bitmap& b, float theta, float len, float tail, float w0, float w1, int col) {
    const float cx = float(kPivot), cy = float(kPivot);
    float dx = std::sin(theta), dy = -std::cos(theta);
    float px = -dy, py = dx;
    float ox = cx - dx * tail, oy = cy - dy * tail;
    float mx = cx + dx * (len * 0.18f), my = cy + dy * (len * 0.18f);
    float tx = cx + dx * len, ty = cy + dy * len;
    b.poly({{ox + px * w0 * 0.4f, oy + py * w0 * 0.4f},
            {mx + px * w0, my + py * w0},
            {tx + px * w1, ty + py * w1},
            {tx - px * w1, ty - py * w1},
            {mx - px * w0, my - py * w0},
            {ox - px * w0 * 0.4f, oy - py * w0 * 0.4f}},
           col);
}

void paintHand(gs::Bitmap& b, int kind, int step) {
    float theta = step * (kTau / float(kHandN));
    float dx = std::sin(theta), dy = -std::cos(theta);
    if (kind == 0) {
        shaft(b, theta, 28, 8, 5.4f, 1.5f, 1);
        shaft(b, theta, 26, 6, 3.3f, 0.9f, 2);
        shaft(b, theta, 22, 4, 1.2f, 0.4f, 3);
    } else if (kind == 1) {
        shaft(b, theta, 38, 9, 3.4f, 0.9f, 1);
        shaft(b, theta, 36, 7, 2.0f, 0.55f, 2);
        shaft(b, theta, 34, 5, 0.7f, 0.25f, 4);
    } else {
        shaft(b, theta, 42, 14, 1.7f, 0.85f, 1);
        shaft(b, theta, 41, 13, 0.85f, 0.4f, 2);
        b.ellipse(kPivot + dx * 26, kPivot + dy * 26, 3.2f, 3.2f, 2);
        b.ellipse(kPivot + dx * 26, kPivot + dy * 26, 1.5f, 1.5f, 4);
        b.ellipse(kPivot - dx * 12, kPivot - dy * 12, 2.5f, 2.5f, 1);
        b.ellipse(kPivot - dx * 12, kPivot - dy * 12, 1.2f, 1.2f, 3);
    }
}

void paintDial(gs::Bitmap& b) {
    const float cx = 56, cy = 96;
    b.rect(cx - 48, cy - 48, 96, 96, 1);
    b.ellipse(cx, cy, 46, 46, 6);
    b.ellipse(cx, cy, 43, 43, 7);
    b.ellipse(cx - 1, cy - 2, 42, 41, 8);
    b.ellipse(cx, cy, 40, 40, 7);
    b.ellipse(cx, cy + 1.5f, 37, 37, 10);
    b.ellipse(cx, cy, 36, 36, 9);
    for (int i = 0; i < 60; i++) {
        float a = i * (kTau / 60.f);
        float s = std::sin(a), c = std::cos(a);
        bool hour = i % 5 == 0;
        float r0 = hour ? 31.f : 35.f;
        b.line(cx + s * r0, cy - c * r0, cx + s * 41.f, cy - c * 41.f, hour ? 11 : 6, hour ? 2.f : 1.f);
    }
    for (int n = 1; n <= 12; n++) {
        float a = n * (kTau / 12.f);
        gs::Bitmap g = gs::textBitmap(std::to_string(n), gs::TextStyle{1, 11, 0, 0, 1});
        float r = n >= 10 ? 26.f : 27.f;
        b.blit(g, int(std::lround(cx + std::sin(a) * r - g.w * 0.5f)),
               int(std::lround(cy - std::cos(a) * r - g.h * 0.5f)));
    }
    for (int k = 0; k < 4; k++) {
        float a = (k + 0.5f) * (kTau / 4.f);
        float x = cx + std::sin(a) * 44.f, y = cy - std::cos(a) * 44.f;
        b.ellipse(x, y, 2.1f, 2.1f, 7);
        b.ellipse(x, y, 0.8f, 0.8f, 4);
    }
    b.ellipse(cx, cy, 5, 5, 7);
    b.ellipse(cx, cy, 2.2f, 2.2f, 6);
}

void paintTower(gs::Bitmap& b) {
    b.rect(8, 46, 96, 130, 2);
    for (int y = 46; y < 176; y += 9) b.rect(8, y, 96, 1, 4);
    for (int row = 0; row < 15; row++) {
        int y = 46 + row * 9;
        int off = (row & 1) ? 16 : 0;
        for (int x = 8 + off; x < 104; x += 24) b.rect(x, y, 1, 9, 4);
    }
    b.rect(8, 46, 3, 130, 3);
    b.rect(101, 46, 3, 130, 1);

    b.poly({{56, 6}, {66, 18}, {46, 18}}, 3);
    b.rect(53, 16, 6, 8, 2);
    b.line(56, 6, 70, 12, 8, 1.4f);
    b.poly({{56, 8}, {68, 11}, {56, 13}}, 8);

    b.rect(26, 18, 60, 30, 2);
    b.rect(26, 18, 10, 30, 1);
    b.rect(76, 18, 10, 30, 1);
    b.rect(26, 18, 60, 5, 1);
    b.rect(28, 20, 6, 2, 3);
    b.rect(78, 20, 6, 2, 3);
    b.ellipse(56, 34, 14, 12, 5);
    b.rect(42, 34, 28, 12, 5);

    paintDial(b);

    b.rect(22, 148, 14, 18, 13);
    b.rect(24, 150, 10, 14, 12);
    b.rect(28, 150, 1, 14, 13);
    b.rect(76, 148, 14, 18, 13);
    b.rect(78, 150, 10, 14, 12);
    b.ellipse(46, 150, 12, 14, 5);
    b.rect(40, 150, 32, 20, 5);
    b.rect(4, 166, 104, 10, 1);
    b.ellipse(16, 124, 9, 16, 14);
    b.ellipse(20, 138, 6, 10, 15);
}

void house(gs::Bitmap& b, int x, int y, int w, int h) {
    b.poly({{float(x - 4), float(y + 16)}, {float(x + w / 2), float(y)}, {float(x + w + 4), float(y + 16)}}, 5);
    b.poly({{float(x - 2), float(y + 16)}, {float(x + w / 2), float(y + 4)}, {float(x + w + 2), float(y + 16)}}, 4);
    b.rect(x, y + 16, w, h - 16, 1);
    b.rect(x, y + 16, 3, h - 16, 3);
    b.rect(x + w - 4, y + 16, 4, h - 16, 2);
    b.rect(x + w / 2 - 4, y - 8, 6, 14, 3);
    int wy = y + 28;
    for (int i = 0; i < 2; i++) {
        int wx = x + 8 + i * (w / 2 - 4);
        b.rect(wx, wy, 12, 14, 3);
        b.rect(wx + 1, wy + 1, 10, 12, 6);
        b.rect(wx + 5, wy + 1, 1, 12, 3);
        b.rect(wx + 1, wy + 6, 10, 1, 3);
        b.set(wx + 3, wy + 3, 7);
    }
    b.rect(x + w / 2 - 6, y + h - 22, 12, 22, 3);
    b.rect(x + w / 2 - 4, y + h - 20, 8, 18, 13);
}

void paintTown(gs::Bitmap& b) {
    b.poly({{0, 156}, {36, 124}, {78, 146}, {120, 118}, {168, 140}, {210, 116}, {258, 138}, {300, 120}, {320, 148}, {320, 168}, {0, 168}},
           12);
    b.poly({{0, 160}, {48, 136}, {96, 154}, {150, 132}, {210, 150}, {270, 134}, {320, 152}, {320, 170}, {0, 170}}, 11);
    house(b, 4, 112, 72, 64);
    house(b, 246, 116, 68, 60);
    gs::Bitmap sign = gs::textBitmap("HOUR", gs::TextStyle{1, 13, 0, 0, 1});
    b.rect(18, 148, sign.w + 6, sign.h + 4, 2);
    b.blit(sign, 21, 150);
    for (int x : {92, 230}) {
        b.rect(x, 132, 2, 36, 3);
        b.ellipse(x + 1, 128, 5, 4, 10);
        b.ellipse(x + 1, 127, 2, 2, 7);
    }
    for (int y = 168; y < 200; y += 6) {
        int off = ((y / 6) & 1) ? 5 : 0;
        for (int x = -8; x < 320; x += 11) {
            int c = ((x + y) & 16) ? 8 : 9;
            b.rect(x + off, y, 10, 5, c);
        }
    }
    b.rect(0, 200, 320, 2, 10);
    b.rect(0, 202, 320, 22, 14);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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
        a.glyph[c - 32] = gs::uploadImage(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

gs::Bitmap bellArt() {
    gs::Bitmap b(28, 24);
    b.rect(12, 0, 4, 4, 3);
    b.poly({{8, 4}, {20, 4}, {25, 16}, {3, 16}}, 4);
    b.poly({{11, 5}, {17, 5}, {20, 14}, {8, 14}}, 5);
    b.ellipse(14, 16, 11, 4, 4);
    b.ellipse(14, 15, 7, 2, 3);
    b.ellipse(14, 13, 2, 3, 6);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(10, 148);
    b.rect(4, 0, 2, 128, 1);
    b.rect(5, 0, 1, 128, 2);
    b.poly({{1, 126}, {9, 126}, {8, 146}, {2, 146}}, 7);
    b.rect(3, 130, 4, 6, 2);
    return b;
}

gs::Bitmap capArt() {
    gs::Bitmap b(13, 13);
    b.ellipse(6, 6, 6, 6, 1);
    b.ellipse(6, 6, 4, 4, 2);
    b.ellipse(5, 5, 1.6f, 1.6f, 4);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(28, 28);
    const float cx = 13.5f, cy = 13.5f, r = 12.f;
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 28; x++) {
            float dx = x - cx, dy = y - cy;
            if (dx * dx + dy * dy > r * r) continue;
            float bx = dx + 7.f, by = dy - 1.f;
            if (bx * bx + by * by < 11.f * 11.f) continue;
            int c = 1;
            float c1x = x - 9.f, c1y = y - 15.f;
            if (c1x * c1x + c1y * c1y < 5.f) c = 2;
            b.set(x, y, c);
        }
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(120, 120);
    const float cx = 59.5f, cy = 59.5f;
    for (int y = 0; y < 120; y++)
        for (int x = 0; x < 120; x++) {
            float dx = x - cx, dy = y - cy;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d >= 50.f && d <= 54.f) b.set(x, y, d < 51.4f ? 2 : 1);
        }
    return b;
}

gs::Bitmap bobArt() {
    gs::Bitmap b(12, 16);
    b.rect(5, 0, 2, 5, 3);
    b.ellipse(6, 10, 5, 5, 4);
    b.ellipse(5, 9, 2, 2, 5);
    return b;
}

gs::Bitmap rodArt() {
    gs::Bitmap b(3, 16);
    b.rect(1, 0, 1, 16, 3);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(3, 3);
    b.set(1, 0, 1);
    b.set(0, 1, 1);
    b.set(1, 1, 1);
    b.set(2, 1, 1);
    b.set(1, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(14, 12, 6), gs::rgb4(9, 9, 11), gs::rgb4(15, 5, 4)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(14, 11, 4)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(8, 8, 10)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 5, 4)});
    shadow(vdp, PAL_HUD, gs::rgb4(1, 1, 2));
    shadow(vdp, PAL_GOLD, gs::rgb4(3, 1, 0));
    shadow(vdp, PAL_DIM, gs::rgb4(1, 1, 2));
    shadow(vdp, PAL_BAD, gs::rgb4(2, 0, 0));

    setPal(vdp, PAL_SCENE,
           {0, gs::rgb4(11, 9, 7), gs::rgb4(7, 6, 5), gs::rgb4(4, 2, 2), gs::rgb4(6, 2, 3), gs::rgb4(3, 1, 2),
            gs::rgb4(15, 12, 5), gs::rgb4(15, 15, 8), gs::rgb4(6, 5, 5), gs::rgb4(4, 3, 4), gs::rgb4(15, 13, 6),
            gs::rgb4(4, 4, 7), gs::rgb4(2, 2, 4), gs::rgb4(2, 1, 2), gs::rgb4(1, 1, 3), gs::rgb4(9, 8, 10)});
    setPal(vdp, PAL_WORK,
           {0, gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2),
            gs::rgb4(4, 3, 1), gs::rgb4(10, 7, 2), gs::rgb4(14, 11, 5), gs::rgb4(14, 13, 10), gs::rgb4(10, 8, 6),
            gs::rgb4(2, 2, 3), gs::rgb4(15, 10, 3), gs::rgb4(2, 2, 2), gs::rgb4(2, 6, 3), gs::rgb4(4, 6, 3)});
    setPal(vdp, PAL_HOUR, {0, gs::rgb4(5, 3, 1), gs::rgb4(11, 7, 2), gs::rgb4(14, 11, 4), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_HLIT, {0, gs::rgb4(7, 4, 1), gs::rgb4(13, 9, 3), gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 11)});
    setPal(vdp, PAL_MIN, {0, gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 9), gs::rgb4(12, 12, 13), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_MLIT, {0, gs::rgb4(5, 5, 7), gs::rgb4(11, 11, 13), gs::rgb4(14, 14, 15), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_SEC, {0, gs::rgb4(5, 0, 1), gs::rgb4(12, 2, 2), gs::rgb4(15, 6, 4), gs::rgb4(15, 12, 9)});
    setPal(vdp, PAL_SLIT, {0, gs::rgb4(8, 1, 1), gs::rgb4(15, 4, 3), gs::rgb4(15, 9, 6), gs::rgb4(15, 14, 12)});
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(5, 3, 1), gs::rgb4(9, 6, 3), gs::rgb4(6, 4, 1), gs::rgb4(12, 8, 2), gs::rgb4(15, 13, 6),
            gs::rgb4(2, 2, 2), gs::rgb4(12, 2, 2)});
    setPal(vdp, PAL_MOON, {0, gs::rgb4(14, 14, 11), gs::rgb4(10, 10, 8), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_RING, {0, gs::rgb4(15, 13, 6), gs::rgb4(12, 8, 2)});
    setPal(vdp, PAL_BOB, {0, gs::rgb4(8, 6, 2), gs::rgb4(14, 11, 4)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::Bitmap town(gs::SCREEN_W, gs::SCREEN_H);
    paintTown(town);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, town, PAL_SCENE);
    gs::Bitmap tower(kTowerW, kTowerH);
    paintTower(tower);
    gs::bitmapToPlane(tiles, vdp.A, kTowerX / 8, kTowerY / 8, tower, PAL_WORK);
    vdp.A.scroll(0, 0);
    vdp.B.scroll(0, 0);

    for (int kind = 0; kind < 3; kind++)
        for (int step = 0; step < kHandN; step++) {
            gs::Bitmap hand(kHandS, kHandS);
            paintHand(hand, kind, step);
            art.hand[kind][step] = gs::uploadImage(vdp, hand);
        }
    art.cap = gs::uploadImage(vdp, capArt());
    art.bell = gs::uploadImage(vdp, bellArt());
    art.rope = gs::uploadImage(vdp, ropeArt());
    art.moon = gs::uploadImage(vdp, moonArt());
    art.star = gs::uploadImage(vdp, starArt());
    art.bob = gs::uploadImage(vdp, bobArt());
    art.rod = gs::uploadImage(vdp, rodArt());
    art.ring = gs::uploadImage(vdp, ringArt());
}

}  // namespace clk
