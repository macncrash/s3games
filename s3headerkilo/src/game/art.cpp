#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace headerkilo {

static gs::Image uploadGlyph(gs::VDP& vdp, char ch) {
    gs::Bitmap b(5, 7);
    const uint8_t* g = gs::glyph(ch);
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 5; x++)
            if (g[y * 5 + x]) b.set(x, y, 1);
    return gs::uploadImage(vdp, b);
}

static void pal(gs::VDP& vdp, int p, std::initializer_list<uint16_t> cols) {
    int i = 1;
    for (uint16_t c : cols) vdp.setColor(p * 16 + i++, c);
}

static void millWheel(gs::Bitmap& b, int cx, int cy, int r, float a0) {
    b.ellipse(float(cx), float(cy), float(r), float(r), 1);
    b.ellipse(float(cx), float(cy), float(r - 3), float(r - 3), 0);
    b.ellipse(float(cx), float(cy), 3.f, 3.f, 4);
    for (int s = 0; s < 8; s++) {
        float a = a0 + s * 0.785398f;
        float ox = std::cos(a);
        float oy = std::sin(a);
        b.line(float(cx), float(cy), cx + ox * (r - 2), cy + oy * (r - 2), 3, 1.2f);
        float bx = cx + ox * (r - 1);
        float by = cy + oy * (r - 1);
        b.rect(bx - 2.2f, by - 2.2f, 4.4f, 3.2f, 2);
    }
}

void buildArt(gs::VDP& vdp, Art& art) {
    const auto C = gs::rgb4;
    vdp.setColor(0, C(0, 0, 0));
    pal(vdp, PAL_INK, {C(15, 15, 14), C(4, 6, 8)});
    pal(vdp, PAL_GOLD, {C(15, 13, 4), C(9, 6, 1)});
    pal(vdp, PAL_HULL, {C(12, 8, 4), C(6, 4, 2), C(15, 14, 11), C(3, 2, 1), C(9, 10, 11), C(14, 4, 2)});
    pal(vdp, PAL_SAIL, {C(15, 15, 13), C(11, 12, 13), C(7, 8, 9), C(13, 3, 2)});
    pal(vdp, PAL_WHEEL, {C(10, 8, 5), C(5, 7, 9), C(8, 6, 3), C(3, 3, 3), C(14, 12, 8)});
    pal(vdp, PAL_MILL, {C(9, 7, 5), C(5, 4, 3), C(12, 10, 8), C(3, 5, 7), C(7, 3, 2)});
    pal(vdp, PAL_REED, {C(4, 9, 3), C(2, 6, 2), C(8, 11, 4), C(6, 5, 2)});
    pal(vdp, PAL_MARK, {C(15, 15, 14), C(13, 3, 2), C(2, 6, 10), C(15, 12, 3)});

    // Canal water in the road palette. Indices match the water style.
    vdp.setColor(PAL_WATER * 16 + 1, C(3, 8, 3));
    vdp.setColor(PAL_WATER * 16 + 2, C(2, 6, 2));
    vdp.setColor(PAL_WATER * 16 + 3, C(5, 9, 4));
    vdp.setColor(PAL_WATER * 16 + 4, C(6, 8, 4));
    vdp.setColor(PAL_WATER * 16 + 5, C(4, 7, 3));
    vdp.setColor(PAL_WATER * 16 + 6, C(2, 6, 10));
    vdp.setColor(PAL_WATER * 16 + 7, C(1, 5, 9));
    vdp.setColor(PAL_WATER * 16 + 8, C(8, 10, 6));
    vdp.setColor(PAL_WATER * 16 + 11, C(2, 7, 12));
    vdp.setColor(PAL_WATER * 16 + 12, C(1, 5, 10));
    vdp.setColor(PAL_WATER * 16 + 13, C(5, 11, 14));
    vdp.setColor(PAL_WATER * 16 + 15, C(3, 8, 12));
    vdp.setFogColor(C(8, 11, 13));

    for (int i = 0; i < 96; i++) art.glyph[i] = uploadGlyph(vdp, char(32 + i));

    {
        gs::Bitmap b(28, 44);
        b.poly({{14, 2}, {24, 14}, {22, 38}, {6, 38}, {4, 14}}, 1);
        b.poly({{14, 6}, {20, 15}, {18, 34}, {10, 34}, {8, 15}}, 2);
        b.rect(12, 16, 4, 8, 5);
        b.ellipse(14, 20, 1.4f, 1.4f, 6);
        b.line(6, 36, 22, 36, 4, 2.f);
        art.hull = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(22, 30);
        b.line(11, 28, 11, 2, 3, 1.4f);
        b.poly({{11, 4}, {20, 16}, {11, 22}}, 1);
        b.poly({{11, 8}, {4, 18}, {11, 24}}, 2);
        art.sail = gs::uploadMipped(vdp, b);
    }
    for (int k = 0; k < 3; k++) {
        gs::Bitmap b(42, 42);
        millWheel(b, 21, 21, 18, k * 0.4f);
        art.wheel[k] = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(36, 28);
        b.rect(2, 8, 32, 12, 1);
        b.rect(4, 10, 28, 8, 2);
        for (int i = 0; i < 5; i++) b.rect(6 + i * 5, 4, 3, 20, 3);
        b.ellipse(18, 14, 3, 3, 4);
        art.paddle = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(30, 36);
        b.rect(4, 14, 22, 20, 1);
        b.poly({{2, 16}, {15, 2}, {28, 16}}, 5);
        b.rect(12, 22, 6, 10, 3);
        b.rect(7, 18, 5, 5, 4);
        b.rect(18, 18, 5, 5, 4);
        art.mill = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(12, 28);
        b.line(6, 26, 4, 4, 1, 1.6f);
        b.line(6, 22, 10, 8, 3, 1.3f);
        b.line(6, 18, 2, 6, 2, 1.2f);
        b.ellipse(4, 4, 2.f, 3.f, 1);
        art.reed = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(12, 20);
        b.ellipse(6, 6, 4.5f, 4.5f, 2);
        b.ellipse(6, 6, 2.f, 2.f, 4);
        b.line(6, 10, 6, 19, 3, 1.4f);
        art.buoy = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(20, 28);
        b.line(3, 2, 3, 26, 1, 1.6f);
        b.poly({{4, 3}, {18, 8}, {4, 14}}, 2);
        art.flag = gs::uploadMipped(vdp, b);
    }
}

}  // namespace headerkilo
