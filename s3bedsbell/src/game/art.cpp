#include "game/art.h"

namespace bedsbell {
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(14, 2, 2)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(4, 13, 5)});
    setPal(vdp, PAL_SOIL, {0, gs::rgb4(6, 3, 1), gs::rgb4(9, 5, 2), gs::rgb4(4, 2, 1), gs::rgb4(12, 8, 3)});
    setPal(vdp, PAL_LEAF, {0, gs::rgb4(2, 8, 2), gs::rgb4(5, 13, 3), gs::rgb4(12, 14, 4), gs::rgb4(10, 3, 4)});
    setPal(vdp, PAL_CAN, {0, gs::rgb4(2, 6, 10), gs::rgb4(6, 11, 15), gs::rgb4(14, 15, 15), gs::rgb4(1, 3, 6)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(10, 7, 2), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 10), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_MAN, {0, gs::rgb4(12, 8, 5), gs::rgb4(4, 8, 3), gs::rgb4(2, 3, 6), gs::rgb4(14, 12, 9),
                          gs::rgb4(8, 4, 2)});
    setPal(vdp, PAL_BAR, {0, gs::rgb4(3, 3, 2), gs::rgb4(8, 10, 14), gs::rgb4(14, 12, 3), gs::rgb4(12, 3, 2)});

    {
        gs::Bitmap b(36, 22);
        b.poly({{2, 6}, {34, 6}, {32, 20}, {4, 20}}, 1);
        b.rect(4, 8, 28, 8, 2);
        b.rect(6, 10, 24, 3, 3);
        b.rect(2, 4, 32, 3, 4);
        art.bed = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 22);
        b.line(8, 20, 8, 8, 1, 1.4f);
        b.ellipse(5, 8, 4, 3, 2);
        b.ellipse(11, 9, 3.5f, 2.6f, 2);
        art.sprout = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(18, 24);
        b.line(9, 22, 9, 8, 1, 1.6f);
        b.ellipse(5, 8, 4.5f, 3.2f, 2);
        b.ellipse(13, 9, 4, 3, 2);
        b.ellipse(9, 5, 3, 2.4f, 3);
        b.ellipse(9, 5, 1.2f, 1.2f, 4);
        art.bloom = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 24);
        b.rect(4, 6, 14, 12, 1);
        b.rect(6, 8, 10, 6, 2);
        b.poly({{18, 8}, {26, 4}, {26, 8}, {18, 12}}, 3);
        b.rect(24, 3, 3, 4, 4);
        b.ellipse(8, 5, 3, 2, 2);
        art.can = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 16);
        b.ellipse(3, 3, 1.4f, 2, 2);
        b.ellipse(6, 8, 1.4f, 2.2f, 1);
        b.ellipse(4, 13, 1.6f, 2, 2);
        art.pour = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 32);
        b.poly({{6, 22}, {18, 4}, {30, 22}}, 1);
        b.poly({{10, 20}, {18, 9}, {26, 20}}, 2);
        b.rect(4, 21, 28, 5, 2);
        b.rect(4, 21, 28, 2, 3);
        b.rect(16, 1, 4, 4, 4);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 10);
        b.ellipse(4, 4, 2.4f, 2.4f, 4);
        b.rect(3, 6, 2, 4, 1);
        art.clapper = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 40);
        b.ellipse(14, 8, 6, 6, 4);
        b.rect(10, 14, 8, 12, 2);
        b.rect(8, 26, 12, 3, 5);
        b.rect(10, 29, 3, 9, 3);
        b.rect(15, 29, 3, 9, 3);
        b.rect(4, 16, 6, 3, 4);
        b.rect(18, 16, 8, 3, 4);
        art.gardener = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(4, 4);
        b.rect(0, 0, 4, 4, 1);
        art.solid = gs::uploadImage(vdp, b);
    }

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    loadFont(vdp, art.font);
}

}  // namespace bedsbell
