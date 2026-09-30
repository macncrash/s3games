#include "game/art.h"

namespace cuegold {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
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

gs::Bitmap ballArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6.2f, 6.2f, 1);
    b.ellipse(5.2f, 5.0f, 2.4f, 1.8f, 2);
    b.set(10, 10, 3);
    return b;
}

gs::Bitmap cueArt() {
    gs::Bitmap b(48, 8);
    b.rect(0, 3, 36, 2, 1);
    b.rect(36, 2, 8, 4, 2);
    b.rect(44, 2, 4, 4, 3);
    b.set(2, 3, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 15, 14), gs::rgb4(2, 3, 3), gs::rgb4(8, 8, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12), gs::rgb4(10, 7, 1)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(14, 12, 8), gs::rgb4(15, 15, 13), gs::rgb4(8, 7, 4)});
    setPal(vdp, PAL_FELT, {0, gs::rgb4(1, 8, 3), gs::rgb4(2, 11, 4), gs::rgb4(0, 5, 2)});
    setPal(vdp, PAL_CUEBALL, {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), gs::rgb4(7, 8, 9)});
    setPal(vdp, PAL_CUE, {0, gs::rgb4(12, 8, 3), gs::rgb4(6, 3, 1), gs::rgb4(15, 14, 10), gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_RAIL, {0, gs::rgb4(5, 3, 2), gs::rgb4(9, 6, 3), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_POCKET, {0, gs::rgb4(1, 1, 1), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_WORD, {0, gs::rgb4(15, 14, 6), gs::rgb4(3, 2, 1), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, gs::rgb4(5, 3, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 8), gs::rgb4(1, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 5, 4), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_METER, {0, gs::rgb4(4, 4, 5), gs::rgb4(8, 15, 7), gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 3)});
    setPal(vdp, PAL_SHADOW, {0, gs::rgb4(0, 3, 1)});

    loadFont(vdp, art);
    art.ball = gs::uploadMipped(vdp, ballArt());
    art.cue = gs::uploadMipped(vdp, cueArt());
    gs::Bitmap one(1, 1);
    one.set(0, 0, 1);
    art.blot = gs::uploadImage(vdp, one);
    gs::Bitmap sh(8, 4);
    sh.ellipse(4, 2, 3.5f, 1.6f, 1);
    art.shadow = gs::uploadImage(vdp, sh);
}

}  // namespace cuegold
