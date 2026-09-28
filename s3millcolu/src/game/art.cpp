#include "game/art.h"

#include <initializer_list>
#include <string>

namespace millc {
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

Bitmap lorryArt() {
    Bitmap b(40, 56);
    b.rect(8, 4, 22, 12, 2);
    b.rect(12, 6, 14, 6, 6);
    b.rect(6, 16, 28, 18, 1);
    b.rect(10, 20, 8, 8, 3);
    b.rect(20, 20, 10, 8, 3);
    b.rect(4, 34, 32, 10, 4);
    b.ellipse(12, 48, 5, 5, 7);
    b.ellipse(28, 48, 5, 5, 7);
    b.ellipse(12, 48, 2, 2, 5);
    b.ellipse(28, 48, 2, 2, 5);
    b.rect(2, 30, 4, 12, 5);
    return b;
}

Bitmap cartArt() {
    Bitmap b(36, 40);
    b.rect(6, 6, 24, 10, 2);
    b.rect(8, 8, 8, 6, 3);
    b.rect(18, 8, 8, 6, 6);
    b.rect(4, 16, 28, 10, 1);
    b.rect(8, 18, 20, 4, 4);
    b.ellipse(10, 32, 5, 5, 7);
    b.ellipse(26, 32, 5, 5, 7);
    b.rect(14, 2, 3, 6, 5);
    return b;
}

Bitmap millArt() {
    Bitmap b(48, 80);
    b.poly({{10, 78}, {16, 28}, {32, 28}, {38, 78}}, 1);
    b.rect(18, 36, 12, 16, 3);
    b.rect(20, 40, 8, 8, 6);
    b.rect(20, 58, 8, 14, 4);
    b.rect(14, 22, 20, 8, 2);
    b.rect(20, 8, 8, 16, 5);
    b.ellipse(24, 8, 6, 4, 2);
    return b;
}

Bitmap sailPlus() {
    Bitmap b(48, 48);
    b.rect(22, 2, 4, 44, 1);
    b.rect(2, 22, 44, 4, 1);
    b.rect(22, 6, 4, 10, 2);
    b.rect(22, 32, 4, 10, 2);
    b.rect(6, 22, 10, 4, 2);
    b.rect(32, 22, 10, 4, 2);
    return b;
}

Bitmap sailCross() {
    Bitmap b(48, 48);
    b.line(6, 6, 42, 42, 1, 4);
    b.line(42, 6, 6, 42, 1, 4);
    b.line(12, 10, 24, 22, 2, 2);
    b.line(36, 10, 26, 20, 2, 2);
    return b;
}

Bitmap capArt() {
    Bitmap b(20, 12);
    b.poly({{2, 10}, {10, 1}, {18, 10}}, 1);
    b.rect(8, 8, 4, 4, 2);
    return b;
}

Bitmap timberArt() {
    Bitmap b(18, 8);
    b.rect(0, 1, 18, 6, 1);
    b.rect(0, 3, 18, 2, 2);
    return b;
}

Bitmap stripeArt() {
    Bitmap b(12, 14);
    b.rect(2, 0, 8, 14, 1);
    b.rect(4, 2, 4, 4, 2);
    b.rect(4, 8, 4, 4, 2);
    return b;
}

Bitmap treeArt() {
    Bitmap b(28, 44);
    b.ellipse(14, 14, 12, 12, 2);
    b.ellipse(14, 16, 6, 6, 3);
    b.rect(12, 24, 4, 18, 4);
    return b;
}

Bitmap dustArt() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 7, 4, 1);
    b.ellipse(8, 6, 3, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 9), gs::rgb4(6, 5, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WHEAT, {0, gs::rgb4(15, 12, 4), gs::rgb4(8, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 2), gs::rgb4(7, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 14, 5), gs::rgb4(2, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LORRY, {0, gs::rgb4(6, 7, 3), gs::rgb4(4, 5, 2), gs::rgb4(8, 8, 4), gs::rgb4(3, 3, 2), gs::rgb4(9, 9, 6),
                            gs::rgb4(12, 12, 8), gs::rgb4(1, 1, 1), gs::rgb4(7, 6, 4), ink});
    setPal(vdp, PAL_CART, {0, gs::rgb4(9, 5, 2), gs::rgb4(12, 8, 3), gs::rgb4(14, 12, 6), gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 1),
                           gs::rgb4(2, 2, 1), gs::rgb4(13, 14, 12), gs::rgb4(1, 1, 1), ink});
    setPal(vdp, PAL_MILL, {0, gs::rgb4(8, 7, 5), gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 4), gs::rgb4(6, 3, 2), gs::rgb4(4, 4, 3),
                           gs::rgb4(11, 12, 13), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_SAIL, {0, gs::rgb4(13, 12, 8), gs::rgb4(7, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TREE, {0, 0, gs::rgb4(2, 6, 2), gs::rgb4(4, 8, 3), gs::rgb4(5, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(11, 9, 6), gs::rgb4(7, 6, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TIMBER, {0, gs::rgb4(11, 7, 3), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(3, 6, 2),
        gs::rgb4(2, 4, 2),
        gs::rgb4(5, 7, 3),
        gs::rgb4(5, 5, 3),
        gs::rgb4(3, 4, 2),
        gs::rgb4(6, 5, 3),
        gs::rgb4(4, 3, 2),
        gs::rgb4(5, 5, 4),
        gs::rgb4(3, 3, 2),
        gs::rgb4(7, 6, 4),
        gs::rgb4(2, 3, 4),
        gs::rgb4(3, 4, 5),
        gs::rgb4(4, 5, 6),
        gs::rgb4(14, 12, 4),
        gs::rgb4(7, 6, 5),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.lorry = gs::uploadMipped(vdp, lorryArt());
    art.cart = gs::uploadMipped(vdp, cartArt());
    art.mill = gs::uploadMipped(vdp, millArt());
    art.sailPlus = gs::uploadMipped(vdp, sailPlus());
    art.sailCross = gs::uploadMipped(vdp, sailCross());
    art.cap = gs::uploadMipped(vdp, capArt());
    art.timber = gs::uploadMipped(vdp, timberArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
}

}  // namespace millc
