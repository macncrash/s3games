#include "game/art.h"

namespace anviltape {
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
    setPal(vdp, PAL_IRON, {0, gs::rgb4(2, 2, 3), gs::rgb4(7, 7, 8), gs::rgb4(11, 11, 12), gs::rgb4(14, 14, 15)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(4, 2, 1), gs::rgb4(8, 4, 2), gs::rgb4(12, 7, 3)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(8, 5, 1), gs::rgb4(14, 10, 2), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_COPPER, {0, gs::rgb4(6, 2, 1), gs::rgb4(12, 5, 2), gs::rgb4(15, 8, 3)});
    setPal(vdp, PAL_SLAG, {0, gs::rgb4(3, 3, 2), gs::rgb4(6, 5, 4), gs::rgb4(9, 8, 6)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(10, 8, 5), gs::rgb4(14, 12, 8), gs::rgb4(4, 3, 2)});

    {
        gs::Bitmap b(104, 40);
        b.poly({{8, 14}, {24, 6}, {84, 6}, {96, 16}, {86, 22}, {18, 22}}, 2);
        b.rect(16, 20, 72, 8, 1);
        b.rect(24, 26, 56, 8, 3);
        b.poly({{8, 14}, {2, 10}, {16, 6}, {24, 6}, {16, 16}}, 3);
        b.rect(78, 10, 12, 6, 3);
        b.line(28, 12, 76, 12, 3, 1.2f);
        art.anvil = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(32, 56);
        b.rect(13, 2, 6, 34, 2);
        b.rect(14, 2, 2, 30, 3);
        b.poly({{2, 32}, {30, 32}, {26, 46}, {6, 46}}, 1);
        b.rect(4, 36, 24, 6, 3);
        art.hammer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 14);
        b.rect(2, 2, 32, 10, 2);
        b.rect(4, 4, 28, 4, 3);
        b.rect(6, 8, 8, 2, 1);
        art.billet = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 18);
        b.ellipse(14, 10, 12, 7, 2);
        b.ellipse(14, 9, 7, 4, 3);
        b.rect(4, 12, 20, 3, 1);
        art.bloom = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(40, 16);
        b.rect(2, 6, 28, 4, 2);
        b.rect(22, 2, 4, 12, 3);
        b.rect(30, 4, 8, 8, 1);
        b.rect(32, 6, 4, 4, 2);
        art.tongs = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 16);
        b.poly({{4, 12}, {10, 2}, {18, 6}, {16, 14}, {6, 15}}, 2);
        b.poly({{8, 8}, {12, 5}, {14, 10}}, 3);
        art.slag = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(48, 22);
        b.rect(1, 1, 46, 20, 1);
        b.rect(4, 4, 40, 12, 2);
        b.rect(6, 14, 36, 3, 3);
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

}  // namespace anviltape
