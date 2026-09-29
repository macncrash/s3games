#include "game/art.h"

#include <initializer_list>

namespace headerboom {

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

void buildArt(gs::VDP& vdp, Art& art) {
    const auto C = gs::rgb4;
    vdp.setColor(0, C(0, 0, 0));
    pal(vdp, PAL_INK, {C(15, 15, 14), C(3, 5, 7)});
    pal(vdp, PAL_GOLD, {C(15, 13, 4), C(8, 5, 1)});
    pal(vdp, PAL_BAD, {C(15, 6, 4), C(8, 2, 2)});
    pal(vdp, PAL_GOOD, {C(8, 15, 8), C(2, 8, 3)});
    pal(vdp, PAL_HULL, {C(10, 4, 3), C(5, 2, 2), C(14, 12, 8), C(2, 2, 2), C(15, 14, 10), C(6, 8, 6)});
    pal(vdp, PAL_DRIVE, {C(11, 8, 4), C(6, 4, 2), C(13, 13, 12), C(4, 4, 5), C(15, 12, 3)});
    pal(vdp, PAL_BOOM, {C(9, 6, 3), C(5, 3, 2), C(12, 9, 5), C(3, 2, 1)});
    pal(vdp, PAL_BANK, {C(3, 8, 3), C(2, 5, 2), C(7, 10, 4), C(5, 4, 2), C(4, 7, 9)});
    pal(vdp, PAL_MILL, {C(8, 6, 5), C(4, 3, 3), C(11, 9, 7), C(6, 2, 2)});
    pal(vdp, PAL_SKY, {C(15, 15, 14), C(12, 13, 14), C(8, 9, 10)});
    vdp.setFogColor(C(8, 11, 13));

    for (int i = 0; i < 96; i++) art.glyph[i] = uploadGlyph(vdp, char(32 + i));

    {
        gs::Bitmap b(96, 28);
        b.poly({{4, 16}, {14, 22}, {88, 22}, {92, 14}, {84, 8}, {18, 8}}, 1);
        b.rect(18, 10, 62, 8, 2);
        b.rect(20, 12, 22, 4, 3);
        b.line(8, 18, 90, 18, 5, 1.2f);
        b.rect(70, 11, 8, 5, 6);
        art.hull = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(28, 22);
        b.rect(2, 8, 24, 12, 3);
        b.poly({{2, 8}, {14, 1}, {26, 8}}, 1);
        b.rect(6, 11, 6, 5, 4);
        b.rect(16, 11, 5, 5, 5);
        b.rect(12, 14, 4, 6, 2);
        art.cabin = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(20, 36);
        b.line(4, 34, 4, 2, 4, 1.5f);
        b.poly({{4, 4}, {18, 14}, {4, 22}}, 3);
        b.poly({{4, 16}, {14, 26}, {4, 32}}, 5);
        art.sail = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(22, 18);
        b.rect(1, 4, 20, 13, 1);
        b.rect(3, 6, 16, 9, 2);
        b.rect(8, 1, 6, 4, 4);
        b.ellipse(11, 3, 2.2f, 2.2f, 5);
        b.line(4, 8, 18, 14, 3, 1.f);
        b.line(4, 14, 18, 8, 3, 1.f);
        art.drive = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(48, 10);
        b.rect(0, 2, 48, 6, 1);
        b.rect(0, 2, 48, 2, 3);
        for (int i = 0; i < 8; i++) b.line(float(i * 6), 3, float(i * 6), 7, 2, 1.f);
        art.plank = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(8, 28);
        b.rect(2, 0, 4, 28, 1);
        b.rect(2, 0, 2, 28, 3);
        b.rect(1, 22, 6, 4, 2);
        art.post = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(14, 24);
        for (int i = 0; i < 5; i++) {
            float x = 2.f + i * 2.2f;
            b.line(x, 22, x + (i - 2) * 0.6f, 2, (i % 2) ? 1 : 3, 1.1f);
        }
        art.reed = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(36, 40);
        b.line(18, 38, 18, 16, 4, 2.f);
        b.ellipse(18, 14, 14, 12, 1);
        b.ellipse(12, 16, 8, 7, 2);
        b.ellipse(24, 15, 8, 8, 3);
        art.willow = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(32, 36);
        b.rect(6, 16, 20, 18, 1);
        b.poly({{4, 16}, {16, 4}, {28, 16}}, 2);
        b.rect(13, 24, 6, 10, 4);
        b.rect(9, 19, 5, 5, 3);
        art.mill = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(32, 14);
        b.ellipse(10, 8, 8, 5, 1);
        b.ellipse(18, 7, 9, 6, 2);
        b.ellipse(24, 9, 6, 4, 1);
        art.cloud = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(12, 8);
        b.poly({{1, 4}, {6, 2}, {11, 4}, {6, 6}}, 1);
        b.poly({{6, 3}, {11, 1}, {10, 4}}, 2);
        art.bird = gs::uploadMipped(vdp, b);
    }
}

}  // namespace headerboom
