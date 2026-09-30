#include "game/art.h"

#include <initializer_list>

namespace tilechime {
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

gs::Bitmap wallArt() {
    gs::Bitmap b(220, 168);
    b.rect(0, 0, 220, 168, 1);
    b.rect(8, 10, 204, 148, 2);
    b.ellipse(110, 78, 62, 54, 3);
    b.ellipse(110, 78, 48, 40, 4);
    b.rect(96, 18, 28, 16, 5);
    b.rect(18, 136, 184, 10, 6);
    for (int i = 0; i < 8; i++) b.rect(float(22 + i * 22), 148.f, 16.f, 10.f, (i & 1) ? 7 : 6);
    return b;
}

gs::Bitmap tileArt() {
    gs::Bitmap b(22, 18);
    b.rect(0, 0, 22, 18, 1);
    b.rect(2, 2, 18, 14, 2);
    b.rect(4, 4, 6, 5, 3);
    b.rect(12, 9, 5, 4, 4);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(28, 32);
    b.rect(12, 0, 4, 6, 1);
    b.ellipse(14, 16, 12, 10, 2);
    b.rect(2, 20, 24, 4, 3);
    b.ellipse(14, 26, 3, 3, 4);
    b.rect(6, 10, 4, 6, 1);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, 1);
    b.rect(1, 1, 2, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_WALL,
           {0, gs::rgb4(2, 2, 4), gs::rgb4(4, 4, 6), gs::rgb4(6, 5, 4), gs::rgb4(3, 3, 5), gs::rgb4(5, 4, 3),
            gs::rgb4(7, 6, 4), gs::rgb4(8, 7, 5)});
    setPal(vdp, PAL_CHIME, {0, gs::rgb4(6, 5, 1), gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_PLAIN, {0, gs::rgb4(5, 5, 6), gs::rgb4(9, 9, 10), gs::rgb4(12, 12, 13), gs::rgb4(14, 14, 15)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(8, 6, 2), gs::rgb4(14, 10, 3), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(12, 3, 3), gs::rgb4(15, 12, 8)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 12), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 12, 4), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 10), gs::rgb4(1, 3, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 6, 5), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 15, 13), gs::rgb4(1, 2, 2));

    loadFont(vdp, art);
    art.wall = gs::uploadImage(vdp, wallArt());
    art.tile = gs::uploadImage(vdp, tileArt());
    art.bell = gs::uploadImage(vdp, bellArt());
    art.pip = gs::uploadImage(vdp, pipArt());
}

}  // namespace tilechime
