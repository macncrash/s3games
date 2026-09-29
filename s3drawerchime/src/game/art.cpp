#include "game/art.h"

#include <cmath>

namespace drawerchime {
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

void hand(gs::Bitmap& b, float ang, float len, int c) {
    float cx = b.w * 0.5f;
    float cy = b.h * 0.5f;
    float x1 = cx + std::sin(ang) * len;
    float y1 = cy - std::cos(ang) * len;
    b.line(cx, cy, x1, y1, c, 2.f);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(3, 2, 1), gs::rgb4(7, 4, 2), gs::rgb4(11, 7, 3), gs::rgb4(5, 3, 2),
                           gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(8, 6, 2), gs::rgb4(14, 11, 4), gs::rgb4(15, 14, 8), gs::rgb4(4, 3, 1)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(6, 5, 2), gs::rgb4(13, 11, 4), gs::rgb4(15, 15, 10), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(10, 9, 7), gs::rgb4(15, 14, 11), gs::rgb4(2, 2, 3), gs::rgb4(8, 2, 2)});
    setPal(vdp, PAL_LIT, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_CLERK, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 7, 9), gs::rgb4(12, 8, 6), gs::rgb4(9, 3, 3),
                            gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(12, 12, 14), gs::rgb4(8, 8, 10)});

    loadFont(vdp, art.font);

    {
        gs::Bitmap b(148, 176);
        b.rect(0, 0, 148, 176, 1);
        b.rect(6, 6, 136, 164, 4);
        b.rect(14, 14, 120, 148, 5);
        for (int i = 0; i < 4; i++) b.rect(18, 18 + i * 36, 112, 32, 2);
        art.cabinet = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(104, 28);
        b.rect(0, 0, 104, 28, 2);
        b.rect(3, 3, 98, 22, 3);
        b.rect(8, 8, 40, 3, 1);
        b.rect(8, 14, 28, 2, 1);
        art.drawer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(104, 28);
        b.rect(0, 0, 104, 28, 2);
        b.rect(3, 3, 98, 22, 3);
        b.rect(8, 10, 18, 8, 1);
        art.chimeBox = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(14, 10);
        b.ellipse(7, 5, 6, 4, 1);
        b.ellipse(7, 5, 3, 2, 2);
        art.knob = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(40, 34);
        b.poly({{20, 2}, {36, 12}, {33, 26}, {7, 26}, {4, 12}}, 1);
        b.poly({{20, 6}, {31, 13}, {29, 23}, {11, 23}, {9, 13}}, 2);
        b.ellipse(20, 8, 5, 3, 3);
        b.rect(18, 26, 4, 6, 4);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 12);
        b.rect(3, 0, 2, 6, 1);
        b.ellipse(4, 8, 3, 3, 4);
        art.clapper = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(64, 64);
        b.ellipse(32, 32, 30, 30, 1);
        b.ellipse(32, 32, 26, 26, 2);
        b.ellipse(32, 32, 3, 3, 3);
        for (int i = 0; i < 12; i++) {
            float a = i * 3.14159265f / 6.f;
            float x0 = 32 + std::sin(a) * 20.f;
            float y0 = 32 - std::cos(a) * 20.f;
            float x1 = 32 + std::sin(a) * 24.f;
            float y1 = 32 - std::cos(a) * 24.f;
            b.line(x0, y0, x1, y1, 3, 1.5f);
        }
        art.face = gs::uploadImage(vdp, b);
    }
    for (int i = 0; i < 12; i++) {
        float a = i * 3.14159265f / 6.f;
        gs::Bitmap h(64, 64);
        hand(h, a, 16.f, 1);
        art.handH[i] = gs::uploadImage(vdp, h);
        gs::Bitmap m(64, 64);
        hand(m, a, 22.f, 1);
        art.handM[i] = gs::uploadImage(vdp, m);
    }
    {
        gs::Bitmap b(26, 48);
        b.ellipse(13, 8, 6, 6, 3);
        b.rect(7, 15, 12, 14, 2);
        b.rect(4, 16, 4, 10, 2);
        b.rect(18, 16, 4, 10, 2);
        b.rect(8, 29, 4, 16, 1);
        b.rect(14, 29, 4, 16, 1);
        b.rect(9, 14, 8, 3, 4);
        art.clerk = gs::uploadImage(vdp, b);
    }
}

}  // namespace drawerchime
