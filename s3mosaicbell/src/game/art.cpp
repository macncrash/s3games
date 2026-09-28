#include "game/art.h"

#include "console/gfx.h"

namespace mosaicbell {
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

// A short chapel bell: plaster, bronze, gold, soot, bone clapper.
constexpr int kMark[CELLS] = {
    0, 3, 3, 3, 0, 3, 2, 1, 2, 3, 3, 1, 1, 1, 3, 3, 1, 4, 1, 3, 0, 3, 1, 3, 0, 0, 0, 3, 0, 0,
};

gs::Bitmap tessera(int fill, int hi) {
    gs::Bitmap b(14, 14);
    b.rect(0, 0, 14, 14, 1);
    b.rect(1, 1, 12, 12, fill);
    b.rect(2, 2, 6, 2, hi);
    b.rect(2, 2, 2, 5, hi);
    b.set(11, 11, 1);
    b.set(10, 12, 1);
    return b;
}

gs::Bitmap plaster() {
    gs::Bitmap b(14, 14);
    b.rect(0, 0, 14, 14, 12);
    b.set(3, 4, 1);
    b.set(10, 3, 13);
    b.set(7, 10, 1);
    b.set(11, 11, 13);
    return b;
}

gs::Bitmap cursor() {
    gs::Bitmap b(16, 16);
    b.rect(0, 0, 16, 2, 14);
    b.rect(0, 14, 16, 2, 14);
    b.rect(0, 0, 2, 16, 14);
    b.rect(14, 0, 2, 16, 14);
    return b;
}

gs::Bitmap bellShape() {
    gs::Bitmap b(28, 36);
    b.rect(12, 0, 4, 4, 5);
    b.ellipse(14, 18, 12, 12, 2);
    b.ellipse(14, 16, 8, 8, 3);
    b.rect(13, 28, 2, 6, 6);
    b.ellipse(14, 34, 3, 2, 6);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    for (int i = 0; i < CELLS; i++) art.mark[i] = kMark[i];

    for (int i = 0; i < 16; i++) vdp.setColor(PAL_TILE * 16 + i, 0);
    vdp.setColor(PAL_TILE * 16 + 1, gs::rgb4(3, 2, 2));
    vdp.setColor(PAL_TILE * 16 + 2, gs::rgb4(10, 5, 2));
    vdp.setColor(PAL_TILE * 16 + 3, gs::rgb4(14, 9, 4));
    vdp.setColor(PAL_TILE * 16 + 4, gs::rgb4(13, 10, 2));
    vdp.setColor(PAL_TILE * 16 + 5, gs::rgb4(15, 13, 5));
    vdp.setColor(PAL_TILE * 16 + 6, gs::rgb4(2, 2, 4));
    vdp.setColor(PAL_TILE * 16 + 7, gs::rgb4(6, 6, 8));
    vdp.setColor(PAL_TILE * 16 + 8, gs::rgb4(14, 13, 11));
    vdp.setColor(PAL_TILE * 16 + 9, gs::rgb4(15, 15, 13));
    vdp.setColor(PAL_TILE * 16 + 12, gs::rgb4(7, 6, 5));
    vdp.setColor(PAL_TILE * 16 + 13, gs::rgb4(5, 4, 4));
    vdp.setColor(PAL_TILE * 16 + 14, gs::rgb4(15, 14, 8));

    textPal(vdp, PAL_CREAM, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 8, 9));
    textPal(vdp, PAL_LEAF, gs::rgb4(7, 15, 8));

    const int fill[5] = {12, 2, 4, 6, 8};
    const int hi[5] = {13, 3, 5, 7, 9};
    art.cell[0] = gs::uploadImage(vdp, plaster());
    for (int i = 1; i <= INKS; i++) art.cell[i] = gs::uploadImage(vdp, tessera(fill[i], hi[i]));
    art.ring = gs::uploadImage(vdp, cursor());
    art.bell = gs::uploadImage(vdp, bellShape());

    gs::Bitmap pic(COLS * 14, ROWS * 14);
    for (int i = 0; i < CELLS; i++) {
        const gs::Bitmap src = (art.mark[i] == 0) ? plaster() : tessera(fill[art.mark[i]], hi[art.mark[i]]);
        pic.blit(src, (i % COLS) * 14, (i / COLS) * 14);
    }
    art.picture = gs::uploadImage(vdp, pic);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(2, 2, 3));
}

}  // namespace mosaicbell
