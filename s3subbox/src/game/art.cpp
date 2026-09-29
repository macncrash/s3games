#include "game/art.h"

namespace subbox {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

gs::Bitmap subArt() {
    gs::Bitmap b(72, 32);
    b.ellipse(34, 18, 28, 8, 1);
    b.ellipse(34, 17, 22, 5, 2);
    b.poly({{30, 10}, {42, 10}, {40, 4}, {32, 4}}, 3);
    b.rect(34, 2, 4, 4, 3);
    b.rect(33, 1, 6, 2, 4);
    b.ellipse(46, 16, 2.2f, 2.2f, 5);
    b.ellipse(38, 16, 1.6f, 1.6f, 6);
    b.ellipse(30, 16, 1.6f, 1.6f, 6);
    b.rect(6, 16, 6, 2, 7);
    b.line(4, 12, 10, 16, 7, 1.4f);
    b.line(4, 22, 10, 18, 7, 1.4f);
    b.rect(58, 14, 8, 2, 8);
    b.rect(64, 12, 2, 6, 7);
    return b;
}

// Open on the left. The right wall is the end of the leg.
gs::Bitmap boxArt() {
    gs::Bitmap b(80, 48);
    b.rect(0, 0, 80, 4, 1);
    b.rect(0, 44, 80, 4, 1);
    b.rect(74, 0, 6, 48, 2);
    b.rect(4, 4, 70, 3, 3);
    b.rect(4, 41, 70, 3, 3);
    for (int i = 0; i < 5; i++) b.rect(70, 6 + i * 8, 4, 3, 4);
    b.rect(8, 20, 10, 2, 5);
    return b;
}

gs::Bitmap rockArt() {
    gs::Bitmap b(28, 18);
    b.poly({{2, 16}, {8, 6}, {16, 3}, {24, 10}, {22, 17}, {4, 17}}, 1);
    b.poly({{8, 14}, {12, 8}, {18, 7}, {20, 14}}, 2);
    return b;
}

gs::Bitmap kelpArt() {
    gs::Bitmap b(12, 36);
    b.line(6, 34, 4, 18, 1, 2.f);
    b.line(4, 18, 8, 4, 1, 2.f);
    b.ellipse(8, 4, 3, 2, 2);
    b.ellipse(3, 16, 2.4f, 1.6f, 2);
    return b;
}

gs::Bitmap bubbleArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 3.2f, 3.2f, 1);
    b.set(4, 3, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 5, 5, 1);
    b.ellipse(7, 7, 2.2f, 2.2f, 2);
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
    uint8_t water[64] = {};
    for (int i = 0; i < 64; i++) water[i] = ((i / 8 + i) & 3) == 0 ? 2 : 1;
    a.waterTile = tiles.alloc(1);
    vdp.loadTile(a.waterTile, water);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(0, 1, 2);
    const uint16_t ink = gs::rgb4(14, 15, 15);
    const uint16_t hud[16] = {0, ink, gs::rgb4(6, 14, 15), gs::rgb4(15, 13, 4), gs::rgb4(15, 5, 3),
                              gs::rgb4(4, 14, 9), gs::rgb4(8, 11, 14), 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t sub[16] = {0, gs::rgb4(3, 8, 7), gs::rgb4(6, 12, 10), gs::rgb4(4, 6, 7), gs::rgb4(12, 14, 8),
                              gs::rgb4(15, 14, 6), gs::rgb4(10, 14, 15), gs::rgb4(2, 4, 5), gs::rgb4(9, 10, 8),
                              0, 0, 0, 0, 0, 0, shadow};
    const uint16_t box[16] = {0, gs::rgb4(15, 12, 3), gs::rgb4(13, 6, 2), gs::rgb4(8, 10, 6), gs::rgb4(15, 15, 8),
                              gs::rgb4(4, 12, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t rock[16] = {0, gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t bub[16] = {0, gs::rgb4(8, 13, 15), gs::rgb4(14, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t kelp[16] = {0, gs::rgb4(2, 8, 4), gs::rgb4(5, 12, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t water[16] = {0, gs::rgb4(1, 4, 8), gs::rgb4(2, 7, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_SUB, sub);
    setPal(vdp, PAL_BOX, box);
    setPal(vdp, PAL_ROCK, rock);
    setPal(vdp, PAL_BUB, bub);
    setPal(vdp, PAL_KELP, kelp);
    setPal(vdp, PAL_WATER, water);
    loadFont(vdp, art);
    art.sub = gs::uploadMipped(vdp, subArt());
    art.box = gs::uploadMipped(vdp, boxArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.kelp = gs::uploadMipped(vdp, kelpArt());
    art.bubble = gs::uploadMipped(vdp, bubbleArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
}

}  // namespace subbox
