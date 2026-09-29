#include "game/art.h"

#include "console/gfx.h"

namespace memorytape {
namespace {

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

// Six distinct marks. Kind is identity, not slot.
gs::Bitmap mark(int kind) {
    gs::Bitmap b(22, 22);
    b.rect(0, 0, 22, 22, 1);
    const int body[KINDS] = {2, 4, 6, 8, 10, 12};
    const int ink[KINDS] = {3, 5, 7, 9, 11, 13};
    b.rect(2, 2, 18, 18, body[kind]);
    switch (kind) {
    case 0:
        b.ellipse(11, 11, 5, 5, ink[kind]);
        break;
    case 1:
        b.rect(6, 5, 10, 3, ink[kind]);
        b.rect(9, 5, 4, 12, ink[kind]);
        break;
    case 2:
        b.rect(5, 5, 5, 5, ink[kind]);
        b.rect(12, 12, 5, 5, ink[kind]);
        break;
    case 3:
        b.rect(5, 10, 12, 3, ink[kind]);
        b.rect(10, 5, 3, 12, ink[kind]);
        break;
    case 4:
        b.ellipse(8, 11, 3, 5, ink[kind]);
        b.ellipse(14, 11, 3, 5, ink[kind]);
        break;
    default:
        b.rect(6, 6, 10, 10, ink[kind]);
        b.rect(9, 9, 4, 4, body[kind]);
        break;
    }
    return b;
}

gs::Bitmap strip(int w, int h, int fill, int edge) {
    gs::Bitmap b(w, h);
    b.rect(0, 0, float(w), float(h), edge);
    b.rect(2, 2, float(w - 4), float(h - 4), fill);
    return b;
}

gs::Bitmap cursorBmp() {
    gs::Bitmap b(26, 6);
    b.rect(0, 2, 26, 2, 14);
    b.rect(11, 0, 4, 6, 14);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const int seq[LEN] = {1, 4, 0, 5, 2};
    for (int i = 0; i < LEN; i++) art.tapeSeq[i] = seq[i];

    for (int i = 0; i < 16; i++) vdp.setColor(PAL_GLYPH * 16 + i, 0);
    vdp.setColor(PAL_GLYPH * 16 + 1, gs::rgb4(2, 2, 4));
    vdp.setColor(PAL_GLYPH * 16 + 2, gs::rgb4(4, 7, 12));
    vdp.setColor(PAL_GLYPH * 16 + 3, gs::rgb4(10, 14, 15));
    vdp.setColor(PAL_GLYPH * 16 + 4, gs::rgb4(12, 5, 3));
    vdp.setColor(PAL_GLYPH * 16 + 5, gs::rgb4(15, 10, 5));
    vdp.setColor(PAL_GLYPH * 16 + 6, gs::rgb4(3, 9, 5));
    vdp.setColor(PAL_GLYPH * 16 + 7, gs::rgb4(8, 15, 7));
    vdp.setColor(PAL_GLYPH * 16 + 8, gs::rgb4(10, 8, 3));
    vdp.setColor(PAL_GLYPH * 16 + 9, gs::rgb4(15, 14, 6));
    vdp.setColor(PAL_GLYPH * 16 + 10, gs::rgb4(8, 3, 10));
    vdp.setColor(PAL_GLYPH * 16 + 11, gs::rgb4(14, 7, 13));
    vdp.setColor(PAL_GLYPH * 16 + 12, gs::rgb4(13, 12, 10));
    vdp.setColor(PAL_GLYPH * 16 + 13, gs::rgb4(6, 5, 4));
    vdp.setColor(PAL_GLYPH * 16 + 14, gs::rgb4(15, 13, 8));
    vdp.setColor(PAL_GLYPH * 16 + 15, gs::rgb4(5, 6, 8));

    textPal(vdp, PAL_CREAM, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_DIM, gs::rgb4(7, 8, 10));
    textPal(vdp, PAL_LEAF, gs::rgb4(6, 15, 9));

    for (int i = 0; i < KINDS; i++) art.glyph[i] = gs::uploadImage(vdp, mark(i));
    art.tape = gs::uploadImage(vdp, strip(LEN * 28 + 16, 36, 15, 1));
    art.drawer = gs::uploadImage(vdp, strip(LEN * 28 + 20, 40, 12, 1));
    art.slot = gs::uploadImage(vdp, strip(24, 24, 15, 1));
    art.cursor = gs::uploadImage(vdp, cursorBmp());
    loadFont(vdp, art);
}

}  // namespace memorytape
