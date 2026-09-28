#include "game/art.h"

namespace cranebox {
namespace {

void textPal(gs::VDP& v, int pal, uint16_t ink) {
    v.setColor(pal * 16 + 1, ink);
    v.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void paintPals(gs::VDP& vdp) {
    textPal(vdp, PAL_WHITE, gs::rgb4(15, 15, 15));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 7));

    const uint16_t yard[16] = {
        0,
        gs::rgb4(10, 8, 5),
        gs::rgb4(7, 6, 4),
        gs::rgb4(12, 11, 8),
        gs::rgb4(5, 5, 4),
        gs::rgb4(8, 9, 10),
        gs::rgb4(13, 13, 12),
        gs::rgb4(4, 7, 4),
        gs::rgb4(3, 5, 8),
        gs::rgb4(9, 10, 11),
        gs::rgb4(14, 12, 6),
        gs::rgb4(2, 2, 3),
        gs::rgb4(6, 8, 11),
        gs::rgb4(11, 7, 4),
        gs::rgb4(15, 14, 10),
        gs::rgb4(1, 1, 2),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_YARD * 16 + i, yard[i]);

    vdp.setColor(PAL_CRANE * 16 + 1, gs::rgb4(14, 12, 3));
    vdp.setColor(PAL_CRANE * 16 + 2, gs::rgb4(11, 9, 2));
    vdp.setColor(PAL_CRANE * 16 + 3, gs::rgb4(4, 4, 5));
    vdp.setColor(PAL_CRANE * 16 + 4, gs::rgb4(8, 12, 15));
    vdp.setColor(PAL_CRANE * 16 + 5, gs::rgb4(13, 13, 14));
    vdp.setColor(PAL_CRANE * 16 + 6, gs::rgb4(2, 2, 3));
    vdp.setColor(PAL_CRANE * 16 + 7, gs::rgb4(15, 8, 2));
    vdp.setColor(PAL_CRANE * 16 + 8, gs::rgb4(6, 6, 7));

    vdp.setColor(PAL_BOX * 16 + 1, gs::rgb4(15, 13, 2));
    vdp.setColor(PAL_BOX * 16 + 2, gs::rgb4(12, 4, 2));
    vdp.setColor(PAL_BOX * 16 + 3, gs::rgb4(15, 15, 12));
    vdp.setColor(PAL_BOX * 16 + 4, gs::rgb4(3, 3, 4));
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

void tiles(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
    uint8_t dirt[64] = {};
    uint8_t curb[64] = {};
    for (int i = 0; i < 64; i++) dirt[i] = (i % 5 == 0) ? 2 : 1;
    for (int x = 0; x < 8; x++) {
        curb[x] = 3;
        curb[8 + x] = 4;
        for (int y = 2; y < 8; y++) curb[y * 8 + x] = (x + y) % 4 == 0 ? 2 : 1;
    }
    art.dirt = tiles.alloc(1);
    art.curb = tiles.alloc(1);
    vdp.loadTile(art.dirt, dirt);
    vdp.loadTile(art.curb, curb);
}

gs::Bitmap craneArt() {
    gs::Bitmap b(78, 62);
    b.line(18, 46, 58, 8, 8, 3);
    b.line(22, 46, 54, 14, 5, 2);
    b.rect(8, 36, 46, 16, 2);
    b.rect(8, 36, 46, 3, 1);
    b.rect(40, 22, 22, 16, 1);
    b.rect(40, 22, 22, 3, 2);
    b.rect(44, 26, 10, 7, 4);
    b.rect(54, 38, 8, 6, 3);
    b.rect(12, 40, 8, 6, 6);
    b.rect(24, 40, 8, 6, 6);
    b.rect(6, 50, 12, 8, 3);
    b.rect(48, 50, 12, 8, 3);
    b.ellipse(12, 54, 5, 5, 6);
    b.ellipse(54, 54, 5, 5, 6);
    b.ellipse(12, 54, 2, 2, 5);
    b.ellipse(54, 54, 2, 2, 5);
    b.rect(58, 6, 4, 6, 7);
    b.line(60, 12, 60, 22, 6, 1);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 3);
    b.ellipse(7, 7, 3, 3, 5);
    b.rect(6, 2, 2, 10, 8);
    b.rect(2, 6, 10, 2, 8);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(8, 40);
    b.rect(2, 0, 4, 40, 2);
    b.rect(0, 0, 8, 4, 1);
    b.rect(1, 18, 6, 3, 3);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(16, 8);
    b.rect(0, 2, 16, 4, 1);
    b.rect(0, 3, 8, 2, 3);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(48, 36);
    b.rect(2, 10, 44, 26, 13);
    b.rect(0, 6, 48, 6, 2);
    b.rect(8, 18, 10, 12, 4);
    b.rect(28, 18, 10, 8, 9);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 28);
    b.rect(3, 8, 2, 20, 5);
    b.rect(1, 2, 6, 6, 10);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    paintPals(vdp);
    gs::TileAlloc tilesAlloc(vdp, 1);
    loadFont(vdp, tilesAlloc, art);
    tiles(vdp, tilesAlloc, art);
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
}

}  // namespace cranebox
