#include "game/art.h"

#include <string>

namespace colu {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; ++i) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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

gs::Bitmap carArt() {
    gs::Bitmap b(44, 34);
    b.rect(12, 2, 20, 6, 1);
    b.rect(10, 6, 24, 7, 4);
    b.rect(12, 7, 9, 5, 5);
    b.rect(23, 7, 9, 5, 5);
    b.rect(21, 7, 2, 5, 3);
    b.rect(8, 13, 28, 6, 2);
    b.rect(10, 13, 24, 2, 1);
    b.ellipse(11, 18, 3, 3, 8);
    b.ellipse(33, 18, 3, 3, 8);
    b.rect(16, 16, 12, 5, 6);
    b.rect(6, 20, 32, 4, 9);
    b.rect(8, 21, 28, 1, 7);
    b.rect(4, 17, 5, 12, 10);
    b.rect(35, 17, 5, 12, 10);
    b.rect(5, 24, 3, 3, 11);
    b.rect(36, 24, 3, 3, 11);
    b.rect(7, 19, 3, 2, 12);
    b.rect(34, 19, 3, 2, 12);
    b.outline(15, false);
    return b;
}

gs::Bitmap vanArt() {
    gs::Bitmap b(46, 48);
    b.rect(8, 2, 30, 5, 6);
    b.rect(7, 7, 32, 12, 4);
    b.rect(10, 9, 12, 8, 5);
    b.rect(24, 9, 12, 8, 5);
    b.rect(22, 9, 2, 8, 3);
    b.rect(6, 19, 34, 14, 2);
    b.rect(6, 22, 34, 3, 13);
    b.rect(8, 19, 30, 2, 1);
    b.rect(14, 26, 18, 6, 7);
    b.ellipse(12, 30, 4, 4, 8);
    b.ellipse(34, 30, 4, 4, 8);
    b.rect(5, 34, 36, 4, 9);
    b.rect(4, 28, 5, 14, 10);
    b.rect(37, 28, 5, 14, 10);
    b.rect(5, 38, 3, 3, 11);
    b.rect(38, 38, 3, 3, 11);
    b.rect(8, 33, 4, 2, 12);
    b.rect(34, 33, 4, 2, 12);
    b.outline(15, false);
    return b;
}

gs::Bitmap truckArt() {
    gs::Bitmap b(52, 58);
    b.rect(12, 1, 28, 8, 1);
    b.rect(14, 3, 24, 3, 14);
    b.rect(16, 8, 20, 2, 13);
    b.rect(10, 10, 32, 12, 4);
    b.rect(13, 12, 11, 8, 5);
    b.rect(28, 12, 11, 8, 5);
    b.rect(24, 12, 4, 8, 3);
    b.rect(8, 22, 36, 7, 2);
    b.rect(10, 22, 32, 2, 1);
    b.rect(14, 29, 24, 10, 6);
    for (int y = 31; y <= 36; y += 3) b.rect(16, y, 20, 1, 7);
    b.ellipse(12, 33, 4, 4, 8);
    b.ellipse(40, 33, 4, 4, 8);
    b.ellipse(12, 33, 2, 2, 14);
    b.ellipse(40, 33, 2, 2, 14);
    b.rect(6, 39, 40, 5, 9);
    b.rect(8, 40, 36, 2, 7);
    b.rect(3, 32, 6, 18, 10);
    b.rect(43, 32, 6, 18, 10);
    b.rect(4, 44, 4, 4, 11);
    b.rect(44, 44, 4, 4, 11);
    b.rect(8, 37, 4, 2, 12);
    b.rect(40, 37, 4, 2, 12);
    b.rect(23, 42, 6, 2, 12);
    b.outline(15, false);
    return b;
}

gs::Bitmap boothArt() {
    gs::Bitmap b(48, 70);
    b.rect(4, 8, 40, 6, 5);
    b.rect(2, 12, 44, 4, 5);
    b.rect(6, 16, 36, 40, 2);
    b.rect(8, 18, 32, 36, 1);
    b.rect(12, 22, 24, 16, 10);
    b.rect(14, 24, 20, 12, 4);
    b.ellipse(24, 28, 4, 4, 7);
    b.rect(18, 32, 12, 5, 6);
    b.rect(14, 42, 20, 12, 3);
    b.rect(30, 46, 2, 2, 7);
    b.rect(4, 56, 40, 8, 8);
    b.rect(6, 58, 10, 5, 9);
    b.rect(18, 58, 12, 5, 9);
    b.rect(32, 58, 10, 5, 9);
    b.outline(15, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(16, 52);
    b.rect(5, 2, 6, 44, 2);
    b.rect(6, 2, 2, 44, 1);
    b.rect(4, 8, 8, 4, 4);
    b.rect(4, 12, 8, 4, 5);
    b.rect(4, 16, 8, 4, 4);
    b.rect(3, 4, 10, 6, 6);
    b.rect(2, 44, 12, 6, 7);
    b.outline(15, false);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2, 2, 1);
    b.outline(15, false);
    return b;
}

