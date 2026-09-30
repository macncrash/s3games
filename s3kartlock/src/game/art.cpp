#include "game/art.h"

namespace kartlock {

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
    pal(vdp, PAL_INK, {C(15, 15, 14), C(8, 9, 10)});
    pal(vdp, PAL_GOLD, {C(15, 13, 3), C(12, 7, 1)});
    pal(vdp, PAL_KART, {C(15, 12, 1), C(13, 4, 2), C(2, 2, 3), C(14, 14, 15), C(6, 4, 2), C(9, 8, 7)});
    pal(vdp, PAL_STEEL, {C(11, 12, 13), C(6, 7, 8), C(3, 4, 5), C(15, 10, 2)});
    pal(vdp, PAL_FLAG, {C(15, 15, 15), C(1, 1, 2), C(13, 3, 2), C(4, 6, 3)});
    pal(vdp, PAL_TREE, {C(2, 7, 3), C(1, 4, 2), C(7, 5, 2), C(10, 12, 8)});

    // Tarmac, grass verge, yellow paint. Indices match the road generator.
    vdp.setColor(PAL_ROAD * 16 + 1, C(3, 8, 3));
    vdp.setColor(PAL_ROAD * 16 + 2, C(2, 6, 2));
    vdp.setColor(PAL_ROAD * 16 + 3, C(5, 7, 3));
    vdp.setColor(PAL_ROAD * 16 + 4, C(5, 6, 3));
    vdp.setColor(PAL_ROAD * 16 + 5, C(3, 5, 2));
    vdp.setColor(PAL_ROAD * 16 + 6, C(4, 4, 5));
    vdp.setColor(PAL_ROAD * 16 + 7, C(3, 3, 4));
    vdp.setColor(PAL_ROAD * 16 + 8, C(6, 6, 6));
    vdp.setColor(PAL_ROAD * 16 + 9, C(5, 5, 6));
    vdp.setColor(PAL_ROAD * 16 + 10, C(7, 7, 8));
    vdp.setColor(PAL_ROAD * 16 + 14, C(14, 12, 2));
    vdp.setColor(PAL_ROAD * 16 + 15, C(6, 6, 7));
    vdp.setFogColor(C(8, 9, 11));

    for (int i = 0; i < 96; i++) art.glyph[i] = uploadGlyph(vdp, char(32 + i));

    {
        gs::Bitmap b(40, 52);
        b.ellipse(20, 28, 12, 16, 1);          // nose and tub
        b.rect(8, 16, 24, 22, 1);
        b.ellipse(20, 18, 6, 5, 4);            // helmet
        b.rect(16, 16, 8, 3, 3);               // visor
        b.ellipse(8, 14, 5, 7, 3);             // rear wheels
        b.ellipse(32, 14, 5, 7, 3);
        b.ellipse(10, 40, 5, 7, 3);            // front wheels
        b.ellipse(30, 40, 5, 7, 3);
        b.rect(14, 36, 12, 4, 2);              // bumper
        b.rect(18, 8, 4, 6, 6);                // exhaust
        art.kart = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(16, 72);
        b.rect(2, 4, 12, 64, 1);
        b.rect(0, 0, 16, 6, 4);
        b.rect(4, 14, 8, 8, 2);
        b.rect(4, 32, 8, 8, 2);
        b.rect(4, 50, 8, 8, 2);
        b.line(2, 4, 14, 68, 3, 1.2f);
        art.gate = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(10, 40);
        b.rect(4, 12, 2, 28, 4);
        for (int i = 0; i < 6; i++) b.rect((i & 1) ? 1 : 5, 2 + i * 2, 4, 2, (i & 1) ? 1 : 2);
        art.post = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(28, 44);
        b.poly({{14, 2}, {26, 28}, {2, 28}}, 1);
        b.poly({{14, 12}, {22, 36}, {6, 36}}, 2);
        b.rect(12, 36, 4, 6, 3);
        art.tree = gs::uploadMipped(vdp, b);
    }
}

}  // namespace kartlock
