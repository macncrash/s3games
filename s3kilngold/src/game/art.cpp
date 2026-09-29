#include "game/art.h"

namespace kilngold {
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

void vase(gs::Bitmap& b, int body, int lip, int shine) {
    b.ellipse(16, 28, 12, 14, body);
    b.rect(8, 10, 16, 8, body);
    b.poly({{6, 12}, {26, 12}, {24, 6}, {8, 6}}, lip);
    b.ellipse(16, 6, 9, 3, lip);
    b.rect(14, 16, 3, 14, shine);
    b.rect(11, 36, 10, 4, lip);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(14, 12, 8), gs::rgb4(6, 5, 3), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(6, 2, 1), gs::rgb4(10, 4, 2), gs::rgb4(13, 6, 3), gs::rgb4(3, 1, 1),
                            gs::rgb4(15, 8, 2), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(12, 3, 1), gs::rgb4(15, 9, 2), gs::rgb4(15, 14, 5), gs::rgb4(8, 2, 1)});
    setPal(vdp, PAL_CLAY, {0, gs::rgb4(8, 5, 3), gs::rgb4(11, 7, 4), gs::rgb4(5, 3, 2)});
    setPal(vdp, PAL_ASH, {0, gs::rgb4(4, 4, 4), gs::rgb4(7, 7, 6), gs::rgb4(3, 3, 3)});

    {
        gs::Bitmap b(96, 88);
        b.rect(8, 18, 80, 62, 1);
        b.rect(14, 24, 68, 50, 4);
        b.poly({{18, 48}, {78, 48}, {70, 70}, {26, 70}}, 5);
        b.ellipse(48, 46, 22, 18, 5);
        b.ellipse(48, 46, 14, 11, 3);
        b.rect(40, 8, 16, 14, 2);
        b.rect(44, 4, 8, 8, 6);
        for (int row = 0; row < 4; row++)
            for (int col = 0; col < 6; col++) b.rect(12 + col * 13, 22 + row * 14, 11, 3, 2);
        b.rect(6, 76, 84, 8, 1);
        art.kiln = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(32, 44);
        vase(b, 1, 2, 3);
        art.potGold = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(32, 44);
        vase(b, 1, 2, 3);
        art.potCream = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(20, 28);
        b.poly({{10, 1}, {16, 12}, {13, 12}, {18, 22}, {10, 16}, {2, 26}, {7, 12}, {4, 12}}, 2);
        b.poly({{10, 6}, {13, 14}, {10, 18}, {7, 14}}, 3);
        b.ellipse(10, 20, 3, 4, 1);
        art.flame = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(140, 12);
        b.rect(0, 2, 140, 8, 1);
        b.rect(0, 0, 140, 3, 2);
        for (int i = 0; i < 10; i++) b.rect(4 + i * 14, 4, 8, 4, 3);
        art.shelf = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace kilngold
