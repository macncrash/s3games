#include "game/art.h"

namespace drumtape {
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 13)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(5, 5, 7)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(13, 3, 3)});
    setPal(vdp, PAL_SHELL, {0, gs::rgb4(3, 2, 1), gs::rgb4(8, 4, 2), gs::rgb4(13, 8, 3), gs::rgb4(15, 12, 6)});
    setPal(vdp, PAL_HEAD, {0, gs::rgb4(8, 6, 3), gs::rgb4(14, 12, 8), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_MALLET, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 8), gs::rgb4(11, 11, 13)});
    setPal(vdp, PAL_CRACK, {0, gs::rgb4(4, 1, 1), gs::rgb4(9, 3, 2), gs::rgb4(13, 6, 4)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(5, 3, 1), gs::rgb4(10, 6, 2), gs::rgb4(14, 9, 4)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(6, 5, 3), gs::rgb4(12, 10, 6), gs::rgb4(15, 13, 9)});
    setPal(vdp, PAL_SKIN, {0, gs::rgb4(9, 7, 4), gs::rgb4(14, 11, 7), gs::rgb4(15, 14, 10)});

    {
        gs::Bitmap b(96, 48);
        b.ellipse(48, 16, 40, 12, 2);
        b.rect(8, 16, 80, 22, 1);
        b.ellipse(48, 38, 40, 10, 3);
        b.ellipse(48, 16, 28, 7, 4);
        b.line(18, 18, 18, 36, 3, 1.4f);
        b.line(78, 18, 78, 36, 3, 1.4f);
        b.rect(44, 20, 8, 14, 3);
        art.drum = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(64, 22);
        b.ellipse(32, 11, 28, 9, 2);
        b.ellipse(32, 10, 16, 5, 3);
        b.ellipse(26, 8, 4, 2, 1);
        art.skin = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(14, 52);
        b.rect(5, 2, 4, 36, 2);
        b.rect(6, 2, 2, 32, 3);
        b.ellipse(7, 42, 6, 6, 1);
        b.ellipse(7, 42, 3, 3, 3);
        art.stick = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(34, 22);
        b.ellipse(17, 6, 14, 5, 3);
        b.rect(3, 6, 28, 12, 2);
        b.ellipse(17, 18, 14, 4, 1);
        b.rect(8, 9, 4, 6, 4);
        art.shell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(30, 16);
        b.ellipse(15, 8, 13, 6, 2);
        b.ellipse(15, 8, 7, 3, 3);
        b.line(6, 8, 24, 8, 1, 1.f);
        art.head = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(12, 28);
        b.rect(4, 2, 4, 16, 2);
        b.ellipse(6, 22, 5, 5, 1);
        b.ellipse(6, 22, 2, 2, 3);
        art.mallet = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(26, 18);
        b.poly({{4, 14}, {12, 2}, {22, 6}, {18, 16}, {6, 16}}, 2);
        b.line(8, 10, 16, 6, 3, 1.2f);
        b.line(10, 13, 18, 9, 1, 1.f);
        art.crack = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(52, 24);
        b.rect(1, 2, 50, 20, 2);
        b.rect(4, 5, 44, 12, 1);
        b.rect(18, 15, 16, 3, 3);
        art.drawer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 10);
        b.ellipse(5, 5, 4, 4, 1);
        b.ellipse(5, 5, 2, 2, 2);
        art.lamp = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace drumtape
