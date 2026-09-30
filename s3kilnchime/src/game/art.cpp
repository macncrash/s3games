#include "game/art.h"

namespace kilnchime {
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
    setPal(vdp, PAL_BELL, {0, gs::rgb4(14, 11, 3), gs::rgb4(8, 6, 2), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(13, 3, 2)});
    setPal(vdp, PAL_CLAY, {0, gs::rgb4(12, 7, 4), gs::rgb4(7, 4, 2), gs::rgb4(15, 12, 8)});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(6, 2, 1), gs::rgb4(10, 4, 2), gs::rgb4(13, 6, 3), gs::rgb4(3, 1, 1),
                            gs::rgb4(15, 8, 2), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(12, 3, 1), gs::rgb4(15, 9, 2), gs::rgb4(15, 14, 5), gs::rgb4(8, 2, 1)});
    setPal(vdp, PAL_ASH, {0, gs::rgb4(4, 4, 4), gs::rgb4(7, 7, 6), gs::rgb4(3, 3, 3)});

    {
        gs::Bitmap b(100, 92);
        b.rect(10, 20, 80, 60, 1);
        b.rect(16, 26, 68, 48, 4);
        b.poly({{20, 50}, {80, 50}, {72, 72}, {28, 72}}, 5);
        b.ellipse(50, 48, 20, 16, 5);
        b.ellipse(50, 48, 12, 9, 3);
        b.rect(42, 8, 16, 16, 2);
        b.rect(46, 2, 8, 10, 6);
        for (int row = 0; row < 4; row++)
            for (int col = 0; col < 5; col++) b.rect(14 + col * 15, 24 + row * 14, 12, 3, 2);
        b.rect(8, 78, 84, 8, 1);
        art.kiln = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 40);
        b.ellipse(14, 24, 10, 12, 1);
        b.rect(6, 10, 16, 8, 1);
        b.poly({{5, 12}, {23, 12}, {20, 5}, {8, 5}}, 2);
        b.ellipse(14, 6, 8, 3, 2);
        b.rect(12, 14, 3, 12, 3);
        b.rect(9, 33, 10, 4, 2);
        art.pot = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(18, 26);
        b.poly({{9, 1}, {15, 11}, {12, 11}, {16, 21}, {9, 15}, {2, 24}, {6, 11}, {3, 11}}, 2);
        b.poly({{9, 6}, {12, 13}, {9, 17}, {6, 13}}, 3);
        b.ellipse(9, 18, 3, 4, 1);
        art.flame = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(150, 14);
        b.rect(0, 3, 150, 8, 1);
        b.rect(0, 0, 150, 4, 2);
        for (int i = 0; i < 11; i++) b.rect(4 + i * 13, 5, 7, 4, 3);
        art.shelf = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 36);
        b.poly({{14, 2}, {24, 10}, {22, 26}, {6, 26}, {4, 10}}, 1);
        b.ellipse(14, 24, 10, 6, 1);
        b.rect(12, 26, 4, 6, 2);
        b.ellipse(14, 32, 6, 2, 2);
        b.rect(13, 0, 2, 4, 3);
        b.ellipse(14, 14, 3, 4, 3);
        art.bell = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace kilnchime
