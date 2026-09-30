#include "game/art.h"

namespace belltape {
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 13)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(5, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(13, 3, 3)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(6, 4, 1), gs::rgb4(12, 8, 2), gs::rgb4(15, 13, 5), gs::rgb4(8, 9, 10)});
    setPal(vdp, PAL_ROPE, {0, gs::rgb4(5, 3, 1), gs::rgb4(10, 6, 2), gs::rgb4(14, 11, 6)});
    setPal(vdp, PAL_PEAL, {0, gs::rgb4(4, 6, 10), gs::rgb4(8, 12, 15), gs::rgb4(13, 15, 15)});
    setPal(vdp, PAL_KNOCK, {0, gs::rgb4(8, 4, 1), gs::rgb4(14, 8, 2), gs::rgb4(15, 13, 5)});
    setPal(vdp, PAL_TOLL, {0, gs::rgb4(3, 6, 3), gs::rgb4(6, 11, 5), gs::rgb4(12, 15, 8)});
    setPal(vdp, PAL_CLANG, {0, gs::rgb4(4, 3, 4), gs::rgb4(8, 6, 7), gs::rgb4(12, 10, 11)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(4, 2, 1), gs::rgb4(9, 5, 2), gs::rgb4(13, 8, 4)});

    {
        gs::Bitmap b(120, 28);
        b.rect(0, 8, 120, 12, 1);
        b.rect(8, 4, 8, 20, 2);
        b.rect(104, 4, 8, 20, 2);
        b.rect(18, 0, 6, 10, 3);
        b.rect(96, 0, 6, 10, 3);
        art.belfry = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(64, 72);
        b.rect(28, 0, 8, 8, 3);
        b.poly({{22, 8}, {42, 8}, {52, 22}, {56, 48}, {50, 64}, {14, 64}, {8, 48}, {12, 22}}, 2);
        b.poly({{26, 12}, {38, 12}, {46, 24}, {48, 46}, {20, 46}, {18, 24}}, 3);
        b.rect(12, 62, 40, 6, 1);
        b.ellipse(32, 66, 18, 4, 2);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(12, 36);
        b.rect(5, 0, 2, 22, 1);
        b.ellipse(6, 28, 5, 6, 2);
        art.clapper = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 48);
        b.rect(3, 0, 2, 36, 2);
        b.ellipse(4, 42, 3, 5, 1);
        art.rope = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 22);
        b.ellipse(11, 12, 8, 8, 2);
        b.rect(8, 2, 6, 6, 3);
        b.ellipse(11, 12, 3, 3, 1);
        art.peal = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(20, 16);
        b.rect(2, 4, 16, 8, 2);
        b.poly({{4, 4}, {10, 1}, {16, 4}}, 3);
        b.rect(8, 6, 4, 4, 1);
        art.knock = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 28);
        b.poly({{6, 2}, {10, 2}, {14, 10}, {13, 24}, {3, 24}, {2, 10}}, 2);
        b.rect(5, 0, 6, 4, 3);
        b.rect(4, 22, 8, 3, 1);
        art.toll = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 18);
        b.poly({{2, 12}, {8, 2}, {14, 6}, {18, 2}, {20, 12}, {12, 16}, {4, 16}}, 2);
        b.line(8, 6, 14, 14, 1, 1.2f);
        art.clang = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(48, 22);
        b.rect(1, 1, 46, 20, 1);
        b.rect(4, 4, 40, 12, 2);
        b.rect(18, 8, 12, 3, 3);
        art.drawer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 10);
        b.ellipse(5, 5, 4, 4, 1);
        b.ellipse(5, 4, 2, 2, 2);
        art.lamp = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace belltape
