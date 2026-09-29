#include "game/art.h"

namespace mushpass {

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
    pal(vdp, PAL_INK, {C(15, 15, 15), C(8, 10, 12)});
    pal(vdp, PAL_GOLD, {C(15, 13, 5), C(10, 6, 2)});
    pal(vdp, PAL_TEAM, {C(14, 14, 15), C(13, 3, 2), C(5, 4, 3), C(2, 2, 4), C(15, 12, 8), C(8, 5, 3)});
    pal(vdp, PAL_ROCK, {C(8, 8, 9), C(5, 5, 6), C(12, 12, 13), C(3, 3, 4), C(9, 7, 6)});
    pal(vdp, PAL_FLAG, {C(14, 3, 2), C(15, 15, 13), C(3, 4, 7), C(11, 8, 3)});
    pal(vdp, PAL_PINE, {C(2, 6, 3), C(1, 3, 2), C(10, 12, 10), C(4, 4, 3)});
    pal(vdp, PAL_STORM, {C(11, 12, 14), C(7, 8, 11), C(14, 14, 15), C(4, 5, 8)});

    vdp.setColor(PAL_SNOW * 16 + 1, C(13, 14, 15));
    vdp.setColor(PAL_SNOW * 16 + 2, C(9, 11, 13));
    vdp.setColor(PAL_SNOW * 16 + 3, C(6, 7, 9));
    vdp.setColor(PAL_SNOW * 16 + 4, C(4, 5, 6));
    vdp.setColor(PAL_SNOW * 16 + 5, C(3, 3, 4));
    vdp.setColor(PAL_SNOW * 16 + 6, C(14, 15, 15));
    vdp.setColor(PAL_SNOW * 16 + 7, C(8, 9, 11));
    vdp.setColor(PAL_SNOW * 16 + 8, C(5, 6, 8));
    vdp.setColor(PAL_SNOW * 16 + 9, C(11, 12, 14));
    vdp.setColor(PAL_SNOW * 16 + 14, C(15, 15, 15));
    vdp.setColor(PAL_SNOW * 16 + 15, C(12, 13, 15));
    vdp.setFogColor(C(8, 9, 12));

    for (int i = 0; i < 96; i++) art.glyph[i] = uploadGlyph(vdp, char(32 + i));

    {
        gs::Bitmap b(40, 50);
        b.ellipse(20, 28, 13, 7, 1);
        b.rect(8, 24, 24, 6, 2);
        b.ellipse(20, 14, 4, 5, 5);
        b.rect(16, 8, 8, 6, 6);
        b.line(6, 32, 2, 46, 3, 2.0f);
        b.line(34, 32, 38, 46, 3, 2.0f);
        b.line(3, 46, 37, 46, 4, 1.6f);
        b.rect(16, 18, 8, 8, 2);
        art.sled = gs::uploadMipped(vdp, b);
    }
    for (int f = 0; f < 3; f++) {
        gs::Bitmap b(30, 18);
        int lift = (f == 1) ? 3 : 0;
        b.ellipse(14, 8, 9, 4, 3);
        b.ellipse(24, 6, 3.4f, 2.8f, 6);
        b.rect(22, 3, 2, 3, 4);
        b.line(8, 12, 5, 16 - lift, 3, 1.4f);
        b.line(18, 12, 21, 16 - (3 - lift), 3, 1.4f);
        b.line(6, 8, 1, 6, 3, 1.2f);
        art.dog[f] = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(48, 64);
        b.poly({{24, 4}, {46, 58}, {2, 58}}, 1);
        b.poly({{24, 16}, {38, 50}, {10, 50}}, 2);
        b.poly({{24, 6}, {34, 22}, {14, 22}}, 5);
        b.rect(22, 54, 4, 8, 4);
        art.peak = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(16, 28);
        b.ellipse(8, 22, 6, 3, 2);
        b.rect(6, 16, 4, 6, 1);
        b.rect(5, 12, 6, 5, 3);
        b.rect(6, 8, 4, 5, 1);
        b.ellipse(8, 6, 3, 2, 5);
        art.cairn = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(10, 42);
        b.rect(4, 8, 2, 34, 4);
        b.poly({{5, 2}, {10, 14}, {5, 12}}, 1);
        b.rect(1, 10, 5, 8, 2);
        b.rect(1, 18, 4, 3, 3);
        art.banner = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(7, 7);
        b.line(1, 3, 5, 3, 1, 1.2f);
        b.line(3, 1, 3, 5, 1, 1.2f);
        b.set(2, 2, 3);
        b.set(4, 4, 3);
        art.flake = gs::uploadMipped(vdp, b);
    }
}

}  // namespace mushpass
