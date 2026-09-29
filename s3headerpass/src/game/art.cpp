#include "game/art.h"

#include <initializer_list>

namespace headerpass {

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
    pal(vdp, PAL_INK, {C(14, 15, 15), C(3, 5, 7)});
    pal(vdp, PAL_GOLD, {C(15, 13, 4), C(8, 5, 1)});
    pal(vdp, PAL_BAD, {C(15, 5, 3), C(8, 2, 1)});
    pal(vdp, PAL_GOOD, {C(8, 15, 10), C(2, 7, 4)});
    pal(vdp, PAL_HULL, {C(11, 7, 4), C(5, 3, 2), C(15, 14, 12), C(2, 2, 2), C(8, 9, 10), C(13, 3, 2)});
    pal(vdp, PAL_SAIL, {C(15, 15, 13), C(10, 11, 12), C(6, 7, 8), C(12, 4, 3)});
    pal(vdp, PAL_ROCK, {C(7, 6, 6), C(4, 3, 4), C(10, 9, 8), C(3, 3, 4), C(12, 11, 9)});
    pal(vdp, PAL_CREST, {C(15, 15, 14), C(13, 3, 2), C(3, 7, 12), C(15, 12, 3)});

    vdp.setColor(PAL_WATER * 16 + 1, C(2, 5, 8));
    vdp.setColor(PAL_WATER * 16 + 2, C(1, 4, 7));
    vdp.setColor(PAL_WATER * 16 + 3, C(4, 8, 10));
    vdp.setColor(PAL_WATER * 16 + 4, C(5, 7, 8));
    vdp.setColor(PAL_WATER * 16 + 5, C(3, 6, 8));
    vdp.setColor(PAL_WATER * 16 + 6, C(2, 6, 11));
    vdp.setColor(PAL_WATER * 16 + 7, C(1, 4, 9));
    vdp.setColor(PAL_WATER * 16 + 8, C(6, 8, 9));
    vdp.setColor(PAL_WATER * 16 + 11, C(2, 7, 13));
    vdp.setColor(PAL_WATER * 16 + 12, C(1, 4, 10));
    vdp.setColor(PAL_WATER * 16 + 13, C(6, 12, 14));
    vdp.setColor(PAL_WATER * 16 + 15, C(3, 8, 12));
    vdp.setFogColor(C(7, 8, 10));

    for (int i = 0; i < 96; i++) art.glyph[i] = uploadGlyph(vdp, char(32 + i));

    {
        gs::Bitmap b(30, 48);
        b.poly({{15, 1}, {26, 16}, {24, 42}, {6, 42}, {4, 16}}, 1);
        b.poly({{15, 6}, {22, 17}, {20, 36}, {10, 36}, {8, 17}}, 2);
        b.rect(12, 18, 6, 9, 5);
        b.ellipse(15, 22, 1.6f, 1.6f, 6);
        b.line(7, 40, 23, 40, 4, 2.f);
        b.rect(13, 8, 4, 6, 3);
        art.hull = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(24, 32);
        b.line(12, 30, 12, 2, 3, 1.5f);
        b.poly({{12, 3}, {22, 16}, {12, 22}}, 1);
        b.poly({{12, 8}, {3, 19}, {12, 26}}, 2);
        b.rect(11, 1, 3, 3, 4);
        art.sail = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(28, 32);
        b.poly({{4, 30}, {8, 10}, {14, 4}, {20, 12}, {24, 30}}, 1);
        b.poly({{10, 28}, {12, 14}, {16, 10}, {18, 28}}, 2);
        b.rect(12, 18, 4, 6, 3);
        b.line(6, 28, 22, 28, 4, 2.f);
        art.rock = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(18, 40);
        b.poly({{9, 1}, {16, 18}, {14, 38}, {4, 38}, {2, 16}}, 1);
        b.poly({{9, 6}, {13, 18}, {11, 32}, {7, 32}, {5, 16}}, 5);
        b.line(9, 2, 9, 36, 3, 1.2f);
        art.spire = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(16, 28);
        b.line(8, 26, 8, 4, 3, 1.4f);
        b.poly({{8, 4}, {15, 8}, {8, 12}}, 2);
        b.poly({{8, 6}, {15, 10}, {8, 14}}, 4);
        b.rect(6, 24, 4, 3, 1);
        art.crest = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(36, 16);
        b.ellipse(10, 9, 8, 5, 1);
        b.ellipse(20, 8, 10, 6, 2);
        b.ellipse(28, 10, 7, 4, 1);
        art.cloud = gs::uploadMipped(vdp, b);
    }
}

}  // namespace headerpass
