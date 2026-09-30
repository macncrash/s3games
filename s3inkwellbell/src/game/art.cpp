#include "game/art.h"

#include <initializer_list>

namespace inkwellbell {
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
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Bitmap deskArt() {
    gs::Bitmap b(300, 64);
    b.rect(0, 8, 300, 56, 1);
    b.rect(0, 0, 300, 10, 2);
    b.line(0, 12, 300, 12, 3, 1.5f);
    for (int i = 0; i < 14; i++) b.line(8.f + i * 21.f, 18.f, 18.f + i * 21.f, 58.f, 4, 1.2f);
    b.rect(0, 56, 300, 8, 5);
    return b;
}

gs::Bitmap pageArt() {
    gs::Bitmap b(168, 120);
    b.rect(4, 2, 160, 116, 1);
    b.rect(0, 0, 8, 120, 2);
    for (int i = 0; i < 8; i++) b.line(18.f, 18.f + i * 12.f, 150.f, 18.f + i * 12.f, 3, 1.f);
    b.rect(148, 8, 10, 14, 4);
    return b;
}

gs::Bitmap wellArt() {
    gs::Bitmap b(56, 36);
    b.ellipse(28.f, 10.f, 24.f, 8.f, 1);
    b.rect(5, 10, 46, 16, 2);
    b.ellipse(28.f, 26.f, 24.f, 8.f, 3);
    b.ellipse(28.f, 10.f, 14.f, 4.5f, 4);
    b.ellipse(22.f, 8.f, 4.f, 1.5f, 5);
    return b;
}

gs::Bitmap quillArt() {
    gs::Bitmap b(14, 56);
    b.line(7, 8, 7, 42, 1, 2.2f);
    b.ellipse(7.f, 8.f, 5.f, 7.f, 2);
    b.ellipse(5.f, 6.f, 1.5f, 2.2f, 3);
    b.poly({{4.f, 40.f}, {10.f, 40.f}, {7.f, 54.f}}, 4);
    return b;
}

gs::Bitmap dropArt() {
    gs::Bitmap b(12, 16);
    b.poly({{6.f, 1.f}, {11.f, 9.f}, {6.f, 15.f}, {1.f, 9.f}}, 1);
    b.ellipse(6.f, 10.f, 4.f, 4.5f, 1);
    b.set(5, 6, 2);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(36, 40);
    b.line(18, 2, 18, 8, 1, 1.5f);
    b.ellipse(18.f, 4.f, 3.f, 3.f, 2);
    b.poly({{6.f, 16.f}, {30.f, 16.f}, {32.f, 30.f}, {4.f, 30.f}}, 3);
    b.ellipse(18.f, 18.f, 12.f, 8.f, 3);
    b.rect(5, 28, 26, 4, 4);
    b.ellipse(18.f, 34.f, 3.f, 3.f, 5);
    b.line(18, 16, 18, 28, 5, 1.2f);
    return b;
}

gs::Bitmap nibArt() {
    gs::Bitmap b(10, 16);
    b.poly({{5.f, 1.f}, {9.f, 8.f}, {5.f, 15.f}, {1.f, 8.f}}, 1);
    b.line(5, 4, 5, 13, 2, 1.f);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(14, 12, 9), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_BRASS, gs::rgb4(15, 12, 4), gs::rgb4(4, 2, 0));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 7, 6), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_INK, gs::rgb4(6, 8, 14), gs::rgb4(1, 1, 3));

    setPal(vdp, PAL_DESK, {0, gs::rgb4(7, 4, 2), gs::rgb4(10, 6, 3), gs::rgb4(4, 2, 1), gs::rgb4(5, 3, 2),
                           gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_QUILL, {0, gs::rgb4(13, 12, 9), gs::rgb4(15, 15, 12), gs::rgb4(9, 7, 5), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_PAGE, {0, gs::rgb4(14, 13, 11), gs::rgb4(9, 6, 4), gs::rgb4(12, 10, 8), gs::rgb4(13, 4, 3)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(8, 6, 2), gs::rgb4(15, 14, 8), gs::rgb4(13, 10, 3), gs::rgb4(6, 4, 1),
                           gs::rgb4(15, 15, 12)});
    vdp.setColor(PAL_INK * 16 + 1, gs::rgb4(3, 4, 12));
    vdp.setColor(PAL_INK * 16 + 2, gs::rgb4(10, 12, 15));

    loadFont(vdp, art);
    art.desk = gs::uploadImage(vdp, deskArt());
    art.page = gs::uploadImage(vdp, pageArt());
    art.well = gs::uploadImage(vdp, wellArt());
    art.quill = gs::uploadImage(vdp, quillArt());
    art.drop = gs::uploadImage(vdp, dropArt());
    art.bell = gs::uploadImage(vdp, bellArt());
    art.nib = gs::uploadImage(vdp, nibArt());
}

}  // namespace inkwellbell
