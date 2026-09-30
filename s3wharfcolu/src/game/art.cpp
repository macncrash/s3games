#include "game/art.h"

#include <initializer_list>
#include <string>

namespace whc {
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
    Bitmap b(44, 58);
    b.rect(10, 4, 20, 12, 2);
    b.rect(14, 6, 12, 6, 6);
    b.rect(6, 16, 32, 14, 1);
    b.rect(8, 18, 12, 8, 3);
    b.rect(22, 18, 12, 8, 4);
    b.rect(4, 30, 36, 12, 5);
    b.rect(8, 32, 28, 4, 8);
    b.ellipse(14, 48, 6, 6, 7);
    b.ellipse(32, 48, 6, 6, 7);
    b.ellipse(14, 48, 2, 2, 5);
    b.ellipse(32, 48, 2, 2, 5);
    return b;
}

Bitmap cartArt() {
    Bitmap b(32, 36);
    b.rect(4, 8, 22, 8, 2);
    b.rect(6, 10, 8, 4, 3);
    b.rect(4, 16, 24, 8, 1);
    b.ellipse(10, 28, 4, 4, 7);
    b.ellipse(22, 28, 4, 4, 7);
    b.rect(24, 4, 3, 14, 5);
    return b;
}

Bitmap shedArt() {
    Bitmap b(52, 72);
    b.poly({{4, 22}, {26, 4}, {48, 22}}, 2);
    b.rect(8, 22, 36, 46, 1);
    b.rect(14, 30, 10, 14, 3);
    b.rect(28, 30, 10, 14, 6);
    b.rect(20, 50, 12, 16, 4);
    b.rect(22, 12, 8, 10, 5);
    return b;
}

Bitmap lampArt() {
    Bitmap b(16, 28);
    b.rect(6, 10, 4, 16, 2);
    b.ellipse(8, 6, 6, 5, 1);
    b.ellipse(8, 6, 3, 2, 3);
    return b;
}

Bitmap hookArt() {
    Bitmap b(16, 18);
    b.rect(6, 0, 4, 8, 1);
    b.poly({{4, 8}, {12, 8}, {10, 16}, {6, 16}}, 2);
    return b;
}

Bitmap chainArt() {
    Bitmap b(18, 8);
    b.ellipse(5, 4, 4, 3, 1);
    b.ellipse(13, 4, 4, 3, 1);
    b.rect(6, 3, 6, 2, 2);
    return b;
}

Bitmap stripeArt() {
    Bitmap b(10, 16);
    b.rect(2, 0, 6, 16, 1);
    b.rect(3, 2, 4, 4, 2);
    b.rect(3, 9, 4, 4, 2);
    return b;
}

Bitmap crateArt() {
    Bitmap b(28, 28);
    b.rect(2, 4, 24, 20, 1);
    b.line(2, 4, 26, 24, 2, 2);
    b.line(26, 4, 2, 24, 2, 2);
    b.rect(12, 2, 4, 6, 3);
    return b;
}

Bitmap buoyArt() {
    Bitmap b(18, 32);
    b.ellipse(9, 10, 7, 7, 1);
    b.rect(8, 16, 2, 10, 2);
    b.ellipse(9, 28, 4, 2, 3);
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
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(13, 14, 15), gs::rgb4(4, 5, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SALT, {0, gs::rgb4(14, 12, 6), gs::rgb4(6, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 2), gs::rgb4(8, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 14, 8), gs::rgb4(2, 6, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LORRY, {0, gs::rgb4(4, 6, 9), gs::rgb4(2, 4, 6), gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 3), gs::rgb4(3, 3, 3),
                            gs::rgb4(10, 12, 14), gs::rgb4(1, 1, 1), gs::rgb4(14, 12, 4), ink});
    setPal(vdp, PAL_CART, {0, gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 3), gs::rgb4(14, 11, 5), gs::rgb4(5, 3, 1), gs::rgb4(3, 2, 1),
                           gs::rgb4(13, 14, 12), gs::rgb4(1, 1, 1), ink});
    setPal(vdp, PAL_SHED, {0, gs::rgb4(7, 7, 8), gs::rgb4(4, 5, 6), gs::rgb4(2, 3, 5), gs::rgb4(5, 3, 2), gs::rgb4(10, 9, 6),
                           gs::rgb4(12, 13, 14), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 4), gs::rgb4(6, 5, 4), gs::rgb4(15, 15, 10), ink});
    setPal(vdp, PAL_CRATE, {0, gs::rgb4(10, 6, 2), gs::rgb4(6, 3, 1), gs::rgb4(3, 2, 1), ink});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(10, 11, 12), gs::rgb4(6, 7, 8), ink});
    setPal(vdp, PAL_CHAIN, {0, gs::rgb4(9, 9, 8), gs::rgb4(4, 4, 5), ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(2, 5, 6),
        gs::rgb4(1, 3, 5),
        gs::rgb4(3, 6, 7),
        gs::rgb4(5, 5, 5),
        gs::rgb4(3, 3, 4),
        gs::rgb4(6, 6, 5),
        gs::rgb4(4, 4, 3),
        gs::rgb4(7, 7, 6),
        gs::rgb4(2, 2, 3),
        gs::rgb4(8, 7, 5),
        gs::rgb4(1, 4, 8),
        gs::rgb4(2, 6, 10),
        gs::rgb4(4, 8, 11),
        gs::rgb4(14, 13, 6),
        gs::rgb4(6, 6, 6),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.lorry = gs::uploadMipped(vdp, lorryArt());
    art.cart = gs::uploadMipped(vdp, cartArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.chain = gs::uploadMipped(vdp, chainArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
}

}  // namespace whc
