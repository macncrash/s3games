#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace inkwellchime {
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
    gs::Bitmap b(300, 48);
    b.rect(0, 0, 300, 48, 1);
    b.rect(0, 0, 300, 8, 2);
    b.line(0, 14, 300, 14, 3, 1);
    for (int x = 8; x < 300; x += 22) b.rect(x, 20, 3, 22, 3);
    return b;
}

gs::Bitmap pageArt() {
    gs::Bitmap b(120, 90);
    b.rect(0, 0, 120, 90, 1);
    b.rect(4, 4, 112, 82, 2);
    for (int y = 18; y < 80; y += 12) b.line(12, float(y), 104, float(y), 3, 1);
    b.rect(0, 0, 8, 90, 4);
    return b;
}

gs::Bitmap wellArt() {
    gs::Bitmap b(64, 40);
    b.ellipse(32.f, 12.f, 28.f, 10.f, 1);
    b.rect(6, 12, 52, 16, 2);
    b.ellipse(32.f, 28.f, 28.f, 10.f, 3);
    b.ellipse(32.f, 12.f, 16.f, 5.f, 4);
    b.ellipse(26.f, 10.f, 4.f, 1.6f, 5);
    return b;
}

gs::Bitmap quillArt() {
    gs::Bitmap b(16, 64);
    b.line(8, 4, 8, 50, 1, 2.2f);
    b.ellipse(8.f, 10.f, 5.f, 8.f, 2);
    b.ellipse(6.f, 8.f, 2.f, 3.f, 3);
    b.poly({{5.f, 48.f}, {11.f, 48.f}, {8.f, 62.f}}, 4);
    return b;
}

gs::Bitmap dropArt() {
    gs::Bitmap b(10, 14);
    b.ellipse(5.f, 9.f, 4.f, 4.2f, 1);
    b.poly({{2.f, 8.f}, {8.f, 8.f}, {5.f, 1.f}}, 1);
    b.ellipse(4.f, 8.f, 1.2f, 1.2f, 2);
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(52, 52);
    b.ellipse(26.f, 26.f, 24.f, 24.f, 1);
    b.ellipse(26.f, 26.f, 20.f, 20.f, 2);
    b.ellipse(26.f, 26.f, 2.f, 2.f, 3);
    for (int i = 0; i < 12; i++) {
        float a = float(i) * 3.14159265f / 6.f;
        float x0 = 26.f + std::cos(a) * 16.f;
        float y0 = 26.f + std::sin(a) * 16.f;
        float x1 = 26.f + std::cos(a) * 19.f;
        float y1 = 26.f + std::sin(a) * 19.f;
        b.line(x0, y0, x1, y1, 3, 1.4f);
    }
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(4, 4);
    b.ellipse(2.f, 2.f, 1.6f, 1.6f, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_DESK,
           {0, gs::rgb4(5, 3, 2), gs::rgb4(8, 5, 3), gs::rgb4(3, 2, 1), gs::rgb4(2, 1, 1), gs::rgb4(12, 10, 6),
            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_PAGE, {0, gs::rgb4(14, 13, 10), gs::rgb4(15, 15, 13), gs::rgb4(10, 9, 7), gs::rgb4(8, 4, 3)});
    setPal(vdp, PAL_INK,
           {0, gs::rgb4(6, 4, 1), gs::rgb4(4, 3, 2), gs::rgb4(3, 2, 1), gs::rgb4(1, 1, 3), gs::rgb4(10, 9, 6),
            gs::rgb4(14, 12, 4)});
    setPal(vdp, PAL_QUILL, {0, gs::rgb4(12, 11, 9), gs::rgb4(15, 14, 12), gs::rgb4(8, 7, 6), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_CLOCK,
           {0, gs::rgb4(12, 9, 4), gs::rgb4(15, 14, 11), gs::rgb4(2, 2, 3), gs::rgb4(8, 6, 3)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 10)});
    textPal(vdp, PAL_HUD, gs::rgb4(14, 13, 11), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_BRASS, gs::rgb4(15, 12, 4), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_DIM, gs::rgb4(7, 7, 8), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 6, 4), gs::rgb4(3, 0, 0));

    loadFont(vdp, art);
    art.desk = gs::uploadImage(vdp, deskArt());
    art.page = gs::uploadImage(vdp, pageArt());
    art.well = gs::uploadImage(vdp, wellArt());
    art.quill = gs::uploadImage(vdp, quillArt());
    art.drop = gs::uploadImage(vdp, dropArt());
    art.face = gs::uploadImage(vdp, faceArt());
    art.dot = gs::uploadImage(vdp, dotArt());
}

}  // namespace inkwellchime
