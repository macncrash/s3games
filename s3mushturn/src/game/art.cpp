#include "game/art.h"

namespace mushturn {

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
    pal(vdp, PAL_INK, {C(15, 15, 15), C(7, 9, 12)});
    pal(vdp, PAL_GOLD, {C(15, 13, 4), C(9, 6, 2)});
    pal(vdp, PAL_TEAM, {C(14, 14, 15), C(12, 3, 2), C(4, 3, 3), C(15, 11, 7), C(7, 5, 3), C(2, 2, 3)});
    pal(vdp, PAL_PINE, {C(2, 7, 3), C(1, 4, 2), C(11, 13, 11), C(5, 4, 2), C(8, 6, 3)});
    pal(vdp, PAL_SIGN, {C(15, 12, 2), C(2, 2, 3), C(14, 4, 2), C(15, 15, 14)});
    pal(vdp, PAL_ARCH, {C(14, 3, 3), C(15, 15, 14), C(3, 5, 9), C(8, 5, 2)});

    vdp.setColor(PAL_SNOW * 16 + 1, C(13, 14, 15));
    vdp.setColor(PAL_SNOW * 16 + 2, C(9, 11, 13));
    vdp.setColor(PAL_SNOW * 16 + 3, C(5, 6, 8));
    vdp.setColor(PAL_SNOW * 16 + 4, C(3, 4, 5));
    vdp.setColor(PAL_SNOW * 16 + 5, C(7, 8, 10));
    vdp.setColor(PAL_SNOW * 16 + 6, C(15, 15, 15));
    vdp.setColor(PAL_SNOW * 16 + 14, C(15, 15, 15));
    vdp.setColor(PAL_SNOW * 16 + 15, C(12, 13, 15));
    vdp.setFogColor(C(8, 10, 13));

    for (int i = 0; i < 96; i++) art.glyph[i] = uploadGlyph(vdp, char(32 + i));

    {
        gs::Bitmap b(22, 36);
        b.ellipse(11, 6, 4, 4, 4);
        b.rect(8, 11, 7, 10, 2);
        b.rect(6, 12, 3, 7, 2);
        b.rect(14, 13, 4, 3, 2);
        b.line(9, 21, 7, 32, 3, 2.0f);
        b.line(14, 21, 16, 32, 3, 2.0f);
        b.rect(5, 31, 5, 2, 5);
        b.rect(13, 31, 5, 2, 5);
        art.musher = gs::uploadMipped(vdp, b);
    }
    for (int f = 0; f < 3; f++) {
        gs::Bitmap b(34, 16);
        int lift = (f == 1) ? 3 : (f == 2) ? 1 : 0;
        b.ellipse(16, 8, 10, 4, 5);
        b.ellipse(27, 6, 4, 3, 4);
        b.rect(26, 2, 2, 3, 6);
        b.set(29, 6, 6);
        b.line(10, 12, 6, 15 - lift, 5, 1.5f);
        b.line(20, 12, 24, 15 - (2 - (lift > 0)), 5, 1.5f);
        b.line(7, 7, 1, 5, 5, 1.3f);
        art.dog[f] = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(36, 28);
        b.poly({{4, 8}, {32, 8}, {30, 16}, {6, 16}}, 1);
        b.rect(8, 4, 16, 5, 2);
        b.line(6, 16, 2, 26, 3, 2.0f);
        b.line(30, 16, 34, 26, 3, 2.0f);
        b.line(2, 26, 34, 26, 6, 1.5f);
        b.rect(14, 10, 8, 4, 3);
        art.sled = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(28, 48);
        b.poly({{14, 2}, {26, 28}, {2, 28}}, 1);
        b.poly({{14, 12}, {22, 32}, {6, 32}}, 2);
        b.poly({{14, 20}, {20, 38}, {8, 38}}, 1);
        b.rect(12, 38, 4, 8, 4);
        b.ellipse(14, 44, 6, 2, 5);
        art.spruce = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(24, 28);
        b.rect(2, 2, 20, 24, 2);
        b.poly({{6, 8}, {16, 14}, {6, 20}}, 1);
        b.poly({{10, 8}, {20, 14}, {10, 20}}, 3);
        art.chevron = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(20, 40);
        b.rect(8, 8, 4, 30, 4);
        b.rect(4, 2, 12, 8, 1);
        b.rect(6, 4, 8, 4, 2);
        b.rect(7, 36, 6, 3, 3);
        art.arch = gs::uploadMipped(vdp, b);
    }
}

}  // namespace mushturn
