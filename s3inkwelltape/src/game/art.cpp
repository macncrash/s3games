#include "game/art.h"

#include <initializer_list>

namespace inkwelltape {
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
    gs::Bitmap b(300, 36);
    b.rect(0, 0, 300, 28, 1);
    b.line(0, 4, 300, 4, 2, 1.2f);
    b.rect(0, 28, 300, 8, 3);
    for (int i = 0; i < 14; i++) b.line(8.f + i * 21.f, 10.f, 16.f + i * 21.f, 24.f, 4, 1.f);
    return b;
}

gs::Bitmap wellArt() {
    gs::Bitmap b(44, 36);
    b.ellipse(22.f, 10.f, 18.f, 8.f, 1);
    b.rect(4, 10, 36, 14, 2);
    b.ellipse(22.f, 24.f, 18.f, 8.f, 3);
    b.ellipse(22.f, 10.f, 11.f, 4.5f, 4);
    b.ellipse(16.f, 8.f, 3.f, 1.4f, 5);
    return b;
}

gs::Bitmap poolArt() {
    gs::Bitmap b(20, 10);
    b.ellipse(10.f, 5.f, 8.f, 3.6f, 1);
    b.ellipse(7.f, 4.f, 2.4f, 1.1f, 2);
    return b;
}

gs::Bitmap quillArt() {
    gs::Bitmap b(14, 58);
    b.line(7, 6, 7, 44, 1, 2.f);
    b.ellipse(7.f, 8.f, 4.5f, 6.5f, 2);
    b.ellipse(5.f, 6.f, 1.4f, 2.2f, 3);
    b.poly({{4.f, 42.f}, {10.f, 42.f}, {7.f, 56.f}}, 4);
    b.line(7, 44, 7, 55, 5, 1.f);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4.f, 4.f, 3.f, 3.f, 1);
    b.set(3, 3, 2);
    return b;
}

gs::Bitmap drawerArt() {
    gs::Bitmap b(52, 28);
    b.rect(1, 1, 50, 26, 1);
    b.rect(4, 4, 44, 16, 2);
    b.rect(20, 20, 12, 4, 3);
    b.line(1, 1, 51, 1, 4, 1.f);
    return b;
}

gs::Bitmap tapeArt() {
    gs::Bitmap b(168, 16);
    b.rect(0, 2, 168, 12, 1);
    b.rect(0, 2, 6, 12, 2);
    b.rect(162, 2, 6, 12, 2);
    for (int i = 0; i < 8; i++) b.line(14.f + i * 18.f, 5.f, 14.f + i * 18.f, 12.f, 3, 1.f);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(14, 13, 11), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 8, 9), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 4), gs::rgb4(3, 0, 0));

    setPal(vdp, PAL_DESK, {0, gs::rgb4(7, 4, 2), gs::rgb4(10, 7, 3), gs::rgb4(4, 2, 1), gs::rgb4(5, 3, 2)});
    setPal(vdp, PAL_QUILL, {0, gs::rgb4(12, 11, 8), gs::rgb4(15, 14, 11), gs::rgb4(8, 6, 5), gs::rgb4(2, 1, 1),
                            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_GALL,
           {0, gs::rgb4(5, 4, 2), gs::rgb4(8, 6, 3), gs::rgb4(6, 5, 2), gs::rgb4(3, 2, 1), gs::rgb4(12, 10, 6),
            gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_SEPIA,
           {0, gs::rgb4(8, 5, 1), gs::rgb4(11, 7, 2), gs::rgb4(7, 4, 1), gs::rgb4(14, 10, 3), gs::rgb4(15, 13, 6),
            gs::rgb4(4, 2, 0)});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), gs::rgb4(0, 0, 1), gs::rgb4(8, 8, 9),
            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WASH,
           {0, gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10), gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5), gs::rgb4(13, 13, 14),
            gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(13, 12, 9), gs::rgb4(8, 5, 3), gs::rgb4(10, 8, 6)});
    setPal(vdp, PAL_DRAWER, {0, gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1), gs::rgb4(12, 10, 4), gs::rgb4(9, 6, 3)});

    loadFont(vdp, art);
    art.desk = gs::uploadImage(vdp, deskArt());
    art.well = gs::uploadImage(vdp, wellArt());
    art.pool = gs::uploadImage(vdp, poolArt());
    art.quill = gs::uploadImage(vdp, quillArt());
    art.bead = gs::uploadImage(vdp, beadArt());
    art.drawer = gs::uploadImage(vdp, drawerArt());
    art.tape = gs::uploadImage(vdp, tapeArt());
}

}  // namespace inkwelltape
