#include "game/art.h"

#include <initializer_list>

namespace inkwellgold {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
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

gs::Bitmap deskArt() {
    gs::Bitmap b(300, 150);
    b.rect(0, 0, 300, 150, 1);
    b.rect(8, 10, 284, 128, 2);
    b.rect(16, 22, 120, 96, 3);
    b.rect(164, 22, 112, 96, 4);
    b.line(20, 118, 280, 118, 5, 2);
    b.rect(28, 40, 96, 8, 6);
    return b;
}

gs::Bitmap wellArt() {
    gs::Bitmap b(64, 48);
    b.ellipse(32.f, 16.f, 26.f, 10.f, 1);
    b.rect(8, 14, 48, 22, 2);
    b.ellipse(32.f, 36.f, 26.f, 10.f, 3);
    b.ellipse(32.f, 16.f, 18.f, 6.f, 4);
    b.ellipse(26.f, 14.f, 5.f, 2.f, 5);
    return b;
}

gs::Bitmap poolArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(14.f, 6.f, 12.f, 5.f, 1);
    b.ellipse(11.f, 5.f, 4.f, 1.6f, 2);
    return b;
}

gs::Bitmap quillArt() {
    gs::Bitmap b(18, 72);
    b.line(9, 4, 9, 58, 1, 2.2f);
    b.ellipse(9.f, 10.f, 6.f, 8.f, 2);
    b.ellipse(7.f, 8.f, 2.f, 3.f, 3);
    b.poly({{6.f, 56.f}, {12.f, 56.f}, {9.f, 70.f}}, 4);
    return b;
}

gs::Bitmap dropArt() {
    gs::Bitmap b(8, 10);
    b.ellipse(4.f, 6.f, 3.f, 3.2f, 1);
    b.poly({{2.f, 5.f}, {6.f, 5.f}, {4.f, 1.f}}, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_DESK,
           {0, gs::rgb4(5, 3, 2), gs::rgb4(8, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(6, 4, 3), gs::rgb4(3, 2, 1),
            gs::rgb4(11, 8, 4)});
    setPal(vdp, PAL_GOLD,
           {0, gs::rgb4(6, 4, 1), gs::rgb4(9, 6, 2), gs::rgb4(7, 5, 2), gs::rgb4(14, 11, 2), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_CREAM,
           {0, gs::rgb4(8, 7, 5), gs::rgb4(11, 9, 7), gs::rgb4(9, 8, 6), gs::rgb4(14, 12, 9), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_QUILL, {0, gs::rgb4(12, 11, 9), gs::rgb4(15, 14, 12), gs::rgb4(8, 7, 6), gs::rgb4(2, 2, 2)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 11), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 12, 3), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 10), gs::rgb4(1, 3, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 6, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 15, 12), gs::rgb4(1, 2, 2));

    loadFont(vdp, art);
    art.desk = gs::uploadImage(vdp, deskArt());
    art.well = gs::uploadImage(vdp, wellArt());
    art.pool = gs::uploadImage(vdp, poolArt());
    art.quill = gs::uploadImage(vdp, quillArt());
    art.drop = gs::uploadImage(vdp, dropArt());
}

}  // namespace inkwellgold
