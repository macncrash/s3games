#include "game/art.h"

#include <initializer_list>

namespace tilegold {
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

gs::Bitmap floorArt() {
    gs::Bitmap b(280, 160);
    b.rect(0, 0, 280, 160, 1);
    b.rect(8, 8, 264, 144, 2);
    for (int i = 0; i < 7; i++) {
        int x = 18 + i * 36;
        b.rect(float(x), 36.f, 28.f, 28.f, 3);
        b.rect(float(x + 3), 39.f, 22.f, 22.f, 4);
    }
    b.rect(18, 88, 244, 8, 5);
    b.rect(18, 112, 90, 22, 6);
    b.rect(172, 112, 90, 22, 7);
    return b;
}

gs::Bitmap tileArt() {
    gs::Bitmap b(26, 26);
    b.rect(0, 0, 26, 26, 1);
    b.rect(2, 2, 22, 22, 2);
    b.rect(5, 5, 7, 7, 3);
    b.rect(14, 14, 6, 6, 3);
    b.rect(14, 5, 4, 4, 4);
    return b;
}

gs::Bitmap chipArt() {
    gs::Bitmap b(10, 10);
    b.rect(0, 0, 10, 10, 1);
    b.rect(2, 2, 6, 6, 2);
    return b;
}

gs::Bitmap trowelArt() {
    gs::Bitmap b(36, 16);
    b.rect(0, 4, 22, 8, 1);
    b.rect(18, 6, 16, 4, 2);
    b.rect(4, 6, 8, 3, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_FLOOR,
           {0, gs::rgb4(3, 3, 4), gs::rgb4(5, 5, 6), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2), gs::rgb4(7, 6, 4),
            gs::rgb4(8, 6, 2), gs::rgb4(9, 8, 6)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(7, 5, 1), gs::rgb4(14, 11, 2), gs::rgb4(15, 14, 5), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(8, 7, 5), gs::rgb4(13, 12, 9), gs::rgb4(15, 14, 11), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_GROUT, {0, gs::rgb4(6, 6, 7), gs::rgb4(10, 9, 6), gs::rgb4(14, 12, 6)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 11), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 12, 3), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 10), gs::rgb4(1, 3, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 6, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 15, 12), gs::rgb4(1, 2, 2));

    loadFont(vdp, art);
    art.floor = gs::uploadImage(vdp, floorArt());
    art.tile = gs::uploadImage(vdp, tileArt());
    art.chip = gs::uploadImage(vdp, chipArt());
    art.trowel = gs::uploadImage(vdp, trowelArt());
}

}  // namespace tilegold
