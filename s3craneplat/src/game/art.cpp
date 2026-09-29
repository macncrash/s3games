#include "game/art.h"

#include <string>

namespace craneplat {
namespace {

void textPal(gs::VDP& v, int pal, uint16_t ink) {
    v.setColor(pal * 16 + 1, ink);
    v.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void paintPals(gs::VDP& vdp) {
    textPal(vdp, PAL_WHITE, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 7));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 13, 6));

    const uint16_t yard[16] = {
        0,
        gs::rgb4(3, 5, 9),     // dusk
        gs::rgb4(6, 8, 12),    // sky
        gs::rgb4(10, 11, 13),  // haze
        gs::rgb4(14, 13, 11),  // lamp glow
        gs::rgb4(4, 4, 5),     // concrete dark
        gs::rgb4(7, 7, 8),     // concrete
        gs::rgb4(11, 11, 10),  // concrete light
        gs::rgb4(15, 12, 2),   // safety yellow
        gs::rgb4(12, 6, 2),    // rust
        gs::rgb4(2, 3, 4),     // shadow
        gs::rgb4(5, 7, 6),     // far shed
        gs::rgb4(8, 9, 8),     // rail
        gs::rgb4(13, 9, 4),    // rival cab
        gs::rgb4(2, 6, 4),     // ground
        gs::rgb4(1, 1, 2),     // ink
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_YARD * 16 + i, yard[i]);

    vdp.setColor(PAL_CRANE * 16 + 1, gs::rgb4(14, 9, 3));
    vdp.setColor(PAL_CRANE * 16 + 2, gs::rgb4(10, 6, 2));
    vdp.setColor(PAL_CRANE * 16 + 3, gs::rgb4(4, 3, 3));
    vdp.setColor(PAL_CRANE * 16 + 4, gs::rgb4(15, 14, 8));
    vdp.setColor(PAL_CRANE * 16 + 5, gs::rgb4(6, 8, 10));

    vdp.setColor(PAL_HOOK * 16 + 1, gs::rgb4(13, 14, 15));
    vdp.setColor(PAL_HOOK * 16 + 2, gs::rgb4(5, 6, 7));
    vdp.setColor(PAL_HOOK * 16 + 3, gs::rgb4(15, 12, 2));
    vdp.setColor(PAL_HOOK * 16 + 4, gs::rgb4(2, 2, 3));

    vdp.setColor(PAL_WOOD * 16 + 1, gs::rgb4(13, 10, 5));
    vdp.setColor(PAL_WOOD * 16 + 2, gs::rgb4(9, 6, 3));
    vdp.setColor(PAL_WOOD * 16 + 3, gs::rgb4(5, 3, 2));
    vdp.setColor(PAL_WOOD * 16 + 4, gs::rgb4(15, 14, 10));
    vdp.setColor(PAL_WOOD * 16 + 5, gs::rgb4(2, 2, 2));

    vdp.setColor(PAL_CABLE * 16 + 1, gs::rgb4(3, 3, 4));
    vdp.setColor(PAL_CABLE * 16 + 2, gs::rgb4(9, 9, 10));

    vdp.setColor(PAL_RIVAL * 16 + 1, gs::rgb4(12, 3, 3));
    vdp.setColor(PAL_RIVAL * 16 + 2, gs::rgb4(6, 2, 2));
    vdp.setColor(PAL_RIVAL * 16 + 3, gs::rgb4(14, 12, 8));
    vdp.setColor(PAL_RIVAL * 16 + 4, gs::rgb4(2, 2, 3));
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

gs::Bitmap yardPicture() {
    gs::Bitmap b(320, 224);
    for (int y = 0; y < 224; y++) {
        int c = y < 40 ? 1 : y < 90 ? 2 : y < 130 ? 3 : 14;
        b.rect(0, float(y), 320, 1, c);
    }
    b.rect(0, 150, 320, 74, 14);
    b.rect(0, 150, 320, 4, 10);
    // Far sheds.
    b.rect(16, 112, 54, 38, 11);
    b.rect(18, 100, 28, 14, 11);
    b.rect(250, 108, 60, 42, 11);
    b.rect(262, 96, 22, 14, 11);
    // Gantry columns and rail. The live rail the trolley rides is y=36.
    b.rect(18, 36, 8, 150, 5);
    b.rect(294, 36, 8, 150, 5);
    b.rect(8, 28, 304, 10, 12);
    b.rect(8, 28, 304, 3, 7);
    for (int i = 0; i < 14; i++) b.rect(20.f + i * 20.f, 30, 8, 4, 10);
    // The platform. Deck top is y=168, matching the stop line in the game.
    b.rect(196, 168, 88, 42, 6);
    b.rect(196, 168, 88, 6, 8);
    b.rect(196, 174, 88, 2, 10);
    for (int i = 0; i < 5; i++) b.rect(204.f + i * 16.f, 186, 8, 18, 5);
    b.rect(188, 206, 104, 6, 5);
    // Level ticks beside the deck.
    for (int i = 0; i < 6; i++) b.rect(184, 156.f + i * 8.f, 8, 2, i == 1 ? 8 : 7);
    // Pit under the open span so a low hook is obviously off the deck.
    b.rect(40, 176, 148, 28, 10);
    b.rect(40, 176, 148, 3, 5);
    return b;
}

gs::Bitmap trolleyArt() {
    gs::Bitmap b(40, 22);
    b.rect(2, 8, 36, 10, 2);
    b.rect(4, 6, 32, 4, 1);
    b.rect(6, 2, 10, 6, 4);
    b.rect(24, 2, 10, 6, 4);
    b.rect(8, 14, 6, 6, 3);
    b.rect(26, 14, 6, 6, 3);
    b.rect(16, 16, 8, 4, 5);
    return b;
}

gs::Bitmap hookArt() {
    gs::Bitmap b(14, 18);
    b.rect(6, 0, 2, 8, 2);
    b.rect(4, 6, 6, 3, 1);
    b.rect(3, 9, 8, 2, 1);
    b.rect(2, 11, 3, 5, 1);
    b.rect(9, 11, 3, 5, 1);
    b.rect(3, 15, 8, 2, 3);
    return b;
}

gs::Bitmap linkArt() {
    gs::Bitmap b(4, 6);
    b.rect(1, 0, 2, 6, 2);
    b.rect(0, 2, 4, 2, 1);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(26, 20);
    b.rect(0, 0, 26, 20, 3);
    b.rect(1, 1, 24, 18, 2);
    b.rect(2, 2, 22, 7, 1);
    b.rect(1, 11, 24, 3, 4);
    b.rect(3, 4, 2, 2, 5);
    b.rect(21, 4, 2, 2, 5);
    b.rect(3, 15, 2, 2, 5);
    b.rect(21, 15, 2, 2, 5);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(28, 36);
    b.rect(2, 8, 24, 10, 1);
    b.rect(6, 0, 6, 12, 2);
    b.rect(16, 2, 8, 8, 3);
    b.rect(12, 16, 3, 18, 4);
    b.rect(8, 30, 12, 4, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 12);
    b.rect(3, 0, 2, 4, 2);
    b.rect(1, 4, 6, 6, 1);
    b.rect(2, 5, 4, 4, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    paintPals(vdp);
    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, yardPicture(), PAL_YARD);
    art.trolley = gs::uploadMipped(vdp, trolleyArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.link = gs::uploadMipped(vdp, linkArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.rival = gs::uploadMipped(vdp, rivalArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
}

}  // namespace craneplat
