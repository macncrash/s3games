#include "game/art.h"

namespace cliffbox {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

gs::Bitmap carArt() {
    gs::Bitmap b(40, 36);
    b.rect(6, 8, 28, 20, 1);
    b.rect(8, 4, 24, 8, 2);
    b.rect(10, 6, 8, 4, 3);
    b.rect(22, 6, 8, 4, 3);
    b.rect(4, 14, 4, 8, 4);
    b.rect(32, 14, 4, 8, 4);
    b.rect(8, 24, 6, 4, 5);
    b.rect(26, 24, 6, 4, 5);
    b.rect(12, 16, 16, 6, 6);
    b.rect(18, 10, 4, 10, 7);
    b.ellipse(12, 28, 4, 4, 8);
    b.ellipse(28, 28, 4, 4, 8);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 28);
    b.rect(4, 4, 2, 24, 1);
    b.rect(2, 0, 6, 6, 2);
    b.rect(3, 6, 4, 3, 3);
    return b;
}

gs::Bitmap rockArt() {
    gs::Bitmap b(28, 40);
    b.poly({{4, 40}, {0, 22}, {8, 10}, {14, 2}, {22, 14}, {28, 28}, {20, 40}}, 1);
    b.poly({{8, 40}, {6, 24}, {12, 14}, {16, 18}, {14, 40}}, 2);
    b.rect(12, 8, 3, 3, 3);
    return b;
}

gs::Bitmap signArt() {
    gs::Bitmap b(36, 16);
    b.rect(0, 2, 36, 12, 1);
    b.rect(2, 4, 32, 8, 2);
    b.rect(6, 6, 4, 4, 3);
    b.rect(14, 6, 8, 4, 3);
    b.rect(26, 6, 4, 4, 3);
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
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t hud[16] = {0, ink, gs::rgb4(8, 12, 15), gs::rgb4(15, 12, 3), gs::rgb4(15, 5, 3),
                              gs::rgb4(4, 14, 7), gs::rgb4(10, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t car[16] = {0,
                              gs::rgb4(13, 12, 10),
                              gs::rgb4(4, 6, 8),
                              gs::rgb4(8, 12, 14),
                              gs::rgb4(15, 4, 2),
                              gs::rgb4(15, 13, 3),
                              gs::rgb4(6, 5, 4),
                              gs::rgb4(15, 15, 13),
                              gs::rgb4(2, 2, 2),
                              0, 0, 0, 0, 0, 0, shadow};
    const uint16_t post[16] = {0, gs::rgb4(14, 14, 12), gs::rgb4(15, 8, 2), gs::rgb4(15, 14, 4), 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t rock[16] = {0, gs::rgb4(7, 6, 5), gs::rgb4(4, 4, 4), gs::rgb4(10, 9, 7), 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t sign[16] = {0, gs::rgb4(12, 4, 2), gs::rgb4(15, 14, 12), gs::rgb4(2, 2, 2), 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, shadow};
    // Road palette: stone shelf, ochre verge, amber paint for the box stripes.
    uint16_t road[16] = {};
    road[1] = gs::rgb4(5, 6, 4);
    road[2] = gs::rgb4(3, 4, 3);
    road[3] = gs::rgb4(8, 7, 5);
    road[4] = gs::rgb4(9, 7, 4);
    road[5] = gs::rgb4(6, 5, 3);
    road[6] = gs::rgb4(8, 8, 7);
    road[7] = gs::rgb4(6, 6, 5);
    road[8] = gs::rgb4(10, 9, 7);
    road[9] = gs::rgb4(5, 5, 4);
    road[10] = gs::rgb4(12, 11, 8);
    road[11] = gs::rgb4(2, 5, 8);
    road[12] = gs::rgb4(1, 3, 6);
    road[13] = gs::rgb4(4, 8, 12);
    road[14] = gs::rgb4(15, 11, 2);
    road[15] = gs::rgb4(11, 10, 8);
    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_CAR, car);
    setPal(vdp, PAL_POST, post);
    setPal(vdp, PAL_ROCK, rock);
    setPal(vdp, PAL_SIGN, sign);
    setPal(vdp, PAL_ROAD, road);
    loadFont(vdp, art);
    art.car = gs::uploadMipped(vdp, carArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.sign = gs::uploadMipped(vdp, signArt());
}

}  // namespace cliffbox
