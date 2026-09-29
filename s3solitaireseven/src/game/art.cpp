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
    gs::Bitmap b(304, 168);
    b.rect(0, 0, 304, 168, 1);
    b.rect(4, 4, 296, 160, 2);
    b.ellipse(152.f, 90.f, 120.f, 46.f, 3);
    for (int i = 0; i < 10; i++) b.rect(8 + i * 22, 40, 18, 28, 4);
    b.rect(12, 12, 70, 18, 5);
    b.rect(220, 12, 70, 18, 6);
    return b;
}

gs::Bitmap cardArt() {
    gs::Bitmap b(22, 32);
    b.rect(0, 0, 22, 32, 1);
    b.rect(2, 2, 18, 28, 2);
    b.rect(3, 3, 8, 6, 3);
    b.ellipse(11.f, 20.f, 5.f, 6.f, 4);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5.f, 5.f, 4.f, 4.f, 1);
    b.ellipse(5.f, 5.f, 2.f, 2.f, 2);
    return b;
}

gs::Bitmap houseArt() {
    gs::Bitmap b(10, 10);
    b.rect(2, 2, 6, 6, 1);
    b.rect(4, 4, 2, 2, 2);
    return b;
}

gs::Bitmap stackArt() {
    gs::Bitmap b(40, 22);
    b.rect(6, 6, 30, 14, 1);
    b.rect(3, 3, 30, 14, 2);
    b.rect(0, 0, 30, 14, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_FELT,
           {0, gs::rgb4(0, 4, 2), gs::rgb4(1, 7, 3), gs::rgb4(2, 10, 4), gs::rgb4(0, 5, 3), gs::rgb4(2, 6, 10),
            gs::rgb4(8, 2, 2), gs::rgb4(0, 3, 1)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(6, 4, 1), gs::rgb4(14, 12, 4), gs::rgb4(15, 15, 8), gs::rgb4(12, 8, 2)});
    setPal(vdp, PAL_HOUSE, {0, gs::rgb4(4, 1, 1), gs::rgb4(12, 4, 4), gs::rgb4(15, 8, 6), gs::rgb4(8, 2, 2)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 12), gs::rgb4(0, 2, 1));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 12, 3), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 10), gs::rgb4(0, 3, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 6, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 15, 12), gs::rgb4(0, 2, 2));
    setPal(vdp, PAL_PIP, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12)});

    loadFont(vdp, art);
    art.felt = gs::uploadImage(vdp, feltArt());
    art.card = gs::uploadImage(vdp, cardArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    art.house = gs::uploadImage(vdp, houseArt());
    art.stack = gs::uploadImage(vdp, stackArt());
}

}  // namespace solitaire
