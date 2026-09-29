#include "game/art.h"

namespace drawergold {
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

void grain(gs::Bitmap& b, int a, int c, int step) {
    for (int y = 2; y < b.h - 2; y += step)
        for (int x = 3; x < b.w - 3; x += step + ((y / step) & 1)) b.set(x, y, (x + y) & 1 ? c : a);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(13, 3, 2)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_DESK, {0, gs::rgb4(4, 2, 1), gs::rgb4(8, 4, 2), gs::rgb4(12, 7, 3), gs::rgb4(2, 1, 1),
                           gs::rgb4(1, 5, 3), gs::rgb4(6, 3, 2)});
    setPal(vdp, PAL_SLIP, {0, gs::rgb4(10, 6, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 8), gs::rgb4(6, 3, 1),
                           gs::rgb4(12, 4, 2)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(10, 8, 5), gs::rgb4(14, 12, 8), gs::rgb4(15, 14, 11), gs::rgb4(6, 5, 3),
                            gs::rgb4(8, 3, 2)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(8, 6, 2), gs::rgb4(14, 11, 4), gs::rgb4(15, 14, 8), gs::rgb4(4, 3, 1)});

    {
        gs::Bitmap b(240, 150);
        b.rect(0, 0, 240, 150, 1);
        b.rect(8, 8, 224, 134, 2);
        b.rect(16, 18, 208, 16, 3);
        grain(b, 2, 6, 7);
        b.rect(20, 48, 200, 86, 4);
        b.rect(28, 56, 184, 70, 5);
        art.desk = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(200, 64);
        b.rect(0, 8, 200, 52, 2);
        b.rect(6, 14, 188, 40, 1);
        b.rect(10, 18, 180, 8, 3);
        b.rect(0, 0, 200, 12, 6);
        b.rect(0, 56, 200, 8, 4);
        for (int i = 0; i < 4; i++) b.rect(18 + i * 44, 24, 32, 26, 5);
        art.drawer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 40);
        b.rect(1, 1, 26, 38, 1);
        b.rect(3, 3, 22, 34, 2);
        b.rect(5, 5, 18, 6, 3);
        b.ellipse(14, 22, 7, 7, 4);
        b.ellipse(14, 22, 4.2f, 4.2f, 2);
        b.ellipse(12, 20, 1.4f, 1.0f, 3);
        art.gold = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 40);
        b.rect(1, 1, 26, 38, 1);
        b.rect(3, 3, 22, 34, 2);
        b.rect(5, 6, 18, 3, 3);
        b.rect(6, 14, 16, 2, 4);
        b.rect(6, 20, 12, 2, 4);
        b.rect(6, 26, 14, 2, 1);
        art.cream = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(18, 18);
        b.ellipse(9, 9, 8, 8, 1);
        b.ellipse(9, 9, 5.5f, 5.5f, 2);
        b.ellipse(7.2f, 7, 1.6f, 1.1f, 3);
        b.rect(8, 3, 2, 12, 4);
        art.knob = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace drawergold
