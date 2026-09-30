#include "game/art.h"

#include <cmath>

namespace hornchime {
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 12)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(6, 4, 1), gs::rgb4(13, 9, 2), gs::rgb4(15, 14, 7)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(12, 3, 3)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(4, 2, 1), gs::rgb4(9, 6, 2), gs::rgb4(14, 11, 4), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(1, 2, 4), gs::rgb4(3, 4, 7), gs::rgb4(8, 6, 4), gs::rgb4(13, 11, 9), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 7), gs::rgb4(9, 8, 7), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(4, 3, 2), gs::rgb4(12, 10, 7), gs::rgb4(2, 2, 2), gs::rgb4(15, 13, 8)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(8, 6, 3), gs::rgb4(12, 8, 3)});

    {
        gs::Bitmap b(40, 78);
        b.ellipse(20, 12, 9, 10, 4);
        b.rect(14, 20, 12, 4, 3);
        b.rect(11, 24, 18, 26, 2);
        b.rect(11, 24, 18, 5, 1);
        b.rect(8, 28, 5, 14, 2);
        b.rect(27, 30, 5, 12, 2);
        b.rect(13, 50, 5, 20, 1);
        b.rect(22, 50, 5, 20, 1);
        b.rect(10, 68, 9, 5, 5);
        b.rect(21, 68, 9, 5, 5);
        art.player = gs::uploadImage(vdp, b);
    }
    {
        // A coiled post horn, mouthpiece left, bell flared right.
        gs::Bitmap b(92, 36);
        b.ellipse(16, 22, 12, 10, 2);
        b.ellipse(16, 22, 6, 5, 0);
        b.rect(22, 16, 28, 7, 2);
        b.rect(24, 18, 22, 3, 3);
        b.rect(0, 17, 10, 5, 1);
        b.ellipse(68, 16, 22, 14, 2);
        b.ellipse(74, 16, 12, 8, 4);
        b.ellipse(78, 16, 6, 4, 0);
        b.rect(48, 14, 8, 4, 3);
        art.horn = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(54, 150);
        b.rect(8, 28, 38, 122, 2);
        b.rect(4, 20, 46, 12, 3);
        b.rect(14, 0, 26, 24, 1);
        b.rect(20, 0, 14, 8, 3);
        b.rect(16, 48, 10, 16, 4);
        b.rect(28, 70, 10, 16, 4);
        b.rect(16, 96, 10, 18, 4);
        b.rect(22, 132, 12, 18, 1);
        art.tower = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(46, 46);
        b.ellipse(23, 23, 22, 22, 2);
        b.ellipse(23, 23, 18, 18, 1);
        b.ellipse(23, 23, 2, 2, 3);
        for (int i = 0; i < 12; i++) {
            float a = i * 3.1415926f / 6.f - 1.5708f;
            int x = int(23 + std::cos(a) * 16);
            int y = int(23 + std::sin(a) * 16);
            b.set(x, y, 4);
            b.set(x + 1, y, 4);
        }
        art.face = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 26);
        b.rect(9, 0, 4, 5, 3);
        b.ellipse(11, 15, 10, 10, 2);
        b.ellipse(11, 14, 5, 6, 1);
        b.rect(7, 22, 8, 3, 3);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(4, 4);
        b.rect(0, 0, 4, 4, 1);
        art.dot = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 8);
        b.rect(0, 0, 8, 8, 1);
        art.bar = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace hornchime
