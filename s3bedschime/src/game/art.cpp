#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace bedschime {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t edge) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 2, edge);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8 && x + 2 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Bitmap bedArt() {
    gs::Bitmap b(72, 40);
    b.rect(2, 8, 68, 28, 1);
    b.rect(0, 6, 72, 6, 2);
    b.rect(0, 32, 72, 6, 3);
    b.rect(4, 12, 64, 18, 4);
    return b;
}

gs::Bitmap soilArt() {
    gs::Bitmap b(60, 16);
    b.rect(0, 0, 60, 16, 1);
    for (int i = 0; i < 8; i++) b.ellipse(6.f + i * 7.f, 8.f, 2.2f, 1.6f, 2);
    return b;
}

gs::Bitmap plantArt() {
    gs::Bitmap b(18, 22);
    b.rect(8, 12, 2, 10, 1);
    b.ellipse(9.f, 8.f, 7.f, 6.f, 2);
    b.ellipse(6.f, 6.f, 2.f, 1.4f, 3);
    return b;
}

gs::Bitmap canArt() {
    gs::Bitmap b(16, 22);
    b.rect(3, 4, 10, 14, 1);
    b.rect(4, 2, 8, 3, 2);
    b.rect(12, 6, 3, 2, 2);
    b.rect(1, 16, 4, 5, 3);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(44, 44);
    b.ellipse(22.f, 22.f, 21.f, 21.f, 1);
    b.ellipse(22.f, 22.f, 17.f, 17.f, 2);
    b.ellipse(22.f, 22.f, 2.f, 2.f, 3);
    for (int i = 0; i < 12; i++) {
        float a = float(i) * 0.5236f - 1.5708f;
        int x = int(22.f + std::cos(a) * 14.f);
        int y = int(22.f + std::sin(a) * 14.f);
        b.rect(x, y, 2, 2, i % 3 == 0 ? 4 : 3);
    }
    return b;
}

gs::Bitmap handArt() {
    gs::Bitmap b(3, 12);
    b.rect(1, 0, 1, 12, 1);
    return b;
}

gs::Bitmap dropArt() {
    gs::Bitmap b(6, 8);
    b.ellipse(3.f, 4.f, 2.2f, 3.f, 1);
    b.rect(2, 1, 2, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_YARD, {0, gs::rgb4(3, 6, 2), gs::rgb4(5, 8, 3), gs::rgb4(8, 6, 3)});
    setPal(vdp, PAL_BED,
           {0, gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 4), gs::rgb4(6, 3, 2), gs::rgb4(10, 7, 3)});
    setPal(vdp, PAL_WET, {0, gs::rgb4(4, 5, 8), gs::rgb4(7, 8, 11)});
    setPal(vdp, PAL_PLANT, {0, gs::rgb4(3, 6, 2), gs::rgb4(6, 12, 3), gs::rgb4(12, 15, 6)});
    setPal(vdp, PAL_CAN, {0, gs::rgb4(4, 8, 10), gs::rgb4(10, 13, 14), gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(8, 6, 2), gs::rgb4(14, 12, 8), gs::rgb4(4, 3, 2), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_DROP, {0, gs::rgb4(8, 12, 15)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 2, 1));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 5), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 11), gs::rgb4(1, 3, 1));
    textPal(vdp, PAL_DEAD, gs::rgb4(15, 8, 6), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 15, 12), gs::rgb4(1, 2, 2));

    loadFont(vdp, art);
    art.bed = gs::uploadImage(vdp, bedArt());
    art.soil = gs::uploadImage(vdp, soilArt());
    art.plant = gs::uploadImage(vdp, plantArt());
    art.can = gs::uploadImage(vdp, canArt());
    art.clock = gs::uploadImage(vdp, clockArt());
    art.hand = gs::uploadImage(vdp, handArt());
    art.drop = gs::uploadImage(vdp, dropArt());
}

}  // namespace bedschime
