#include "game/art.h"

#include <string>

namespace mill {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap millArt(int turn) {
    Bitmap b(96, 128);
    b.rect(34, 58, 28, 62, 3);
    b.rect(36, 60, 24, 56, 2);
    b.rect(44, 96, 10, 22, 5);
    b.rect(40, 72, 8, 8, 1);
    b.rect(54, 72, 6, 8, 4);
    b.poly({{30, 60}, {48, 36}, {66, 60}}, 6);
    b.poly({{34, 58}, {48, 40}, {62, 58}}, 7);
    b.ellipse(48, 44, 6, 5, 4);
    // Four sails. turn swaps the pair that reads as the near blades.
    const int a = turn ? 8 : 9;
    const int c = turn ? 9 : 8;
    b.rect(44, 8, 8, 40, a);
    b.rect(44, 48, 8, 36, a);
    b.rect(8, 40, 36, 8, c);
    b.rect(56, 40, 34, 8, c);
    b.rect(20, 18, 22, 6, a);
    b.rect(54, 54, 22, 6, c);
    b.outline(15, false);
    return b;
}

Bitmap wellArt() {
    Bitmap b(72, 80);
    b.rect(22, 8, 6, 28, 4);
    b.rect(46, 8, 6, 28, 4);
    b.rect(18, 6, 38, 6, 5);
    b.rect(34, 12, 4, 16, 6);
    b.ellipse(36, 30, 5, 4, 7);
    b.rect(30, 28, 12, 8, 3);
    b.ellipse(36, 52, 28, 16, 2);
    b.ellipse(36, 48, 24, 12, 1);
    b.ellipse(36, 50, 14, 7, 8);
    b.ellipse(36, 50, 8, 4, 9);
    b.rect(18, 58, 36, 10, 3);
    b.outline(15, false);
    return b;
}

Bitmap manArt(bool swing) {
    Bitmap b(40, 52);
    b.ellipse(20, 8, 6, 6, 4);
    b.rect(16, 14, 8, 12, 2);
    b.rect(14, 16, 4, 8, 3);
    b.rect(12, 26, 16, 10, 2);
    b.rect(14, 36, 5, 12, 5);
    b.rect(22, 36, 5, 12, 5);
    b.rect(12, 46, 7, 4, 6);
    b.rect(22, 46, 7, 4, 6);
    if (swing) {
        b.rect(24, 16, 14, 4, 3);
        b.rect(34, 12, 5, 12, 7);
    } else {
        b.rect(8, 18, 6, 12, 3);
        b.rect(26, 18, 6, 12, 3);
        b.rect(28, 28, 4, 10, 7);
    }
    b.outline(15, false);
    return b;
}

Bitmap crowArt() {
    Bitmap b(36, 28);
    b.ellipse(16, 16, 10, 7, 2);
    b.poly({{24, 12}, {34, 8}, {28, 16}}, 3);
    b.ellipse(10, 14, 4, 4, 2);
    b.set(9, 13, 4);
    b.poly({{6, 14}, {2, 12}, {6, 16}}, 5);
    b.rect(14, 20, 3, 5, 6);
    b.rect(20, 20, 3, 5, 6);
    b.ellipse(22, 10, 8, 3, 1);
    b.outline(15, false);
    return b;
}

Bitmap boarArt() {
    Bitmap b(52, 32);
    b.ellipse(26, 18, 18, 10, 2);
    b.ellipse(40, 16, 8, 6, 3);
    b.rect(44, 14, 6, 3, 5);
    b.rect(8, 22, 4, 8, 4);
    b.rect(16, 22, 4, 8, 4);
    b.rect(30, 22, 4, 8, 4);
    b.rect(38, 22, 4, 8, 4);
    b.set(42, 14, 6);
    b.ellipse(14, 12, 3, 4, 3);
    b.outline(15, false);
    return b;
}

Bitmap raiderArt() {
    Bitmap b(36, 48);
    b.ellipse(16, 8, 6, 6, 4);
    b.rect(12, 14, 10, 14, 2);
    b.rect(10, 28, 14, 6, 3);
    b.rect(12, 34, 4, 10, 5);
    b.rect(20, 34, 4, 10, 5);
    b.rect(22, 10, 4, 16, 6);
    b.rect(24, 8, 10, 4, 7);
    b.outline(15, false);
    return b;
}

Bitmap sackArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 9, 6, 5, 2);
    b.rect(6, 3, 4, 4, 3);
    b.outline(15, false);
    return b;
}

