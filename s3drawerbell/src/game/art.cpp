#include "game/art.h"

namespace drawerbell {
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

void disk(gs::Bitmap& b, float cx, float cy, float r, int rim, int face, int shine) {
    b.ellipse(cx, cy, r, r, rim);
    b.ellipse(cx, cy, r * 0.78f, r * 0.78f, face);
    b.ellipse(cx - r * 0.22f, cy - r * 0.22f, r * 0.22f, r * 0.18f, shine);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(4, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 4), gs::rgb4(3, 2, 1),
                           gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_COIN, {0, gs::rgb4(10, 6, 2), gs::rgb4(14, 10, 4), gs::rgb4(15, 14, 8), gs::rgb4(8, 8, 9),
                           gs::rgb4(13, 13, 14)});
    setPal(vdp, PAL_SLIP, {0, gs::rgb4(14, 13, 10), gs::rgb4(3, 3, 4), gs::rgb4(10, 3, 3)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(8, 6, 2), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 9), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_LIT, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_CLERK, {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 6, 8), gs::rgb4(12, 8, 6), gs::rgb4(3, 2, 2),
                            gs::rgb4(9, 3, 3)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(2, 2, 2)});

    loadFont(vdp, art.font);

    const float rad[4] = {7.f, 9.f, 8.f, 12.f};
    const int face[4] = {2, 4, 4, 2};
    for (int i = 0; i < 4; i++) {
        int s = int(rad[i] * 2 + 6);
        gs::Bitmap b(s, s);
        disk(b, s * 0.5f, s * 0.5f, rad[i], 1, face[i], 3);
        art.coin[i] = gs::uploadImage(vdp, b);
    }

    {
        gs::Bitmap b(280, 48);
        b.rect(0, 0, 280, 48, 1);
        b.rect(6, 6, 268, 36, 4);
        b.rect(10, 10, 260, 28, 2);
        b.rect(128, 16, 24, 8, 3);
        b.rect(132, 18, 16, 4, 5);
        art.drawer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 32);
        b.poly({{18, 2}, {32, 10}, {30, 26}, {6, 26}, {4, 10}}, 1);
        b.poly({{18, 5}, {28, 11}, {26, 23}, {10, 23}, {8, 11}}, 2);
        b.ellipse(18, 8, 4, 3, 3);
        b.rect(16, 26, 4, 5, 4);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 10);
        b.ellipse(4, 6, 3, 3, 4);
        b.rect(3, 0, 2, 5, 1);
        art.clapper = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 52);
        b.ellipse(14, 8, 7, 7, 3);
        b.rect(8, 16, 12, 16, 2);
        b.rect(6, 18, 4, 12, 2);
        b.rect(18, 18, 4, 12, 2);
        b.rect(9, 32, 4, 16, 1);
        b.rect(15, 32, 4, 16, 1);
        b.rect(10, 14, 8, 3, 5);
        art.clerk = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(52, 64);
        b.rect(2, 0, 48, 62, 1);
        b.rect(6, 8, 28, 3, 2);
        b.rect(6, 16, 36, 2, 2);
        b.rect(6, 22, 22, 2, 2);
        b.rect(6, 36, 30, 4, 3);
        art.slip = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 10);
        b.rect(1, 1, 8, 8, 3);
        b.rect(3, 3, 4, 4, 2);
        art.mark = gs::uploadImage(vdp, b);
    }
}

}  // namespace drawerbell
