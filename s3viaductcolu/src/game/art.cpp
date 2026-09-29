#include "game/art.h"

#include <initializer_list>
#include <string>

namespace vcol {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap lorryArt() {
    Bitmap b(72, 64);
    b.poly({{8, 18}, {64, 18}, {68, 28}, {6, 28}}, 2);
    b.rect(8, 26, 56, 16, 3);
    b.rect(12, 30, 14, 8, 6);
    b.rect(30, 30, 14, 8, 1);
    b.rect(10, 42, 52, 6, 4);
    b.ellipse(20, 52, 7, 7, 5);
    b.ellipse(52, 52, 7, 7, 5);
    b.ellipse(20, 52, 3, 3, 7);
    b.ellipse(52, 52, 3, 3, 7);
    b.rect(4, 40, 6, 4, 8);
    b.rect(62, 40, 6, 4, 8);
    b.outline(15, false);
    return b;
}

Bitmap pennantArt() {
    Bitmap b(22, 28);
    b.rect(4, 2, 3, 24, 3);
    b.poly({{7, 4}, {18, 10}, {7, 16}}, 2);
    b.outline(15, false);
    return b;
}

Bitmap chainArt() {
    Bitmap b(64, 16);
    b.rect(2, 4, 60, 8, 1);
    b.rect(2, 4, 60, 3, 2);
    for (int i = 0; i < 6; ++i) b.rect(6 + i * 10, 5, 4, 6, 3);
    b.outline(15, false);
    return b;
}

Bitmap sentryArt() {
    Bitmap b(32, 56);
    b.ellipse(16, 8, 6, 6, 2);
    b.rect(10, 3, 12, 4, 3);
    b.poly({{16, 14}, {7, 22}, {6, 38}, {26, 38}, {25, 22}}, 1);
    b.rect(12, 20, 8, 8, 4);
    b.rect(22, 22, 8, 3, 6);
    b.rect(9, 38, 5, 14, 5);
    b.rect(18, 38, 5, 14, 5);
    b.outline(15, false);
    return b;
}

Bitmap pierArt() {
    Bitmap b(36, 96);
    b.poly({{6, 92}, {8, 28}, {18, 8}, {28, 28}, {30, 92}}, 2);
    b.poly({{12, 78}, {14, 40}, {22, 32}, {24, 78}}, 3);
    b.rect(10, 86, 16, 6, 4);
    b.rect(14, 18, 8, 6, 1);
    b.outline(15, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(16, 40);
    b.rect(7, 10, 3, 28, 3);
    b.ellipse(8, 8, 6, 5, 2);
    b.ellipse(8, 8, 3, 2, 1);
    b.outline(15, false);
    return b;
}

Bitmap railArt() {
    Bitmap b(28, 18);
    b.rect(2, 6, 24, 4, 2);
    b.rect(4, 10, 3, 6, 3);
    b.rect(21, 10, 3, 6, 3);
    b.outline(15, false);
    return b;
}

Bitmap smokeArt() {
    Bitmap b(18, 14);
    b.ellipse(9, 7, 7, 4, 1);
    b.ellipse(6, 6, 3, 2, 2);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(26, 10);
    b.ellipse(13, 5, 11, 3, 1);
    return b;
}

Bitmap stripeArt() {
    Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 1);
    gs::TextStyle big{2, 1, 0, 0, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        if (g) {
            for (int row = 0; row < 7; ++row)
                for (int col = 0; col < 5; ++col)
                    if (g[row] & (1 << (4 - col))) px[row * 8 + col] = 1;
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(13, 14, 15), gs::rgb4(7, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MIST, {0, gs::rgb4(12, 13, 14), gs::rgb4(6, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 12, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 14, 8), gs::rgb4(13, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LORRY, {0, gs::rgb4(12, 13, 12), gs::rgb4(7, 8, 6), gs::rgb4(4, 5, 4), gs::rgb4(3, 3, 3),
                            gs::rgb4(1, 1, 1), gs::rgb4(8, 10, 12), gs::rgb4(14, 14, 12), gs::rgb4(12, 3, 2), 0, 0, 0,
                            0, 0, 0, ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(10, 10, 11), gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5), gs::rgb4(12, 11, 10),
                            gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(9, 8, 7), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0,
                            0, ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 12, 4), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4), 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CHAIN, {0, gs::rgb4(14, 3, 2), gs::rgb4(15, 14, 11), gs::rgb4(8, 8, 9), gs::rgb4(2, 2, 3), 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(5, 7, 9), gs::rgb4(12, 11, 9), gs::rgb4(3, 3, 4), gs::rgb4(9, 12, 14),
                           gs::rgb4(2, 2, 3), gs::rgb4(14, 12, 4), gs::rgb4(15, 8, 3), gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(11, 12, 13), gs::rgb4(7, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(4, 8, 11), gs::rgb4(2, 5, 8), gs::rgb4(8, 12, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(5, 6, 7),
        gs::rgb4(3, 4, 5),
        gs::rgb4(6, 7, 8),
        gs::rgb4(8, 8, 8),
        gs::rgb4(5, 5, 6),
        gs::rgb4(4, 4, 5),
        gs::rgb4(3, 3, 4),
        gs::rgb4(7, 7, 6),
        gs::rgb4(5, 5, 4),
        gs::rgb4(6, 6, 5),
        gs::rgb4(3, 6, 8),
        gs::rgb4(2, 5, 7),
        gs::rgb4(5, 9, 11),
        gs::rgb4(14, 13, 8),
        gs::rgb4(6, 6, 7),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    art.lorry = gs::uploadMipped(vdp, lorryArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.chain = gs::uploadMipped(vdp, chainArt());
    art.sentry = gs::uploadMipped(vdp, sentryArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.rail = gs::uploadMipped(vdp, railArt());
    art.smoke = gs::uploadMipped(vdp, smokeArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    loadFont(vdp, art);
}

}  // namespace vcol
