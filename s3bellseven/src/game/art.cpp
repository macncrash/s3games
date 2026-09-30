#include "game/art.h"

namespace bellseven {
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
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(7, 4, 1), gs::rgb4(13, 9, 2), gs::rgb4(15, 14, 6), gs::rgb4(5, 3, 1)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(13, 3, 3)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(6, 5, 4), gs::rgb4(12, 10, 7), gs::rgb4(15, 14, 11), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_BRONZE, {0, gs::rgb4(3, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 3)});
    setPal(vdp, PAL_ROPE, {0, gs::rgb4(4, 3, 1), gs::rgb4(8, 5, 2), gs::rgb4(11, 8, 3)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(1, 2, 5), gs::rgb4(3, 5, 9), gs::rgb4(8, 9, 12), gs::rgb4(13, 11, 8),
                          gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_THEM, {0, gs::rgb4(4, 1, 2), gs::rgb4(8, 3, 3), gs::rgb4(12, 8, 6), gs::rgb4(14, 12, 9),
                           gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 6), gs::rgb4(7, 7, 9), gs::rgb4(11, 11, 12)});
    setPal(vdp, PAL_PEG, {0, gs::rgb4(3, 3, 4), gs::rgb4(14, 11, 3), gs::rgb4(12, 8, 6)});

    {
        gs::Bitmap b(96, 148);
        b.rect(36, 0, 24, 120, 2);
        b.rect(42, 4, 8, 108, 3);
        b.rect(8, 18, 80, 12, 1);
        b.rect(14, 22, 68, 4, 3);
        b.rect(20, 40, 14, 22, 0);
        b.rect(62, 40, 14, 22, 0);
        b.rect(22, 42, 10, 16, 4);
        b.rect(64, 42, 10, 16, 4);
        b.poly({{4, 128}, {92, 128}, {96, 147}, {0, 147}}, 2);
        b.rect(18, 132, 60, 8, 1);
        art.belfry = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(48, 56);
        b.rect(18, 0, 12, 6, 4);
        b.rect(16, 5, 16, 5, 1);
        b.poly({{12, 10}, {36, 10}, {44, 40}, {4, 40}}, 2);
        b.poly({{18, 12}, {30, 12}, {34, 34}, {14, 34}}, 3);
        b.ellipse(24, 44, 20, 8, 2);
        b.ellipse(24, 42, 12, 4, 3);
        b.rect(8, 48, 32, 4, 1);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 24);
        b.rect(4, 0, 2, 14, 1);
        b.ellipse(5, 18, 4, 4, 2);
        art.clapper = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 72);
        b.rect(3, 0, 2, 60, 2);
        b.rect(4, 0, 1, 56, 3);
        b.ellipse(4, 66, 3, 4, 1);
        art.rope = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 56);
        b.ellipse(14, 8, 7, 8, 3);
        b.rect(9, 15, 10, 14, 2);
        b.poly({{9, 20}, {1, 34}, {4, 36}, {11, 24}}, 1);
        b.poly({{18, 18}, {26, 30}, {23, 33}, {15, 22}}, 4);
        b.rect(8, 28, 5, 18, 1);
        b.rect(15, 28, 5, 18, 1);
        b.rect(7, 45, 7, 4, 5);
        b.rect(14, 45, 7, 4, 5);
        art.you = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 56);
        b.ellipse(14, 8, 7, 8, 3);
        b.rect(9, 15, 10, 14, 2);
        b.poly({{9, 20}, {2, 32}, {5, 35}, {11, 24}}, 1);
        b.poly({{17, 18}, {25, 34}, {22, 36}, {14, 22}}, 4);
        b.rect(8, 28, 5, 18, 1);
        b.rect(15, 28, 5, 18, 1);
        b.rect(7, 45, 7, 4, 5);
        b.rect(14, 45, 7, 4, 5);
        art.them = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 12);
        b.ellipse(11, 6, 10, 4, 2);
        b.ellipse(11, 6, 5, 2, 3);
        art.wave = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 16);
        b.rect(3, 0, 4, 10, 1);
        b.ellipse(5, 12, 4, 3, 1);
        art.peg = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace bellseven
