#include "game/art.h"

#include <initializer_list>
#include <string>

namespace pcol {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
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

Bitmap wagonArt() {
    Bitmap b(40, 56);
    b.rect(8, 4, 24, 8, 3);
    b.rect(6, 12, 28, 18, 2);
    b.rect(10, 16, 8, 6, 6);
    b.rect(22, 16, 8, 6, 6);
    b.rect(4, 30, 32, 10, 1);
    b.rect(8, 33, 24, 3, 4);
    b.ellipse(12, 46, 5, 5, 7);
    b.ellipse(28, 46, 5, 5, 7);
    b.ellipse(12, 46, 2, 2, 8);
    b.ellipse(28, 46, 2, 2, 8);
    b.outline(8, false);
    return b;
}

Bitmap cartArt() {
    Bitmap b(36, 32);
    b.rect(6, 6, 24, 8, 2);
    b.rect(10, 8, 6, 4, 6);
    b.rect(18, 8, 6, 4, 6);
    b.rect(4, 14, 28, 8, 1);
    b.ellipse(10, 26, 4, 4, 7);
    b.ellipse(26, 26, 4, 4, 7);
    b.outline(8, false);
    return b;
}

Bitmap stakeArt() {
    Bitmap b(18, 64);
    b.poly({{9, 2}, {3, 16}, {3, 62}, {15, 62}, {15, 16}}, 1);
    b.rect(6, 18, 6, 40, 2);
    b.rect(5, 28, 8, 3, 3);
    b.rect(5, 42, 8, 3, 3);
    b.outline(4, false);
    return b;
}

Bitmap boomArt() {
    Bitmap b(16, 8);
    b.rect(0, 1, 16, 6, 1);
    b.rect(0, 3, 16, 2, 2);
    return b;
}

Bitmap tipArt() {
    Bitmap b(10, 10);
    b.rect(0, 0, 10, 10, 1);
    b.rect(2, 2, 6, 6, 2);
    return b;
}

Bitmap sentryArt() {
    Bitmap b(16, 28);
    b.ellipse(8, 6, 4, 4, 3);
    b.rect(5, 11, 6, 10, 1);
    b.rect(11, 12, 3, 8, 2);
    b.rect(5, 21, 2, 6, 4);
    b.rect(9, 21, 2, 6, 4);
    return b;
}

Bitmap treeArt() {
    Bitmap b(24, 40);
    b.poly({{12, 2}, {2, 22}, {22, 22}}, 2);
    b.poly({{12, 10}, {4, 28}, {20, 28}}, 3);
    b.rect(10, 26, 4, 12, 4);
    return b;
}

Bitmap dustArt() {
    Bitmap b(14, 12);
    b.ellipse(7, 6, 6, 4, 1);
    b.ellipse(7, 6, 3, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 10), gs::rgb4(8, 7, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 4), gs::rgb4(10, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 14, 6), gs::rgb4(3, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_WAGON,
           {0, gs::rgb4(6, 5, 3), gs::rgb4(9, 7, 4), gs::rgb4(12, 10, 6), gs::rgb4(4, 3, 2), gs::rgb4(8, 8, 7),
            gs::rgb4(5, 7, 9), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CART,
           {0, gs::rgb4(8, 4, 3), gs::rgb4(12, 7, 4), gs::rgb4(14, 12, 8), gs::rgb4(5, 3, 2), gs::rgb4(7, 7, 6),
            gs::rgb4(10, 12, 13), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3), gs::rgb4(5, 3, 1), gs::rgb4(3, 2, 1), gs::rgb4(13, 10, 5)});
    setPal(vdp, PAL_BOOM, {0, gs::rgb4(10, 6, 2), gs::rgb4(14, 9, 3), gs::rgb4(6, 3, 1)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 5, 2), gs::rgb4(3, 8, 3), gs::rgb4(5, 11, 4), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(10, 9, 6), gs::rgb4(13, 12, 8)});
    setPal(vdp, PAL_SENTRY, {0, gs::rgb4(4, 6, 8), gs::rgb4(9, 8, 4), gs::rgb4(12, 9, 6), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(4, 6, 2), gs::rgb4(3, 5, 2), gs::rgb4(6, 7, 3), gs::rgb4(5, 4, 2), gs::rgb4(4, 3, 2),
            gs::rgb4(7, 6, 4), gs::rgb4(6, 5, 3), gs::rgb4(8, 7, 5), gs::rgb4(5, 4, 3), gs::rgb4(9, 8, 5),
            gs::rgb4(3, 5, 7), gs::rgb4(4, 6, 8), gs::rgb4(6, 8, 10), gs::rgb4(12, 10, 4), gs::rgb4(9, 8, 6)});
    loadFont(vdp, art);
    art.wagon = gs::uploadMipped(vdp, wagonArt());
    art.cart = gs::uploadMipped(vdp, cartArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.boom = gs::uploadMipped(vdp, boomArt());
    art.tip = gs::uploadMipped(vdp, tipArt());
    art.sentry = gs::uploadMipped(vdp, sentryArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
}

}  // namespace pcol
