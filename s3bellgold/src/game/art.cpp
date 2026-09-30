#include "game/art.h"

namespace bellgold {
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
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(8, 5, 1), gs::rgb4(14, 10, 2), gs::rgb4(15, 14, 5), gs::rgb4(6, 3, 1)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(7, 6, 4), gs::rgb4(13, 11, 8), gs::rgb4(15, 14, 11), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_BRONZE, {0, gs::rgb4(4, 3, 2), gs::rgb4(8, 6, 3), gs::rgb4(12, 9, 4)});
    setPal(vdp, PAL_ROPE, {0, gs::rgb4(5, 3, 1), gs::rgb4(9, 6, 2), gs::rgb4(12, 9, 4)});
    setPal(vdp, PAL_RINGER, {0, gs::rgb4(2, 2, 4), gs::rgb4(5, 4, 6), gs::rgb4(9, 7, 8), gs::rgb4(13, 10, 8),
                             gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 8), gs::rgb4(10, 10, 11)});

    {
        gs::Bitmap b(72, 160);
        b.rect(28, 0, 16, 148, 2);
        b.rect(32, 0, 6, 140, 3);
        b.rect(8, 8, 56, 10, 1);
        b.rect(12, 10, 48, 4, 3);
        b.poly({{6, 148}, {66, 148}, {72, 159}, {0, 159}}, 2);
        b.rect(18, 150, 36, 6, 1);
        art.tower = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(56, 64);
        b.rect(22, 0, 12, 8, 4);
        b.rect(18, 6, 20, 6, 1);
        b.poly({{16, 12}, {40, 12}, {50, 48}, {6, 48}}, 2);
        b.poly({{22, 14}, {34, 14}, {40, 40}, {16, 40}}, 3);
        b.ellipse(28, 52, 24, 8, 2);
        b.ellipse(28, 50, 16, 4, 3);
        b.rect(10, 54, 36, 4, 1);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(12, 28);
        b.rect(5, 0, 2, 16, 1);
        b.ellipse(6, 22, 5, 5, 2);
        art.clapper = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 80);
        b.rect(4, 0, 3, 68, 2);
        b.rect(5, 0, 1, 64, 3);
        b.ellipse(5, 74, 4, 5, 1);
        art.rope = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 72);
        b.ellipse(18, 10, 8, 9, 3);
        b.rect(12, 18, 12, 18, 2);
        b.poly({{12, 24}, {2, 42}, {6, 46}, {14, 30}}, 2);
        b.poly({{22, 22}, {32, 36}, {28, 40}, {18, 28}}, 4);
        b.rect(11, 36, 6, 24, 1);
        b.rect(19, 36, 6, 24, 1);
        b.rect(9, 58, 9, 5, 5);
        b.rect(18, 58, 9, 5, 5);
        art.ringer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 10);
        b.ellipse(8, 5, 7, 4, 2);
        b.ellipse(8, 4, 3, 2, 3);
        art.lip = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace bellgold