gs::Bitmap broadTree() {
    gs::Bitmap b(40, 48);
    b.ellipse(20, 18, 16, 14, 2);
    b.ellipse(14, 20, 9, 8, 1);
    b.ellipse(26, 16, 8, 7, 3);
    b.rect(17, 30, 6, 14, 4);
    b.rect(18, 32, 2, 12, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap cypressArt() {
    gs::Bitmap b(24, 56);
    b.ellipse(12, 26, 8, 20, 2);
    b.ellipse(12, 16, 5, 12, 1);
    b.ellipse(12, 34, 6, 10, 3);
    b.rect(10, 44, 4, 10, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap signArt() {
    gs::Bitmap b(36, 40);
    b.rect(16, 18, 4, 18, 4);
    b.rect(4, 2, 28, 18, 2);
    b.rect(7, 6, 22, 3, 3);
    b.rect(7, 12, 14, 3, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap pennantArt() {
    gs::Bitmap b(16, 18);
    b.rect(2, 1, 2, 16, 4);
    b.rect(4, 2, 10, 6, 1);
    b.rect(4, 2, 10, 2, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 12);
    b.ellipse(8, 7, 7, 4, 4);
    b.ellipse(8, 6, 4, 3, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 9, 3, 1);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(32, 16);
    b.ellipse(16, 9, 14, 5, 2);
    b.ellipse(11, 8, 7, 4, 1);
    b.ellipse(21, 7, 6, 4, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 5);
    b.ellipse(8, 8, 3, 3, 6);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_TEXT, gs::rgb4(14, 14, 13));
    textPal(vdp, PAL_GOLD, gs::rgb4(14, 11, 3));
    textPal(vdp, PAL_ALERT, gs::rgb4(14, 3, 2));
    textPal(vdp, PAL_GOOD, gs::rgb4(4, 13, 5));

    setPal(vdp, PAL_CAR,
           {0, gs::rgb4(14, 13, 11), gs::rgb4(12, 10, 8), gs::rgb4(6, 5, 4), gs::rgb4(3, 5, 7), gs::rgb4(8, 10, 12),
            gs::rgb4(4, 4, 4), gs::rgb4(9, 9, 8), gs::rgb4(15, 15, 10), gs::rgb4(5, 5, 5), gs::rgb4(1, 1, 1),
            gs::rgb4(8, 8, 7), gs::rgb4(13, 5, 1), gs::rgb4(6, 8, 10), gs::rgb4(14, 14, 12), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_VAN,
           {0, gs::rgb4(8, 10, 14), gs::rgb4(4, 6, 11), gs::rgb4(2, 3, 7), gs::rgb4(3, 5, 8), gs::rgb4(7, 10, 13),
            gs::rgb4(13, 13, 14), gs::rgb4(2, 2, 3), gs::rgb4(15, 15, 11), gs::rgb4(7, 7, 8), gs::rgb4(1, 1, 1),
            gs::rgb4(8, 8, 7), gs::rgb4(14, 6, 2), gs::rgb4(10, 12, 15), gs::rgb4(12, 13, 15), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_TRUCK,
           {0, gs::rgb4(9, 10, 5), gs::rgb4(6, 7, 3), gs::rgb4(3, 4, 2), gs::rgb4(2, 3, 4), gs::rgb4(6, 8, 9),
            gs::rgb4(2, 2, 1), gs::rgb4(5, 5, 4), gs::rgb4(15, 14, 8), gs::rgb4(3, 3, 2), gs::rgb4(1, 1, 1),
            gs::rgb4(6, 6, 5), gs::rgb4(12, 7, 2), gs::rgb4(8, 8, 4), gs::rgb4(11, 12, 6), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_POST,
           {0, gs::rgb4(7, 7, 7), gs::rgb4(4, 4, 4), gs::rgb4(2, 2, 2), gs::rgb4(12, 2, 1), gs::rgb4(14, 14, 13),
            gs::rgb4(3, 3, 3), gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 5), gs::rgb4(6, 6, 6), gs::rgb4(1, 1, 1),
            gs::rgb4(9, 9, 8), gs::rgb4(10, 4, 2), gs::rgb4(4, 4, 5), gs::rgb4(12, 12, 11), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BOOTH,
           {0, gs::rgb4(11, 8, 5), gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(6, 9, 11), gs::rgb4(6, 2, 2),
            gs::rgb4(3, 4, 3), gs::rgb4(13, 9, 6), gs::rgb4(9, 8, 5), gs::rgb4(6, 5, 3), gs::rgb4(2, 2, 2),
            gs::rgb4(1, 1, 2), gs::rgb4(12, 10, 7), gs::rgb4(5, 4, 3), gs::rgb4(10, 8, 6), gs::rgb4(14, 12, 9),
            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_TREE,
           {0, gs::rgb4(8, 10, 3), gs::rgb4(4, 6, 2), gs::rgb4(2, 4, 2), gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1),
            gs::rgb4(8, 7, 3), gs::rgb4(5, 6, 2), gs::rgb4(3, 3, 2), gs::rgb4(9, 8, 4), gs::rgb4(1, 2, 1),
            gs::rgb4(7, 5, 3), gs::rgb4(4, 5, 2), gs::rgb4(10, 9, 4), gs::rgb4(2, 3, 1), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(14, 14, 13), gs::rgb4(10, 11, 12), gs::rgb4(12, 11, 8), gs::rgb4(8, 7, 5), gs::rgb4(15, 12, 5),
            gs::rgb4(15, 15, 11), gs::rgb4(9, 8, 6), gs::rgb4(6, 5, 4), gs::rgb4(13, 12, 9), gs::rgb4(4, 4, 3),
            gs::rgb4(11, 10, 8), gs::rgb4(7, 6, 4), gs::rgb4(14, 13, 10), gs::rgb4(5, 5, 4), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(13, 2, 1), gs::rgb4(8, 1, 1), gs::rgb4(15, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WHITE, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 12, 11), gs::rgb4(9, 9, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_FLAG,
           {0, gs::rgb4(14, 12, 2), gs::rgb4(12, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(5, 4, 3), gs::rgb4(2, 2, 1),
            gs::rgb4(10, 8, 2), gs::rgb4(8, 1, 1), gs::rgb4(13, 13, 12), gs::rgb4(6, 5, 3), gs::rgb4(1, 1, 1),
            gs::rgb4(11, 9, 3), gs::rgb4(4, 3, 2), gs::rgb4(14, 10, 4), gs::rgb4(7, 6, 4), gs::rgb4(1, 1, 1)});

    int r = PAL_ROAD * 16;
    vdp.setColor(r + 0, 0);
    vdp.setColor(r + 1, gs::rgb4(8, 9, 4));
    vdp.setColor(r + 2, gs::rgb4(5, 6, 3));
    vdp.setColor(r + 3, gs::rgb4(10, 9, 5));
    vdp.setColor(r + 4, gs::rgb4(9, 8, 5));
    vdp.setColor(r + 5, gs::rgb4(6, 5, 3));
    vdp.setColor(r + 6, gs::rgb4(3, 3, 3));
    vdp.setColor(r + 7, gs::rgb4(5, 5, 5));
    vdp.setColor(r + 8, gs::rgb4(7, 6, 4));
    vdp.setColor(r + 9, gs::rgb4(2, 2, 2));
    vdp.setColor(r + 10, gs::rgb4(4, 4, 4));
    vdp.setColor(r + 11, gs::rgb4(3, 5, 7));
    vdp.setColor(r + 12, gs::rgb4(4, 6, 8));
    vdp.setColor(r + 13, gs::rgb4(8, 10, 12));
    vdp.setColor(r + 14, gs::rgb4(13, 12, 7));
    vdp.setColor(r + 15, gs::rgb4(7, 7, 6));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.car = gs::uploadMipped(vdp, carArt());
    art.van = gs::uploadMipped(vdp, vanArt());
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.booth = gs::uploadMipped(vdp, boothArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.tree[0] = gs::uploadMipped(vdp, broadTree());
    art.tree[1] = gs::uploadMipped(vdp, cypressArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
}

}  // namespace colu
