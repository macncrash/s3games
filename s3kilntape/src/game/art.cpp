#include "game/art.h"

namespace kilntape {
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 12)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 5, 5)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 2)});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(6, 2, 1), gs::rgb4(10, 4, 2), gs::rgb4(14, 7, 3), gs::rgb4(3, 1, 1),
                            gs::rgb4(15, 11, 4)});
    setPal(vdp, PAL_GLAZE, {0, gs::rgb4(2, 6, 10), gs::rgb4(4, 10, 14), gs::rgb4(12, 15, 15)});
    setPal(vdp, PAL_BOWL, {0, gs::rgb4(8, 4, 2), gs::rgb4(13, 7, 3), gs::rgb4(15, 12, 6)});
    setPal(vdp, PAL_CONE, {0, gs::rgb4(10, 8, 2), gs::rgb4(14, 12, 3), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_BISQUE, {0, gs::rgb4(8, 7, 6), gs::rgb4(12, 11, 9), gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_CLAY, {0, gs::rgb4(7, 4, 2), gs::rgb4(12, 8, 4), gs::rgb4(4, 2, 1)});

    loadFont(vdp, art.font);

    {
        gs::Bitmap b(96, 88);
        b.rect(8, 16, 80, 64, 1);
        b.rect(14, 22, 68, 50, 4);
        b.poly({{20, 48}, {76, 48}, {68, 68}, {28, 68}}, 5);
        b.ellipse(48, 46, 18, 14, 5);
        b.ellipse(48, 46, 8, 6, 3);
        b.rect(38, 4, 20, 16, 2);
        b.rect(44, 0, 8, 8, 3);
        for (int row = 0; row < 4; row++)
            for (int col = 0; col < 5; col++) b.rect(12 + col * 15, 18 + row * 14, 13, 4, (row + col) & 1 ? 2 : 3);
        b.rect(4, 76, 88, 8, 1);
        art.kiln = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 48);
        b.rect(2, 4, 24, 40, 2);
        b.rect(6, 8, 16, 28, 4);
        b.rect(10, 36, 8, 6, 1);
        art.door = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 26);
        b.ellipse(11, 16, 8, 7, 1);
        b.poly({{4, 14}, {18, 14}, {16, 6}, {6, 6}}, 2);
        b.ellipse(11, 6, 6, 2, 3);
        b.rect(10, 8, 2, 6, 2);
        art.glaze = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(24, 18);
        b.ellipse(12, 11, 10, 6, 1);
        b.ellipse(12, 8, 8, 3, 2);
        b.ellipse(12, 7, 4, 2, 3);
        art.bowl = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 28);
        b.poly({{8, 2}, {13, 24}, {3, 24}}, 2);
        b.poly({{8, 6}, {11, 22}, {5, 22}}, 1);
        b.rect(6, 24, 4, 3, 3);
        art.cone = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 22);
        b.ellipse(11, 12, 8, 7, 1);
        b.ellipse(11, 8, 6, 2, 2);
        b.line(5, 6, 16, 18, 3, 1.2f);
        b.line(16, 7, 6, 18, 3, 1.2f);
        art.bisque = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(40, 16);
        b.rect(1, 2, 38, 12, 2);
        b.rect(3, 4, 34, 8, 1);
        b.rect(16, 0, 8, 3, 3);
        art.drawer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(18, 22);
        b.poly({{9, 20}, {2, 10}, {6, 8}, {9, 2}, {12, 8}, {16, 10}}, 1);
        b.poly({{9, 18}, {6, 11}, {9, 6}, {12, 11}}, 2);
        b.ellipse(9, 16, 2, 2, 3);
        art.flame = gs::uploadImage(vdp, b);
    }
}

}  // namespace kilntape
