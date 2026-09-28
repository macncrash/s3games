#include "game/art.h"

#include "console/gfx.h"

namespace mosaicchime {
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

// Clock face: soot rim, bone dial, bronze hands, gold noon pip.
constexpr int kMark[CELLS] = {
    3, 3, 4, 4, 3, 3, 3, 2, 1, 1, 2, 3, 3, 2, 2, 1, 2, 3, 3, 2, 2, 1, 2, 3, 3, 3, 2, 2, 3, 3,
};

gs::Bitmap tessera(int fill, int hi) {
    gs::Bitmap b(12, 12);
    b.rect(0, 0, 12, 12, 1);
    b.rect(1, 1, 10, 10, fill);
    b.rect(2, 2, 5, 2, hi);
    b.set(9, 9, 1);
    return b;
}

gs::Bitmap plaster() {
    gs::Bitmap b(12, 12);
    b.rect(0, 0, 12, 12, 12);
    b.set(2, 3, 13);
    b.set(8, 2, 1);
    b.set(6, 8, 13);
    return b;
}

gs::Bitmap cursor() {
    gs::Bitmap b(14, 14);
    b.rect(0, 0, 14, 2, 14);
    b.rect(0, 12, 14, 2, 14);
    b.rect(0, 0, 2, 14, 14);
    b.rect(12, 0, 2, 14, 14);
    return b;
}

gs::Bitmap tower() {
    gs::Bitmap b(22, 40);
    b.rect(8, 0, 6, 6, 5);
    b.rect(4, 6, 14, 4, 3);
    b.rect(6, 10, 10, 22, 2);
    b.rect(9, 14, 4, 4, 8);
    b.rect(9, 22, 4, 6, 6);
    b.rect(2, 32, 18, 8, 7);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    for (int i = 0; i < CELLS; i++) art.mark[i] = kMark[i];

    for (int i = 0; i < 16; i++) vdp.setColor(PAL_TILE * 16 + i, 0);
    vdp.setColor(PAL_TILE * 16 + 1, gs::rgb4(2, 2, 3));
    vdp.setColor(PAL_TILE * 16 + 2, gs::rgb4(9, 6, 4));
    vdp.setColor(PAL_TILE * 16 + 3, gs::rgb4(14, 10, 5));
    vdp.setColor(PAL_TILE * 16 + 4, gs::rgb4(15, 12, 3));
    vdp.setColor(PAL_TILE * 16 + 5, gs::rgb4(15, 14, 6));
    vdp.setColor(PAL_TILE * 16 + 6, gs::rgb4(3, 3, 5));
    vdp.setColor(PAL_TILE * 16 + 7, gs::rgb4(5, 5, 7));
    vdp.setColor(PAL_TILE * 16 + 8, gs::rgb4(14, 13, 11));
    vdp.setColor(PAL_TILE * 16 + 9, gs::rgb4(15, 15, 13));
    vdp.setColor(PAL_TILE * 16 + 12, gs::rgb4(6, 6, 6));
    vdp.setColor(PAL_TILE * 16 + 13, gs::rgb4(4, 4, 5));
    vdp.setColor(PAL_TILE * 16 + 14, gs::rgb4(15, 14, 8));

    textPal(vdp, PAL_CREAM, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 8, 10));
    textPal(vdp, PAL_LEAF, gs::rgb4(6, 15, 9));

    const int fill[5] = {12, 2, 8, 6, 4};
    const int hi[5] = {13, 3, 9, 7, 5};
    art.cell[0] = gs::uploadImage(vdp, plaster());
    for (int i = 1; i <= INKS; i++) art.cell[i] = gs::uploadImage(vdp, tessera(fill[i], hi[i]));
    art.cursor = gs::uploadImage(vdp, cursor());
    art.tower = gs::uploadImage(vdp, tower());

    gs::Bitmap pic(COLS * 12, ROWS * 12);
    for (int i = 0; i < CELLS; i++) {
        const gs::Bitmap src = (art.mark[i] == 0) ? plaster() : tessera(fill[art.mark[i]], hi[art.mark[i]]);
        pic.blit(src, (i % COLS) * 12, (i / COLS) * 12);
    }
    art.picture = gs::uploadImage(vdp, pic);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(2, 2, 4));
}

}  // namespace mosaicchime
