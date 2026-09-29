#include "game/art.h"

#include <cmath>

namespace slip {
namespace {

void textPal(gs::VDP& v, int pal, uint16_t ink) {
    v.setColor(pal * 16 + 1, ink);
    v.setColor(pal * 16 + 15, gs::rgb4(1, 2, 3));
}

void paintPals(gs::VDP& vdp) {
    textPal(vdp, PAL_WHITE, gs::rgb4(15, 15, 15));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 8));
    textPal(vdp, PAL_SIGN, gs::rgb4(15, 14, 9));

    const uint16_t harbor[16] = {
        0,
        gs::rgb4(11, 13, 15),
        gs::rgb4(7, 9, 12),
        gs::rgb4(14, 12, 9),
        gs::rgb4(9, 7, 5),
        gs::rgb4(5, 6, 8),
        gs::rgb4(13, 10, 6),
        gs::rgb4(3, 4, 6),
        gs::rgb4(15, 14, 10),
        gs::rgb4(8, 9, 11),
        gs::rgb4(4, 5, 4),
        gs::rgb4(12, 8, 4),
        gs::rgb4(2, 3, 4),
        gs::rgb4(6, 8, 10),
        gs::rgb4(10, 11, 12),
        gs::rgb4(1, 2, 3),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_HARBOR * 16 + i, harbor[i]);

    vdp.setColor(PAL_CRANE * 16 + 1, gs::rgb4(15, 12, 2));
    vdp.setColor(PAL_CRANE * 16 + 2, gs::rgb4(11, 7, 1));
    vdp.setColor(PAL_CRANE * 16 + 3, gs::rgb4(4, 4, 5));
    vdp.setColor(PAL_CRANE * 16 + 4, gs::rgb4(9, 13, 15));
    vdp.setColor(PAL_CRANE * 16 + 5, gs::rgb4(15, 15, 14));
    vdp.setColor(PAL_CRANE * 16 + 6, gs::rgb4(2, 2, 3));

    vdp.setColor(PAL_HOOK * 16 + 1, gs::rgb4(14, 14, 15));
    vdp.setColor(PAL_HOOK * 16 + 2, gs::rgb4(6, 6, 8));
    vdp.setColor(PAL_HOOK * 16 + 3, gs::rgb4(15, 11, 2));
    vdp.setColor(PAL_HOOK * 16 + 4, gs::rgb4(2, 2, 3));

    vdp.setColor(PAL_BARGE * 16 + 1, gs::rgb4(12, 7, 3));
    vdp.setColor(PAL_BARGE * 16 + 2, gs::rgb4(8, 4, 2));
    vdp.setColor(PAL_BARGE * 16 + 3, gs::rgb4(14, 11, 6));
    vdp.setColor(PAL_BARGE * 16 + 4, gs::rgb4(3, 2, 1));
    vdp.setColor(PAL_BARGE * 16 + 5, gs::rgb4(6, 8, 5));
    vdp.setColor(PAL_BARGE * 16 + 6, gs::rgb4(15, 14, 8));

    vdp.setColor(PAL_PILE * 16 + 1, gs::rgb4(8, 8, 9));
    vdp.setColor(PAL_PILE * 16 + 2, gs::rgb4(5, 5, 6));
    vdp.setColor(PAL_PILE * 16 + 3, gs::rgb4(12, 12, 13));
    vdp.setColor(PAL_PILE * 16 + 4, gs::rgb4(13, 9, 3));
    vdp.setColor(PAL_PILE * 16 + 5, gs::rgb4(3, 3, 4));

    vdp.setColor(PAL_WATER * 16 + 1, gs::rgb4(2, 6, 11));
    vdp.setColor(PAL_WATER * 16 + 2, gs::rgb4(4, 9, 13));
    vdp.setColor(PAL_WATER * 16 + 3, gs::rgb4(8, 13, 15));
    vdp.setColor(PAL_WATER * 16 + 4, gs::rgb4(1, 3, 6));

    vdp.setColor(PAL_CABLE * 16 + 1, gs::rgb4(3, 3, 4));
    vdp.setColor(PAL_CABLE * 16 + 2, gs::rgb4(10, 10, 11));

