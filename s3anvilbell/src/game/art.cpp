#include "game/art.h"

namespace anvilbell {
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
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 7), gs::rgb4(10, 10, 11), gs::rgb4(14, 14, 15),
                           gs::rgb4(5, 3, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(4, 2, 1), gs::rgb4(8, 4, 2), gs::rgb4(12, 7, 3), gs::rgb4(6, 5, 2)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(10, 7, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 8), gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(8, 10, 12), gs::rgb4(3, 6, 4), gs::rgb4(12, 4, 2)});

    {
        gs::Bitmap b(96, 36);
        b.poly({{6, 16}, {22, 8}, {78, 8}, {90, 18}, {80, 22}, {16, 22}}, 2);
        b.rect(14, 20, 68, 8, 1);
        b.rect(22, 26, 52, 6, 5);
        b.poly({{6, 16}, {2, 12}, {14, 8}, {22, 8}, {16, 16}}, 4);
        b.rect(74, 12, 10, 6, 3);
        b.line(24, 14, 72, 14, 3, 1.2f);
        art.anvil = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 52);
        b.rect(12, 2, 5, 32, 2);
        b.rect(13, 2, 2, 28, 3);
        b.poly({{2, 30}, {26, 30}, {24, 42}, {4, 42}}, 1);
        b.rect(4, 34, 20, 5, 4);
        art.hammer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 140);
        b.rect(10, 0, 8, 140, 1);
        b.rect(12, 0, 3, 140, 2);
        b.rect(4, 0, 20, 6, 3);
        b.rect(2, 132, 24, 8, 3);
        for (int y = 16; y < 128; y += 14) b.rect(11, y, 6, 2, 4);
        art.tower = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 24);
        b.poly({{14, 1}, {4, 8}, {24, 8}}, 3);
        b.poly({{5, 8}, {3, 18}, {25, 18}, {23, 8}}, 2);
        b.rect(8, 10, 12, 4, 3);
        b.ellipse(14, 20, 3, 3, 1);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(12, 10);
        b.rect(1, 1, 10, 8, 1);
        b.rect(3, 2, 6, 3, 3);
        art.slug = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 10);
        b.ellipse(5, 5, 4, 4, 1);
        b.ellipse(5, 5, 2, 2, 2);
        art.lamp = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace anvilbell
