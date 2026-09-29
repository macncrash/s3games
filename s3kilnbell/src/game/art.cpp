#include "game/art.h"

namespace kilnbell {
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
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(6, 2, 1), gs::rgb4(11, 4, 2), gs::rgb4(14, 7, 3), gs::rgb4(3, 1, 1),
                            gs::rgb4(15, 10, 4), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_CLAY, {0, gs::rgb4(11, 6, 3), gs::rgb4(14, 9, 5), gs::rgb4(7, 4, 2), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(12, 3, 1), gs::rgb4(15, 9, 2), gs::rgb4(15, 14, 5), gs::rgb4(7, 2, 0)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(9, 6, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 8), gs::rgb4(5, 3, 1)});
    setPal(vdp, PAL_ASH, {0, gs::rgb4(5, 5, 4), gs::rgb4(8, 8, 6), gs::rgb4(2, 2, 2), gs::rgb4(4, 12, 5)});

    {
        gs::Bitmap b(100, 108);
        b.rect(12, 28, 76, 68, 1);
        b.rect(18, 34, 64, 52, 4);
        b.ellipse(50, 58, 18, 16, 5);
        b.ellipse(50, 58, 9, 8, 3);
        b.rect(40, 8, 20, 24, 2);
        b.rect(44, 2, 12, 10, 6);
        for (int row = 0; row < 5; row++)
            for (int col = 0; col < 5; col++) b.rect(16 + col * 14, 30 + row * 13, 12, 4, (row + col) & 1 ? 2 : 3);
        b.rect(6, 92, 88, 10, 1);
        b.rect(22, 96, 56, 4, 4);
        b.rect(28, 70, 44, 10, 2);
        art.kiln = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 26);
        b.ellipse(11, 17, 8, 6, 1);
        b.poly({{4, 15}, {18, 15}, {16, 7}, {6, 7}}, 2);
        b.ellipse(11, 7, 6, 2, 2);
        b.rect(10, 9, 2, 7, 2);
        art.pot = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 26);
        b.ellipse(11, 17, 8, 6, 1);
        b.poly({{4, 15}, {18, 15}, {16, 7}, {6, 7}}, 2);
        b.ellipse(11, 7, 6, 2, 2);
        b.line(6, 8, 16, 20, 3, 1.2f);
        b.line(15, 8, 7, 19, 3, 1.2f);
        art.crack = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(18, 24);
        b.poly({{9, 1}, {14, 10}, {11, 10}, {16, 20}, {9, 14}, {2, 22}, {7, 10}, {4, 10}}, 2);
        b.poly({{9, 7}, {11, 13}, {9, 17}, {7, 13}}, 3);
        art.flame = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 18);
        b.poly({{4, 1}, {7, 16}, {1, 16}}, 1);
        b.rect(2, 15, 4, 2, 2);
        art.cone = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(132, 12);
        b.rect(0, 3, 132, 6, 1);
        int x0 = 132 * kSweetLo / kDieAt;
        int x1 = 132 * kSweetHi / kDieAt;
        b.rect(float(x0), 1, float(x1 - x0), 10, 4);
        art.bar = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(30, 26);
        b.poly({{15, 1}, {5, 8}, {25, 8}}, 3);
        b.poly({{6, 8}, {3, 18}, {27, 18}, {24, 8}}, 2);
        b.rect(9, 10, 12, 4, 3);
        b.ellipse(15, 21, 3, 3, 1);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 10);
        b.ellipse(5, 5, 4, 4, 1);
        b.ellipse(5, 5, 2, 2, 2);
        art.lamp = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace kilnbell
