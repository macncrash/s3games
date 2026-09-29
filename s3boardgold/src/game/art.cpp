#include "game/art.h"

#include <initializer_list>

namespace boardgold {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
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
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Bitmap cabinetArt() {
    gs::Bitmap b(280, 176);
    b.rect(0, 0, 280, 176, 1);
    b.rect(6, 6, 268, 164, 2);
    b.rect(8, 8, 264, 18, 3);
    b.rect(14, 36, 70, 124, 4);
    b.rect(196, 36, 70, 50, 5);
    b.rect(196, 100, 70, 50, 6);
    b.ellipse(49.f, 98.f, 16.f, 16.f, 7);
    b.ellipse(231.f, 61.f, 12.f, 12.f, 7);
    b.ellipse(231.f, 125.f, 12.f, 12.f, 7);
    for (int i = 0; i < 9; i++) b.rect(100 + (i % 3) * 22, 48 + (i / 3) * 28, 8, 16, 3);
    b.rect(18, 150, 62, 6, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(28, 36);
    b.rect(10, 22, 8, 12, 1);
    b.ellipse(14.f, 14.f, 12.f, 12.f, 2);
    b.ellipse(14.f, 14.f, 8.f, 8.f, 3);
    b.ellipse(10.f, 10.f, 3.f, 2.5f, 4);
    return b;
}

gs::Bitmap plugArt() {
    gs::Bitmap b(18, 22);
    b.rect(4, 8, 10, 12, 1);
    b.rect(6, 2, 2, 8, 2);
    b.rect(10, 2, 2, 8, 2);
    b.rect(6, 14, 6, 3, 3);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4.f, 4.f, 3.2f, 3.2f, 1);
    b.ellipse(3.f, 3.f, 1.1f, 1.1f, 2);
    return b;
}

gs::Bitmap plateArt() {
    gs::Bitmap b(54, 16);
    b.rect(0, 0, 54, 16, 1);
    b.rect(2, 2, 50, 12, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(4, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(12, 9, 4), gs::rgb4(3, 3, 4), gs::rgb4(6, 5, 2),
            gs::rgb4(7, 6, 4), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(6, 4, 1), gs::rgb4(14, 10, 2), gs::rgb4(15, 14, 4), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(8, 7, 5), gs::rgb4(13, 11, 8), gs::rgb4(15, 14, 11), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_CORD, {0, gs::rgb4(10, 8, 3), gs::rgb4(15, 13, 6)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 11), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 12, 3), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 10), gs::rgb4(1, 3, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 6, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 15, 12), gs::rgb4(1, 2, 2));

    loadFont(vdp, art);
    art.cabinet = gs::uploadImage(vdp, cabinetArt());
    art.lamp = gs::uploadImage(vdp, lampArt());
    art.plug = gs::uploadImage(vdp, plugArt());
    art.bead = gs::uploadImage(vdp, beadArt());
    art.plate = gs::uploadImage(vdp, plateArt());
}

}  // namespace boardgold
