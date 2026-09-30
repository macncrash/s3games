#include "game/art.h"

#include <initializer_list>

namespace inkwellseven {
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
    gs::Bitmap b(280, 90);
    b.rect(0, 18, 280, 72, 1);
    b.rect(0, 14, 280, 10, 2);
    b.line(0, 24, 280, 24, 3, 1.4f);
    for (int i = 0; i < 9; i++) b.line(12.f + i * 30.f, 40.f, 28.f + i * 30.f, 78.f, 4, 1.2f);
    b.rect(0, 84, 280, 6, 5);
    return b;
}

gs::Bitmap pageArt() {
    gs::Bitmap b(96, 70);
    b.rect(2, 2, 92, 66, 1);
    b.rect(0, 0, 6, 70, 2);
    for (int i = 0; i < 6; i++) b.line(14.f, 14.f + i * 9.f, 84.f, 14.f + i * 9.f, 3, 1.f);
    return b;
}

gs::Bitmap wellArt() {
    gs::Bitmap b(52, 40);
    b.ellipse(26.f, 12.f, 22.f, 9.f, 1);
    b.rect(5, 12, 42, 16, 2);
    b.ellipse(26.f, 28.f, 22.f, 9.f, 3);
    b.ellipse(26.f, 12.f, 14.f, 5.f, 4);
    b.ellipse(20.f, 10.f, 4.f, 1.6f, 5);
    return b;
}

gs::Bitmap poolArt() {
    gs::Bitmap b(22, 10);
    b.ellipse(11.f, 5.f, 9.f, 4.f, 1);
    b.ellipse(8.f, 4.f, 3.f, 1.3f, 2);
    return b;
}

gs::Bitmap quillArt() {
    gs::Bitmap b(16, 64);
    b.line(8, 6, 8, 50, 1, 2.f);
    b.ellipse(8.f, 8.f, 5.f, 7.f, 2);
    b.ellipse(6.f, 6.f, 1.6f, 2.4f, 3);
    b.poly({{5.f, 48.f}, {11.f, 48.f}, {8.f, 62.f}}, 4);
    b.line(8, 50, 8, 61, 5, 1.f);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4.f, 4.f, 3.2f, 3.2f, 1);
    b.set(3, 3, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(14, 13, 11), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 8, 9), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_CREAM, gs::rgb4(15, 14, 10), gs::rgb4(2, 2, 1));

    setPal(vdp, PAL_DESK,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(5, 3, 2), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_QUILL, {0, gs::rgb4(12, 11, 8), gs::rgb4(15, 14, 11), gs::rgb4(8, 6, 5), gs::rgb4(2, 1, 1),
                            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(7, 5, 1), gs::rgb4(10, 7, 2), gs::rgb4(8, 5, 1), gs::rgb4(14, 11, 3), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(8, 7, 6), gs::rgb4(12, 10, 8), gs::rgb4(9, 8, 7), gs::rgb4(14, 13, 11), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_PAGE, {0, gs::rgb4(14, 13, 10), gs::rgb4(8, 6, 4), gs::rgb4(11, 9, 7)});

    loadFont(vdp, art);
    art.desk = gs::uploadImage(vdp, deskArt());
    art.page = gs::uploadImage(vdp, pageArt());
    art.well = gs::uploadImage(vdp, wellArt());
    art.pool = gs::uploadImage(vdp, poolArt());
    art.quill = gs::uploadImage(vdp, quillArt());
    art.bead = gs::uploadImage(vdp, beadArt());
}

}  // namespace inkwellseven
