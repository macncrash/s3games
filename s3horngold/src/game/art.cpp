#include "game/art.h"

namespace horngold {
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 10)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(8, 5, 1), gs::rgb4(14, 10, 2), gs::rgb4(15, 14, 6), gs::rgb4(6, 3, 1)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(13, 3, 3)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(8, 7, 5), gs::rgb4(13, 12, 9), gs::rgb4(15, 14, 12), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(6, 4, 1), gs::rgb4(11, 8, 2), gs::rgb4(15, 12, 4), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 6), gs::rgb4(8, 7, 9), gs::rgb4(12, 9, 7),
                           gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_YARD, {0, gs::rgb4(2, 2, 2), gs::rgb4(4, 3, 3), gs::rgb4(7, 5, 4), gs::rgb4(10, 8, 6)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(8, 4, 1), gs::rgb4(14, 9, 2), gs::rgb4(15, 13, 5)});

    {
        gs::Bitmap b(40, 88);
        b.ellipse(20, 12, 9, 10, 3);
        b.rect(14, 20, 12, 6, 4);
        b.rect(12, 26, 16, 28, 2);
        b.poly({{12, 30}, {2, 52}, {8, 56}, {16, 36}}, 2);
        b.poly({{28, 28}, {38, 18}, {40, 22}, {30, 34}}, 3);
        b.rect(13, 54, 6, 26, 1);
        b.rect(21, 54, 6, 26, 1);
        b.rect(11, 78, 9, 6, 4);
        b.rect(20, 78, 9, 6, 4);
        art.player = gs::uploadImage(vdp, b);
    }
    {
        // Short straight post horn, mouthpiece at the left, flare at the right.
        gs::Bitmap b(96, 22);
        b.rect(0, 8, 8, 6, 1);
        b.rect(6, 7, 52, 8, 2);
        b.rect(8, 9, 46, 3, 3);
        b.poly({{56, 4}, {94, 0}, {94, 21}, {56, 17}}, 2);
        b.poly({{70, 6}, {90, 3}, {90, 18}, {70, 15}}, 3);
        b.ellipse(92, 11, 4, 9, 1);
        art.horn = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(18, 22);
        b.ellipse(9, 11, 8, 10, 2);
        b.ellipse(9, 11, 4, 6, 3);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 40);
        b.rect(2, 0, 4, 40, 2);
        b.rect(3, 0, 2, 40, 3);
        art.breath = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(12, 12);
        b.ellipse(6, 6, 5, 5, 2);
        b.ellipse(6, 6, 2, 2, 3);
        art.note = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(70, 48);
        b.rect(4, 16, 62, 22, 2);
        b.rect(8, 8, 28, 12, 3);
        b.rect(10, 20, 10, 8, 1);
        b.rect(24, 20, 10, 8, 1);
        b.ellipse(16, 40, 8, 8, 1);
        b.ellipse(52, 40, 8, 8, 1);
        art.coach = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(14, 28);
        b.rect(6, 10, 2, 16, 1);
        b.ellipse(7, 8, 6, 6, 2);
        b.ellipse(7, 8, 3, 3, 3);
        art.lamp = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(200, 6);
        b.rect(0, 2, 200, 2, 2);
        art.rail = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace horngold
