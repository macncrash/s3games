#include "game/art.h"

namespace maskbell {
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
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(12, 8, 5), gs::rgb4(15, 12, 8), gs::rgb4(8, 5, 3), gs::rgb4(4, 2, 2),
                           gs::rgb4(14, 10, 7)});
    setPal(vdp, PAL_HOLE, {0, gs::rgb4(2, 1, 2), gs::rgb4(15, 13, 5)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(8, 6, 2), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 10), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_LIT, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_TICK, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 3, 2)});
    setPal(vdp, PAL_BENCH, {0, gs::rgb4(5, 3, 2), gs::rgb4(8, 5, 3)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 4), gs::rgb4(3, 2, 1)});

    {
        gs::Bitmap b(88, 108);
        b.ellipse(44, 56, 36, 46, 1);
        b.ellipse(44, 52, 28, 36, 2);
        b.ellipse(30, 48, 10, 7, 3);
        b.ellipse(58, 48, 10, 7, 3);
        b.ellipse(44, 78, 12, 6, 3);
        b.rect(28, 28, 32, 4, 5);
        b.ellipse(30, 48, 4, 3, 4);
        b.ellipse(58, 48, 4, 3, 4);
        b.ellipse(44, 78, 5, 2, 4);
        art.mask = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 10);
        b.ellipse(8, 5, 7, 4, 1);
        b.ellipse(8, 5, 3, 2, 2);
        art.hole = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 32);
        b.poly({{18, 2}, {30, 14}, {28, 26}, {8, 26}, {6, 14}}, 1);
        b.poly({{18, 6}, {26, 14}, {24, 22}, {12, 22}, {10, 14}}, 2);
        b.rect(15, 26, 6, 4, 3);
        b.ellipse(18, 8, 3, 2, 4);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 12);
        b.poly({{4, 0}, {8, 6}, {4, 12}, {0, 6}}, 1);
        art.tick = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(200, 8);
        b.rect(0, 2, 200, 4, 1);
        b.rect(0, 3, 200, 2, 2);
        art.rail = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace maskbell
