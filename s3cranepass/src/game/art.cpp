#include "game/art.h"

#include <cmath>

namespace pass {
namespace {

void textPal(gs::VDP& v, int pal, uint16_t ink) {
    v.setColor(pal * 16 + 1, ink);
    v.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void paintPals(gs::VDP& vdp) {
    textPal(vdp, PAL_WHITE, gs::rgb4(15, 15, 15));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 7));
    textPal(vdp, PAL_SIGN, gs::rgb4(15, 14, 8));

    const uint16_t land[16] = {
        0,
        gs::rgb4(10, 12, 14),
        gs::rgb4(6, 8, 11),
        gs::rgb4(14, 14, 15),
        gs::rgb4(15, 15, 15),
        gs::rgb4(7, 7, 8),
        gs::rgb4(2, 5, 3),
        gs::rgb4(5, 5, 6),
        gs::rgb4(8, 8, 9),
        gs::rgb4(12, 12, 13),
        gs::rgb4(13, 10, 2),
        gs::rgb4(9, 3, 2),
        gs::rgb4(4, 4, 5),
        gs::rgb4(3, 6, 8),
        gs::rgb4(11, 11, 12),
        gs::rgb4(2, 2, 3),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_LAND * 16 + i, land[i]);

    vdp.setColor(PAL_CRANE * 16 + 1, gs::rgb4(15, 13, 3));
    vdp.setColor(PAL_CRANE * 16 + 2, gs::rgb4(12, 8, 1));
    vdp.setColor(PAL_CRANE * 16 + 3, gs::rgb4(4, 4, 5));
    vdp.setColor(PAL_CRANE * 16 + 4, gs::rgb4(8, 12, 15));
    vdp.setColor(PAL_CRANE * 16 + 5, gs::rgb4(15, 15, 14));
    vdp.setColor(PAL_CRANE * 16 + 6, gs::rgb4(2, 2, 3));

    vdp.setColor(PAL_HOOK * 16 + 1, gs::rgb4(14, 14, 15));
    vdp.setColor(PAL_HOOK * 16 + 2, gs::rgb4(6, 6, 7));
    vdp.setColor(PAL_HOOK * 16 + 3, gs::rgb4(15, 12, 2));
    vdp.setColor(PAL_HOOK * 16 + 4, gs::rgb4(2, 2, 3));

    vdp.setColor(PAL_ROCK * 16 + 1, gs::rgb4(9, 9, 10));
    vdp.setColor(PAL_ROCK * 16 + 2, gs::rgb4(6, 6, 7));
    vdp.setColor(PAL_ROCK * 16 + 3, gs::rgb4(13, 13, 14));
    vdp.setColor(PAL_ROCK * 16 + 4, gs::rgb4(3, 3, 4));

    vdp.setColor(PAL_LOG * 16 + 1, gs::rgb4(12, 8, 4));
    vdp.setColor(PAL_LOG * 16 + 2, gs::rgb4(8, 5, 2));
    vdp.setColor(PAL_LOG * 16 + 3, gs::rgb4(4, 8, 3));
    vdp.setColor(PAL_LOG * 16 + 4, gs::rgb4(3, 2, 1));

    vdp.setColor(PAL_JEEP * 16 + 1, gs::rgb4(8, 10, 6));
    vdp.setColor(PAL_JEEP * 16 + 2, gs::rgb4(4, 5, 3));
    vdp.setColor(PAL_JEEP * 16 + 3, gs::rgb4(12, 13, 10));
    vdp.setColor(PAL_JEEP * 16 + 4, gs::rgb4(2, 2, 2));
    vdp.setColor(PAL_JEEP * 16 + 5, gs::rgb4(6, 8, 10));

    vdp.setColor(PAL_SLAB * 16 + 1, gs::rgb4(14, 15, 15));
    vdp.setColor(PAL_SLAB * 16 + 2, gs::rgb4(9, 11, 13));
    vdp.setColor(PAL_SLAB * 16 + 3, gs::rgb4(5, 7, 9));

    vdp.setColor(PAL_CABLE * 16 + 1, gs::rgb4(3, 3, 4));
    vdp.setColor(PAL_CABLE * 16 + 2, gs::rgb4(10, 10, 11));

    vdp.setColor(PAL_RIVAL * 16 + 1, gs::rgb4(14, 3, 2));
    vdp.setColor(PAL_RIVAL * 16 + 2, gs::rgb4(8, 2, 2));
    vdp.setColor(PAL_RIVAL * 16 + 3, gs::rgb4(15, 12, 4));
    vdp.setColor(PAL_RIVAL * 16 + 4, gs::rgb4(3, 3, 4));
    vdp.setColor(PAL_RIVAL * 16 + 5, gs::rgb4(12, 12, 13));

