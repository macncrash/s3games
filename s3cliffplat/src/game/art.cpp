#include "game/art.h"

namespace cliffplat {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

gs::Bitmap cageArt() {
    gs::Bitmap b(44, 36);
    b.rect(6, 8, 32, 22, 1);
    b.poly({{4, 10}, {22, 2}, {40, 10}}, 2);
    b.rect(10, 12, 8, 8, 3);
    b.rect(26, 12, 8, 8, 3);
    b.rect(8, 22, 28, 3, 4);
    b.rect(4, 28, 6, 6, 5);
    b.rect(34, 28, 6, 6, 5);
    b.rect(18, 16, 8, 10, 6);
    b.line(22, 2, 22, 0, 7, 1);
    b.ellipse(8, 32, 4, 3, 8);
    b.ellipse(36, 32, 4, 3, 8);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(44, 36);
    b.rect(6, 8, 32, 22, 1);
    b.poly({{4, 10}, {22, 2}, {40, 10}}, 2);
    b.rect(10, 12, 8, 8, 3);
    b.rect(26, 12, 8, 8, 3);
    b.rect(8, 22, 28, 3, 4);
    b.rect(20, 14, 4, 12, 5);
    b.ellipse(8, 32, 4, 3, 6);
    b.ellipse(36, 32, 4, 3, 6);
    return b;
}

gs::Bitmap rockArt() {
    gs::Bitmap b(28, 48);
    b.poly({{2, 48}, {0, 28}, {6, 14}, {14, 2}, {22, 16}, {28, 30}, {24, 48}}, 1);
    b.poly({{8, 48}, {7, 30}, {13, 18}, {18, 32}, {16, 48}}, 2);
    b.rect(12, 8, 4, 3, 3);
    b.line(4, 36, 20, 40, 4, 1);
    return b;
}

gs::Bitmap platArt() {
    gs::Bitmap b(72, 28);
    b.rect(0, 6, 72, 10, 1);
    b.rect(0, 4, 72, 3, 2);
    b.rect(8, 16, 6, 12, 3);
    b.rect(32, 16, 6, 12, 3);
    b.rect(56, 16, 6, 12, 3);
    b.rect(4, 8, 10, 3, 4);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(10, 22);
    b.rect(3, 0, 4, 22, 1);
    b.rect(1, 2, 8, 5, 2);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(18, 8);
    b.poly({{0, 4}, {8, 2}, {9, 5}, {17, 3}, {9, 6}}, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t ink = gs::rgb4(15, 15, 13);
    const uint16_t hud[16] = {0, ink, gs::rgb4(8, 14, 15), gs::rgb4(15, 12, 4), gs::rgb4(15, 6, 3),
                              gs::rgb4(6, 14, 8), gs::rgb4(12, 10, 7), 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t cage[16] = {0,
                               gs::rgb4(13, 10, 3),
                               gs::rgb4(10, 6, 2),
                               gs::rgb4(8, 13, 15),
                               gs::rgb4(4, 3, 2),
                               gs::rgb4(6, 5, 4),
                               gs::rgb4(15, 14, 8),
                               gs::rgb4(14, 14, 12),
                               gs::rgb4(2, 2, 2),
                               0, 0, 0, 0, 0, 0, shadow};
    const uint16_t rival[16] = {0,
                                gs::rgb4(6, 7, 8),
                                gs::rgb4(4, 4, 6),
                                gs::rgb4(10, 12, 13),
                                gs::rgb4(12, 4, 3),
                                gs::rgb4(14, 12, 6),
                                gs::rgb4(2, 2, 3),
                                0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t rock[16] = {0, gs::rgb4(7, 6, 5), gs::rgb4(10, 8, 6), gs::rgb4(13, 11, 8),
                               gs::rgb4(4, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t plat[16] = {0, gs::rgb4(8, 8, 7), gs::rgb4(12, 11, 8), gs::rgb4(5, 5, 4),
                               gs::rgb4(14, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t sea[16] = {0, gs::rgb4(15, 15, 14), gs::rgb4(14, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t good[16] = {0, ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t bad[16] = {0, gs::rgb4(15, 8, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_CAGE, cage);
    setPal(vdp, PAL_RIVAL, rival);
    setPal(vdp, PAL_ROCK, rock);
    setPal(vdp, PAL_PLAT, plat);
    setPal(vdp, PAL_SEA, sea);
    setPal(vdp, PAL_GOOD, good);
    setPal(vdp, PAL_BAD, bad);
    art.cage = gs::uploadMipped(vdp, cageArt());
    art.rival = gs::uploadMipped(vdp, rivalArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.plat = gs::uploadMipped(vdp, platArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    loadFont(vdp, art);
}

}  // namespace cliffplat
