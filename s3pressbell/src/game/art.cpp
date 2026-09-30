#include "game/art.h"

namespace pressbell {
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 10)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 2)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(3, 3, 4), gs::rgb4(7, 7, 8), gs::rgb4(11, 11, 12), gs::rgb4(14, 14, 15)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(4, 2, 1), gs::rgb4(8, 4, 2), gs::rgb4(12, 7, 3), gs::rgb4(6, 3, 2),
                           gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(10, 7, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 8), gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(12, 11, 8), gs::rgb4(15, 14, 11), gs::rgb4(8, 7, 5), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(1, 1, 4), gs::rgb4(2, 2, 8), gs::rgb4(8, 2, 2), gs::rgb4(4, 1, 1)});

    {
        gs::Bitmap b(120, 150);
        b.rect(4, 4, 14, 142, 1);
        b.rect(102, 4, 14, 142, 1);
        b.rect(4, 4, 112, 16, 2);
        b.rect(4, 130, 112, 16, 2);
        b.rect(8, 8, 104, 6, 3);
        b.rect(18, 20, 84, 8, 4);
        b.line(18, 40, 18, 128, 5, 2.f);
        b.line(102, 40, 102, 128, 5, 2.f);
        art.frame = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(96, 16);
        b.rect(0, 4, 96, 10, 1);
        b.rect(2, 5, 92, 4, 3);
        b.rect(8, 0, 8, 16, 2);
        b.rect(80, 0, 8, 16, 2);
        art.platen = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(70, 78);
        b.rect(2, 2, 66, 74, 1);
        b.rect(6, 8, 58, 48, 2);
        for (int y = 12; y < 50; y += 6) b.line(10, float(y), 58, float(y), 3, 1.f);
        b.rect(10, 58, 40, 6, 3);
        art.sheet = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 70);
        b.rect(6, 0, 4, 70, 2);
        b.rect(7, 0, 2, 70, 3);
        for (int y = 4; y < 66; y += 6) b.ellipse(8, float(y), 6, 2, 1);
        art.screw = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(26, 22);
        b.poly({{13, 1}, {4, 7}, {22, 7}}, 3);
        b.poly({{5, 7}, {3, 16}, {23, 16}, {21, 7}}, 2);
        b.rect(8, 9, 10, 4, 3);
        b.ellipse(13, 18, 3, 3, 1);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 90);
        b.rect(3, 0, 4, 90, 1);
        b.rect(4, 0, 2, 90, 2);
        b.rect(0, 84, 10, 6, 3);
        art.post = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 22);
        b.ellipse(11, 11, 10, 10, 1);
        b.ellipse(11, 11, 6, 6, 2);
        b.ellipse(8, 8, 2, 2, 3);
        art.roller = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 10);
        b.ellipse(5, 5, 4, 4, 1);
        b.ellipse(5, 5, 2, 2, 2);
        art.lamp = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace pressbell
