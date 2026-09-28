#include "game/art.h"

#include <initializer_list>
#include <string>

namespace bcol {
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

Bitmap truckArt() {
    Bitmap b(48, 64);
    b.rect(10, 2, 28, 10, 3);
    b.rect(14, 4, 20, 5, 6);
    b.rect(8, 12, 32, 22, 2);
    b.rect(12, 16, 10, 8, 5);
    b.rect(26, 16, 10, 8, 5);
    b.rect(6, 34, 36, 14, 1);
    b.rect(10, 38, 28, 4, 4);
    b.ellipse(14, 52, 6, 6, 7);
    b.ellipse(34, 52, 6, 6, 7);
    b.ellipse(14, 52, 2, 2, 8);
    b.ellipse(34, 52, 2, 2, 8);
    b.rect(4, 30, 4, 18, 4);
    b.rect(40, 30, 4, 18, 4);
    b.outline(8, false);
    return b;
}

Bitmap carArt() {
    Bitmap b(44, 36);
    b.rect(8, 8, 28, 10, 2);
    b.rect(14, 10, 8, 6, 6);
    b.rect(24, 10, 8, 6, 6);
    b.rect(4, 18, 36, 8, 1);
    b.ellipse(12, 28, 5, 5, 7);
    b.ellipse(32, 28, 5, 5, 7);
    b.ellipse(12, 28, 2, 2, 3);
    b.ellipse(32, 28, 2, 2, 3);
    b.outline(8, false);
    return b;
}

Bitmap bunkerArt() {
    Bitmap b(56, 72);
    b.poly({{4, 70}, {10, 18}, {46, 18}, {52, 70}}, 2);
    b.rect(14, 22, 28, 16, 3);
    b.rect(18, 26, 20, 8, 1);
    b.rect(16, 44, 24, 6, 4);
    b.rect(22, 8, 12, 12, 5);
    b.rect(24, 4, 8, 6, 6);
    b.outline(8, false);
    return b;
}

Bitmap slitArt() {
    Bitmap b(28, 10);
    b.rect(0, 2, 28, 6, 1);
    b.rect(2, 3, 24, 4, 2);
    return b;
}

Bitmap barArt() {
    Bitmap b(16, 10);
    b.rect(0, 1, 16, 8, 1);
    b.rect(0, 3, 16, 4, 2);
    return b;
}

Bitmap stripeArt() {
    Bitmap b(12, 12);
    b.rect(0, 0, 12, 12, 1);
    b.rect(2, 2, 8, 8, 2);
    return b;
}

Bitmap treeArt() {
    Bitmap b(28, 48);
    b.ellipse(14, 14, 12, 12, 2);
    b.ellipse(14, 16, 7, 7, 3);
    b.rect(12, 24, 4, 22, 4);
    return b;
}

Bitmap dustArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 5, 1);
    b.ellipse(8, 8, 3, 2, 2);
    return b;
}

Bitmap lampArt() {
    Bitmap b(10, 16);
    b.rect(4, 6, 2, 10, 2);
    b.ellipse(5, 4, 4, 4, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 6, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(8, 5, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 2), gs::rgb4(6, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 14, 6), gs::rgb4(2, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TRUCK, {0, gs::rgb4(5, 7, 3), gs::rgb4(3, 5, 2), gs::rgb4(7, 8, 4), gs::rgb4(2, 2, 1), gs::rgb4(9, 10, 6),
                            gs::rgb4(12, 13, 10), gs::rgb4(1, 1, 1), gs::rgb4(8, 8, 7), ink});
    setPal(vdp, PAL_CAR, {0, gs::rgb4(10, 4, 3), gs::rgb4(13, 8, 5), gs::rgb4(4, 2, 2), gs::rgb4(2, 2, 2), 0, gs::rgb4(12, 14, 15),
                          gs::rgb4(1, 1, 1), gs::rgb4(6, 6, 6), ink});
    setPal(vdp, PAL_BUNKER, {0, gs::rgb4(4, 4, 4), gs::rgb4(7, 7, 6), gs::rgb4(5, 5, 4), gs::rgb4(9, 8, 5), gs::rgb4(3, 3, 3),
                             gs::rgb4(14, 10, 3), 0, gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_BAR, {0, gs::rgb4(12, 10, 3), gs::rgb4(3, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TREE, {0, 0, gs::rgb4(3, 6, 2), gs::rgb4(5, 8, 3), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(10, 9, 7), gs::rgb4(6, 5, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 4), gs::rgb4(5, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(3, 5, 2),
        gs::rgb4(2, 4, 2),
        gs::rgb4(5, 6, 3),
        gs::rgb4(4, 4, 3),
        gs::rgb4(3, 3, 2),
        gs::rgb4(5, 5, 5),
        gs::rgb4(3, 3, 4),
        gs::rgb4(6, 6, 5),
        gs::rgb4(4, 4, 4),
        gs::rgb4(7, 7, 6),
        gs::rgb4(2, 3, 4),
        gs::rgb4(3, 4, 5),
        gs::rgb4(4, 5, 6),
        gs::rgb4(14, 12, 3),
        gs::rgb4(7, 7, 6),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.car = gs::uploadMipped(vdp, carArt());
    art.bunker = gs::uploadMipped(vdp, bunkerArt());
    art.slit = gs::uploadMipped(vdp, slitArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
}

}  // namespace bcol
