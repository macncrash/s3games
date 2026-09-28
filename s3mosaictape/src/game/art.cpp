#include "game/art.h"

#include "console/gfx.h"

namespace tape {
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

// Each tessera is a different mosaic chip. Index is identity, not position.
gs::Bitmap chip(int kind) {
    gs::Bitmap b(20, 20);
    b.rect(0, 0, 20, 20, 1);
    const int body[N] = {2, 4, 6, 8, 10, 12, 3, 5};
    const int hi[N] = {14, 15, 7, 9, 11, 13, 14, 15};
    b.rect(1, 1, 18, 18, body[kind]);
    b.rect(2, 2, 8, 2, hi[kind]);
    switch (kind % 4) {
    case 0:
        b.rect(6, 6, 8, 8, hi[kind]);
        break;
    case 1:
        b.ellipse(10, 10, 5, 5, hi[kind]);
        break;
    case 2:
        b.rect(4, 9, 12, 3, hi[kind]);
        b.rect(9, 4, 3, 12, hi[kind]);
        break;
    default:
        b.rect(4, 4, 4, 4, hi[kind]);
        b.rect(12, 12, 4, 4, hi[kind]);
        break;
    }
    b.set(16, 16, 1);
    return b;
}

gs::Bitmap strip(int w, int h, int fill, int edge) {
    gs::Bitmap b(w, h);
    b.rect(0, 0, w, h, edge);
    b.rect(2, 2, w - 4, h - 4, fill);
    return b;
}

gs::Bitmap cursor() {
    gs::Bitmap b(24, 6);
    b.rect(0, 2, 24, 2, 14);
    b.rect(10, 0, 4, 6, 14);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    // The tape's picture: a fixed mosaic sequence, not sorted by colour.
    const int order[N] = {3, 0, 6, 1, 7, 4, 2, 5};
    for (int i = 0; i < N; i++) art.order[i] = order[i];

    for (int i = 0; i < 16; i++) vdp.setColor(PAL_TILE * 16 + i, 0);
    vdp.setColor(PAL_TILE * 16 + 1, gs::rgb4(3, 2, 2));
    vdp.setColor(PAL_TILE * 16 + 2, gs::rgb4(12, 4, 3));
    vdp.setColor(PAL_TILE * 16 + 3, gs::rgb4(15, 8, 4));
    vdp.setColor(PAL_TILE * 16 + 4, gs::rgb4(13, 10, 2));
    vdp.setColor(PAL_TILE * 16 + 5, gs::rgb4(15, 14, 5));
    vdp.setColor(PAL_TILE * 16 + 6, gs::rgb4(2, 3, 10));
    vdp.setColor(PAL_TILE * 16 + 7, gs::rgb4(6, 8, 15));
    vdp.setColor(PAL_TILE * 16 + 8, gs::rgb4(13, 12, 9));
    vdp.setColor(PAL_TILE * 16 + 9, gs::rgb4(15, 15, 12));
    vdp.setColor(PAL_TILE * 16 + 10, gs::rgb4(2, 8, 4));
    vdp.setColor(PAL_TILE * 16 + 11, gs::rgb4(6, 13, 6));
    vdp.setColor(PAL_TILE * 16 + 12, gs::rgb4(9, 3, 8));
    vdp.setColor(PAL_TILE * 16 + 13, gs::rgb4(14, 6, 10));
    vdp.setColor(PAL_TILE * 16 + 14, gs::rgb4(15, 13, 6));
    vdp.setColor(PAL_TILE * 16 + 15, gs::rgb4(8, 6, 4));

    textPal(vdp, PAL_CREAM, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 8, 9));
    textPal(vdp, PAL_LEAF, gs::rgb4(7, 15, 8));

    for (int i = 0; i < N; i++) art.piece[i] = gs::uploadImage(vdp, chip(i));
    art.tape = gs::uploadImage(vdp, strip(N * 24 + 8, 28, 8, 1));
    art.drawer = gs::uploadImage(vdp, strip(N * 24 + 12, 36, 15, 1));
    art.cursor = gs::uploadImage(vdp, cursor());
    loadFont(vdp, art);
}

}  // namespace tape
