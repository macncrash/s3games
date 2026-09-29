#include "game/art.h"

namespace cliffgrass {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

gs::Bitmap carArt() {
    gs::Bitmap b(44, 32);
    b.rect(4, 14, 36, 10, 1);
    b.rect(10, 6, 22, 10, 2);
    b.rect(12, 8, 8, 5, 3);
    b.rect(22, 8, 8, 5, 3);
    b.rect(2, 16, 4, 6, 4);
    b.rect(38, 16, 4, 6, 4);
    b.rect(8, 22, 28, 3, 5);
    b.ellipse(12, 26, 5, 5, 6);
    b.ellipse(32, 26, 5, 5, 6);
    b.ellipse(12, 26, 2, 2, 7);
    b.ellipse(32, 26, 2, 2, 7);
    b.rect(18, 4, 3, 6, 8);
    return b;
}

gs::Bitmap tuftArt() {
    gs::Bitmap b(16, 20);
    b.poly({{2, 20}, {4, 6}, {7, 20}}, 1);
    b.poly({{6, 20}, {9, 2}, {12, 20}}, 2);
    b.poly({{10, 20}, {13, 8}, {16, 20}}, 3);
    return b;
}

gs::Bitmap rockArt() {
    gs::Bitmap b(26, 36);
    b.poly({{3, 36}, {0, 20}, {7, 8}, {13, 1}, {20, 12}, {26, 24}, {18, 36}}, 1);
    b.poly({{8, 36}, {6, 22}, {12, 14}, {15, 22}, {13, 36}}, 2);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(22, 28);
    b.rect(2, 4, 2, 24, 1);
    b.poly({{4, 4}, {20, 8}, {4, 14}}, 2);
    b.rect(4, 6, 10, 4, 3);
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
    const uint16_t hud[16] = {0, ink, gs::rgb4(8, 13, 15), gs::rgb4(15, 13, 4), gs::rgb4(15, 5, 3),
                              gs::rgb4(4, 14, 6), gs::rgb4(8, 10, 6), 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t car[16] = {0,
                              gs::rgb4(12, 5, 3),
                              gs::rgb4(3, 5, 7),
                              gs::rgb4(10, 14, 15),
                              gs::rgb4(15, 12, 3),
                              gs::rgb4(4, 10, 4),
                              gs::rgb4(2, 2, 2),
                              gs::rgb4(10, 10, 9),
                              gs::rgb4(14, 14, 12),
                              0, 0, 0, 0, 0, 0, shadow};
    const uint16_t tuft[16] = {0, gs::rgb4(2, 8, 2), gs::rgb4(5, 12, 3), gs::rgb4(8, 14, 4), 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t rock[16] = {0, gs::rgb4(7, 6, 5), gs::rgb4(4, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t flag[16] = {0, gs::rgb4(12, 11, 8), gs::rgb4(14, 12, 3), gs::rgb4(6, 12, 4), 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, shadow};
    uint16_t rockRoad[16] = {};
    rockRoad[1] = gs::rgb4(5, 5, 4);
    rockRoad[2] = gs::rgb4(3, 3, 3);
    rockRoad[3] = gs::rgb4(8, 7, 6);
    rockRoad[4] = gs::rgb4(6, 5, 4);
    rockRoad[5] = gs::rgb4(4, 4, 3);
    rockRoad[6] = gs::rgb4(8, 7, 6);
    rockRoad[7] = gs::rgb4(6, 5, 4);
    rockRoad[8] = gs::rgb4(9, 8, 7);
    rockRoad[9] = gs::rgb4(5, 5, 4);
    rockRoad[10] = gs::rgb4(7, 6, 5);
    rockRoad[14] = gs::rgb4(12, 10, 6);
    rockRoad[15] = gs::rgb4(10, 9, 8);
    uint16_t grass[16] = {};
    grass[1] = gs::rgb4(3, 9, 3);
    grass[2] = gs::rgb4(2, 6, 2);
    grass[3] = gs::rgb4(6, 12, 4);
    grass[4] = gs::rgb4(4, 10, 3);
    grass[5] = gs::rgb4(2, 7, 2);
    grass[6] = gs::rgb4(4, 11, 3);
    grass[7] = gs::rgb4(3, 8, 2);
    grass[8] = gs::rgb4(7, 13, 5);
    grass[9] = gs::rgb4(2, 7, 2);
    grass[10] = gs::rgb4(5, 12, 4);
    grass[14] = gs::rgb4(14, 13, 4);
    grass[15] = gs::rgb4(8, 13, 5);
    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_CAR, car);
    setPal(vdp, PAL_TUFT, tuft);
    setPal(vdp, PAL_ROCK, rock);
    setPal(vdp, PAL_FLAG, flag);
    setPal(vdp, PAL_ROCKROAD, rockRoad);
    setPal(vdp, PAL_GRASS, grass);
    loadFont(vdp, art);
    art.car = gs::uploadMipped(vdp, carArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
}

}  // namespace cliffgrass
