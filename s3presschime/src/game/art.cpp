#include "game/art.h"

#include <cmath>

namespace presschime {
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
    setPal(vdp, PAL_FACE, {0, gs::rgb4(14, 12, 8), gs::rgb4(4, 3, 2), gs::rgb4(10, 8, 5)});

    loadFont(vdp, art.font);

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
        gs::Bitmap b(52, 52);
        b.ellipse(26, 26, 24, 24, 1);
        b.ellipse(26, 26, 20, 20, 2);
        for (int i = 0; i < 12; i++) {
            float a = kPi * 2.f * float(i) / 12.f;
            float x = 26 + std::sin(a) * 17.f;
            float y = 26 - std::cos(a) * 17.f;
            b.rect(x - 1, y - 1, 2, 2, 3);
        }
        art.clock = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(4, 4);
        b.rect(0, 0, 4, 4, 1);
        art.pip = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(14, 40);
        b.ellipse(7, 20, 6, 18, 1);
        b.ellipse(7, 20, 3, 14, 2);
        art.roller = gs::uploadImage(vdp, b);
    }
}

}  // namespace presschime
