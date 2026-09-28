#include "game/art.h"

#include "console/gfx.h"

namespace mark {
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

// 0 plaster, 1 clay, 2 gold, 3 ink, 4 bone, 5 leaf.
constexpr int kMark[CELLS] = {
    2, 2, 2, 2, 2, 2, 2, 2, 5, 5, 4, 5, 5, 2, 2, 5, 3, 4, 3, 5, 2, 2, 4, 4, 1, 4, 4, 2,
    2, 5, 3, 4, 3, 5, 2, 2, 5, 5, 4, 5, 5, 2, 2, 2, 2, 2, 2, 2, 2,
};

gs::Bitmap tessera(int fill, int hi) {
    gs::Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 1);
    b.rect(1, 1, 14, 14, fill);
    b.rect(2, 2, 7, 2, hi);
    b.rect(2, 2, 2, 6, hi);
    b.set(12, 12, 1);
    b.set(13, 11, 1);
    return b;
}

gs::Bitmap plaster() {
    gs::Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 12);
    b.set(3, 5, 1);
    b.set(11, 4, 13);
    b.set(8, 11, 1);
    b.set(13, 13, 13);
    b.set(5, 13, 1);
    return b;
}

gs::Bitmap ring() {
    gs::Bitmap b(18, 18);
    b.rect(0, 0, 18, 2, 14);
    b.rect(0, 16, 18, 2, 14);
    b.rect(0, 0, 2, 18, 14);
    b.rect(16, 0, 2, 18, 14);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    for (int i = 0; i < CELLS; i++) art.mark[i] = kMark[i];

    for (int i = 0; i < 16; i++) vdp.setColor(PAL_TILE * 16 + i, 0);
    vdp.setColor(PAL_TILE * 16 + 1, gs::rgb4(4, 3, 3));
    vdp.setColor(PAL_TILE * 16 + 2, gs::rgb4(12, 4, 3));
    vdp.setColor(PAL_TILE * 16 + 3, gs::rgb4(15, 8, 5));
    vdp.setColor(PAL_TILE * 16 + 4, gs::rgb4(13, 9, 2));
    vdp.setColor(PAL_TILE * 16 + 5, gs::rgb4(15, 13, 4));
    vdp.setColor(PAL_TILE * 16 + 6, gs::rgb4(2, 3, 9));
    vdp.setColor(PAL_TILE * 16 + 7, gs::rgb4(5, 7, 14));
    vdp.setColor(PAL_TILE * 16 + 8, gs::rgb4(13, 12, 9));
    vdp.setColor(PAL_TILE * 16 + 9, gs::rgb4(15, 15, 12));
    vdp.setColor(PAL_TILE * 16 + 10, gs::rgb4(3, 8, 4));
    vdp.setColor(PAL_TILE * 16 + 11, gs::rgb4(7, 13, 6));
    vdp.setColor(PAL_TILE * 16 + 12, gs::rgb4(8, 7, 6));
    vdp.setColor(PAL_TILE * 16 + 13, gs::rgb4(6, 5, 4));
    vdp.setColor(PAL_TILE * 16 + 14, gs::rgb4(15, 14, 8));

    textPal(vdp, PAL_CREAM, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 8, 9));
    textPal(vdp, PAL_LEAF, gs::rgb4(7, 15, 8));

    const int fill[6] = {12, 2, 4, 6, 8, 10};
    const int hi[6] = {13, 3, 5, 7, 9, 11};
    art.cell[0] = gs::uploadImage(vdp, plaster());
    for (int i = 1; i <= INKS; i++) art.cell[i] = gs::uploadImage(vdp, tessera(fill[i], hi[i]));
    art.ring = gs::uploadImage(vdp, ring());

    gs::Bitmap pic(N * 16, N * 16);
    for (int i = 0; i < CELLS; i++) {
        const gs::Bitmap& src = (art.mark[i] == 0) ? plaster() : tessera(fill[art.mark[i]], hi[art.mark[i]]);
        pic.blit(src, (i % N) * 16, (i / N) * 16);
    }
    art.picture = gs::uploadImage(vdp, pic);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(3, 2, 2));
}

}  // namespace mark
