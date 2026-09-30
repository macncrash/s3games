#include "game/art.h"

namespace maskchime {
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
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(13, 9, 6), gs::rgb4(15, 13, 9), gs::rgb4(8, 5, 3), gs::rgb4(4, 2, 2),
                           gs::rgb4(14, 11, 7), gs::rgb4(9, 2, 3)});
    setPal(vdp, PAL_HOLE, {0, gs::rgb4(2, 1, 3), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(9, 7, 2), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 11), gs::rgb4(4, 3, 1)});
    setPal(vdp, PAL_LIT, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_TICK, {0, gs::rgb4(15, 13, 4), gs::rgb4(10, 3, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(6, 4, 2), gs::rgb4(11, 7, 4), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(12, 12, 13), gs::rgb4(3, 3, 5), gs::rgb4(15, 14, 8), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 15, 14)});

    {
        gs::Bitmap b(92, 112);
        b.ellipse(46, 58, 38, 48, 1);
        b.ellipse(46, 54, 30, 38, 2);
        b.rect(18, 22, 56, 8, 5);
        b.ellipse(30, 50, 11, 8, 3);
        b.ellipse(62, 50, 11, 8, 3);
        b.ellipse(46, 82, 14, 7, 3);
        b.ellipse(46, 36, 6, 4, 6);
        b.line(46, 58, 46, 74, 4, 1.2f);
        art.mask = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(18, 12);
        b.ellipse(9, 6, 8, 5, 1);
        b.ellipse(9, 6, 3, 2, 2);
        art.hole = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 34);
        b.poly({{14, 2}, {24, 12}, {22, 26}, {6, 26}, {4, 12}}, 1);
        b.poly({{14, 7}, {20, 13}, {18, 22}, {10, 22}, {8, 13}}, 2);
        b.rect(11, 26, 6, 5, 3);
        b.ellipse(14, 8, 2, 2, 4);
        b.rect(13, 0, 2, 4, 3);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 14);
        b.poly({{5, 0}, {10, 7}, {5, 14}, {0, 7}}, 1);
        b.poly({{5, 4}, {7, 7}, {5, 10}, {3, 7}}, 2);
        art.tick = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(220, 10);
        b.rect(0, 3, 220, 5, 1);
        b.rect(0, 4, 220, 2, 2);
        b.rect(96, 0, 28, 10, 3);
        art.rail = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 36);
        b.ellipse(18, 18, 16, 16, 1);
        b.ellipse(18, 18, 13, 13, 2);
        b.ellipse(18, 18, 2, 2, 3);
        b.rect(17, 3, 2, 3, 3);
        b.rect(17, 30, 2, 3, 3);
        b.rect(3, 17, 3, 2, 3);
        b.rect(30, 17, 3, 2, 3);
        art.face = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(3, 3);
        b.rect(0, 0, 3, 3, 1);
        art.pip = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace maskchime
