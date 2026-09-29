#include "game/art.h"

#include <initializer_list>

namespace solitaire {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(0, 2, 1));
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

gs::Bitmap feltArt() {
    gs::Bitmap b(300, 150);
    b.rect(0, 0, 300, 150, 1);
    b.rect(4, 4, 292, 142, 2);
    b.ellipse(150.f, 78.f, 120.f, 48.f, 3);
    for (int i = 0; i < 7; i++) b.rect(18 + i * 36, 28, 28, 40, 4);
    b.rect(236, 18, 48, 46, 5);
    b.rect(236, 86, 48, 46, 6);
    b.rect(14, 128, 80, 8, 5);
    return b;
}

gs::Bitmap cardArt() {
    gs::Bitmap b(28, 40);
    b.rect(0, 0, 28, 40, 1);
    b.rect(2, 2, 24, 36, 2);
    b.rect(4, 4, 20, 8, 3);
    b.ellipse(14.f, 24.f, 7.f, 8.f, 4);
    return b;
}

gs::Bitmap coinArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6.f, 6.f, 5.f, 5.f, 1);
    b.ellipse(6.f, 6.f, 3.f, 3.f, 2);
    return b;
}

gs::Bitmap ovalArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6.f, 6.f, 5.f, 4.f, 1);
    b.ellipse(6.f, 5.f, 2.f, 1.5f, 2);
    return b;
}

gs::Bitmap stackArt() {
    gs::Bitmap b(36, 28);
    b.rect(4, 6, 28, 18, 1);
    b.rect(2, 4, 28, 18, 2);
    b.rect(0, 2, 28, 18, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_FELT,
           {0, gs::rgb4(0, 4, 2), gs::rgb4(1, 7, 3), gs::rgb4(2, 9, 4), gs::rgb4(0, 5, 2), gs::rgb4(8, 6, 1),
            gs::rgb4(9, 8, 6), gs::rgb4(0, 3, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(6, 4, 1), gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 6), gs::rgb4(15, 12, 2)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(7, 6, 4), gs::rgb4(13, 12, 9), gs::rgb4(15, 14, 12), gs::rgb4(12, 10, 7)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 12), gs::rgb4(0, 2, 1));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 12, 3), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 10), gs::rgb4(0, 3, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 6, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 15, 12), gs::rgb4(0, 2, 2));
    setPal(vdp, PAL_PIP, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12)});

    loadFont(vdp, art);
    art.felt = gs::uploadImage(vdp, feltArt());
    art.card = gs::uploadImage(vdp, cardArt());
    art.coin = gs::uploadImage(vdp, coinArt());
    art.oval = gs::uploadImage(vdp, ovalArt());
    art.stack = gs::uploadImage(vdp, stackArt());
}

}  // namespace solitaire
