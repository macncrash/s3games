#include "game/art.h"

namespace chefbell {
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
    b.ellipse(24, 14, 12, 8, puff);
    b.ellipse(20, 12, 5, 3, puff);
    b.rect(12, 18, 24, 5, brim);
    b.rect(14, 20, 20, 2, puff);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(14, 2, 2)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(4, 13, 5)});
    setPal(vdp, PAL_CHEF, {0, gs::rgb4(15, 14, 12), gs::rgb4(12, 8, 5), gs::rgb4(2, 2, 3), gs::rgb4(14, 3, 3),
                           gs::rgb4(8, 5, 3), gs::rgb4(4, 3, 3)});
    setPal(vdp, PAL_FOOD, {0, gs::rgb4(10, 4, 2), gs::rgb4(14, 7, 3), gs::rgb4(15, 12, 6), gs::rgb4(6, 3, 2),
                           gs::rgb4(13, 12, 10), gs::rgb4(4, 10, 3)});
    setPal(vdp, PAL_STEEL, {0, gs::rgb4(6, 7, 8), gs::rgb4(11, 12, 13), gs::rgb4(3, 3, 4), gs::rgb4(15, 14, 10)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(14, 4, 1), gs::rgb4(15, 10, 2), gs::rgb4(15, 14, 5)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(10, 7, 2), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 10), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_BAR, {0, gs::rgb4(3, 2, 2), gs::rgb4(12, 4, 2), gs::rgb4(14, 12, 3), gs::rgb4(4, 4, 5)});

    {
        gs::Bitmap b(48, 64);
        toque(b, 1, 2);
        b.rect(18, 24, 12, 6, 2);
        b.ellipse(22, 28, 1.2f, 1.2f, 3);
        b.ellipse(27, 28, 1.2f, 1.2f, 3);
        b.rect(22, 31, 5, 1, 3);
        b.rect(16, 34, 16, 16, 1);
        b.rect(16, 34, 16, 3, 4);
        b.rect(14, 48, 20, 4, 1);
        b.rect(18, 52, 5, 10, 5);
        b.rect(25, 52, 5, 10, 6);
        b.rect(10, 36, 6, 12, 1);
        b.rect(32, 36, 6, 12, 1);
        b.ellipse(12, 48, 3, 2, 2);
        b.ellipse(36, 48, 3, 2, 2);
        art.chef = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(48, 64);
        toque(b, 1, 2);
        b.rect(18, 24, 12, 6, 2);
        b.ellipse(22, 28, 1.2f, 1.2f, 3);
        b.ellipse(27, 28, 1.2f, 1.2f, 3);
        b.rect(16, 34, 16, 16, 1);
        b.rect(16, 34, 16, 3, 4);
        b.rect(14, 48, 20, 4, 1);
        b.rect(18, 52, 5, 10, 5);
        b.rect(25, 52, 5, 10, 6);
        b.rect(10, 36, 6, 8, 1);
        b.poly({{32, 36}, {44, 28}, {46, 32}, {34, 42}}, 1);
        b.ellipse(45, 29, 2.4f, 2.0f, 2);
        art.chefReach = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(40, 28);
        b.ellipse(20, 20, 16, 6, 5);
        b.ellipse(20, 18, 12, 4, 4);
        b.ellipse(20, 14, 11, 6, 2);
        b.ellipse(16, 13, 6, 3, 1);
        b.ellipse(24, 15, 4, 2, 3);
        b.rect(12, 13, 10, 2, 6);
        art.steak = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(56, 22);
        b.ellipse(28, 10, 22, 8, 1);
        b.ellipse(28, 9, 16, 5, 2);
        b.rect(6, 12, 44, 4, 3);
        b.rect(4, 14, 6, 4, 4);
        b.rect(46, 14, 6, 4, 4);
        art.pan = gs::uploadImage(vdp, b);
    }
    for (int f = 0; f < 2; f++) {
        gs::Bitmap b(28, 18);
        float lean = f ? 2.f : -2.f;
        b.poly({{8, 16}, {14 + lean, 2}, {18, 16}}, 1);
        b.poly({{12, 16}, {14 + lean, 6}, {16, 16}}, 2);
        b.poly({{16, 16}, {20 - lean, 4}, {24, 16}}, 3);
        art.flame[f] = gs::uploadImage(vdp, b);
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
        gs::Bitmap b(4, 4);
        b.rect(0, 0, 4, 4, 1);
        art.solid = gs::uploadImage(vdp, b);
    }

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    loadFont(vdp, art.font);
}

}  // namespace chefbell
