#include "game/art.h"

namespace craneturn {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

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

gs::Bitmap bodyArt() {
    gs::Bitmap b(96, 56);
    b.rect(8, 28, 70, 16, 4);          // chassis
    b.rect(46, 16, 22, 16, 5);         // cab
    b.rect(50, 20, 12, 7, 8);          // glass
    b.poly({{18, 28}, {30, 10}, {44, 10}, {48, 28}}, 6);  // turret
    b.rect(14, 40, 10, 8, 2);
    b.rect(62, 40, 10, 8, 2);
    b.rect(36, 42, 8, 6, 2);
    b.rect(10, 26, 6, 4, 7);           // lamp
    b.outline(1, false);
    return b;
}

gs::Bitmap boomArt() {
    gs::Bitmap b(88, 20);
    b.poly({{4, 14}, {78, 4}, {82, 8}, {8, 18}}, 5);
    b.line(10, 15, 76, 6, 9, 1);
    b.rect(74, 4, 8, 8, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap hookArt() {
    gs::Bitmap b(16, 24);
    b.line(8, 2, 8, 12, 9, 2);
    b.ellipse(8, 16, 5, 6, 3);
    b.rect(6, 12, 4, 3, 1);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 1);
    b.ellipse(9, 9, 4, 4, 7);
    return b;
}

gs::Bitmap pylonArt() {
    gs::Bitmap b(16, 40);
    b.poly({{8, 2}, {14, 36}, {2, 36}}, 6);
    b.rect(4, 34, 8, 4, 1);
    b.rect(6, 10, 4, 4, 9);
    return b;
}

gs::Bitmap crewArt() {
    gs::Bitmap b(64, 40);
    b.rect(6, 20, 48, 12, 10);
    b.rect(32, 10, 16, 12, 11);
    b.rect(34, 13, 9, 5, 8);
    b.poly({{12, 20}, {20, 8}, {30, 8}, {32, 20}}, 12);
    b.rect(10, 30, 8, 6, 1);
    b.rect(40, 30, 8, 6, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 28);
    b.rect(5, 8, 2, 18, 2);
    b.ellipse(6, 6, 4, 4, 9);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    auto ink = [&](int p) {
        pal(vdp, p, 1, 15, 15, 15);
        pal(vdp, p, 15, 2, 2, 3);
    };
    ink(PAL_HUD);
    pal(vdp, PAL_INK, 1, 15, 14, 8);
    pal(vdp, PAL_INK, 15, 3, 2, 1);
    pal(vdp, PAL_WARN, 1, 15, 5, 3);
    pal(vdp, PAL_WARN, 15, 4, 1, 1);

    pal(vdp, PAL_CRANE, 1, 2, 2, 3);
    pal(vdp, PAL_CRANE, 2, 3, 3, 4);
    pal(vdp, PAL_CRANE, 3, 12, 9, 3);
    pal(vdp, PAL_CRANE, 4, 14, 11, 3);
    pal(vdp, PAL_CRANE, 5, 15, 13, 5);
    pal(vdp, PAL_CRANE, 6, 11, 8, 2);
    pal(vdp, PAL_CRANE, 7, 6, 6, 7);
    pal(vdp, PAL_CRANE, 8, 8, 12, 15);
    pal(vdp, PAL_CRANE, 9, 15, 14, 6);

    pal(vdp, PAL_BOOM, 1, 2, 2, 2);
    pal(vdp, PAL_BOOM, 3, 8, 8, 9);
    pal(vdp, PAL_BOOM, 5, 13, 11, 4);
    pal(vdp, PAL_BOOM, 9, 4, 4, 5);

    pal(vdp, PAL_YARD, 1, 2, 2, 2);
    pal(vdp, PAL_YARD, 2, 5, 5, 6);
    pal(vdp, PAL_YARD, 6, 14, 6, 2);
    pal(vdp, PAL_YARD, 9, 15, 12, 3);

    pal(vdp, PAL_CREW, 1, 1, 1, 2);
    pal(vdp, PAL_CREW, 8, 7, 10, 13);
    pal(vdp, PAL_CREW, 10, 4, 6, 10);
    pal(vdp, PAL_CREW, 11, 6, 8, 12);
    pal(vdp, PAL_CREW, 12, 8, 7, 3);

    for (int i = 1; i < 16; i++) pal(vdp, PAL_ROAD, i, 3, 3, 4);
    pal(vdp, PAL_ROAD, 1, 2, 7, 3);
    pal(vdp, PAL_ROAD, 2, 1, 5, 2);
    pal(vdp, PAL_ROAD, 3, 4, 9, 3);
    pal(vdp, PAL_ROAD, 4, 6, 6, 4);
    pal(vdp, PAL_ROAD, 5, 4, 4, 3);
    pal(vdp, PAL_ROAD, 6, 3, 3, 4);
    pal(vdp, PAL_ROAD, 7, 5, 5, 6);
    pal(vdp, PAL_ROAD, 8, 7, 7, 6);
    pal(vdp, PAL_ROAD, 9, 2, 2, 3);
    pal(vdp, PAL_ROAD, 10, 6, 6, 5);
    pal(vdp, PAL_ROAD, 14, 14, 12, 2);
    pal(vdp, PAL_ROAD, 15, 8, 8, 9);
    vdp.setFogColor(gs::rgb4(6, 5, 6));

    art.body = gs::uploadMipped(vdp, bodyArt());
    art.boom = gs::uploadMipped(vdp, boomArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.pylon = gs::uploadMipped(vdp, pylonArt());
    art.crew = gs::uploadMipped(vdp, crewArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    loadFont(vdp, art);
}

}  // namespace craneturn
