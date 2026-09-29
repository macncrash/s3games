#include "game/art.h"

#include <cmath>

namespace clockbell {
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_DIAL, {0, gs::rgb4(2, 2, 3), gs::rgb4(10, 8, 6), gs::rgb4(15, 13, 9), gs::rgb4(5, 4, 4),
                           gs::rgb4(15, 12, 4), gs::rgb4(3, 3, 4), gs::rgb4(8, 7, 6)});
    setPal(vdp, PAL_HOUR, {0, gs::rgb4(3, 2, 2), gs::rgb4(12, 8, 4), gs::rgb4(15, 13, 8)});
    setPal(vdp, PAL_MIN, {0, gs::rgb4(2, 3, 4), gs::rgb4(8, 11, 13), gs::rgb4(13, 15, 15)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(8, 6, 2), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 9), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_LIT, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_TOWER, {0, gs::rgb4(3, 2, 4), gs::rgb4(6, 5, 7), gs::rgb4(2, 2, 3), gs::rgb4(9, 8, 6),
                            gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_ROPE, {0, gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 4)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(9, 9, 11)});

    for (int step = 0; step < kSteps; step++) {
        float theta = step * (kTau / float(kSteps));
        {
            gs::Bitmap b(kPivot * 2, kPivot * 2);
            shaft(b, theta, 22, 7, 3.6f, 1.4f, 1);
            shaft(b, theta, 20, 5, 1.8f, 0.5f, 2);
            shaft(b, theta, 16, 3, 0.7f, 0.25f, 3);
            art.hour[step] = gs::uploadImage(vdp, b);
        }
        {
            gs::Bitmap b(kPivot * 2, kPivot * 2);
            shaft(b, theta, 32, 6, 2.0f, 0.55f, 1);
            shaft(b, theta, 30, 4, 0.9f, 0.25f, 2);
            art.minute[step] = gs::uploadImage(vdp, b);
        }
    }

    {
        gs::Bitmap b(120, 120);
        const float cx = 60, cy = 60;
        b.ellipse(cx, cy, 58, 58, 1);
        b.ellipse(cx, cy, 54, 54, 2);
        b.ellipse(cx, cy - 1, 50, 50, 3);
        b.ellipse(cx, cy, 46, 46, 4);
        for (int i = 0; i < 60; i++) {
            float a = i * (kTau / 60.f);
            float s = std::sin(a), c = std::cos(a);
            bool hour = i % 5 == 0;
            float r0 = hour ? 36.f : 40.f;
            int col = (i / 5) == (kTarget % 12) ? 5 : (hour ? 6 : 7);
            b.line(cx + s * r0, cy - c * r0, cx + s * 48.f, cy - c * 48.f, col, hour ? 2.4f : 1.f);
        }
        float ma = (kTarget % 12) * (kTau / 12.f);
        b.ellipse(cx + std::sin(ma) * 30.f, cy - std::cos(ma) * 30.f, 3.2f, 3.2f, 5);
        art.dial = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(12, 12);
        b.ellipse(6, 6, 4.2f, 4.2f, 2);
        b.ellipse(6, 6, 2.0f, 2.0f, 3);
        art.cap = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 30);
        b.poly({{6, 24}, {18, 3}, {30, 24}}, 1);
        b.poly({{10, 22}, {18, 8}, {26, 22}}, 2);
        b.rect(4, 23, 28, 5, 2);
        b.rect(4, 23, 28, 2, 3);
        b.ellipse(18, 16, 3.2f, 3.2f, 4);
        b.ellipse(18, 16, 1.4f, 1.4f, 3);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(6, 70);
        b.rect(2, 0, 2, 70, 1);
        for (int y = 4; y < 68; y += 8) b.rect(1, y, 4, 2, 2);
        art.rope = gs::uploadImage(vdp, b);
    }

    gs::Bitmap tower(88, 168);
    tower.rect(18, 36, 52, 132, 1);
    for (int y = 36; y < 168; y += 12) tower.rect(18, y, 52, 1, 2);
    tower.rect(18, 36, 5, 132, 3);
    tower.rect(65, 36, 5, 132, 5);
    tower.poly({{44, 6}, {78, 40}, {10, 40}}, 3);
    tower.rect(30, 8, 8, 22, 4);
    tower.rect(34, 78, 20, 26, 3);
    gs::TileAlloc tiles(vdp, 256);
    gs::bitmapToPlane(tiles, vdp.B, 15, 3, tower, PAL_TOWER);
    vdp.B.enabled = true;
    vdp.A.enabled = false;

    loadFont(vdp, art.font);
}

}  // namespace clockbell