    vdp.setColor(PAL_GULL * 16 + 1, gs::rgb4(15, 15, 15));
    vdp.setColor(PAL_GULL * 16 + 2, gs::rgb4(12, 12, 13));
    vdp.setColor(PAL_GULL * 16 + 3, gs::rgb4(15, 10, 2));
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

void paintHarbor(gs::VDP& vdp, gs::TileAlloc& tiles) {
    gs::Bitmap b(320, 224);
    for (int x = 0; x < 320; x++) {
        int hill = 92 + int(10 * std::sin(x * 0.03) + 6 * std::sin(x * 0.09));
        for (int y = hill; y < 128; y++) b.set(x, y, y < hill + 6 ? 3 : 5);
    }
    for (int i = 0; i < 6; i++) {
        int x = 12 + i * 52;
        int h = 28 + (i % 3) * 10;
        b.rect(float(x), float(128 - h), 22, float(h), i % 2 ? 7 : 9);
        b.rect(float(x + 4), float(128 - h + 6), 5, 6, 8);
        b.rect(float(x + 12), float(128 - h + 6), 5, 6, 8);
    }
    b.rect(0, 124, 320, 6, 4);
    b.rect(0, 16, 320, 8, 14);
    b.rect(0, 16, 320, 2, 9);
    b.rect(0, 206, 320, 18, 12);
    for (int x = 0; x < 320; x += 16) b.rect(float(x), 206, 8, 3, 14);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, b, PAL_HARBOR);
}

gs::Bitmap trolleyArt() {
    gs::Bitmap b(44, 24);
    b.rect(2, 0, 12, 6, 3);
    b.rect(30, 0, 12, 6, 3);
    b.rect(4, 1, 8, 4, 5);
    b.rect(32, 1, 8, 4, 5);
    b.rect(0, 5, 44, 7, 1);
    b.rect(0, 5, 44, 2, 2);
    b.rect(10, 12, 24, 10, 2);
    b.rect(14, 14, 8, 6, 4);
    b.rect(24, 14, 6, 6, 6);
    b.rect(18, 20, 8, 4, 3);
    return b;
}

gs::Bitmap hookArt() {
    gs::Bitmap b(18, 18);
    b.rect(7, 0, 4, 5, 1);
    b.rect(8, 4, 2, 4, 2);
    b.poly({{2, 8}, {16, 8}, {14, 13}, {4, 13}}, 3);
    b.rect(5, 13, 8, 3, 4);
    b.rect(7, 15, 4, 3, 2);
    return b;
}

gs::Bitmap linkArt() {
    gs::Bitmap b(3, 8);
    b.rect(1, 0, 1, 8, 1);
    b.set(0, 2, 2);
    b.set(2, 5, 2);
    return b;
}

gs::Bitmap bargeArt() {
    gs::Bitmap b(52, 22);
    b.poly({{2, 8}, {6, 18}, {46, 18}, {50, 8}, {44, 6}, {8, 6}}, 1);
    b.rect(8, 6, 36, 4, 2);
    b.rect(12, 2, 28, 6, 3);
    b.rect(16, 3, 8, 3, 6);
    b.rect(28, 3, 8, 3, 5);
    b.rect(4, 14, 8, 3, 4);
    b.rect(40, 14, 8, 3, 4);
    return b;
}

gs::Bitmap pileArt() {
    gs::Bitmap b(18, 72);
    b.rect(3, 8, 12, 64, 1);
    b.rect(3, 8, 3, 64, 2);
    b.rect(12, 8, 3, 64, 3);
    b.rect(0, 0, 18, 10, 4);
    b.rect(0, 0, 18, 3, 3);
    b.rect(6, 18, 2, 40, 5);
    return b;
}

gs::Bitmap waterArt() {
    gs::Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 1);
    for (int x = 0; x < 16; x++) {
        b.set(x, (x * 3) % 8, 2);
        b.set(x, 8 + (x * 5) % 7, 3);
    }
    b.rect(0, 14, 16, 2, 4);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(16, 8);
    b.poly({{0, 4}, {6, 2}, {8, 4}, {10, 2}, {15, 4}, {8, 5}}, 1);
    b.set(8, 6, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 16);
    b.rect(3, 4, 2, 12, 2);
    b.rect(1, 0, 6, 5, 1);
    b.rect(2, 1, 4, 3, 5);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    paintPals(vdp);
    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    paintHarbor(vdp, tiles);
    art.trolley = gs::uploadMipped(vdp, trolleyArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.link = gs::uploadMipped(vdp, linkArt());
    art.barge = gs::uploadMipped(vdp, bargeArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
}

}  // namespace slip
