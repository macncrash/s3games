#include "game/art.h"

#include <cmath>

namespace drumchime {
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

gs::Image head(gs::VDP& vdp, bool lugs) {
    gs::Bitmap b(36, 28);
    b.ellipse(18, 16, 16, 12, 1);
    b.ellipse(18, 15, 13, 9, 2);
    b.ellipse(14, 12, 4, 2.2f, 3);
    if (lugs) {
        for (int i = 0; i < 6; i++) {
            float a = i * 1.0472f;
            b.ellipse(18 + std::cos(a) * 14.f, 16 + std::sin(a) * 10.f, 1.4f, 1.2f, 4);
        }
    }
    return gs::uploadImage(vdp, b);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(12, 8, 2), gs::rgb4(15, 15, 8), gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_SHELL, {0, gs::rgb4(4, 2, 1), gs::rgb4(9, 5, 2), gs::rgb4(13, 8, 3), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_HEAD, {0, gs::rgb4(6, 5, 4), gs::rgb4(12, 11, 9), gs::rgb4(15, 15, 13), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_LIT, {0, gs::rgb4(8, 5, 1), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 10), gs::rgb4(4, 3, 1)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(8, 7, 4), gs::rgb4(14, 12, 8), gs::rgb4(15, 15, 12), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(10, 8, 2), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 10), gs::rgb4(5, 3, 1)});
    setPal(vdp, PAL_STICK, {0, gs::rgb4(8, 5, 2), gs::rgb4(14, 11, 6), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_STAGE, {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 4, 6), gs::rgb4(8, 3, 2), gs::rgb4(11, 9, 7)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(12, 11, 9), gs::rgb4(3, 3, 4), gs::rgb4(15, 14, 10), gs::rgb4(8, 2, 2)});

    loadFont(vdp, art.font);

    {
        gs::Bitmap b(40, 16);
        b.rect(2, 2, 36, 12, 1);
        b.rect(5, 4, 30, 8, 2);
        b.rect(8, 6, 6, 4, 3);
        art.shell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(72, 44);
        b.ellipse(36, 24, 32, 16, 1);
        b.ellipse(36, 22, 26, 11, 2);
        b.ellipse(28, 18, 8, 4, 3);
        art.bass = gs::uploadImage(vdp, b);
    }
    art.headWood = head(vdp, true);
    art.headGold = head(vdp, false);
    art.headCream = head(vdp, true);
    {
        gs::Bitmap b(40, 16);
        b.ellipse(20, 10, 18, 5, 1);
        b.ellipse(20, 9, 12, 3, 2);
        b.ellipse(14, 8, 3, 1.2f, 3);
        art.cym = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(6, 28);
        b.rect(2, 2, 2, 22, 1);
        b.rect(1, 20, 4, 6, 2);
        art.stick = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 32);
        b.rect(12, 1, 4, 6, 4);
        b.ellipse(14, 18, 10, 11, 1);
        b.ellipse(14, 16, 7, 8, 2);
        b.ellipse(11, 13, 2, 2, 3);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(48, 48);
        b.ellipse(24, 24, 22, 22, 1);
        b.ellipse(24, 24, 18, 18, 2);
        b.ellipse(24, 24, 2, 2, 3);
        for (int i = 0; i < 12; i++) {
            float a = i * 0.5236f - 1.5708f;
            float r0 = (i % 3 == 0) ? 12.f : 15.f;
            b.line(24 + std::cos(a) * r0, 24 + std::sin(a) * r0, 24 + std::cos(a) * 17.f, 24 + std::sin(a) * 17.f, 3,
                   1.2f);
        }
        art.face = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(4, 16);
        b.rect(1, 1, 2, 14, 4);
        art.hand = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(20, 36);
        b.ellipse(10, 6, 5, 5, 1);
        b.rect(6, 12, 8, 12, 2);
        b.rect(3, 13, 4, 8, 3);
        b.rect(13, 13, 4, 8, 3);
        b.rect(6, 24, 3, 10, 4);
        b.rect(11, 24, 3, 10, 4);
        art.player = gs::uploadImage(vdp, b);
    }
}

}  // namespace drumchime
