#include "game/art.h"

#include <string>

namespace mush {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap mushBody(int step) {
    Bitmap b(48, 56);
    int bob = step ? 2 : 0;
    b.ellipse(24, 38 + bob, 10, 8, 4);
    b.rect(16, 40 + bob, 6, 10, 4);
    b.rect(26, 40 + bob, 6, 10, 4);
    b.ellipse(18, 50, 5, 3, 5);
    b.ellipse(30, 50, 5, 3, 5);
    b.ellipse(24, 22, 18, 14, 1);
    b.ellipse(24, 24, 14, 10, 2);
    b.ellipse(16, 16, 4, 3, 3);
    b.ellipse(30, 14, 3, 2, 3);
    b.ellipse(22, 18, 2, 2, 3);
    b.rect(18, 26, 3, 4, 6);
    b.rect(27, 26, 3, 4, 6);
    b.rect(21, 32, 6, 2, 6);
    b.outline(7, false);
    return b;
}

Bitmap wheelArt() {
    Bitmap b(40, 40);
    b.ellipse(20, 20, 16, 16, 1);
    b.ellipse(20, 20, 12, 12, 2);
    b.ellipse(20, 20, 3, 3, 3);
    b.line(20, 6, 20, 34, 3, 2);
    b.line(6, 20, 34, 20, 3, 2);
    b.line(10, 10, 30, 30, 3, 2);
    b.line(30, 10, 10, 30, 3, 2);
    b.outline(4, false);
    return b;
}

Bitmap cartArt() {
    Bitmap b(72, 48);
    b.rect(8, 10, 52, 22, 1);
    b.rect(10, 12, 48, 16, 2);
    b.rect(4, 18, 8, 8, 3);
    b.ellipse(16, 36, 10, 10, 4);
    b.ellipse(16, 36, 4, 4, 5);
    b.ellipse(52, 36, 10, 10, 4);
    b.ellipse(52, 36, 4, 4, 5);
    b.outline(6, false);
    return b;
}

Bitmap bikeArt() {
    Bitmap b(64, 48);
    b.ellipse(14, 32, 12, 12, 1);
    b.ellipse(14, 32, 5, 5, 2);
    b.ellipse(50, 32, 12, 12, 1);
    b.ellipse(50, 32, 5, 5, 2);
    b.line(14, 32, 34, 16, 3, 2);
    b.line(34, 16, 50, 32, 3, 2);
    b.line(28, 16, 28, 28, 3, 2);
    b.rect(24, 8, 10, 4, 4);
    b.ellipse(30, 14, 5, 6, 5);
    b.outline(6, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(24, 64);
    b.rect(10, 16, 4, 46, 1);
    b.rect(4, 8, 16, 12, 2);
    b.rect(6, 10, 12, 8, 3);
    b.outline(4, false);
    return b;
}

Bitmap treeArt() {
    Bitmap b(40, 64);
    b.rect(17, 36, 6, 26, 1);
    b.ellipse(20, 24, 16, 18, 2);
    b.ellipse(14, 22, 8, 8, 3);
    b.outline(4, false);
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
    const uint16_t ink = gs::rgb4(15, 15, 15);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(10, 10, 12), gs::rgb4(15, 15, 15), gs::rgb4(15, 4, 3), gs::rgb4(3, 13, 5),
                          gs::rgb4(15, 12, 3), gs::rgb4(4, 8, 15), gs::rgb4(5, 5, 6), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 2), gs::rgb4(12, 8, 1), gs::rgb4(15, 15, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 3, 2), gs::rgb4(10, 1, 1), gs::rgb4(15, 12, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 6), gs::rgb4(3, 10, 3), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    setPal(vdp, PAL_MUSH, {0, gs::rgb4(14, 3, 2), gs::rgb4(10, 1, 1), gs::rgb4(15, 13, 8), gs::rgb4(15, 14, 11),
                           gs::rgb4(12, 8, 5), gs::rgb4(2, 1, 1), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 9), gs::rgb4(12, 12, 12), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CART, {0, gs::rgb4(10, 6, 2), gs::rgb4(13, 9, 4), gs::rgb4(6, 4, 2), gs::rgb4(3, 3, 4), gs::rgb4(9, 9, 10),
                           gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BIKE, {0, gs::rgb4(2, 2, 3), gs::rgb4(7, 8, 9), gs::rgb4(12, 6, 2), gs::rgb4(4, 4, 4), gs::rgb4(14, 12, 8),
                           gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_POST, {0, gs::rgb4(12, 11, 8), gs::rgb4(15, 12, 2), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), gs::rgb4(3, 8, 2),
                           gs::rgb4(2, 6, 2), gs::rgb4(5, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t field[16] = {
        0,
        gs::rgb4(8, 10, 4), gs::rgb4(6, 8, 3), gs::rgb4(5, 4, 2),
        gs::rgb4(7, 8, 3), gs::rgb4(5, 6, 2),
        gs::rgb4(11, 9, 5), gs::rgb4(8, 6, 3),
        gs::rgb4(5, 4, 3), gs::rgb4(6, 5, 2), gs::rgb4(4, 3, 2),
        gs::rgb4(4, 7, 10), gs::rgb4(3, 6, 9), gs::rgb4(10, 12, 13),
        gs::rgb4(14, 12, 6), gs::rgb4(12, 10, 6),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FIELD * 16 + i, field[i]);

    loadFont(vdp, art);
    art.mush[0] = gs::uploadMipped(vdp, mushBody(0));
    art.mush[1] = gs::uploadMipped(vdp, mushBody(1));
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.cart = gs::uploadMipped(vdp, cartArt());
    art.bike = gs::uploadMipped(vdp, bikeArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
}

}  // namespace mush
