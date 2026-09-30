#include "game/art.h"

namespace hornbell {
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(8, 5, 1), gs::rgb4(14, 10, 2), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(5, 5, 6)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(12, 3, 3)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(5, 3, 1), gs::rgb4(10, 7, 2), gs::rgb4(15, 12, 4), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(2, 2, 4), gs::rgb4(4, 5, 8), gs::rgb4(9, 8, 6), gs::rgb4(13, 11, 8)});
    setPal(vdp, PAL_YARD, {0, gs::rgb4(2, 3, 2), gs::rgb4(4, 5, 3), gs::rgb4(7, 6, 4), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(6, 6, 7), gs::rgb4(11, 11, 12), gs::rgb4(15, 15, 14), gs::rgb4(4, 4, 5)});

    {
        gs::Bitmap b(36, 72);
        b.ellipse(18, 10, 8, 9, 3);
        b.rect(12, 18, 12, 5, 4);
        b.rect(10, 22, 16, 24, 2);
        b.rect(10, 22, 16, 6, 1);
        b.rect(11, 46, 5, 18, 1);
        b.rect(20, 46, 5, 18, 1);
        b.rect(9, 62, 8, 5, 4);
        b.rect(19, 62, 8, 5, 4);
        art.player = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(78, 18);
        b.rect(0, 6, 6, 6, 1);
        b.rect(5, 5, 40, 8, 2);
        b.rect(7, 7, 34, 3, 3);
        b.ellipse(62, 9, 16, 8, 2);
        b.ellipse(66, 9, 8, 4, 3);
        art.horn = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 32);
        b.rect(12, 0, 4, 6, 1);
        b.ellipse(14, 18, 12, 12, 2);
        b.ellipse(14, 16, 6, 7, 3);
        b.rect(10, 26, 8, 4, 1);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(48, 8);
        b.rect(0, 2, 48, 4, 1);
        b.rect(22, 0, 4, 8, 3);
        art.yoke = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 10);
        b.ellipse(5, 5, 4, 4, 2);
        b.ellipse(5, 5, 2, 2, 3);
        art.lamp = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 90);
        b.rect(6, 0, 4, 90, 2);
        b.rect(2, 80, 12, 8, 3);
        art.stand = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace hornbell
