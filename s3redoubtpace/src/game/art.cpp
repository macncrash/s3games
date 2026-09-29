#include "game/art.h"

#include <initializer_list>

namespace redoubtpace {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

Bitmap raiderArt(bool step) {
    Bitmap b(20, 42);
    b.ellipse(10, 7, 5, 5, 1);
    b.rect(6, 2, 8, 3, 5);
    b.rect(8, 11, 4, 3, 1);
    b.rect(5, 14, 11, 12, 2);
    b.rect(3, 16, 3, 8, 3);
    b.rect(15, 16, 3, 7, 3);
    b.rect(16, 18, 3, 2, 6);
    if (step) {
        b.rect(6, 26, 3, 12, 4);
        b.rect(11, 26, 3, 9, 4);
        b.rect(5, 37, 4, 3, 6);
        b.rect(11, 34, 4, 3, 6);
    } else {
        b.rect(6, 26, 3, 9, 4);
        b.rect(11, 26, 3, 12, 4);
        b.rect(5, 34, 4, 3, 6);
        b.rect(11, 37, 4, 3, 6);
    }
    b.rect(9, 15, 2, 8, 6);
    b.set(8, 7, 7);
    b.set(12, 7, 7);
    return b;
}

Bitmap fallenArt() {
    Bitmap b(44, 16);
    b.ellipse(8, 7, 5, 4, 1);
    b.rect(12, 5, 18, 6, 2);
    b.rect(28, 6, 10, 4, 4);
    b.rect(14, 10, 8, 3, 3);
    b.rect(30, 4, 8, 2, 6);
    b.set(7, 6, 7);
    return b;
}

Bitmap gabionArt() {
    Bitmap b(28, 36);
    b.ellipse(14, 18, 12, 14, 2);
    b.ellipse(14, 18, 8, 10, 3);
    for (int y = 6; y < 32; y += 4) b.rect(3, y, 22, 1, 1);
    b.rect(12, 4, 3, 28, 4);
    b.ellipse(14, 8, 6, 3, 5);
    return b;
}

Bitmap parapetArt() {
    Bitmap b(40, 18);
    b.poly({{0, 17}, {4, 4}, {36, 4}, {40, 17}}, 2);
    b.rect(2, 6, 36, 3, 3);
    b.rect(6, 10, 8, 5, 1);
    b.rect(22, 10, 10, 5, 4);
    b.rect(0, 15, 40, 3, 5);
    return b;
}

Bitmap stakeArt() {
    Bitmap b(8, 28);
    b.rect(3, 2, 2, 22, 1);
    b.poly({{1, 2}, {4, 0}, {7, 2}}, 2);
    b.rect(1, 22, 6, 4, 3);
    return b;
}

Bitmap flagArt() {
    Bitmap b(22, 32);
    b.rect(2, 2, 2, 28, 4);
    b.rect(4, 3, 16, 10, 1);
    b.rect(4, 13, 12, 4, 2);
    b.rect(8, 5, 4, 6, 3);
    return b;
}

Bitmap beadArt() {
    Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(7, 7, 3, 3, 0);
    b.rect(6, 1, 2, 3, 2);
    b.rect(6, 10, 2, 3, 2);
    b.rect(1, 6, 3, 2, 2);
    b.rect(10, 6, 3, 2, 2);
    return b;
}

Bitmap flashArt() {
    Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 1);
    b.ellipse(9, 9, 4, 4, 2);
    b.rect(8, 1, 2, 16, 3);
    b.rect(1, 8, 16, 2, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 0)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 14, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 2, 0)});
    setPal(vdp, PAL_EARTH,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(8, 6, 3), gs::rgb4(5, 5, 3), gs::rgb4(10, 8, 4), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_FIGURE,
           {0, gs::rgb4(12, 8, 6), gs::rgb4(4, 5, 7), gs::rgb4(7, 7, 8), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 2),
            gs::rgb4(9, 8, 6), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(9, 6, 2), gs::rgb4(12, 9, 3), gs::rgb4(5, 3, 1), gs::rgb4(7, 5, 2)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 10, 2), gs::rgb4(15, 15, 12), 0});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(12, 2, 2), gs::rgb4(14, 12, 4), gs::rgb4(2, 2, 6), gs::rgb4(6, 4, 2)});
    loadFont(vdp, art);
    art.raider[0] = gs::uploadMipped(vdp, raiderArt(false));
    art.raider[1] = gs::uploadMipped(vdp, raiderArt(true));
    art.fallen = gs::uploadMipped(vdp, fallenArt());
    art.gabion = gs::uploadMipped(vdp, gabionArt());
    art.parapet = gs::uploadMipped(vdp, parapetArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
}

}  // namespace redoubtpace
