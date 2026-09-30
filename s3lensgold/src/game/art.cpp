#include "game/art.h"

namespace lensgold {
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
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 8), gs::rgb4(12, 8, 2)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(14, 12, 8), gs::rgb4(6, 5, 3), gs::rgb4(15, 15, 12), gs::rgb4(10, 8, 5)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(8, 6, 2), gs::rgb4(13, 10, 3), gs::rgb4(15, 13, 6), gs::rgb4(4, 3, 1),
                            gs::rgb4(6, 8, 9)});
    setPal(vdp, PAL_GLASS, {0, gs::rgb4(4, 8, 10), gs::rgb4(10, 14, 15), gs::rgb4(2, 4, 6), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_BENCH, {0, gs::rgb4(3, 2, 2), gs::rgb4(6, 4, 3), gs::rgb4(9, 7, 4), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_SPARK, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 12, 4)});

    loadFont(vdp, art.font);

    {
        gs::Bitmap b(44, 56);
        b.rect(8, 6, 28, 44, 1);
        b.rect(6, 4, 32, 6, 2);
        b.rect(6, 46, 32, 6, 2);
        b.ellipse(22, 28, 12, 14, 5);
        b.ellipse(22, 28, 7, 9, 3);
        b.rect(18, 18, 3, 16, 4);
        b.rect(10, 22, 4, 3, 3);
        art.lens = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(48, 48);
        b.ellipse(24, 24, 22, 22, 2);
        b.ellipse(24, 24, 16, 16, 1);
        b.ellipse(24, 24, 8, 8, 4);
        b.ellipse(24, 24, 3, 3, 3);
        art.plateGold = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(48, 48);
        b.ellipse(24, 24, 22, 22, 2);
        b.ellipse(24, 24, 16, 16, 4);
        b.ellipse(24, 24, 8, 8, 1);
        b.ellipse(24, 24, 3, 3, 3);
        art.plateCream = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(200, 28);
        b.rect(0, 10, 200, 16, 1);
        b.rect(0, 8, 200, 3, 2);
        b.rect(4, 18, 192, 2, 4);
        for (int i = 0; i < 8; i++) b.rect(12 + i * 24, 12, 3, 10, 3);
        art.bench = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(12, 12);
        b.poly({{6, 0}, {8, 4}, {12, 6}, {8, 8}, {6, 12}, {4, 8}, {0, 6}, {4, 4}}, 1);
        b.ellipse(6, 6, 2, 2, 2);
        art.spark = gs::uploadImage(vdp, b);
    }
}

}  // namespace lensgold
