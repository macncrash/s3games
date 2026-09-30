#include "game/art.h"

#include <initializer_list>

namespace maskgold {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t edge) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 2, edge);
    vdp.setColor(pal * 16 + 15, edge);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    }
}

gs::Image up(gs::VDP& vdp, const gs::Bitmap& b) { return gs::uploadImage(vdp, b); }

void paintMask(gs::Bitmap& b) {
    b.ellipse(48, 58, 40.f, 50.f, 4);
    b.ellipse(48, 54, 32.f, 40.f, 5);
    b.ellipse(34, 50, 9.f, 6.f, 2);
    b.ellipse(62, 50, 9.f, 6.f, 2);
    b.poly({{44, 58}, {52, 58}, {48, 70}}, 3);
    b.ellipse(48, 82, 12.f, 5.f, 2);
    b.rect(28, 22, 40, 5, 6);
    b.rect(20, 18, 8, 10, 7);
    b.rect(68, 18, 8, 10, 7);
    b.outline(1, false);
}

void paintBlade(gs::Bitmap& b) {
    b.rect(6, 0, 4, 16, 3);
    b.poly({{4, 14}, {12, 14}, {8, 24}}, 5);
    b.rect(4, 0, 8, 4, 2);
    b.rect(7, 4, 2, 10, 4);
    b.outline(1, false);
}

void paintStroke(gs::Bitmap& b) {
    b.rect(0, 2, 32, 5, 2);
    b.rect(2, 3, 28, 2, 3);
}

void paintLeaf(gs::Bitmap& b) {
    b.ellipse(16, 14, 12.f, 8.f, 3);
    b.line(6, 16, 26, 10, 4, 1.4f);
    b.ellipse(16, 14, 4.f, 2.f, 5);
    b.outline(1, false);
}

void paintPot(gs::Bitmap& b) {
    b.ellipse(14, 18, 11.f, 9.f, 3);
    b.ellipse(14, 12, 8.f, 3.f, 4);
    b.ellipse(14, 12, 4.f, 1.5f, 5);
    b.rect(12, 4, 4, 8, 2);
    b.outline(1, false);
}

void paintRibbon(gs::Bitmap& b) {
    b.poly({{2, 8}, {20, 2}, {22, 10}, {6, 16}}, 3);
    b.poly({{22, 6}, {36, 4}, {34, 14}, {20, 12}}, 4);
    b.ellipse(18, 9, 3.f, 3.f, 5);
    b.outline(1, false);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_CLAY, {0, gs::rgb4(2, 1, 1), gs::rgb4(6, 3, 2), gs::rgb4(9, 5, 3), gs::rgb4(12, 8, 5),
                           gs::rgb4(14, 11, 8), gs::rgb4(8, 2, 3), gs::rgb4(13, 4, 5)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(6, 4, 1), gs::rgb4(12, 8, 2), gs::rgb4(15, 12, 4), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(8, 7, 5), gs::rgb4(13, 12, 9), gs::rgb4(15, 14, 12), gs::rgb4(10, 9, 6)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_BLADE, {0, gs::rgb4(2, 2, 3), gs::rgb4(8, 8, 9), gs::rgb4(5, 5, 6), gs::rgb4(12, 13, 14),
                            gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_RIBBON, {0, gs::rgb4(4, 1, 2), gs::rgb4(8, 2, 3), gs::rgb4(13, 3, 4), gs::rgb4(15, 8, 6),
                             gs::rgb4(15, 13, 8)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(5, 3, 2), gs::rgb4(9, 6, 4), gs::rgb4(12, 8, 6), gs::rgb4(7, 5, 4)});
    setPal(vdp, PAL_LEAF, {0, gs::rgb4(4, 3, 1), gs::rgb4(8, 6, 1), gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 6),
                           gs::rgb4(12, 9, 2)});
    setPal(vdp, PAL_POT, {0, gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10), gs::rgb4(14, 13, 11),
                          gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_BENCH, {0, gs::rgb4(5, 3, 2), gs::rgb4(7, 5, 3)});
    textPal(vdp, PAL_INK, gs::rgb4(14, 12, 9), gs::rgb4(3, 2, 1));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 6), gs::rgb4(5, 2, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3), gs::rgb4(4, 1, 1));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 7, 5), gs::rgb4(2, 2, 1));
    loadFont(vdp, art);

    gs::Bitmap mask(96, 112);
    paintMask(mask);
    art.mask = up(vdp, mask);

    gs::Bitmap blade(16, 28);
    paintBlade(blade);
    art.blade = up(vdp, blade);

    gs::Bitmap stroke(32, 8);
    paintStroke(stroke);
    art.stroke = up(vdp, stroke);

    gs::Bitmap leaf(32, 28);
    paintLeaf(leaf);
    art.leaf = up(vdp, leaf);

    gs::Bitmap pot(28, 32);
    paintPot(pot);
    art.pot = up(vdp, pot);

    gs::Bitmap ribbon(40, 20);
    paintRibbon(ribbon);
    art.ribbon = up(vdp, ribbon);
}

}  // namespace maskgold
