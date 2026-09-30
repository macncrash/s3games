#include "game/art.h"

#include "console/gfx.h"

namespace tiletape {
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

// A short floor tile: wide, low, a different mark on each.
gs::Bitmap shortTile(int kind) {
    gs::Bitmap b(36, 16);
    const int body[N] = {4, 6, 8, 10};
    const int hi[N] = {5, 7, 9, 11};
    const int ink[N] = {12, 13, 14, 15};
    b.rect(0, 0, 36, 16, 1);
    b.rect(1, 1, 34, 12, body[kind]);
    b.rect(1, 13, 34, 2, 2);
    b.rect(2, 2, 14, 2, hi[kind]);
    switch (kind) {
    case 0:
        b.rect(6, 6, 24, 2, ink[kind]);
        b.rect(6, 10, 24, 1, ink[kind]);
        break;
    case 1:
        b.rect(16, 5, 4, 6, ink[kind]);
        b.rect(12, 7, 12, 2, ink[kind]);
        break;
    case 2:
        b.line(8, 10, 18, 5, ink[kind], 1.5f);
        b.line(18, 5, 28, 10, ink[kind], 1.5f);
        break;
    default:
        b.rect(8, 6, 3, 3, ink[kind]);
        b.rect(16, 6, 3, 3, ink[kind]);
        b.rect(24, 6, 3, 3, ink[kind]);
        break;
    }
    return b;
}

gs::Bitmap tray(int w, int h, int fill, int lip) {
    gs::Bitmap b(w, h);
    b.rect(0, 0, w, h, 1);
    b.rect(2, 2, w - 4, h - 4, fill);
    b.rect(2, 2, w - 4, 3, lip);
    return b;
}

gs::Bitmap cursor() {
    gs::Bitmap b(36, 6);
    b.rect(0, 0, 6, 2, 14);
    b.rect(30, 0, 6, 2, 14);
    b.rect(0, 0, 2, 6, 14);
    b.rect(34, 0, 2, 6, 14);
    return b;
}

gs::Bitmap liftMark() {
    gs::Bitmap b(8, 8);
    b.rect(3, 0, 2, 8, 14);
    b.rect(0, 2, 8, 2, 14);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const int order[N] = {2, 0, 3, 1};
    for (int i = 0; i < N; i++) art.order[i] = order[i];

    for (int i = 0; i < 16; i++) vdp.setColor(PAL_TILE * 16 + i, 0);
    vdp.setColor(PAL_TILE * 16 + 1, gs::rgb4(2, 2, 3));
    vdp.setColor(PAL_TILE * 16 + 2, gs::rgb4(5, 4, 3));
    vdp.setColor(PAL_TILE * 16 + 3, gs::rgb4(10, 8, 5));
    vdp.setColor(PAL_TILE * 16 + 4, gs::rgb4(11, 4, 3));
    vdp.setColor(PAL_TILE * 16 + 5, gs::rgb4(15, 8, 5));
    vdp.setColor(PAL_TILE * 16 + 6, gs::rgb4(3, 5, 10));
    vdp.setColor(PAL_TILE * 16 + 7, gs::rgb4(7, 10, 15));
    vdp.setColor(PAL_TILE * 16 + 8, gs::rgb4(12, 9, 3));
    vdp.setColor(PAL_TILE * 16 + 9, gs::rgb4(15, 13, 6));
    vdp.setColor(PAL_TILE * 16 + 10, gs::rgb4(3, 8, 5));
    vdp.setColor(PAL_TILE * 16 + 11, gs::rgb4(7, 13, 8));
    vdp.setColor(PAL_TILE * 16 + 12, gs::rgb4(4, 2, 2));
    vdp.setColor(PAL_TILE * 16 + 13, gs::rgb4(1, 2, 5));
    vdp.setColor(PAL_TILE * 16 + 14, gs::rgb4(6, 4, 1));
    vdp.setColor(PAL_TILE * 16 + 15, gs::rgb4(1, 4, 2));

    textPal(vdp, PAL_CREAM, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 8, 9));
    textPal(vdp, PAL_LEAF, gs::rgb4(7, 15, 8));

    for (int i = 0; i < N; i++) art.tile[i] = gs::uploadImage(vdp, shortTile(i));
    art.tape = gs::uploadImage(vdp, tray(N * 44 + 16, 28, 3, 9));
    art.drawer = gs::uploadImage(vdp, tray(N * 44 + 20, 40, 2, 5));
    art.cursor = gs::uploadImage(vdp, cursor());
    art.lift = gs::uploadImage(vdp, liftMark());
    loadFont(vdp, art);
}

}  // namespace tiletape
