#include "game/art.h"

namespace flutetape {
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
    setPal(vdp, PAL_LIP, {0, gs::rgb4(8, 3, 2), gs::rgb4(13, 6, 4), gs::rgb4(15, 11, 7)});
    setPal(vdp, PAL_BODY, {0, gs::rgb4(2, 5, 3), gs::rgb4(5, 10, 6), gs::rgb4(10, 14, 9)});
    setPal(vdp, PAL_FOOT, {0, gs::rgb4(2, 3, 7), gs::rgb4(5, 7, 12), gs::rgb4(10, 12, 15)});
    setPal(vdp, PAL_WHISTLE, {0, gs::rgb4(6, 2, 6), gs::rgb4(11, 4, 10), gs::rgb4(15, 9, 13)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(4, 2, 1), gs::rgb4(9, 5, 2), gs::rgb4(13, 8, 4)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(7, 6, 3), gs::rgb4(12, 11, 7), gs::rgb4(15, 14, 10)});
    setPal(vdp, PAL_SILVER, {0, gs::rgb4(4, 5, 6), gs::rgb4(9, 10, 11), gs::rgb4(14, 15, 15), gs::rgb4(6, 8, 9)});

    {
        gs::Bitmap b(140, 22);
        b.rect(8, 6, 124, 10, 2);
        b.rect(8, 6, 124, 4, 3);
        b.ellipse(8, 11, 8, 8, 2);
        b.ellipse(8, 11, 4, 4, 1);
        b.ellipse(132, 11, 7, 6, 4);
        for (int i = 0; i < 6; i++) b.ellipse(36.f + i * 14.f, 11, 3.2f, 3.2f, 1);
        art.flute = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 10);
        b.ellipse(5, 5, 4, 4, 1);
        b.ellipse(5, 5, 2, 2, 2);
        art.hole = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 16);
        b.ellipse(11, 8, 9, 6, 2);
        b.ellipse(8, 7, 3, 2, 3);
        b.rect(16, 6, 5, 4, 1);
        art.lip = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 14);
        b.rect(2, 3, 32, 8, 2);
        b.rect(2, 3, 32, 3, 3);
        b.ellipse(8, 7, 2, 2, 1);
        b.ellipse(18, 7, 2, 2, 1);
        b.ellipse(28, 7, 2, 2, 1);
        art.body = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(18, 18);
        b.ellipse(9, 9, 8, 8, 2);
        b.ellipse(9, 9, 4, 4, 3);
        b.rect(14, 7, 4, 4, 1);
        art.foot = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 20);
        b.poly({{8, 1}, {15, 18}, {1, 18}}, 2);
        b.poly({{8, 6}, {12, 16}, {4, 16}}, 3);
        art.whistle = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(56, 26);
        b.rect(1, 1, 54, 22, 2);
        b.rect(4, 4, 48, 14, 1);
        b.rect(20, 18, 16, 6, 3);
        art.drawer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(18, 12);
        b.ellipse(8, 6, 7, 4, 2);
        b.ellipse(5, 5, 3, 2, 3);
        art.breath = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 48);
        b.ellipse(14, 8, 7, 7, 2);
        b.ellipse(12, 7, 2, 2, 3);
        b.rect(10, 15, 8, 16, 1);
        b.rect(4, 16, 6, 3, 2);
        b.rect(18, 16, 7, 3, 2);
        b.rect(11, 30, 3, 14, 2);
        b.rect(16, 30, 3, 14, 2);
        art.player = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace flutetape
