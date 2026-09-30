#include "game/art.h"

#include <initializer_list>

namespace pressseven {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
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

gs::Bitmap frameArt() {
    gs::Bitmap b(88, 150);
    b.rect(0, 0, 10, 150, 2);
    b.rect(78, 0, 10, 150, 2);
    b.rect(0, 0, 88, 12, 3);
    b.rect(0, 128, 88, 10, 3);
    b.rect(4, 138, 80, 12, 4);
    for (int y = 16; y < 124; y += 14) {
        b.rect(2, y, 6, 2, 5);
        b.rect(80, y, 6, 2, 5);
    }
    b.rect(36, 12, 16, 10, 6);
    return b;
}

gs::Bitmap platenArt() {
    gs::Bitmap b(64, 14);
    b.rect(0, 3, 64, 10, 2);
    b.rect(3, 5, 58, 6, 3);
    b.rect(6, 7, 52, 2, 4);
    b.rect(28, 0, 8, 4, 5);
    return b;
}

gs::Bitmap bedArt() {
    gs::Bitmap b(76, 22);
    b.rect(0, 2, 76, 16, 2);
    b.rect(4, 5, 68, 10, 3);
    b.rect(36, 4, 2, 14, 5);
    b.rect(2, 18, 72, 4, 4);
    return b;
}

gs::Bitmap sheetArt(bool gold) {
    gs::Bitmap b(44, 32);
    int paper = gold ? 2 : 1;
    int rule = gold ? 4 : 3;
    b.rect(0, 0, 44, 32, gold ? 3 : 5);
    b.rect(2, 2, 40, 28, paper);
    for (int i = 0; i < 4; i++) b.rect(5, 7 + i * 5, 18, 1, rule);
    b.rect(28, 7, 10, 10, gold ? 6 : 4);
    return b;
}

gs::Bitmap screwArt() {
    gs::Bitmap b(12, 70);
    b.rect(4, 0, 4, 70, 2);
    for (int y = 4; y < 66; y += 6) b.rect(2, y, 8, 2, 3);
    b.rect(1, 0, 10, 4, 4);
    return b;
}

gs::Bitmap printerArt() {
    gs::Bitmap b(26, 48);
    b.ellipse(13, 7, 6, 6, 1);
    b.rect(8, 14, 10, 13, 2);
    b.rect(3, 15, 5, 10, 3);
    b.rect(18, 15, 5, 10, 3);
    b.rect(8, 27, 4, 14, 4);
    b.rect(14, 27, 4, 14, 4);
    b.rect(6, 41, 6, 5, 5);
    b.rect(14, 41, 6, 5, 5);
    b.set(11, 6, 6);
    b.set(15, 6, 6);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 8)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 3), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(9, 8, 7), gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 4, 3), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 14, 11), gs::rgb4(10, 8, 5), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_WOOD, {0, 0, gs::rgb4(9, 5, 2), gs::rgb4(13, 8, 3), gs::rgb4(5, 3, 1), gs::rgb4(11, 9, 4),
                           gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_IRON, {0, 0, gs::rgb4(6, 6, 7), gs::rgb4(11, 11, 12), gs::rgb4(3, 3, 4), gs::rgb4(14, 12, 5)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(1, 1, 5), gs::rgb4(4, 3, 11), gs::rgb4(8, 7, 9), gs::rgb4(12, 11, 6)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(13, 9, 6), gs::rgb4(3, 5, 9), gs::rgb4(9, 6, 3), gs::rgb4(2, 2, 3),
                          gs::rgb4(1, 1, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_RIVAL, {0, gs::rgb4(12, 8, 6), gs::rgb4(8, 3, 3), gs::rgb4(6, 3, 2), gs::rgb4(3, 2, 2),
                            gs::rgb4(2, 1, 1), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_PAPER,
           {0, gs::rgb4(14, 13, 10), gs::rgb4(15, 12, 3), gs::rgb4(11, 8, 2), gs::rgb4(7, 5, 3), gs::rgb4(10, 9, 8),
            gs::rgb4(13, 10, 2)});
    art.frame = gs::uploadImage(vdp, frameArt());
    art.platen = gs::uploadImage(vdp, platenArt());
    art.bed = gs::uploadImage(vdp, bedArt());
    art.sheetGold = gs::uploadImage(vdp, sheetArt(true));
    art.sheetCream = gs::uploadImage(vdp, sheetArt(false));
    art.screw = gs::uploadImage(vdp, screwArt());
    art.printer = gs::uploadImage(vdp, printerArt());
    loadFont(vdp, art);
}

}  // namespace pressseven
