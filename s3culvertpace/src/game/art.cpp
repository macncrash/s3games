#include "game/art.h"

#include <cstdint>

namespace culvertpace {
namespace {

void putPal(gs::VDP& v, int pal, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) v.setColor(pal * 16 + i, i < n ? c[i] : 0);
}

void loadFont(gs::VDP& v, Art& art) {
    gs::TileAlloc tiles(v, 1);
    for (int ch = 0; ch < 96; ch++) {
        const uint8_t* g = gs::glyph(char(32 + ch));
        uint8_t px[64] = {};
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (g[y * 5 + x]) px[y * 8 + (x + 1)] = 1;
            }
        }
        art.font[ch] = tiles.shared(px);
    }
}

gs::Bitmap ringBmp() {
    gs::Bitmap b(64, 48);
    for (int y = 0; y < 48; y++) {
        for (int x = 0; x < 64; x++) {
            float nx = (float(x) - 31.5f) / 28.f;
            float ny = (float(y) - 23.5f) / 19.5f;
            float r2 = nx * nx + ny * ny;
            if (r2 > 1.f || r2 < 0.62f) continue;
            int rib = ((x / 3) + (y / 5)) & 1;
            int c = rib ? 2 : 1;
            if (((x * 13 + y * 7) % 29) == 0) c = 3;
            if (r2 > 0.90f) c = 4;
            b.set(x, y, c);
        }
    }
    // A single stamped numeral, so the pipe reads as one culvert.
    for (int y = 18; y < 30; y++) b.set(30, y, 5);
    for (int x = 27; x < 34; x++) b.set(x, 18, 5);
    return b;
}

gs::Bitmap walkerBmp(bool down) {
    gs::Bitmap b(24, 40);
    auto coat = [&](int x, int y, int c) { b.set(x, y, c); };
    if (!down) {
        b.ellipse(12, 6, 4.2f, 4.6f, 3);
        b.rect(10, 10, 4, 3, 3);
        b.rect(7, 13, 10, 14, 1);
        b.rect(6, 15, 2, 8, 2);
        b.rect(16, 15, 2, 8, 2);
        b.rect(9, 26, 3, 10, 4);
        b.rect(13, 26, 3, 10, 4);
        b.rect(8, 35, 4, 3, 5);
        b.rect(13, 35, 4, 3, 5);
        b.rect(17, 18, 5, 2, 6);  // lamp
        coat(19, 17, 7);
    } else {
        b.ellipse(8, 22, 4.f, 4.2f, 3);
        b.rect(11, 18, 10, 8, 1);
        b.rect(12, 26, 8, 4, 4);
        b.rect(18, 28, 4, 3, 5);
        b.rect(6, 16, 3, 2, 6);
    }
    return b;
}

gs::Bitmap dripBmp() {
    gs::Bitmap b(6, 12);
    b.ellipse(3, 3, 2.2f, 2.4f, 1);
    for (int y = 5; y < 11; y++) b.set(3, y, 2);
    b.set(3, 11, 1);
    return b;
}

gs::Bitmap flashBmp() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 3.f, 3.f, 1);
    b.line(8, 1, 8, 14, 2, 1);
    b.line(1, 8, 14, 8, 2, 1);
    b.line(3, 3, 12, 12, 3, 1);
    b.line(12, 3, 3, 12, 3, 1);
    return b;
}

gs::Bitmap sightBmp() {
    gs::Bitmap b(16, 16);
    for (int i = 1; i < 6; i++) {
        b.set(8, i, 1);
        b.set(8, 15 - i, 1);
        b.set(i, 8, 1);
        b.set(15 - i, 8, 1);
    }
    b.set(8, 8, 2);
    b.rect(2, 2, 3, 1, 1);
    b.rect(2, 2, 1, 3, 1);
    b.rect(11, 2, 3, 1, 1);
    b.rect(13, 2, 1, 3, 1);
    b.rect(2, 13, 3, 1, 1);
    b.rect(2, 11, 1, 3, 1);
    b.rect(11, 13, 3, 1, 1);
    b.rect(13, 11, 1, 3, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t text[] = {0, gs::rgb4(14, 13, 9), gs::rgb4(6, 8, 7)};
    const uint16_t pipe[] = {0, gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 5), gs::rgb4(3, 4, 3), gs::rgb4(10, 10, 9),
                             gs::rgb4(12, 11, 6)};
    const uint16_t moss[] = {0, gs::rgb4(3, 6, 3), gs::rgb4(5, 8, 4)};
    const uint16_t water[] = {0, gs::rgb4(6, 10, 12), gs::rgb4(3, 7, 9)};
    const uint16_t fig[] = {0, gs::rgb4(4, 5, 4), gs::rgb4(3, 3, 3), gs::rgb4(11, 8, 6), gs::rgb4(2, 2, 3),
                            gs::rgb4(2, 2, 2), gs::rgb4(12, 10, 4), gs::rgb4(15, 14, 8)};
    const uint16_t alert[] = {0, gs::rgb4(15, 6, 4), gs::rgb4(8, 2, 2)};
    const uint16_t good[] = {0, gs::rgb4(8, 14, 7), gs::rgb4(3, 7, 3)};
    const uint16_t flash[] = {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 12, 4), gs::rgb4(12, 8, 3)};
    const uint16_t sight[] = {0, gs::rgb4(14, 13, 8), gs::rgb4(15, 4, 3)};
    putPal(vdp, PAL_TEXT, text, 3);
    putPal(vdp, PAL_PIPE, pipe, 6);
    putPal(vdp, PAL_MOSS, moss, 3);
    putPal(vdp, PAL_WATER, water, 3);
    putPal(vdp, PAL_FIGURE, fig, 8);
    putPal(vdp, PAL_ALERT, alert, 3);
    putPal(vdp, PAL_GOOD, good, 3);
    putPal(vdp, PAL_FLASH, flash, 4);
    putPal(vdp, PAL_SIGHT, sight, 3);

    uint16_t road[16] = {};
    road[1] = gs::rgb4(6, 6, 5);
    road[2] = gs::rgb4(4, 4, 4);
    road[3] = gs::rgb4(7, 7, 6);
    road[4] = gs::rgb4(6, 6, 5);
    road[5] = gs::rgb4(4, 5, 4);
    road[6] = gs::rgb4(5, 5, 4);
    road[7] = gs::rgb4(3, 3, 3);
    road[8] = gs::rgb4(8, 8, 7);
    road[11] = gs::rgb4(3, 6, 8);
    road[12] = gs::rgb4(2, 4, 6);
    road[13] = gs::rgb4(7, 11, 12);
    road[14] = gs::rgb4(9, 9, 7);
    road[15] = gs::rgb4(5, 6, 5);
    putPal(vdp, PAL_ROAD, road, 16);

    vdp.setFogColor(gs::rgb4(1, 2, 3));
    art.ring = gs::uploadMipped(vdp, ringBmp());
    art.walker = gs::uploadMipped(vdp, walkerBmp(false));
    art.fallen = gs::uploadMipped(vdp, walkerBmp(true));
    art.drip = gs::uploadMipped(vdp, dripBmp());
    art.flash = gs::uploadMipped(vdp, flashBmp());
    art.sight = gs::uploadMipped(vdp, sightBmp());
    loadFont(vdp, art);
}

}  // namespace culvertpace
