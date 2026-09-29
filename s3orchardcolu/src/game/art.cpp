#include "game/art.h"

#include <initializer_list>
#include <string>

namespace orchard {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap treeArt() {
    Bitmap b(40, 64);
    b.ellipse(20, 18, 16, 14, 2);
    b.ellipse(12, 22, 8, 7, 3);
    b.ellipse(27, 24, 8, 7, 1);
    b.rect(17, 30, 6, 26, 4);
    b.rect(14, 54, 12, 4, 5);
    b.ellipse(12, 12, 2, 2, 6);
    b.ellipse(22, 16, 2, 2, 6);
    b.ellipse(28, 14, 2, 2, 7);
    b.outline(8, false);
    return b;
}

Bitmap binArt() {
    Bitmap b(36, 28);
    b.poly({{3, 8}, {33, 8}, {30, 24}, {6, 24}}, 2);
    b.rect(4, 6, 28, 4, 3);
    b.rect(8, 12, 20, 3, 1);
    b.ellipse(10, 7, 3, 3, 4);
    b.ellipse(18, 6, 3, 3, 5);
    b.ellipse(26, 7, 3, 3, 4);
    b.outline(8, false);
    return b;
}

Bitmap appleArt() {
    Bitmap b(12, 12);
    b.ellipse(6, 7, 4, 4, 1);
    b.rect(5, 1, 2, 3, 2);
    b.outline(8, false);
    return b;
}

Bitmap carArt() {
    Bitmap b(48, 28);
    b.poly({{4, 16}, {10, 10}, {28, 9}, {36, 14}, {44, 16}, {44, 22}, {4, 22}}, 2);
    b.rect(14, 11, 10, 5, 3);
    b.ellipse(12, 22, 4, 4, 4);
    b.ellipse(36, 22, 4, 4, 4);
    b.rect(40, 16, 3, 2, 5);
    b.outline(8, false);
    return b;
}

Bitmap vanArt() {
    Bitmap b(52, 36);
    b.rect(6, 10, 36, 18, 2);
    b.rect(8, 12, 10, 8, 3);
    b.poly({{42, 14}, {48, 18}, {48, 28}, {42, 28}}, 1);
    b.ellipse(14, 28, 4, 4, 4);
    b.ellipse(36, 28, 4, 4, 4);
    b.outline(8, false);
    return b;
}

Bitmap lorryArt() {
    Bitmap b(64, 40);
    b.rect(4, 8, 40, 22, 2);
    b.rect(6, 10, 36, 6, 3);
    b.ellipse(12, 12, 2, 2, 5);
    b.ellipse(22, 12, 2, 2, 6);
    b.ellipse(32, 12, 2, 2, 5);
    b.rect(44, 14, 16, 16, 1);
    b.rect(46, 16, 8, 6, 4);
    b.ellipse(16, 32, 5, 5, 7);
    b.ellipse(36, 32, 5, 5, 7);
    b.ellipse(52, 32, 4, 4, 7);
    b.outline(8, false);
    return b;
}

Bitmap dustArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 5, 1);
    b.ellipse(6, 7, 3, 2, 2);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(28, 8);
    b.ellipse(14, 4, 12, 3, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 13);
    const uint16_t shade = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_TEXT, {0, ink, gs::rgb4(6, 8, 4), gs::rgb4(14, 12, 5), gs::rgb4(12, 6, 3), shade});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), shade});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), shade});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 14, 5), shade});
    setPal(vdp, PAL_CAR, {0, gs::rgb4(8, 10, 12), gs::rgb4(12, 13, 14), gs::rgb4(4, 8, 12), gs::rgb4(2, 2, 2),
                          gs::rgb4(14, 12, 4), shade});
    setPal(vdp, PAL_VAN, {0, gs::rgb4(10, 8, 4), gs::rgb4(13, 11, 6), gs::rgb4(6, 10, 12), gs::rgb4(2, 2, 2), shade});
    setPal(vdp, PAL_LORRY, {0, gs::rgb4(7, 8, 4), gs::rgb4(10, 11, 6), gs::rgb4(13, 12, 8), gs::rgb4(5, 7, 10),
                            gs::rgb4(13, 3, 2), gs::rgb4(14, 10, 3), gs::rgb4(2, 2, 2), shade});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(4, 10, 3), gs::rgb4(2, 7, 2), gs::rgb4(5, 12, 4), gs::rgb4(6, 4, 2),
                           gs::rgb4(4, 3, 1), gs::rgb4(13, 3, 2), gs::rgb4(14, 11, 3), shade});
    setPal(vdp, PAL_BIN, {0, gs::rgb4(9, 6, 3), gs::rgb4(7, 5, 2), gs::rgb4(11, 8, 4), gs::rgb4(13, 3, 2),
                          gs::rgb4(12, 10, 3), shade});
    setPal(vdp, PAL_APPLE, {0, gs::rgb4(13, 3, 2), gs::rgb4(3, 8, 2), shade});
    setPal(vdp, PAL_FX, {0, gs::rgb4(12, 11, 8), gs::rgb4(8, 7, 5), shade});

    int r = PAL_ROAD * 16;
    vdp.setColor(r + 0, 0);
    vdp.setColor(r + 1, gs::rgb4(3, 9, 2));
    vdp.setColor(r + 2, gs::rgb4(2, 6, 2));
    vdp.setColor(r + 3, gs::rgb4(4, 10, 3));
    vdp.setColor(r + 4, gs::rgb4(5, 8, 3));
    vdp.setColor(r + 5, gs::rgb4(3, 6, 2));
    vdp.setColor(r + 6, gs::rgb4(8, 6, 3));
    vdp.setColor(r + 7, gs::rgb4(6, 5, 2));
    vdp.setColor(r + 8, gs::rgb4(5, 4, 2));
    vdp.setColor(r + 9, gs::rgb4(4, 3, 2));
    vdp.setColor(r + 10, gs::rgb4(9, 7, 3));
    vdp.setColor(r + 11, gs::rgb4(3, 5, 6));
    vdp.setColor(r + 12, gs::rgb4(4, 6, 7));
    vdp.setColor(r + 13, gs::rgb4(6, 8, 9));
    vdp.setColor(r + 14, gs::rgb4(12, 11, 6));
    vdp.setColor(r + 15, gs::rgb4(10, 8, 4));

    art.tree = gs::uploadMipped(vdp, treeArt());
    art.bin = gs::uploadMipped(vdp, binArt());
    art.apple = gs::uploadMipped(vdp, appleArt());
    art.car = gs::uploadMipped(vdp, carArt());
    art.van = gs::uploadMipped(vdp, vanArt());
    art.lorry = gs::uploadMipped(vdp, lorryArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    loadFont(vdp, art);
}

}  // namespace orchard
