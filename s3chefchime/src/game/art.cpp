#include "game/art.h"

namespace chefchime {
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

void toque(gs::Bitmap& b, int brim, int puff) {
    b.ellipse(20, 12, 11, 7, puff);
    b.ellipse(16, 10, 4, 3, puff);
    b.rect(9, 16, 22, 5, brim);
    b.rect(11, 18, 18, 2, puff);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(14, 2, 2)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(4, 13, 5)});
    setPal(vdp, PAL_CHEF, {0, gs::rgb4(15, 14, 12), gs::rgb4(12, 8, 5), gs::rgb4(2, 2, 3), gs::rgb4(13, 3, 3),
                           gs::rgb4(8, 5, 3), gs::rgb4(4, 3, 3)});
    setPal(vdp, PAL_FOOD, {0, gs::rgb4(10, 4, 2), gs::rgb4(14, 7, 3), gs::rgb4(15, 12, 6), gs::rgb4(6, 3, 2),
                           gs::rgb4(4, 11, 3)});
    setPal(vdp, PAL_STEEL, {0, gs::rgb4(6, 7, 8), gs::rgb4(12, 13, 14), gs::rgb4(3, 3, 4), gs::rgb4(15, 14, 10)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(14, 4, 1), gs::rgb4(15, 10, 2), gs::rgb4(15, 14, 5)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(8, 6, 3), gs::rgb4(14, 12, 8), gs::rgb4(2, 2, 2), gs::rgb4(15, 14, 6),
                            gs::rgb4(5, 3, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 4, 2), gs::rgb4(12, 8, 4), gs::rgb4(4, 2, 1), gs::rgb4(15, 13, 8)});

    loadFont(vdp, art.font);

    {
        gs::Bitmap b(40, 58);
        toque(b, 1, 2);
        b.rect(14, 22, 12, 6, 2);
        b.ellipse(18, 26, 1.1f, 1.1f, 3);
        b.ellipse(23, 26, 1.1f, 1.1f, 3);
        b.rect(18, 29, 5, 1, 3);
        b.rect(12, 32, 16, 14, 1);
        b.rect(12, 32, 16, 3, 4);
        b.rect(10, 44, 20, 4, 1);
        b.rect(14, 48, 4, 9, 5);
        b.rect(22, 48, 4, 9, 6);
        b.rect(6, 34, 6, 10, 1);
        b.rect(28, 34, 6, 10, 1);
        b.ellipse(8, 44, 2.5f, 2.f, 2);
        b.ellipse(32, 44, 2.5f, 2.f, 2);
        art.chef = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(40, 58);
        toque(b, 1, 2);
        b.rect(14, 22, 12, 6, 2);
        b.ellipse(18, 26, 1.1f, 1.1f, 3);
        b.ellipse(23, 26, 1.1f, 1.1f, 3);
        b.rect(12, 32, 16, 14, 1);
        b.rect(12, 32, 16, 3, 4);
        b.rect(10, 44, 20, 4, 1);
        b.rect(13, 48, 4, 9, 5);
        b.rect(23, 48, 4, 9, 6);
        b.poly({{6, 36}, {4, 28}, {10, 28}, {12, 38}}, 1);
        b.poly({{28, 34}, {36, 40}, {34, 44}, {26, 40}}, 1);
        art.chefWalk = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 22);
        b.ellipse(18, 14, 14, 6, 2);
        b.ellipse(18, 12, 10, 4, 3);
        b.ellipse(14, 11, 5, 2, 1);
        b.ellipse(22, 13, 3, 2, 4);
        b.rect(10, 10, 8, 2, 5);
        art.roast = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(52, 18);
        b.ellipse(24, 8, 18, 6, 1);
        b.ellipse(24, 7, 13, 4, 2);
        b.rect(6, 10, 36, 3, 3);
        b.rect(4, 11, 5, 3, 4);
        b.rect(40, 11, 8, 3, 4);
        art.pan = gs::uploadImage(vdp, b);
    }
    for (int f = 0; f < 2; f++) {
        gs::Bitmap b(24, 16);
        float lean = f ? 2.f : -1.5f;
        b.poly({{6, 14}, {11 + lean, 2}, {14, 14}}, 1);
        b.poly({{9, 14}, {11 + lean, 6}, {13, 14}}, 2);
        b.poly({{13, 14}, {17 - lean, 3}, {21, 14}}, 3);
        art.flame[f] = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(40, 40);
        b.rect(4, 4, 32, 32, 1);
        b.rect(8, 8, 24, 24, 2);
        b.ellipse(20, 20, 10, 10, 2);
        b.ellipse(20, 20, 8, 8, 0);
        b.rect(19, 8, 2, 6, 4);
        b.rect(19, 18, 2, 8, 3);
        b.rect(19, 18, 6, 2, 3);
        b.rect(18, 2, 4, 4, 5);
        art.clock = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 18);
        b.rect(3, 0, 2, 10, 5);
        b.ellipse(4, 14, 3.2f, 3.2f, 4);
        art.pendulum = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 64);
        b.rect(2, 2, 32, 60, 1);
        b.rect(6, 6, 24, 50, 2);
        b.rect(8, 8, 9, 14, 3);
        b.rect(19, 8, 9, 14, 3);
        b.rect(8, 26, 20, 26, 0);
        b.ellipse(24, 40, 1.4f, 1.4f, 4);
        art.door = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 8);
        b.rect(0, 0, 8, 8, 1);
        art.solid = gs::uploadImage(vdp, b);
    }
}

}  // namespace chefchime
