#include "game/art.h"

namespace lensseven {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, int* font) {
    gs::TileAlloc tiles(vdp, 1);
    for (int c = 0; c < 96; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c + 32));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + (x + 1)] = 1;
        font[c] = tiles.shared(px);
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 2), gs::rgb4(9, 6, 1), gs::rgb4(15, 15, 8), gs::rgb4(12, 9, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(14, 12, 9), gs::rgb4(7, 5, 3), gs::rgb4(15, 15, 12), gs::rgb4(10, 8, 6)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(5, 4, 2), gs::rgb4(10, 7, 2), gs::rgb4(14, 11, 4), gs::rgb4(3, 2, 1),
                            gs::rgb4(7, 9, 10)});
    setPal(vdp, PAL_GLASS, {0, gs::rgb4(3, 7, 9), gs::rgb4(9, 14, 15), gs::rgb4(2, 3, 5), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_RAIL, {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6), gs::rgb4(8, 7, 5), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_FLARE, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 11, 3), gs::rgb4(12, 14, 15)});

    loadFont(vdp, art.font);

    {
        gs::Bitmap b(40, 52);
        b.rect(10, 4, 20, 44, 1);
        b.rect(6, 2, 28, 7, 2);
        b.rect(6, 43, 28, 7, 2);
        b.ellipse(20, 26, 13, 15, 5);
        b.ellipse(20, 26, 8, 10, 3);
        b.ellipse(20, 26, 3, 4, 2);
        b.rect(16, 14, 2, 10, 4);
        b.rect(8, 20, 4, 3, 3);
        art.lens = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(46, 46);
        b.ellipse(23, 23, 21, 21, 2);
        b.ellipse(23, 23, 15, 15, 1);
        b.ellipse(23, 23, 8, 8, 4);
        b.ellipse(23, 23, 3, 3, 3);
        art.plateGold = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(46, 46);
        b.ellipse(23, 23, 21, 21, 2);
        b.ellipse(23, 23, 15, 15, 4);
        b.ellipse(23, 23, 8, 8, 1);
        b.ellipse(23, 23, 3, 3, 3);
        art.plateCream = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(220, 22);
        b.rect(0, 6, 220, 12, 1);
        b.rect(0, 4, 220, 3, 2);
        b.rect(0, 16, 220, 3, 4);
        for (int i = 0; i < 10; i++) b.rect(8 + i * 22, 8, 2, 8, 3);
        art.rail = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(14, 14);
        b.poly({{7, 0}, {9, 5}, {14, 7}, {9, 9}, {7, 14}, {5, 9}, {0, 7}, {5, 5}}, 1);
        b.ellipse(7, 7, 2, 2, 2);
        b.rect(6, 2, 2, 3, 3);
        art.flare = gs::uploadImage(vdp, b);
    }
}

}  // namespace lensseven
