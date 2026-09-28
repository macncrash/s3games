#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace plow {

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

static void spokeWheel(gs::Bitmap& b, int cx, int cy, int r, int rim, int hub, int spoke, float a0) {
    b.ellipse(float(cx), float(cy), float(r), float(r), rim);
    b.ellipse(float(cx), float(cy), float(r - 4), float(r - 4), 0);
    b.ellipse(float(cx), float(cy), 3.2f, 3.2f, hub);
    for (int s = 0; s < 6; s++) {
        float a = a0 + s * 1.0472f;
        b.line(float(cx), float(cy), cx + std::cos(a) * (r - 2), cy + std::sin(a) * (r - 2), spoke, 1.3f);
    }
}

void buildArt(gs::VDP& vdp, Art& art) {
    const auto C = gs::rgb4;
    vdp.setColor(0, C(0, 0, 0));
    pal(vdp, PAL_INK, {C(15, 15, 13), C(6, 5, 4)});
    pal(vdp, PAL_GOLD, {C(15, 12, 3), C(10, 6, 1)});
    pal(vdp, PAL_PLOW, {C(8, 8, 9), C(4, 4, 5), C(12, 8, 3), C(6, 4, 2), C(14, 13, 10), C(3, 3, 3)});
    pal(vdp, PAL_HORSE, {C(8, 5, 2), C(4, 2, 1), C(12, 9, 5), C(2, 2, 2), C(14, 12, 9)});
    pal(vdp, PAL_WHEEL, {C(11, 11, 12), C(3, 3, 4), C(7, 7, 8), C(14, 11, 5)});
    pal(vdp, PAL_WAGON, {C(9, 5, 2), C(5, 3, 1), C(12, 8, 4), C(3, 2, 1)});
    pal(vdp, PAL_TYRE, {C(2, 2, 2), C(5, 5, 5), C(10, 8, 3), C(8, 3, 2)});
    pal(vdp, PAL_FIELD, {C(4, 8, 2), C(2, 5, 1), C(7, 5, 2), C(9, 10, 4), C(5, 3, 1)});
    pal(vdp, PAL_MARK, {C(15, 15, 14), C(13, 3, 2), C(2, 2, 2)});

    // Dirt road palette: ground, verge, furrow, pebbles. See VDP road colours.
    vdp.setColor(PAL_DIRT * 16 + 1, C(5, 8, 2));
    vdp.setColor(PAL_DIRT * 16 + 2, C(3, 6, 1));
    vdp.setColor(PAL_DIRT * 16 + 3, C(7, 8, 3));
    vdp.setColor(PAL_DIRT * 16 + 4, C(8, 6, 2));
    vdp.setColor(PAL_DIRT * 16 + 5, C(6, 4, 2));
    vdp.setColor(PAL_DIRT * 16 + 6, C(9, 6, 3));
    vdp.setColor(PAL_DIRT * 16 + 7, C(7, 5, 2));
    vdp.setColor(PAL_DIRT * 16 + 8, C(11, 9, 5));
    vdp.setColor(PAL_DIRT * 16 + 9, C(5, 3, 1));
    vdp.setColor(PAL_DIRT * 16 + 10, C(12, 8, 3));
    vdp.setColor(PAL_DIRT * 16 + 15, C(10, 7, 3));
    vdp.setFogColor(C(12, 9, 6));

    for (int i = 0; i < 96; i++) art.glyph[i] = uploadGlyph(vdp, char(32 + i));

    {
        gs::Bitmap b(36, 48);
        b.rect(16, 2, 4, 16, 3);             // beam
        b.line(10, 6, 4, 14, 3, 1.6f);       // handles
        b.line(26, 6, 32, 14, 3, 1.6f);
        b.ellipse(6, 16, 2.2f, 2.2f, 5);
        b.ellipse(30, 16, 2.2f, 2.2f, 5);
        b.rect(14, 16, 8, 6, 4);             // frog
        b.poly({{8, 22}, {28, 22}, {30, 40}, {6, 40}}, 1);
        b.poly({{12, 26}, {24, 26}, {22, 38}, {14, 38}}, 2);
        b.line(8, 40, 28, 40, 6, 2.2f);      // share edge
        art.plow = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(22, 36);
        b.ellipse(11, 8, 5, 4, 1);           // head
        b.rect(9, 6, 2, 3, 4);               // ear
        b.rect(14, 7, 5, 2, 5);              // muzzle
        b.ellipse(11, 20, 7, 8, 1);
        b.rect(6, 26, 3, 8, 2);
        b.rect(13, 26, 3, 8, 2);
        b.rect(8, 32, 2, 3, 4);
        b.rect(14, 32, 2, 3, 4);
        b.line(16, 14, 20, 18, 3, 1.2f);     // tail
        art.horse = gs::uploadMipped(vdp, b);
    }
    for (int k = 0; k < 3; k++) {
        gs::Bitmap b(40, 40);
        spokeWheel(b, 20, 20, 17, 1, 4, 3, k * 0.45f);
        art.wheel[k] = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(28, 22);
        b.rect(2, 4, 24, 12, 1);
        b.rect(4, 6, 20, 8, 3);
        b.rect(0, 14, 6, 6, 2);
        b.rect(22, 14, 6, 6, 2);
        art.wagon = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(44, 44);
        b.ellipse(22, 22, 20, 20, 1);
        b.ellipse(22, 22, 14, 14, 2);
        b.ellipse(22, 22, 5, 5, 3);
        b.rect(20, 4, 4, 8, 4);
        art.tyre = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(8, 26);
        b.rect(3, 6, 2, 18, 3);
        b.rect(1, 2, 6, 5, 1);
        art.stake = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(28, 40);
        b.ellipse(14, 12, 12, 10, 1);
        b.ellipse(9, 14, 5, 4, 4);
        b.rect(12, 20, 4, 16, 3);
        art.tree = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(40, 28);
        b.poly({{2, 14}, {20, 2}, {38, 14}}, 2);
        b.rect(6, 14, 28, 12, 1);
        b.rect(17, 18, 6, 8, 4);
        b.rect(8, 17, 5, 4, 3);
        art.barn = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(16, 28);
        b.rect(2, 4, 2, 22, 3);
        b.poly({{4, 4}, {14, 8}, {4, 13}}, 2);
        art.flag = gs::uploadMipped(vdp, b);
    }
}

}  // namespace plow
