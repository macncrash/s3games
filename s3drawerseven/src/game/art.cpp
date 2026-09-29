#include "game/art.h"

namespace drawerseven {
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
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(13, 3, 2)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(12, 11, 8)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(5, 3, 1), gs::rgb4(9, 5, 2), gs::rgb4(13, 8, 3), gs::rgb4(3, 2, 1),
                           gs::rgb4(7, 4, 2), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_COIN, {0, gs::rgb4(8, 5, 2), gs::rgb4(13, 8, 3), gs::rgb4(15, 12, 6), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_NICK, {0, gs::rgb4(7, 7, 8), gs::rgb4(12, 12, 13), gs::rgb4(15, 15, 15), gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_JUNK, {0, gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 7), gs::rgb4(9, 8, 7), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(6, 5, 2), gs::rgb4(15, 12, 3), gs::rgb4(15, 14, 8), gs::rgb4(3, 2, 1)});

    {
        gs::Bitmap b(260, 160);
        b.rect(0, 0, 260, 160, 1);
        b.rect(8, 10, 244, 140, 2);
        b.rect(16, 18, 228, 22, 3);
        for (int y = 48; y < 150; y += 8)
            for (int x = 12; x < 248; x += 10) b.set(x, y, (x + y) & 8 ? 5 : 2);
        b.rect(18, 70, 224, 78, 4);
        b.rect(26, 78, 208, 62, 1);
        art.desk = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(210, 58);
        b.rect(0, 6, 210, 46, 2);
        b.rect(6, 12, 198, 34, 1);
        b.rect(8, 14, 194, 6, 3);
        b.rect(0, 0, 210, 8, 5);
        b.rect(0, 50, 210, 8, 4);
        for (int i = 0; i < 5; i++) b.rect(12 + i * 38, 22, 28, 18, 4);
        art.drawer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 22);
        b.ellipse(11, 11, 10, 10, 1);
        b.ellipse(11, 11, 7.5f, 7.5f, 2);
        b.ellipse(11, 11, 4.2f, 4.2f, 3);
        b.ellipse(8, 8, 1.4f, 1.0f, 3);
        art.penny = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 28);
        b.ellipse(14, 14, 13, 13, 1);
        b.ellipse(14, 14, 10, 10, 2);
        b.ellipse(14, 14, 5.5f, 5.5f, 3);
        b.rect(13, 6, 2, 16, 1);
        b.rect(8, 13, 12, 2, 1);
        art.nickel = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 16);
        b.ellipse(8, 8, 7, 7, 1);
        b.ellipse(8, 8, 3.2f, 3.2f, 2);
        b.ellipse(8, 8, 1.2f, 1.2f, 4);
        art.button = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 8);
        b.poly({{1, 1}, {9, 1}, {5, 7}}, 1);
        art.caret = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 48);
        b.rect(16, 20, 4, 26, 1);
        b.ellipse(18, 14, 12, 10, 4);
        b.ellipse(18, 14, 8, 6, 2);
        b.ellipse(15, 12, 2.2f, 1.4f, 3);
        art.lamp = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace drawerseven
