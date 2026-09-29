#include "game/art.h"

#include <cmath>

namespace clockmark {
namespace {

constexpr float kTau = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void shaft(gs::Bitmap& b, float theta, float len, float tail, float w0, float w1, int col) {
    const float cx = float(kPivot), cy = float(kPivot);
    float dx = std::sin(theta), dy = -std::cos(theta);
    float px = -dy, py = dx;
    float ox = cx - dx * tail, oy = cy - dy * tail;
    float tx = cx + dx * len, ty = cy + dy * len;
    b.poly({{ox + px * w0, oy + py * w0},
            {tx + px * w1, ty + py * w1},
            {tx - px * w1, ty - py * w1},
            {ox - px * w0, oy - py * w0}},
           col);
}

void loadFont(gs::VDP& vdp, int* font) {
    gs::TileAlloc tiles(vdp, 1);
    for (int c = 0; c < 96; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c + 32));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + (x + 1)] = 1;
        font[c] = tiles.shared(px);
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_DIAL, {0, gs::rgb4(3, 2, 2), gs::rgb4(12, 9, 6), gs::rgb4(15, 13, 9), gs::rgb4(6, 4, 3),
                           gs::rgb4(15, 12, 4), gs::rgb4(2, 2, 3), gs::rgb4(8, 8, 9)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(2, 2, 3), gs::rgb4(9, 9, 11), gs::rgb4(14, 14, 15)});
    setPal(vdp, PAL_LIT, {0, gs::rgb4(8, 5, 1), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_COIN, {0, gs::rgb4(10, 6, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 8), gs::rgb4(6, 3, 1)});
    setPal(vdp, PAL_TOWER, {0, gs::rgb4(3, 2, 4), gs::rgb4(6, 5, 7), gs::rgb4(2, 2, 3), gs::rgb4(9, 8, 7)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(8, 8, 10)});

    for (int step = 0; step < kHours; step++) {
        gs::Bitmap b(kPivot * 2, kPivot * 2);
        float theta = step * (kTau / float(kHours));
        shaft(b, theta, 30, 8, 4.2f, 1.2f, 1);
        shaft(b, theta, 28, 6, 2.2f, 0.6f, 2);
        shaft(b, theta, 24, 4, 0.8f, 0.3f, 3);
        b.ellipse(float(kPivot), float(kPivot), 3.2f, 3.2f, 2);
        art.hand[step] = gs::uploadImage(vdp, b);
    }

    {
        gs::Bitmap b(112, 112);
        const float cx = 56, cy = 56;
        b.ellipse(cx, cy, 54, 54, 1);
        b.ellipse(cx, cy, 50, 50, 2);
        b.ellipse(cx, cy - 1, 46, 46, 3);
        b.ellipse(cx, cy, 42, 42, 4);
        for (int i = 0; i < 60; i++) {
            float a = i * (kTau / 60.f);
            float s = std::sin(a), c = std::cos(a);
            bool hour = i % 5 == 0;
            float r0 = hour ? 34.f : 38.f;
            int col = (i / 5) == kMark ? 5 : (hour ? 6 : 7);
            b.line(cx + s * r0, cy - c * r0, cx + s * 44.f, cy - c * 44.f, col, hour ? 2.2f : 1.f);
        }
        float ma = kMark * (kTau / 12.f);
        b.ellipse(cx + std::sin(ma) * 28.f, cy - std::cos(ma) * 28.f, 3.4f, 3.4f, 5);
        b.ellipse(cx, cy, 4.f, 4.f, 6);
        art.dial = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 16);
        b.ellipse(8, 8, 7, 7, 1);
        b.ellipse(8, 7.5f, 5.2f, 5.2f, 2);
        b.ellipse(6.5f, 6.2f, 2.1f, 1.4f, 3);
        b.ellipse(8, 8, 1.3f, 1.3f, 4);
        art.coin = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 22);
        b.poly({{4, 18}, {14, 2}, {24, 18}}, 1);
        b.rect(3, 17, 22, 4, 2);
        b.ellipse(14, 10, 3, 3, 3);
        art.bell = gs::uploadImage(vdp, b);
    }

    gs::Bitmap tower(96, 160);
    tower.rect(16, 28, 64, 132, 1);
    for (int y = 28; y < 160; y += 10) tower.rect(16, y, 64, 1, 2);
    tower.rect(16, 28, 4, 132, 3);
    tower.rect(76, 28, 4, 132, 4);
    tower.poly({{48, 4}, {80, 30}, {16, 30}}, 3);
    tower.rect(36, 70, 24, 28, 3);
    gs::TileAlloc tiles(vdp, 200);
    gs::bitmapToPlane(tiles, vdp.B, 14, 4, tower, PAL_TOWER);
    vdp.B.enabled = true;
    vdp.A.enabled = false;

    loadFont(vdp, art.font);
}

}  // namespace clockmark