Bitmap puffArt() {
    Bitmap b(24, 24);
    b.ellipse(12, 12, 9, 7, 2);
    b.ellipse(8, 10, 4, 3, 1);
    b.ellipse(15, 13, 4, 3, 3);
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

void buildArt(gs::VDP& vdp, Art& a) {
    const auto C = gs::rgb4;
    setPal(vdp, PAL_HUD, {0, C(15, 15, 14), C(15, 12, 4), C(8, 4, 2), C(4, 12, 6), C(14, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, C(2, 1, 1)});
    setPal(vdp, PAL_MILL, {0, C(14, 13, 10), C(11, 10, 8), C(7, 6, 5), C(6, 8, 10), C(4, 3, 2), C(9, 4, 2), C(12, 6, 3), C(13, 12, 8), C(8, 7, 5), 0, 0, 0, 0, 0, C(1, 1, 1)});
    setPal(vdp, PAL_WELL, {0, C(12, 12, 11), C(8, 8, 7), C(5, 5, 5), C(10, 6, 3), C(7, 4, 2), C(6, 4, 2), C(4, 6, 8), C(3, 5, 8), C(6, 9, 12), 0, 0, 0, 0, 0, C(1, 1, 1)});
    setPal(vdp, PAL_MAN, {0, C(15, 14, 12), C(3, 5, 12), C(12, 9, 6), C(14, 10, 7), C(4, 3, 6), C(2, 2, 2), C(10, 8, 5), 0, 0, 0, 0, 0, 0, 0, C(1, 1, 1)});
    setPal(vdp, PAL_CROW, {0, C(4, 4, 5), C(2, 2, 3), C(6, 6, 7), C(14, 12, 2), C(12, 8, 3), C(8, 6, 4), 0, 0, 0, 0, 0, 0, 0, 0, C(1, 1, 1)});
    setPal(vdp, PAL_BOAR, {0, C(12, 8, 6), C(8, 5, 3), C(10, 6, 4), C(4, 3, 2), C(15, 14, 12), C(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, C(1, 1, 1)});
    setPal(vdp, PAL_RAIDER, {0, C(14, 12, 8), C(8, 3, 3), C(5, 2, 2), C(13, 9, 6), C(3, 3, 4), C(9, 8, 6), C(6, 5, 3), 0, 0, 0, 0, 0, 0, 0, C(1, 1, 1)});
    setPal(vdp, PAL_FX, {0, C(15, 15, 12), C(14, 12, 8), C(10, 8, 5), C(15, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(1, 1, 1)});
    setPal(vdp, PAL_YARD,
           {0, C(6, 10, 4), C(4, 8, 3), C(8, 11, 5), C(10, 8, 4), C(8, 6, 3), C(11, 9, 5), C(9, 7, 4), C(7, 6, 4),
            C(12, 10, 6), C(8, 7, 4), C(5, 8, 10), C(7, 10, 12), C(10, 12, 14), C(14, 13, 8), C(13, 11, 7)});

    a.mill[0] = gs::uploadMipped(vdp, millArt(0));
    a.mill[1] = gs::uploadMipped(vdp, millArt(1));
    a.well = gs::uploadMipped(vdp, wellArt());
    a.man[0] = gs::uploadMipped(vdp, manArt(false));
    a.man[1] = gs::uploadMipped(vdp, manArt(true));
    a.crow = gs::uploadMipped(vdp, crowArt());
    a.boar = gs::uploadMipped(vdp, boarArt());
    a.raider = gs::uploadMipped(vdp, raiderArt());
    a.sack = gs::uploadMipped(vdp, sackArt());
    a.puff = gs::uploadMipped(vdp, puffArt());
    loadFont(vdp, a);
    vdp.setFogColor(C(10, 12, 8));
}

}  // namespace mill
