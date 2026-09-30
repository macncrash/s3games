#include "game/art.h"

#include <cmath>

namespace slip {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap kartArt() {
    Bitmap b(96, 80);
    // Rear three-quarter of an open harbour kart. Nose is up-screen.
    b.ellipse(48, 62, 28, 8, 4);
    b.rect(22, 28, 52, 30, 3);
    b.rect(26, 24, 44, 28, 2);
    b.rect(30, 22, 36, 16, 1);
    b.poly({{30, 24}, {48, 8}, {66, 24}}, 2);
    b.poly({{36, 22}, {48, 12}, {60, 22}}, 1);
    b.rect(40, 30, 16, 12, 6);
    b.ellipse(48, 34, 7, 7, 7);
    b.ellipse(48, 33, 4, 4, 8);
    b.rect(34, 18, 4, 22, 5);
    b.rect(58, 18, 4, 22, 5);
    b.rect(34, 16, 28, 4, 5);
    b.ellipse(18, 40, 10, 14, 4);
    b.ellipse(78, 40, 10, 14, 4);
    b.ellipse(18, 40, 5, 8, 9);
    b.ellipse(78, 40, 5, 8, 9);
    b.rect(40, 52, 16, 6, 4);
    b.rect(44, 8, 8, 6, 5);
    b.outline(9, false);
    return b;
}

Bitmap crateArt() {
    Bitmap b(48, 48);
    b.rect(6, 10, 36, 30, 2);
    b.rect(8, 12, 32, 26, 1);
    b.line(8, 12, 40, 38, 3, 2);
    b.line(40, 12, 8, 38, 3, 2);
    b.rect(20, 20, 8, 8, 4);
    b.rect(6, 8, 36, 4, 3);
    b.outline(3, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(24, 64);
    b.rect(8, 8, 8, 52, 2);
    b.rect(9, 8, 3, 52, 1);
    b.rect(4, 4, 16, 8, 4);
    b.rect(6, 56, 12, 6, 3);
    b.outline(3, false);
    return b;
}

Bitmap boatArt() {
    Bitmap b(120, 72);
    b.poly({{8, 40}, {20, 58}, {100, 58}, {112, 40}, {96, 36}, {24, 36}}, 1);
    b.poly({{24, 36}, {96, 36}, {90, 44}, {30, 44}}, 5);
    b.rect(40, 18, 40, 20, 3);
    b.rect(44, 20, 14, 10, 8);
    b.rect(62, 20, 14, 10, 6);
    b.rect(56, 8, 6, 12, 2);
    b.poly({{54, 10}, {64, 4}, {64, 14}}, 2);
    b.rect(18, 40, 10, 6, 4);
    b.outline(9, false);
    return b;
}

Bitmap buoyArt() {
    Bitmap b(32, 48);
    b.ellipse(16, 16, 12, 12, 1);
    b.ellipse(16, 16, 7, 7, 2);
    b.rect(14, 26, 4, 16, 3);
    b.ellipse(16, 42, 6, 3, 4);
    b.outline(4, false);
    return b;
}

Bitmap splashArt() {
    Bitmap b(48, 32);
    b.ellipse(24, 18, 18, 8, 1);
    b.ellipse(14, 14, 8, 6, 2);
    b.ellipse(34, 14, 8, 6, 2);
    b.ellipse(24, 12, 6, 5, 3);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(10, 12, 14), gs::rgb4(15, 12, 4), gs::rgb4(15, 5, 3), gs::rgb4(4, 14, 8),
                          gs::rgb4(6, 10, 15), gs::rgb4(15, 9, 3), gs::rgb4(5, 6, 8), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(15, 10, 2), gs::rgb4(13, 6, 1), gs::rgb4(8, 3, 1), gs::rgb4(2, 2, 2),
                          gs::rgb4(11, 12, 13), gs::rgb4(13, 8, 5), gs::rgb4(15, 15, 14), gs::rgb4(3, 8, 14),
                          gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(4, 9, 15), gs::rgb4(2, 5, 12), gs::rgb4(1, 2, 7), gs::rgb4(2, 2, 2),
                           gs::rgb4(11, 12, 13), gs::rgb4(13, 8, 5), gs::rgb4(15, 15, 14), gs::rgb4(15, 4, 3),
                           gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(12, 8, 3), gs::rgb4(9, 6, 2), gs::rgb4(5, 3, 1), gs::rgb4(14, 13, 8),
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOAT, {0, gs::rgb4(14, 14, 13), gs::rgb4(12, 3, 2), gs::rgb4(4, 7, 10), gs::rgb4(8, 2, 2),
                           gs::rgb4(6, 10, 12), gs::rgb4(2, 3, 5), 0, gs::rgb4(8, 12, 15), gs::rgb4(1, 1, 2),
                           0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_POST, {0, gs::rgb4(13, 12, 8), gs::rgb4(8, 7, 4), gs::rgb4(4, 3, 2), gs::rgb4(3, 10, 6),
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(12, 14, 15), gs::rgb4(8, 12, 14), gs::rgb4(15, 15, 15), gs::rgb4(15, 12, 4),
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    // Road chip indices: 1-3 ground, 4-5 verge, 6-7 quay, 8 grit, 11-13 water, 14 paint, 15 speck.
    const uint16_t field[16] = {
        0,
        gs::rgb4(3, 5, 4), gs::rgb4(2, 4, 3), gs::rgb4(4, 6, 4),
        gs::rgb4(7, 6, 4), gs::rgb4(5, 4, 3),
        gs::rgb4(8, 8, 8), gs::rgb4(5, 5, 6),
        gs::rgb4(6, 6, 6), gs::rgb4(4, 4, 5), gs::rgb4(9, 9, 8),
        gs::rgb4(2, 5, 8), gs::rgb4(1, 4, 7), gs::rgb4(8, 12, 14),
        gs::rgb4(15, 12, 3), gs::rgb4(10, 10, 10),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FIELD * 16 + i, field[i]);
    vdp.setFogColor(gs::rgb4(12, 7, 4));

    loadFont(vdp, art);
    art.kart = gs::uploadMipped(vdp, kartArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.boat = gs::uploadMipped(vdp, boatArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.splash = gs::uploadMipped(vdp, splashArt());
}

}  // namespace slip
