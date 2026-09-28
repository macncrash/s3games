#include "game/art.h"

namespace scull {

static void putGlyph(gs::Bitmap& b, int dx, int dy, char ch, int c) {
    const uint8_t* g = gs::glyph(ch);
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 5; x++)
            if (g[y * 5 + x]) b.set(dx + x, dy + y, c);
}

static gs::Image uploadGlyph(gs::VDP& vdp, char ch) {
    gs::Bitmap b(5, 7);
    putGlyph(b, 0, 0, ch, 1);
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
    pal(vdp, PAL_GOLD, {C(15, 13, 5), C(12, 8, 2)});
    pal(vdp, PAL_HULL, {C(14, 14, 13), C(12, 2, 2), C(13, 9, 6), C(3, 3, 4), C(6, 8, 10), C(15, 12, 8)});
    pal(vdp, PAL_WOOD, {C(8, 5, 2), C(5, 3, 1), C(11, 8, 4), C(3, 3, 3), C(13, 12, 8)});
    pal(vdp, PAL_STONE, {C(9, 9, 8), C(6, 6, 6), C(4, 4, 4), C(12, 12, 11), C(7, 6, 5)});
    pal(vdp, PAL_BUOY, {C(15, 8, 1), C(15, 15, 15), C(12, 3, 1), C(2, 6, 3)});
    pal(vdp, PAL_OAR, {C(10, 7, 3), C(14, 13, 10), C(4, 3, 2)});
    pal(vdp, PAL_BANK, {C(3, 7, 2), C(2, 5, 2), C(5, 4, 2), C(6, 8, 3), C(8, 6, 3)});

    // Road bank: grass, verge stone, canal water and sparkle. Palette 12.
    vdp.setColor(PAL_WATER * 16 + 1, C(4, 8, 3));
    vdp.setColor(PAL_WATER * 16 + 2, C(2, 6, 2));
    vdp.setColor(PAL_WATER * 16 + 3, C(6, 7, 3));
    vdp.setColor(PAL_WATER * 16 + 4, C(8, 8, 7));
    vdp.setColor(PAL_WATER * 16 + 5, C(5, 5, 4));
    vdp.setColor(PAL_WATER * 16 + 6, C(2, 5, 9));
    vdp.setColor(PAL_WATER * 16 + 7, C(1, 4, 8));
    vdp.setColor(PAL_WATER * 16 + 8, C(7, 7, 6));
    vdp.setColor(PAL_WATER * 16 + 11, C(2, 6, 11));
    vdp.setColor(PAL_WATER * 16 + 12, C(1, 4, 8));
    vdp.setColor(PAL_WATER * 16 + 13, C(8, 12, 14));
    vdp.setColor(PAL_WATER * 16 + 14, C(14, 14, 12));
    vdp.setColor(PAL_WATER * 16 + 15, C(3, 7, 12));
    vdp.setFogColor(C(8, 11, 13));

    for (int i = 0; i < 96; i++) art.glyph[i] = uploadGlyph(vdp, char(32 + i));

    {
        gs::Bitmap b(32, 56);
        b.rect(12, 4, 8, 6, 6);             // hair
        b.rect(11, 10, 10, 8, 3);           // shoulders
        b.ellipse(16, 22, 5, 7, 3);         // back
        b.rect(14, 28, 4, 8, 2);            // singlet stripe
        b.ellipse(16, 36, 4.2f, 10, 1);     // shell
        b.rect(15, 30, 2, 22, 2);
        b.ellipse(16, 50, 3.2f, 4, 1);
        b.rect(10, 48, 12, 2, 5);           // stern deck
        b.line(8, 18, 2, 28, 4, 1.4f);      // riggers
        b.line(24, 18, 30, 28, 4, 1.4f);
        art.hull = gs::uploadMipped(vdp, b);
    }

    for (int f = 0; f < 5; f++) {
        gs::Bitmap b(52, 18);
        float ang = (f - 2) * 0.22f;
        float y1 = 9 + ang * 16;
        b.line(2, 9, 46, y1, 1, 2.0f);
        b.ellipse(48, y1, 3.2f, 2.2f, 2);  // blade
        b.rect(0, 7, 4, 4, 3);             // handle
        art.oar[f] = gs::uploadMipped(vdp, b);
    }

    {
        gs::Bitmap b(28, 72);
        b.rect(2, 0, 24, 70, 1);
        for (int y = 4; y < 68; y += 8) b.rect(2, y, 24, 1, 2);
        b.rect(0, 8, 4, 48, 4);            // hinge strap
        b.rect(6, 16, 16, 3, 4);
        b.rect(6, 40, 16, 3, 4);
        b.rect(4, 62, 20, 6, 3);           // sill
        b.rect(10, 28, 6, 8, 5);           // pale plank
        art.leaf = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(10, 40);
        b.rect(3, 0, 4, 36, 1);
        b.ellipse(5, 4, 4, 4, 2);
        b.rect(2, 34, 6, 4, 3);
        art.post = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(36, 48);
        b.rect(2, 8, 32, 38, 1);
        b.rect(0, 18, 36, 8, 2);
        b.rect(6, 4, 24, 8, 4);
        for (int y = 16; y < 44; y += 7) b.rect(4, y, 28, 1, 3);
        b.rect(8, 40, 20, 6, 5);
        art.pier = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(28, 40);
        b.ellipse(14, 14, 12, 12, 1);
        b.ellipse(10, 16, 6, 5, 4);
        b.rect(12, 22, 4, 16, 3);
        b.rect(8, 30, 6, 2, 5);
        art.tree = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(14, 22);
        b.line(3, 20, 5, 2, 1, 1.2f);
        b.line(7, 20, 6, 4, 2, 1.2f);
        b.line(10, 20, 11, 6, 4, 1.0f);
        art.reed = gs::uploadMipped(vdp, b);
    }
}

}  // namespace scull
