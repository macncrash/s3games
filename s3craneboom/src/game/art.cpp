#include "game/art.h"

namespace craneboom {
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
        gs::rgb4(9, 8, 6),
        gs::rgb4(6, 6, 5),
        gs::rgb4(12, 11, 8),
        gs::rgb4(4, 5, 4),
        gs::rgb4(7, 9, 11),
        gs::rgb4(13, 13, 12),
        gs::rgb4(3, 6, 4),
        gs::rgb4(4, 5, 8),
        gs::rgb4(10, 11, 12),
        gs::rgb4(14, 13, 8),
        gs::rgb4(2, 2, 3),
        gs::rgb4(8, 7, 6),
        gs::rgb4(11, 8, 5),
        gs::rgb4(15, 14, 11),
        gs::rgb4(1, 1, 2),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_YARD * 16 + i, yard[i]);

    vdp.setColor(PAL_CRANE * 16 + 1, gs::rgb4(14, 11, 2));
    vdp.setColor(PAL_CRANE * 16 + 2, gs::rgb4(10, 8, 2));
    vdp.setColor(PAL_CRANE * 16 + 3, gs::rgb4(3, 3, 4));
    vdp.setColor(PAL_CRANE * 16 + 4, gs::rgb4(6, 10, 14));
    vdp.setColor(PAL_CRANE * 16 + 5, gs::rgb4(14, 14, 13));
    vdp.setColor(PAL_CRANE * 16 + 6, gs::rgb4(2, 2, 3));
    vdp.setColor(PAL_CRANE * 16 + 7, gs::rgb4(15, 7, 2));
    vdp.setColor(PAL_CRANE * 16 + 8, gs::rgb4(7, 7, 8));

    vdp.setColor(PAL_BOOM * 16 + 1, gs::rgb4(9, 11, 13));
    vdp.setColor(PAL_BOOM * 16 + 2, gs::rgb4(5, 7, 9));
    vdp.setColor(PAL_BOOM * 16 + 3, gs::rgb4(14, 12, 4));
    vdp.setColor(PAL_BOOM * 16 + 4, gs::rgb4(12, 4, 3));
    vdp.setColor(PAL_BOOM * 16 + 5, gs::rgb4(3, 3, 4));
    vdp.setColor(PAL_BOOM * 16 + 6, gs::rgb4(15, 14, 8));

    vdp.setColor(PAL_DRIVE * 16 + 1, gs::rgb4(13, 5, 2));
    vdp.setColor(PAL_DRIVE * 16 + 2, gs::rgb4(8, 3, 2));
    vdp.setColor(PAL_DRIVE * 16 + 3, gs::rgb4(15, 12, 4));
    vdp.setColor(PAL_DRIVE * 16 + 4, gs::rgb4(4, 4, 5));
    vdp.setColor(PAL_DRIVE * 16 + 5, gs::rgb4(14, 14, 13));
    vdp.setColor(PAL_DRIVE * 16 + 6, gs::rgb4(2, 2, 3));
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
    for (int i = 0; i < 64; i++) dirt[i] = ((i * 3) % 7 == 0) ? 2 : 1;
    for (int x = 0; x < 8; x++) {
        curb[x] = 3;
        curb[8 + x] = 10;
        for (int y = 2; y < 8; y++) curb[y * 8 + x] = (x + y) % 5 == 0 ? 2 : 1;
    }
    art.dirt = tiles.alloc(1);
    art.curb = tiles.alloc(1);
    vdp.loadTile(art.dirt, dirt);
    vdp.loadTile(art.curb, curb);
}

gs::Bitmap cabArt() {
    gs::Bitmap b(72, 48);
    b.rect(4, 18, 50, 18, 1);
    b.rect(4, 18, 50, 4, 2);
    b.rect(34, 6, 26, 16, 1);
    b.rect(38, 9, 14, 8, 4);
    b.rect(54, 22, 10, 8, 3);
    b.rect(8, 24, 8, 6, 6);
    b.rect(6, 34, 14, 10, 3);
    b.rect(40, 34, 16, 10, 3);
    b.ellipse(13, 40, 6, 6, 8);
    b.ellipse(48, 40, 6, 6, 8);
    b.ellipse(13, 40, 2, 2, 5);
    b.ellipse(48, 40, 2, 2, 5);
    b.rect(18, 10, 4, 10, 7);
    return b;
}

gs::Bitmap boomArt() {
    gs::Bitmap b(64, 16);
    b.line(2, 12, 60, 3, 2, 2);
    b.line(2, 8, 60, 2, 1, 2);
    b.line(10, 12, 18, 3, 8, 1);
    b.line(26, 11, 34, 3, 8, 1);
    b.line(42, 10, 50, 3, 8, 1);
    return b;
}

gs::Bitmap driveArt() {
    gs::Bitmap b(28, 20);
    b.rect(2, 3, 24, 14, 1);
    b.rect(2, 3, 24, 3, 2);
    b.ellipse(14, 10, 5, 5, 4);
    b.ellipse(14, 10, 2, 2, 3);
    b.rect(6, 8, 3, 4, 5);
    b.rect(19, 8, 3, 4, 5);
    b.rect(4, 15, 20, 2, 6);
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(24, 10);
    b.rect(0, 2, 24, 6, 1);
    b.rect(0, 2, 24, 2, 2);
    b.rect(11, 3, 2, 4, 5);
    return b;
}

gs::Bitmap saddleArt() {
    gs::Bitmap b(24, 12);
    b.rect(0, 3, 24, 6, 3);
    b.rect(0, 3, 8, 6, 6);
    b.rect(16, 3, 8, 6, 4);
    b.rect(10, 4, 4, 4, 5);
    return b;
}

gs::Bitmap pierArt() {
    gs::Bitmap b(10, 56);
    b.rect(3, 0, 4, 56, 2);
    b.rect(1, 0, 8, 5, 1);
    b.rect(0, 48, 10, 8, 5);
    return b;
}

gs::Bitmap hookArt() {
    gs::Bitmap b(8, 10);
    b.rect(3, 0, 2, 6, 5);
    b.ellipse(4, 7, 3, 3, 3);
    return b;
}

gs::Bitmap linkArt() {
    gs::Bitmap b(4, 8);
    b.rect(1, 0, 2, 8, 5);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(56, 32);
    b.rect(4, 10, 48, 22, 13);
    b.rect(0, 6, 56, 6, 2);
    b.rect(10, 16, 12, 12, 8);
    b.rect(32, 16, 12, 8, 9);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    paintPals(vdp);
    gs::TileAlloc tilesAlloc(vdp, 1);
    loadFont(vdp, tilesAlloc, art);
    tiles(vdp, tilesAlloc, art);
    art.cab = gs::uploadMipped(vdp, cabArt());
    art.boom = gs::uploadMipped(vdp, boomArt());
    art.drive = gs::uploadMipped(vdp, driveArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.saddle = gs::uploadMipped(vdp, saddleArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.link = gs::uploadMipped(vdp, linkArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
}

}  // namespace craneboom
