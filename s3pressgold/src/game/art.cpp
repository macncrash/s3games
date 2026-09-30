#include "game/art.h"

#include <initializer_list>

namespace pressgold {
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
    gs::Bitmap b(96, 168);
    for (int y = 0; y < 168; y++) {
        for (int x = 0; x < 96; x++) {
            bool post = x < 12 || x >= 84;
            bool cap = y < 14 || (y > 140 && y < 154);
            int c = 0;
            if (post || cap) c = ((x + y) & 3) == 0 ? 3 : 2;
            else if (y >= 154) c = ((x * 3 + y) & 4) == 0 ? 4 : 5;
            b.set(x, y, c);
        }
    }
    b.rect(22, 18, 52, 8, 6);
    b.rect(44, 26, 8, 78, 7);
    return b;
}

gs::Bitmap platenArt() {
    gs::Bitmap b(70, 16);
    b.rect(0, 2, 70, 12, 2);
    b.rect(2, 4, 66, 8, 3);
    b.rect(4, 7, 62, 2, 4);
    b.rect(30, 0, 10, 5, 5);
    return b;
}

gs::Bitmap bedArt() {
    gs::Bitmap b(80, 28);
    b.rect(0, 4, 80, 20, 2);
    b.rect(4, 8, 72, 12, 3);
    b.rect(36, 6, 2, 18, 6);
    b.rect(6, 0, 68, 4, 4);
    return b;
}

gs::Bitmap sheetArt(bool gold) {
    gs::Bitmap b(48, 36);
    int paper = gold ? 2 : 1;
    int rule = gold ? 4 : 3;
    b.rect(1, 1, 46, 34, paper);
    b.rect(0, 0, 48, 36, gold ? 3 : 5);
    b.rect(2, 2, 44, 32, paper);
    for (int i = 0; i < 4; i++) b.rect(6, 8 + i * 5, 22, 1, rule);
    b.rect(32, 8, 10, 12, gold ? 6 : 4);
    return b;
}

gs::Bitmap rollerArt() {
    gs::Bitmap b(36, 14);
    b.ellipse(18, 7, 16, 5, 1);
    b.ellipse(18, 7, 12, 3, 2);
    b.rect(0, 5, 4, 4, 3);
    b.rect(32, 5, 4, 4, 3);
    return b;
}

gs::Bitmap printerArt() {
    gs::Bitmap b(28, 52);
    b.ellipse(14, 8, 6, 6, 1);
    b.rect(9, 15, 10, 14, 2);
    b.rect(4, 16, 5, 11, 3);
    b.rect(19, 16, 5, 11, 3);
    b.rect(9, 29, 4, 16, 4);
    b.rect(15, 29, 4, 16, 4);
    b.rect(7, 44, 6, 5, 5);
    b.rect(15, 44, 6, 5, 5);
    b.set(12, 7, 6);
    b.set(16, 7, 6);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 8)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 3), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(9, 8, 7), gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 4, 3), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 14, 11), gs::rgb4(10, 8, 5), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_WOOD, {0, 0, gs::rgb4(8, 4, 2), gs::rgb4(12, 7, 3), gs::rgb4(5, 3, 1), gs::rgb4(3, 2, 1),
                           gs::rgb4(10, 8, 4), gs::rgb4(6, 6, 6)});
    setPal(vdp, PAL_IRON, {0, 0, gs::rgb4(7, 7, 8), gs::rgb4(12, 12, 13), gs::rgb4(4, 4, 5), gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(2, 2, 6), gs::rgb4(5, 4, 12), gs::rgb4(9, 8, 10)});
    setPal(vdp, PAL_MAN, {0, gs::rgb4(13, 9, 6), gs::rgb4(4, 5, 8), gs::rgb4(10, 7, 4), gs::rgb4(3, 3, 4),
                          gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_PAPER,
           {0, gs::rgb4(14, 13, 11), gs::rgb4(15, 13, 4), gs::rgb4(12, 9, 2), gs::rgb4(8, 6, 3), gs::rgb4(11, 10, 9),
            gs::rgb4(14, 11, 2)});
    art.frame = gs::uploadImage(vdp, frameArt());
    art.platen = gs::uploadImage(vdp, platenArt());
    art.bed = gs::uploadImage(vdp, bedArt());
    art.sheetGold = gs::uploadImage(vdp, sheetArt(true));
    art.sheetCream = gs::uploadImage(vdp, sheetArt(false));
    art.roller = gs::uploadImage(vdp, rollerArt());
    art.printer = gs::uploadImage(vdp, printerArt());
    loadFont(vdp, art);
}

}  // namespace pressgold
