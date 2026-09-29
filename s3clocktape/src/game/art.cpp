#include "game/art.h"

#include <cmath>

namespace clocktape {
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 10)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(7, 7, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_DIAL, {0, gs::rgb4(3, 2, 2), gs::rgb4(12, 9, 6), gs::rgb4(15, 14, 11), gs::rgb4(4, 3, 3),
                           gs::rgb4(14, 11, 4), gs::rgb4(2, 2, 3), gs::rgb4(8, 6, 5)});
    setPal(vdp, PAL_HOUR, {0, gs::rgb4(4, 2, 1), gs::rgb4(12, 6, 2), gs::rgb4(15, 12, 6)});
    setPal(vdp, PAL_MIN, {0, gs::rgb4(1, 3, 4), gs::rgb4(6, 10, 12), gs::rgb4(12, 15, 15)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(5, 3, 1), gs::rgb4(9, 6, 2), gs::rgb4(13, 9, 4), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_TAPE, {0, gs::rgb4(14, 12, 8), gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(11, 9, 6)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(2, 2, 6), gs::rgb4(8, 2, 2), gs::rgb4(2, 6, 3)});
    setPal(vdp, PAL_SLIP, {0, gs::rgb4(15, 14, 10), gs::rgb4(3, 3, 8), gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_CAB, {0, gs::rgb4(4, 3, 3), gs::rgb4(7, 5, 4), gs::rgb4(2, 2, 2), gs::rgb4(10, 8, 6),
                          gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_LIT, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12)});

    for (int step = 0; step < kSteps; step++) {
        float theta = step * (kTau / float(kSteps));
        {
            gs::Bitmap b(kPivot * 2, kPivot * 2);
            shaft(b, theta, 18, 6, 3.4f, 1.2f, 1);
            shaft(b, theta, 16, 4, 1.6f, 0.4f, 2);
            shaft(b, theta, 12, 2, 0.6f, 0.2f, 3);
            art.hour[step] = gs::uploadImage(vdp, b);
        }
        {
            gs::Bitmap b(kPivot * 2, kPivot * 2);
            shaft(b, theta, 28, 5, 1.7f, 0.45f, 1);
            shaft(b, theta, 26, 3, 0.7f, 0.2f, 2);
            art.minute[step] = gs::uploadImage(vdp, b);
        }
    }

    {
        gs::Bitmap b(96, 96);
        const float cx = 48, cy = 48;
        b.ellipse(cx, cy, 46, 46, 1);
        b.ellipse(cx, cy, 42, 42, 2);
        b.ellipse(cx, cy, 38, 38, 3);
        b.ellipse(cx, cy, 36, 36, 4);
        for (int i = 0; i < 12; i++) {
            float a = i * (kTau / 12.f);
            float s = std::sin(a), c = std::cos(a);
            int col = (i == 4 || i == 7 || i == 10) ? 5 : 6;
            b.line(cx + s * 28.f, cy - c * 28.f, cx + s * 34.f, cy - c * 34.f, col, i % 3 == 0 ? 2.2f : 1.4f);
        }
        art.dial = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 10);
        b.ellipse(5, 5, 4, 4, 2);
        b.ellipse(5, 5, 1.6f, 1.6f, 3);
        art.cap = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(150, 168);
        b.rect(0, 0, 150, 168, 1);
        b.rect(6, 6, 138, 156, 2);
        b.rect(10, 10, 130, 148, 1);
        b.rect(18, 18, 114, 100, 3);
        for (int y = 0; y < 168; y += 14) b.rect(0, y, 8, 2, 5);
        b.rect(0, 0, 150, 6, 4);
        b.rect(0, 162, 150, 6, 3);
        art.cabinet = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(86, 78);
        b.rect(0, 8, 86, 62, 1);
        b.rect(0, 0, 86, 10, 3);
        b.ellipse(43, 6, 18, 6, 4);
        b.ellipse(43, 6, 8, 3, 2);
        for (int i = 0; i < 3; i++) {
            int y = 16 + i * 16;
            b.rect(6, y, 74, 12, 2);
            b.rect(8, y + 2, 70, 8, 1);
        }
        art.tape = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(168, 28);
        b.rect(0, 0, 168, 28, 1);
        b.rect(3, 3, 162, 22, 2);
        b.rect(6, 6, 48, 16, 3);
        b.rect(60, 6, 48, 16, 3);
        b.rect(114, 6, 48, 16, 3);
        b.ellipse(84, 14, 3, 3, 4);
        art.drawer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(44, 12);
        b.rect(0, 0, 44, 12, 1);
        b.rect(2, 3, 28, 2, 2);
        b.rect(2, 7, 18, 2, 3);
        art.slip = gs::uploadImage(vdp, b);
    }

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    loadFont(vdp, art.font);
}

}  // namespace clocktape