    vdp.setColor(PAL_SNOW * 16 + 1, gs::rgb4(15, 15, 15));
    vdp.setColor(PAL_SNOW * 16 + 2, gs::rgb4(11, 13, 15));
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
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

void paintLand(gs::VDP& vdp, gs::TileAlloc& tiles) {
    gs::Bitmap b(320, 224);
    for (int x = 0; x < 320; x++) {
        int ridge = 78 + int(18 * std::sin(x * 0.04) + 10 * std::sin(x * 0.11));
        for (int y = ridge; y < 150; y++) b.set(x, y, y < ridge + 10 ? 4 : 5);
    }
    for (int i = 0; i < 7; i++) {
        int x = 18 + i * 46;
        b.poly({{float(x), 150.f}, {float(x + 14), 112.f}, {float(x + 28), 150.f}}, 6);
        b.rect(float(x + 12), 146, 4, 10, 12);
    }
    b.rect(0, 156, 320, 18, 9);
    b.rect(0, 168, 320, 28, 7);
    b.rect(0, 168, 320, 2, 8);
    b.rect(8, 160, 78, 8, 10);
    b.rect(12, 168, 70, 6, 10);
    b.rect(248, 120, 8, 48, 11);
    b.rect(286, 112, 8, 56, 11);
    b.rect(248, 116, 46, 6, 11);
    b.rect(0, 196, 320, 28, 5);
    for (int x = 0; x < 320; x += 28) b.rect(float(x), 178, 14, 3, 14);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, b, PAL_LAND);
}

gs::Bitmap trolleyArt() {
    gs::Bitmap b(40, 22);
    b.rect(2, 0, 10, 5, 3);
    b.rect(28, 0, 10, 5, 3);
    b.rect(4, 1, 6, 3, 5);
    b.rect(30, 1, 6, 3, 5);
    b.rect(0, 4, 40, 6, 1);
    b.rect(0, 4, 40, 2, 2);
    b.rect(8, 10, 24, 10, 2);
    b.rect(11, 12, 10, 6, 4);
    b.rect(23, 12, 6, 6, 6);
    b.rect(16, 18, 6, 4, 3);
    return b;
}

gs::Bitmap hookArt() {
    gs::Bitmap b(16, 16);
    b.rect(6, 0, 4, 4, 1);
    b.rect(7, 3, 2, 4, 2);
    b.poly({{2, 7}, {14, 7}, {12, 12}, {4, 12}}, 3);
    b.rect(4, 12, 8, 3, 4);
    return b;
}

gs::Bitmap linkArt() {
    gs::Bitmap b(3, 6);
    b.rect(1, 0, 1, 6, 1);
    b.set(0, 2, 2);
    b.set(2, 3, 2);
    return b;
}

gs::Bitmap rockArt() {
    gs::Bitmap b(28, 22);
    b.poly({{4, 20}, {2, 12}, {8, 4}, {16, 2}, {24, 8}, {26, 18}, {14, 21}}, 1);
    b.poly({{8, 16}, {7, 10}, {14, 6}, {18, 12}}, 3);
    b.rect(6, 16, 14, 3, 2);
    b.set(10, 9, 4);
    return b;
}

gs::Bitmap logArt() {
    gs::Bitmap b(36, 16);
    b.ellipse(18, 8, 16, 6, 1);
    b.ellipse(18, 8, 12, 3, 2);
    b.rect(2, 6, 3, 4, 4);
    b.rect(31, 6, 3, 4, 4);
    b.ellipse(6, 8, 2, 3, 3);
    return b;
}

gs::Bitmap jeepArt() {
    gs::Bitmap b(36, 20);
    b.rect(4, 6, 26, 8, 1);
    b.rect(16, 2, 12, 6, 3);
    b.rect(18, 3, 8, 4, 5);
    b.rect(2, 10, 6, 4, 2);
    b.ellipse(10, 15, 4, 4, 4);
    b.ellipse(26, 15, 4, 4, 4);
    b.ellipse(10, 15, 2, 2, 3);
    b.ellipse(26, 15, 2, 2, 3);
    return b;
}

gs::Bitmap slabArt() {
    gs::Bitmap b(32, 14);
    b.rect(1, 2, 30, 10, 1);
    b.rect(1, 2, 30, 3, 2);
    b.line(4, 6, 28, 11, 3, 1);
    b.line(8, 4, 12, 11, 3, 1);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(28, 20);
    b.rect(2, 8, 24, 4, 1);
    b.rect(4, 4, 6, 6, 2);
    b.rect(10, 12, 3, 8, 4);
    b.rect(18, 6, 6, 4, 5);
    b.rect(20, 7, 2, 2, 3);
    return b;
}

gs::Bitmap flakeArt() {
    gs::Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 4, 1);
    b.set(0, 2, 1);
    b.set(4, 2, 1);
    b.set(2, 2, 2);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(8, 28);
    b.rect(2, 0, 4, 28, 1);
    b.rect(3, 0, 2, 28, 2);
    b.rect(1, 2, 6, 3, 3);
    return b;
}

gs::Bitmap arrowArt() {
    gs::Bitmap b(9, 10);
    b.poly({{4, 9}, {0, 3}, {8, 3}}, 1);
    b.rect(3, 0, 3, 4, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    paintPals(vdp);
    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    paintLand(vdp, tiles);
    art.trolley = gs::uploadMipped(vdp, trolleyArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.link = gs::uploadMipped(vdp, linkArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.log = gs::uploadMipped(vdp, logArt());
    art.jeep = gs::uploadMipped(vdp, jeepArt());
    art.slab = gs::uploadMipped(vdp, slabArt());
    art.rival = gs::uploadMipped(vdp, rivalArt());
    art.flake = gs::uploadMipped(vdp, flakeArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.arrow = gs::uploadMipped(vdp, arrowArt());
}

}  // namespace pass
