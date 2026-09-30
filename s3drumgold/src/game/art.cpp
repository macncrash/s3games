#include "game/art.h"

#include <cmath>

namespace drumgold {
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

gs::Image drumHead(gs::VDP& vdp, int rim, int skin, int shine, int lug) {
    gs::Bitmap b(36, 28);
    b.ellipse(18, 16, 16, 12, rim);
    b.ellipse(18, 15, 13, 9, skin);
    b.ellipse(14, 12, 4, 2.2f, shine);
    for (int i = 0; i < 6; i++) {
        float a = i * 1.0472f;
        b.ellipse(18 + std::cos(a) * 14.f, 16 + std::sin(a) * 10.f, 1.4f, 1.2f, lug);
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
    setPal(vdp, PAL_CYM, {0, gs::rgb4(7, 7, 6), gs::rgb4(12, 12, 9), gs::rgb4(15, 15, 12), gs::rgb4(4, 4, 3)});
    setPal(vdp, PAL_STICK, {0, gs::rgb4(8, 5, 2), gs::rgb4(14, 11, 6), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_STAGE, {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 4, 6), gs::rgb4(8, 3, 2)});

    {
        gs::Bitmap b(40, 18);
        b.rect(2, 2, 36, 14, 1);
        b.rect(4, 4, 32, 10, 2);
        b.rect(6, 6, 8, 6, 3);
        art.shell = gs::uploadImage(vdp, b);
    }
    art.headWood = drumHead(vdp, 1, 2, 3, 4);
    {
        gs::Bitmap b(36, 28);
        b.ellipse(18, 16, 16, 12, 1);
        b.ellipse(18, 15, 13, 9, 2);
        b.ellipse(14, 12, 4, 2.2f, 3);
        art.headGold = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 28);
        b.ellipse(18, 16, 16, 12, 1);
        b.ellipse(18, 15, 13, 9, 2);
        b.ellipse(14, 12, 4, 2.2f, 3);
        art.headCream = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(70, 48);
        b.ellipse(35, 26, 32, 18, 1);
        b.ellipse(35, 24, 26, 13, 2);
        b.ellipse(28, 20, 8, 4, 3);
        b.ellipse(35, 26, 6, 4, 4);
        art.bass = gs::uploadImage(vdp, b);
    }
    auto cym = [&](gs::Image& img) {
        gs::Bitmap b(44, 18);
        b.ellipse(22, 10, 20, 7, 1);
        b.ellipse(22, 9, 16, 4.5f, 2);
        b.ellipse(18, 8, 4, 1.4f, 3);
        b.ellipse(22, 9, 1.6f, 1.2f, 4);
        img = gs::uploadImage(vdp, b);
    };
    cym(art.cymWood);
    cym(art.cymGold);
    cym(art.cymCream);
    {
        gs::Bitmap b(28, 8);
        b.poly({{1, 6}, {6, 4}, {26, 1}, {24, 3}, {5, 6}}, 1);
        b.line(4, 5, 25, 2, 2, 1.2f);
        art.stick = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 40);
        b.rect(3, 0, 2, 36, 1);
        b.rect(1, 34, 6, 4, 2);
        art.stand = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace drumgold
