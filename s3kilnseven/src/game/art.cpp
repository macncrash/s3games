#include "game/art.h"

namespace kilnseven {
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

void bowl(gs::Bitmap& b, int body, int lip, int split) {
    b.ellipse(10, 16, 8, 6, body);
    b.poly({{3, 14}, {17, 14}, {15, 6}, {5, 6}}, lip);
    b.ellipse(10, 6, 6, 2, lip);
    b.rect(9, 8, 2, 7, lip);
    if (split) {
        b.line(6, 8, 14, 18, split, 1.2f);
        b.line(13, 7, 7, 17, split, 1.2f);
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 12)});
    setPal(vdp, PAL_CLAY, {0, gs::rgb4(11, 6, 3), gs::rgb4(14, 9, 5), gs::rgb4(7, 4, 2)});
    setPal(vdp, PAL_RIVAL, {0, gs::rgb4(5, 5, 7), gs::rgb4(8, 8, 10), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(7, 2, 1), gs::rgb4(11, 4, 2), gs::rgb4(14, 7, 3), gs::rgb4(3, 1, 1),
                            gs::rgb4(15, 10, 3), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(13, 3, 1), gs::rgb4(15, 10, 2), gs::rgb4(15, 14, 6), gs::rgb4(8, 2, 0)});
    setPal(vdp, PAL_ASH, {0, gs::rgb4(5, 5, 4), gs::rgb4(8, 8, 6), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(4, 12, 5), gs::rgb4(10, 15, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(13, 3, 3), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(14, 12, 8), gs::rgb4(2, 1, 1)});

    {
        gs::Bitmap b(88, 96);
        b.rect(10, 22, 68, 64, 1);
        b.rect(16, 28, 56, 50, 4);
        b.poly({{22, 52}, {66, 52}, {58, 74}, {30, 74}}, 5);
        b.ellipse(44, 50, 16, 14, 5);
        b.ellipse(44, 50, 8, 7, 3);
        b.rect(36, 8, 16, 18, 2);
        b.rect(40, 2, 8, 10, 6);
        for (int row = 0; row < 5; row++)
            for (int col = 0; col < 5; col++) b.rect(14 + col * 13, 24 + row * 12, 11, 3, (row + col) & 1 ? 2 : 3);
        b.rect(6, 82, 76, 8, 1);
        b.rect(18, 86, 52, 4, 4);
        art.kiln = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(48, 56);
        b.rect(6, 14, 36, 36, 1);
        b.rect(10, 18, 28, 26, 4);
        b.ellipse(24, 28, 8, 7, 5);
        b.rect(20, 4, 8, 12, 2);
        b.rect(4, 48, 40, 5, 1);
        art.kilnFar = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(20, 24);
        bowl(b, 1, 2, 0);
        art.pot = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(20, 24);
        bowl(b, 1, 2, 3);
        art.crack = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 22);
        b.poly({{8, 1}, {13, 9}, {10, 9}, {14, 18}, {8, 13}, {2, 20}, {6, 9}, {3, 9}}, 2);
        b.poly({{8, 6}, {10, 12}, {8, 16}, {6, 12}}, 3);
        art.flame = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 18);
        b.poly({{4, 1}, {7, 16}, {1, 16}}, 1);
        b.rect(2, 15, 4, 2, 2);
        art.cone = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(120, 10);
        b.rect(0, 2, 120, 6, 1);
        b.rect(42, 1, 28, 8, 2);
        b.rect(0, 0, 120, 1, 1);
        art.bar = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace kilnseven
