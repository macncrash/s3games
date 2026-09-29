#include "game/art.h"

#include <cmath>

namespace clockgold {
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
    setPal(vdp, PAL_DIAL, {0, gs::rgb4(3, 2, 2), gs::rgb4(11, 8, 5), gs::rgb4(15, 13, 9), gs::rgb4(5, 4, 3),
                           gs::rgb4(15, 11, 2), gs::rgb4(14, 12, 8), gs::rgb4(2, 2, 3), gs::rgb4(7, 7, 8)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(2, 2, 3), gs::rgb4(8, 8, 10), gs::rgb4(14, 14, 15)});
    setPal(vdp, PAL_LIT, {0, gs::rgb4(8, 5, 1), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(8, 7, 4), gs::rgb4(14, 12, 8), gs::rgb4(15, 15, 12), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_TOWER, {0, gs::rgb4(3, 2, 4), gs::rgb4(6, 5, 7), gs::rgb4(2, 2, 3), gs::rgb4(9, 8, 7)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(6, 5, 3), gs::rgb4(12, 10, 5), gs::rgb4(15, 13, 7), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_SEC, {0, gs::rgb4(10, 3, 3), gs::rgb4(14, 6, 5)});

    for (int step = 0; step < kHours; step++) {
        float theta = step * (kTau / float(kHours));
        gs::Bitmap hour(kPivot * 2, kPivot * 2);
        shaft(hour, theta, 28, 8, 4.4f, 1.1f, 1);
        shaft(hour, theta, 26, 6, 2.2f, 0.5f, 2);
        shaft(hour, theta, 22, 4, 0.7f, 0.25f, 3);
        hour.ellipse(float(kPivot), float(kPivot), 3.4f, 3.4f, 2);
        art.hour[step] = gs::uploadImage(vdp, hour);

        gs::Bitmap sec(kPivot * 2, kPivot * 2);
        shaft(sec, theta, 36, 6, 1.1f, 0.35f, 1);
        shaft(sec, theta, 34, 4, 0.4f, 0.15f, 2);
        art.second[step] = gs::uploadImage(vdp, sec);
    }
    {
        gs::Bitmap m(kPivot * 2, kPivot * 2);
        shaft(m, 0, 34, 7, 2.4f, 0.6f, 1);
        shaft(m, 0, 32, 5, 1.1f, 0.3f, 2);
        m.ellipse(float(kPivot), float(kPivot), 2.4f, 2.4f, 3);
        art.minute = gs::uploadImage(vdp, m);
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
            int h = (i / 5) % 12;
            int col = goldHour(h) && hour ? 5 : (creamHour(h) && hour ? 6 : (hour ? 7 : 8));
            b.line(cx + s * r0, cy - c * r0, cx + s * 48.f, cy - c * 48.f, col, hour ? 2.4f : 1.f);
        }
        for (int h = 0; h < 12; h++) {
            if (!goldHour(h) && !creamHour(h)) continue;
            float a = h * (kTau / 12.f);
            b.ellipse(cx + std::sin(a) * 30.f, cy - std::cos(a) * 30.f, 3.2f, 3.2f, goldHour(h) ? 5 : 6);
        }
        b.ellipse(cx, cy, 4.2f, 4.2f, 7);
        art.dial = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(14, 14);
        b.ellipse(7, 7, 6, 6, 1);
        b.ellipse(7, 6.4f, 4.2f, 4.2f, 2);
        b.ellipse(5.6f, 5.2f, 1.6f, 1.1f, 3);
        art.pipGold = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(14, 14);
        b.ellipse(7, 7, 6, 6, 1);
        b.ellipse(7, 6.4f, 4.2f, 4.2f, 2);
        b.ellipse(5.6f, 5.2f, 1.6f, 1.1f, 3);
        art.pipCream = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(30, 24);
        b.poly({{5, 18}, {15, 2}, {25, 18}}, 2);
        b.poly({{8, 17}, {15, 5}, {22, 17}}, 3);
        b.rect(3, 18, 24, 4, 1);
        b.ellipse(15, 12, 2.2f, 2.2f, 4);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 48);
        b.rect(3, 0, 2, 40, 1);
        b.ellipse(4, 42, 3.2f, 4.2f, 2);
        art.rope = gs::uploadImage(vdp, b);
    }

    gs::Bitmap tower(104, 168);
    tower.rect(20, 32, 64, 136, 1);
    for (int y = 32; y < 168; y += 12) tower.rect(20, y, 64, 1, 2);
    tower.rect(20, 32, 5, 136, 3);
    tower.rect(79, 32, 5, 136, 4);
    tower.poly({{52, 2}, {88, 34}, {16, 34}}, 3);
    tower.rect(40, 78, 24, 30, 3);
    tower.rect(46, 140, 12, 28, 3);
    gs::TileAlloc tiles(vdp, 220);
    gs::bitmapToPlane(tiles, vdp.B, 13, 3, tower, PAL_TOWER);
    vdp.B.enabled = true;
    vdp.A.enabled = false;
    vdp.HUD.enabled = true;

    loadFont(vdp, art.font);
}

}  // namespace clockgold
