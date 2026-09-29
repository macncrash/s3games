#include "game/art.h"

namespace mushlock {

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
    pal(vdp, PAL_INK, {C(15, 15, 15), C(9, 11, 13)});
    pal(vdp, PAL_GOLD, {C(15, 13, 4), C(11, 7, 2)});
    pal(vdp, PAL_TEAM, {C(14, 14, 15), C(12, 3, 2), C(6, 4, 3), C(2, 2, 3), C(15, 12, 8), C(9, 6, 3)});
    pal(vdp, PAL_WOOD, {C(9, 6, 2), C(5, 3, 1), C(12, 9, 4), C(3, 3, 4)});
    pal(vdp, PAL_STONE, {C(10, 10, 11), C(6, 6, 7), C(4, 4, 5), C(13, 13, 14)});
    pal(vdp, PAL_FLAG, {C(14, 4, 2), C(15, 15, 14), C(3, 5, 8)});
    pal(vdp, PAL_PINE, {C(2, 6, 3), C(1, 4, 2), C(8, 10, 8), C(4, 5, 3)});

    // Packed snow, verge, and the blue cut of the lock water under the ice.
    vdp.setColor(PAL_SNOW * 16 + 1, C(12, 13, 14));
    vdp.setColor(PAL_SNOW * 16 + 2, C(8, 10, 12));
    vdp.setColor(PAL_SNOW * 16 + 3, C(6, 8, 9));
    vdp.setColor(PAL_SNOW * 16 + 4, C(5, 6, 7));
    vdp.setColor(PAL_SNOW * 16 + 5, C(3, 4, 5));
    vdp.setColor(PAL_SNOW * 16 + 6, C(13, 14, 15));
    vdp.setColor(PAL_SNOW * 16 + 7, C(10, 12, 14));
    vdp.setColor(PAL_SNOW * 16 + 8, C(7, 8, 10));
    vdp.setColor(PAL_SNOW * 16 + 9, C(11, 12, 14));
    vdp.setColor(PAL_SNOW * 16 + 14, C(15, 15, 15));
    vdp.setColor(PAL_SNOW * 16 + 15, C(14, 15, 15));
    vdp.setFogColor(C(10, 12, 14));

    for (int i = 0; i < 96; i++) art.glyph[i] = uploadGlyph(vdp, char(32 + i));

    {
        gs::Bitmap b(36, 48);
        b.ellipse(18, 30, 12, 6, 1);          // basket
        b.rect(8, 26, 20, 5, 2);              // red blanket
        b.ellipse(18, 16, 4, 5, 5);           // musher
        b.rect(15, 10, 6, 5, 6);              // hood
        b.line(6, 34, 2, 44, 3, 2.0f);        // runners
        b.line(30, 34, 34, 44, 3, 2.0f);
        b.line(4, 44, 32, 44, 4, 1.6f);
        b.rect(14, 20, 8, 8, 2);
        art.sled = gs::uploadMipped(vdp, b);
    }
    for (int f = 0; f < 3; f++) {
        gs::Bitmap b(28, 16);
        int lift = (f == 1) ? 2 : 0;
        b.ellipse(14, 8, 8, 4, 3);            // body
        b.ellipse(22, 6, 3.2f, 2.6f, 6);      // head
        b.rect(20, 4, 2, 2, 4);               // ear
        b.line(8, 11, 6, 14 - lift, 3, 1.4f);
        b.line(16, 11, 18, 14 - (2 - lift), 3, 1.4f);
        b.line(6, 8, 2, 7, 3, 1.2f);          // tail
        art.dog[f] = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(20, 64);
        b.rect(2, 8, 16, 52, 1);
        b.rect(0, 6, 20, 6, 3);
        b.rect(4, 18, 12, 4, 2);
        b.rect(4, 34, 12, 4, 2);
        b.line(2, 8, 18, 60, 4, 1.2f);
        art.leaf = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(18, 40);
        b.rect(2, 8, 14, 32, 1);
        b.rect(0, 4, 18, 6, 4);
        b.rect(6, 16, 6, 18, 2);
        art.pier = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(28, 48);
        b.poly({{14, 2}, {26, 30}, {2, 30}}, 1);
        b.poly({{14, 12}, {24, 40}, {4, 40}}, 2);
        b.rect(12, 40, 4, 6, 4);
        b.rect(8, 18, 6, 3, 3);
        art.pine = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(8, 36);
        b.rect(3, 10, 2, 26, 3);
        b.poly({{4, 2}, {8, 12}, {4, 10}}, 1);
        b.rect(2, 10, 3, 3, 2);
        art.post = gs::uploadMipped(vdp, b);
    }
}

}  // namespace mushlock
