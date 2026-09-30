#include "game/art.h"

namespace horntape {
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
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(4, 5, 6)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(13, 3, 4)});
    setPal(vdp, PAL_LIP, {0, gs::rgb4(6, 2, 2), gs::rgb4(12, 5, 4), gs::rgb4(15, 9, 7)});
    setPal(vdp, PAL_VALVE, {0, gs::rgb4(2, 3, 5), gs::rgb4(6, 8, 11), gs::rgb4(12, 14, 15)});
    setPal(vdp, PAL_FLARE, {0, gs::rgb4(6, 4, 1), gs::rgb4(12, 8, 2), gs::rgb4(15, 13, 5)});
    setPal(vdp, PAL_SQUEAL, {0, gs::rgb4(5, 1, 4), gs::rgb4(11, 3, 8), gs::rgb4(15, 8, 12)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(5, 3, 1), gs::rgb4(11, 7, 2), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 9)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(5, 4, 2), gs::rgb4(11, 9, 5), gs::rgb4(15, 13, 8)});
    setPal(vdp, PAL_AIR, {0, gs::rgb4(6, 10, 12), gs::rgb4(12, 15, 15)});

    {
        gs::Bitmap b(120, 72);
        b.ellipse(46, 38, 34, 26, 2);
        b.ellipse(46, 38, 22, 14, 1);
        b.ellipse(58, 30, 10, 8, 3);
        b.line(78, 28, 108, 12, 2, 3.2f);
        b.line(80, 36, 112, 22, 3, 2.2f);
        b.ellipse(112, 16, 8, 10, 3);
        b.ellipse(114, 16, 4, 6, 4);
        b.rect(18, 48, 8, 16, 2);
        b.rect(20, 50, 4, 12, 3);
        art.horn = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 32);
        b.poly({{4, 4}, {24, 2}, {26, 28}, {2, 30}}, 2);
        b.poly({{8, 8}, {20, 7}, {21, 24}, {7, 25}}, 3);
        b.ellipse(14, 16, 4, 6, 4);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 18);
        b.ellipse(11, 9, 9, 7, 2);
        b.ellipse(11, 9, 4, 3, 3);
        b.line(4, 12, 18, 6, 1, 1.2f);
        art.lip = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(18, 26);
        b.rect(6, 2, 6, 22, 2);
        b.ellipse(9, 8, 7, 4, 3);
        b.ellipse(9, 16, 7, 4, 1);
        b.rect(8, 6, 2, 12, 3);
        art.valve = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(30, 20);
        b.poly({{2, 16}, {10, 4}, {28, 6}, {22, 18}}, 2);
        b.line(8, 12, 22, 8, 3, 1.4f);
        b.ellipse(24, 8, 3, 3, 1);
        art.flare = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 22);
        b.line(2, 18, 8, 4, 2, 2.f);
        b.line(8, 4, 12, 16, 3, 2.f);
        b.line(12, 16, 18, 2, 2, 2.f);
        b.line(18, 2, 26, 18, 1, 2.f);
        art.squeal = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(48, 22);
        b.rect(1, 1, 46, 20, 2);
        b.rect(4, 4, 40, 12, 1);
        b.rect(18, 14, 12, 3, 3);
        art.drawer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 28);
        b.rect(2, 2, 4, 24, 1);
        b.rect(3, 8, 2, 14, 2);
        art.breath = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace horntape
