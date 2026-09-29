#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace clifflane {
namespace {

void setPal(gs::VDP& v, int pal, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) {
        if (i < 16) v.setColor(pal * 16 + i, c);
        i++;
    }
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp, 1);
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
        art.font[c - 32] = t;
    }
}

gs::Bitmap cartArt() {
    gs::Bitmap b(96, 56);
    b.rect(8, 22, 72, 18, 1);
    b.rect(12, 16, 40, 10, 2);
    b.rect(18, 10, 16, 10, 3);
    b.rect(22, 12, 6, 4, 4);
    b.rect(58, 18, 18, 8, 5);
    b.rect(4, 36, 10, 14, 6);
    b.rect(74, 36, 10, 14, 6);
    b.rect(6, 40, 6, 6, 7);
    b.rect(76, 40, 6, 6, 7);
    b.line(10, 22, 78, 22, 8, 1);
    b.rect(46, 26, 8, 6, 9);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(14, 14, 7, 7, 2);
    b.ellipse(14, 14, 2, 2, 3);
    b.line(14, 3, 14, 25, 4, 1);
    b.line(3, 14, 25, 14, 4, 1);
    return b;
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(10, 48);
    b.rect(4, 6, 2, 40, 1);
    b.rect(2, 2, 6, 6, 2);
    b.rect(3, 40, 4, 6, 3);
    return b;
}

gs::Bitmap dashArt() {
    gs::Bitmap b(36, 8);
    b.rect(0, 2, 36, 4, 1);
    b.rect(2, 3, 32, 2, 2);
    return b;
}

gs::Bitmap cragArt() {
    gs::Bitmap b(48, 80);
    b.poly({{6, 78}, {16, 28}, {24, 40}, {36, 6}, {46, 78}}, 1);
    b.poly({{12, 78}, {20, 40}, {28, 50}, {38, 18}, {42, 78}}, 2);
    b.rect(14, 58, 8, 6, 3);
    return b;
}

gs::Bitmap scrubArt() {
    gs::Bitmap b(32, 24);
    b.ellipse(10, 14, 8, 6, 1);
    b.ellipse(20, 10, 9, 7, 2);
    b.rect(14, 16, 3, 6, 3);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 72);
    b.rect(4, 4, 4, 66, 1);
    b.rect(2, 0, 8, 6, 2);
    b.rect(3, 64, 6, 8, 3);
    return b;
}

gs::Bitmap ribbonArt() {
    gs::Bitmap b(80, 14);
    b.rect(0, 3, 80, 8, 1);
    b.rect(0, 5, 80, 3, 2);
    for (int x = 4; x < 76; x += 12) b.rect(float(x), 3, 4, 8, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    for (int p = 0; p < gs::NUM_PALETTES; p++)
        for (int i = 0; i < 16; i++) vdp.setColor(p * 16 + i, 0);

    auto ink = [&](int pal, uint16_t c) {
        vdp.setColor(pal * 16 + 1, c);
        vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
    };
    ink(PAL_HUD, gs::rgb4(14, 14, 13));
    ink(PAL_AMBER, gs::rgb4(15, 12, 3));
    ink(PAL_BAD, gs::rgb4(15, 4, 3));
    ink(PAL_GOOD, gs::rgb4(6, 14, 7));
    loadFont(vdp, art);

    setPal(vdp, PAL_CART,
           {0, gs::rgb4(9, 6, 3), gs::rgb4(12, 9, 5), gs::rgb4(6, 8, 11), gs::rgb4(14, 12, 8),
            gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 3), gs::rgb4(8, 8, 7), gs::rgb4(13, 11, 6), gs::rgb4(7, 5, 3)});
    setPal(vdp, PAL_STAKE, {0, gs::rgb4(12, 11, 8), gs::rgb4(14, 6, 2), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_PAINT, {0, gs::rgb4(14, 12, 4), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_ROCK,
           {0, gs::rgb4(7, 6, 5), gs::rgb4(5, 4, 4), gs::rgb4(9, 8, 6), gs::rgb4(3, 5, 3)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(13, 13, 11), gs::rgb4(14, 4, 3), gs::rgb4(15, 12, 3), gs::rgb4(4, 4, 4)});

    auto roadPal = [&](int pal) {
        for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, gs::rgb4(5, 4, 3));
        vdp.setColor(pal * 16 + 1, gs::rgb4(6, 5, 3));
        vdp.setColor(pal * 16 + 2, gs::rgb4(4, 3, 2));
        vdp.setColor(pal * 16 + 3, gs::rgb4(8, 7, 5));
        vdp.setColor(pal * 16 + 4, gs::rgb4(7, 6, 3));
        vdp.setColor(pal * 16 + 8, gs::rgb4(8, 7, 6));
        vdp.setColor(pal * 16 + 11, gs::rgb4(2, 6, 8));
        vdp.setColor(pal * 16 + 12, gs::rgb4(1, 4, 7));
        vdp.setColor(pal * 16 + 14, gs::rgb4(12, 10, 4));
    };
    roadPal(PAL_ROAD);
    vdp.setFogColor(gs::rgb4(8, 7, 6));

    art.cart = gs::uploadMipped(vdp, cartArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.dash = gs::uploadMipped(vdp, dashArt());
    art.crag = gs::uploadMipped(vdp, cragArt());
    art.scrub = gs::uploadMipped(vdp, scrubArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.ribbon = gs::uploadMipped(vdp, ribbonArt());
}

}  // namespace clifflane
