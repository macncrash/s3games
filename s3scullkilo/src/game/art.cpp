#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace kilo {

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
    pal(vdp, PAL_INK, {C(15, 15, 14), C(7, 9, 11)});
    pal(vdp, PAL_GOLD, {C(15, 13, 4), C(11, 7, 2)});
    pal(vdp, PAL_HULL, {C(14, 14, 12), C(12, 3, 3), C(4, 5, 7), C(9, 7, 5), C(15, 12, 8), C(2, 2, 3)});
    pal(vdp, PAL_OAR, {C(11, 8, 3), C(15, 14, 11), C(5, 4, 2), C(8, 12, 14)});
    pal(vdp, PAL_WHEEL, {C(13, 13, 12), C(4, 4, 5), C(8, 8, 9), C(15, 12, 6)});
    pal(vdp, PAL_CART, {C(6, 6, 6), C(3, 3, 3), C(10, 8, 4), C(12, 4, 3)});
    pal(vdp, PAL_MILL, {C(9, 6, 3), C(5, 3, 1), C(12, 9, 5), C(3, 5, 7)});
    pal(vdp, PAL_BANK, {C(3, 8, 3), C(2, 5, 2), C(6, 4, 2), C(7, 9, 4), C(4, 6, 3)});
    pal(vdp, PAL_MARK, {C(15, 15, 15), C(14, 3, 3), C(2, 2, 2)});

    vdp.setColor(PAL_WATER * 16 + 1, C(3, 8, 3));
    vdp.setColor(PAL_WATER * 16 + 2, C(2, 5, 2));
    vdp.setColor(PAL_WATER * 16 + 3, C(6, 7, 3));
    vdp.setColor(PAL_WATER * 16 + 4, C(8, 8, 6));
    vdp.setColor(PAL_WATER * 16 + 5, C(5, 5, 4));
    vdp.setColor(PAL_WATER * 16 + 6, C(2, 6, 10));
    vdp.setColor(PAL_WATER * 16 + 7, C(1, 4, 8));
    vdp.setColor(PAL_WATER * 16 + 8, C(7, 7, 6));
    vdp.setColor(PAL_WATER * 16 + 11, C(2, 7, 12));
    vdp.setColor(PAL_WATER * 16 + 12, C(1, 4, 8));
    vdp.setColor(PAL_WATER * 16 + 13, C(9, 13, 15));
    vdp.setColor(PAL_WATER * 16 + 14, C(14, 14, 12));
    vdp.setColor(PAL_WATER * 16 + 15, C(3, 8, 13));
    vdp.setFogColor(C(7, 11, 13));

    for (int i = 0; i < 96; i++) art.glyph[i] = uploadGlyph(vdp, char(32 + i));

    {
        gs::Bitmap b(28, 64);
        b.ellipse(14, 10, 5, 4, 5);          // hair
        b.rect(10, 13, 8, 6, 4);             // shoulders
        b.rect(12, 18, 4, 10, 2);            // singlet
        b.ellipse(14, 34, 3.4f, 12, 1);      // shell
        b.rect(13, 24, 2, 28, 2);
        b.ellipse(14, 54, 2.4f, 5, 1);       // bow
        b.line(8, 22, 2, 30, 3, 1.3f);       // riggers
        b.line(20, 22, 26, 30, 3, 1.3f);
        b.rect(6, 46, 16, 2, 6);             // splash rail
        art.hull = gs::uploadMipped(vdp, b);
    }

    for (int f = 0; f < 4; f++) {
        gs::Bitmap b(48, 16);
        float y1 = 4.0f + f * 2.4f;
        b.line(2, 8, 40, y1, 1, 1.8f);
        b.ellipse(43, y1, 3.4f, 2.1f, f == 0 || f == 3 ? 2 : 4);
        b.rect(0, 6, 4, 4, 3);
        art.oar[f] = gs::uploadMipped(vdp, b);
    }

    for (int k = 0; k < 3; k++) {
        gs::Bitmap b(40, 40);
        b.ellipse(20, 20, 17, 17, 1);
        b.ellipse(20, 20, 12, 12, 2);
        b.ellipse(20, 20, 4, 4, 4);
        float a0 = k * 0.55f;
        for (int s = 0; s < 6; s++) {
            float a = a0 + s * 1.0472f;
            b.line(20, 20, 20 + std::cos(a) * 15, 20 + std::sin(a) * 15, 3, 1.4f);
        }
        art.wheel[k] = gs::uploadMipped(vdp, b);
    }

    {
        gs::Bitmap b(22, 36);
        b.rect(8, 8, 6, 26, 2);
        b.rect(2, 4, 18, 6, 1);
        b.rect(4, 0, 4, 6, 3);
        b.rect(14, 0, 4, 6, 3);
        b.rect(6, 28, 10, 6, 4);
        art.mill = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(12, 20);
        b.line(2, 18, 4, 2, 1, 1.1f);
        b.line(6, 18, 5, 4, 2, 1.1f);
        b.line(9, 18, 10, 6, 4, 1.0f);
        art.reed = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(26, 36);
        b.ellipse(13, 12, 11, 10, 1);
        b.ellipse(9, 13, 5, 4, 4);
        b.rect(11, 20, 4, 14, 3);
        art.tree = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(8, 28);
        b.rect(3, 4, 2, 22, 3);
        b.ellipse(4, 4, 3, 3, 1);
        art.post = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(18, 12);
        b.rect(0, 2, 16, 8, 2);
        b.rect(2, 4, 10, 2, 1);
        art.flag = gs::uploadMipped(vdp, b);
    }
}

}  // namespace kilo
