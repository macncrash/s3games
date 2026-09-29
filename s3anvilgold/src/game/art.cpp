#include "game/art.h"

namespace anvilgold {
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

void bar(gs::Bitmap& b, int body, int edge, int shine) {
    b.rect(1, 3, 46, 12, edge);
    b.rect(3, 1, 42, 14, body);
    b.rect(6, 3, 36, 3, shine);
    b.rect(8, 10, 18, 2, edge);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6), gs::rgb4(9, 9, 10), gs::rgb4(13, 13, 14),
                           gs::rgb4(4, 3, 3), gs::rgb4(7, 6, 5)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(12, 4, 1), gs::rgb4(15, 10, 2), gs::rgb4(15, 14, 6), gs::rgb4(8, 2, 1)});
    setPal(vdp, PAL_SMITH, {0, gs::rgb4(3, 2, 2), gs::rgb4(6, 4, 3), gs::rgb4(10, 7, 5), gs::rgb4(14, 10, 7),
                            gs::rgb4(2, 2, 4)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(5, 3, 1), gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3)});

    {
        gs::Bitmap b(120, 48);
        b.poly({{8, 22}, {28, 14}, {96, 14}, {112, 26}, {100, 30}, {22, 30}}, 2);
        b.rect(18, 28, 86, 10, 1);
        b.rect(28, 36, 64, 8, 5);
        b.rect(36, 16, 48, 6, 3);
        b.poly({{8, 22}, {4, 18}, {18, 14}, {28, 14}, {22, 22}}, 4);
        b.rect(96, 18, 12, 8, 6);
        b.line(30, 20, 90, 20, 3, 1.2f);
        art.anvil = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 64);
        b.rect(15, 2, 6, 40, 2);
        b.rect(16, 2, 3, 36, 3);
        b.poly({{4, 36}, {32, 36}, {30, 48}, {6, 48}}, 1);
        b.rect(6, 40, 24, 6, 4);
        b.rect(10, 42, 10, 2, 3);
        art.hammer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(48, 18);
        bar(b, 1, 2, 3);
        art.barGold = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(48, 18);
        bar(b, 1, 2, 3);
        art.barCream = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 16);
        b.line(8, 1, 8, 15, 2, 1.4f);
        b.line(1, 8, 15, 8, 2, 1.4f);
        b.line(3, 3, 13, 13, 1, 1.2f);
        b.line(13, 3, 3, 13, 3, 1.2f);
        art.spark = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(40, 72);
        b.ellipse(20, 10, 8, 9, 4);
        b.rect(14, 18, 12, 22, 2);
        b.rect(16, 20, 8, 16, 3);
        b.poly({{14, 22}, {2, 40}, {6, 44}, {16, 30}}, 2);
        b.poly({{26, 20}, {36, 8}, {38, 12}, {28, 26}}, 3);
        b.rect(12, 38, 7, 22, 1);
        b.rect(21, 38, 7, 22, 1);
        b.rect(10, 58, 10, 5, 5);
        b.rect(20, 58, 10, 5, 5);
        art.smith = gs::uploadImage(vdp, b);
    }
    {
        setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 8)});
        setPal(vdp, PAL_CREAM, {0, gs::rgb4(14, 12, 8), gs::rgb4(6, 5, 3), gs::rgb4(15, 15, 12)});
    }
    loadFont(vdp, art.font);
}

}  // namespace anvilgold
