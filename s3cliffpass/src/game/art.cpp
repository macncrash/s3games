#include "game/art.h"

namespace cliffpass {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

gs::Bitmap carArt() {
    gs::Bitmap b(36, 32);
    b.poly({{6, 30}, {4, 16}, {10, 8}, {26, 8}, {32, 16}, {30, 30}}, 1);
    b.poly({{12, 10}, {14, 4}, {22, 4}, {24, 10}}, 2);
    b.rect(13, 5, 4, 3, 3);
    b.rect(19, 5, 4, 3, 3);
    b.rect(2, 16, 4, 8, 4);
    b.rect(30, 16, 4, 8, 4);
    b.rect(8, 22, 6, 4, 5);
    b.rect(22, 22, 6, 4, 5);
    b.rect(16, 14, 4, 10, 6);
    b.ellipse(11, 28, 4, 3, 7);
    b.ellipse(25, 28, 4, 3, 7);
    return b;
}

gs::Bitmap crewArt() {
    gs::Bitmap b(36, 32);
    b.poly({{6, 30}, {5, 15}, {11, 7}, {25, 7}, {31, 15}, {30, 30}}, 1);
    b.rect(12, 3, 12, 6, 2);
    b.rect(13, 4, 4, 3, 3);
    b.rect(19, 4, 4, 3, 3);
    b.rect(3, 14, 5, 7, 4);
    b.rect(28, 14, 5, 7, 4);
    b.rect(15, 12, 6, 8, 5);
    b.ellipse(11, 27, 4, 3, 6);
    b.ellipse(25, 27, 4, 3, 6);
    b.rect(16, 18, 4, 3, 7);
    return b;
}

gs::Bitmap rockArt() {
    gs::Bitmap b(24, 36);
    b.poly({{3, 36}, {0, 20}, {7, 8}, {12, 1}, {18, 10}, {24, 22}, {18, 36}}, 1);
    b.poly({{8, 36}, {6, 22}, {11, 12}, {15, 20}, {13, 36}}, 2);
    b.rect(10, 6, 3, 3, 3);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(18, 40);
    b.rect(6, 8, 6, 32, 1);
    b.rect(2, 0, 14, 10, 2);
    b.rect(4, 2, 10, 5, 3);
    b.rect(7, 18, 4, 4, 4);
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
    const uint16_t hud[16] = {0, ink, gs::rgb4(8, 13, 15), gs::rgb4(15, 12, 3), gs::rgb4(15, 5, 2),
                              gs::rgb4(5, 14, 8), gs::rgb4(11, 9, 7), 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t car[16] = {0,
                              gs::rgb4(14, 11, 3),
                              gs::rgb4(3, 5, 8),
                              gs::rgb4(10, 14, 15),
                              gs::rgb4(12, 3, 2),
                              gs::rgb4(2, 2, 2),
                              gs::rgb4(15, 15, 12),
                              gs::rgb4(4, 4, 5),
                              0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t crew[16] = {0,
                               gs::rgb4(6, 7, 9),
                               gs::rgb4(2, 3, 5),
                               gs::rgb4(15, 14, 6),
                               gs::rgb4(13, 4, 2),
                               gs::rgb4(12, 12, 13),
                               gs::rgb4(1, 1, 2),
                               gs::rgb4(15, 6, 2),
                               0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t rock[16] = {0, gs::rgb4(6, 5, 4), gs::rgb4(9, 8, 6), gs::rgb4(12, 11, 8), 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t gate[16] = {0, gs::rgb4(8, 8, 7), gs::rgb4(12, 3, 2), gs::rgb4(15, 14, 10), gs::rgb4(15, 12, 2),
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    uint16_t road[16] = {};
    road[1] = gs::rgb4(5, 5, 4);
    road[2] = gs::rgb4(3, 3, 3);
    road[3] = gs::rgb4(8, 7, 5);
    road[4] = gs::rgb4(7, 6, 4);
    road[5] = gs::rgb4(6, 5, 3);
    road[6] = gs::rgb4(9, 8, 7);
    road[7] = gs::rgb4(5, 5, 5);
    road[8] = gs::rgb4(11, 10, 8);
    road[9] = gs::rgb4(4, 4, 4);
    road[10] = gs::rgb4(12, 11, 9);
    road[11] = gs::rgb4(2, 4, 7);
    road[12] = gs::rgb4(1, 2, 5);
    road[13] = gs::rgb4(4, 7, 10);
    road[14] = gs::rgb4(14, 10, 2);
    road[15] = gs::rgb4(10, 9, 7);
    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_CAR, car);
    setPal(vdp, PAL_CREW, crew);
    setPal(vdp, PAL_ROCK, rock);
    setPal(vdp, PAL_GATE, gate);
    setPal(vdp, PAL_ROAD, road);
    loadFont(vdp, art);
    art.car = gs::uploadMipped(vdp, carArt());
    art.crew = gs::uploadMipped(vdp, crewArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
}

}  // namespace cliffpass
