#include "game/art.h"

namespace anvilseven {
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

void ingot(gs::Bitmap& b, int body, int edge, int shine) {
    b.poly({{2, 8}, {8, 2}, {40, 2}, {46, 8}, {44, 16}, {4, 16}}, edge);
    b.poly({{6, 8}, {11, 4}, {37, 4}, {42, 8}, {40, 13}, {8, 13}}, body);
    b.rect(12, 5, 18, 2, shine);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 13)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 11, 2), gs::rgb4(9, 5, 1), gs::rgb4(15, 15, 7)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(7, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(13, 3, 2)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(13, 11, 7), gs::rgb4(7, 5, 3), gs::rgb4(15, 14, 11)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 7), gs::rgb4(10, 10, 11), gs::rgb4(14, 14, 15),
                           gs::rgb4(5, 3, 2), gs::rgb4(8, 6, 4)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(13, 3, 1), gs::rgb4(15, 9, 1), gs::rgb4(15, 14, 5), gs::rgb4(7, 2, 1)});
    setPal(vdp, PAL_SMITH, {0, gs::rgb4(4, 2, 2), gs::rgb4(7, 4, 3), gs::rgb4(11, 7, 4), gs::rgb4(14, 11, 7),
                            gs::rgb4(2, 2, 5), gs::rgb4(8, 2, 2)});
    setPal(vdp, PAL_RIVAL, {0, gs::rgb4(2, 3, 4), gs::rgb4(4, 5, 7), gs::rgb4(7, 8, 10), gs::rgb4(12, 13, 14),
                            gs::rgb4(3, 2, 2), gs::rgb4(9, 3, 3)});

    {
        gs::Bitmap b(128, 52);
        b.poly({{10, 20}, {26, 12}, {102, 12}, {118, 24}, {108, 32}, {20, 32}}, 2);
        b.rect(16, 30, 96, 8, 1);
        b.rect(34, 36, 60, 10, 5);
        b.rect(40, 14, 48, 5, 3);
        b.poly({{10, 22}, {2, 16}, {16, 10}, {28, 12}, {20, 24}}, 4);
        b.rect(104, 16, 14, 8, 6);
        b.line(36, 18, 96, 18, 3, 1.4f);
        b.rect(48, 38, 8, 8, 4);
        b.rect(72, 38, 8, 8, 4);
        art.anvil = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(40, 70);
        b.rect(17, 2, 6, 42, 2);
        b.rect(18, 3, 3, 38, 3);
        b.poly({{3, 40}, {37, 40}, {34, 54}, {6, 54}}, 1);
        b.rect(6, 44, 28, 6, 4);
        b.rect(12, 46, 12, 2, 3);
        b.rect(16, 54, 8, 6, 5);
        art.hammer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(48, 18);
        ingot(b, 1, 2, 3);
        art.barGold = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(48, 18);
        ingot(b, 1, 2, 3);
        art.barCream = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(14, 14);
        b.line(7, 1, 7, 13, 2, 1.5f);
        b.line(1, 7, 13, 7, 2, 1.5f);
        b.line(3, 3, 11, 11, 1, 1.2f);
        b.line(11, 3, 3, 11, 3, 1.2f);
        art.spark = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(44, 76);
        b.ellipse(22, 11, 9, 10, 4);
        b.ellipse(22, 12, 5, 5, 3);
        b.rect(15, 20, 14, 22, 2);
        b.rect(17, 22, 10, 14, 3);
        b.poly({{15, 24}, {3, 42}, {8, 46}, {18, 32}}, 2);
        b.poly({{29, 22}, {40, 10}, {42, 14}, {31, 28}}, 3);
        b.rect(14, 40, 7, 22, 1);
        b.rect(23, 40, 7, 22, 1);
        b.rect(12, 60, 11, 6, 5);
        b.rect(22, 60, 11, 6, 5);
        b.rect(18, 6, 8, 3, 6);
        art.smith = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace anvilseven
