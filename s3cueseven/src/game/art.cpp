#include "game/art.h"

namespace cueseven {
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
                if (g[y * 5 + x]) px[(y + 1) * 8 + x + 1] = 1;
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Bitmap ballArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7.1f, 7.1f, 1);
    b.ellipse(6.2f, 5.6f, 2.6f, 2.0f, 2);
    b.ellipse(11, 11, 1.4f, 1.2f, 3);
    return b;
}

gs::Bitmap cueArt() {
    gs::Bitmap b(56, 6);
    b.rect(0, 2, 40, 2, 1);
    b.rect(40, 1, 10, 4, 2);
    b.rect(50, 1, 6, 4, 3);
    b.set(1, 2, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_INK, {0, gs::rgb4(14, 15, 13), gs::rgb4(2, 3, 2)});
    setPal(vdp, PAL_FELT, {0, gs::rgb4(1, 9, 4), gs::rgb4(2, 12, 5), gs::rgb4(0, 6, 2)});
    setPal(vdp, PAL_RAIL, {0, gs::rgb4(6, 3, 2), gs::rgb4(10, 6, 3), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_POCKET, {0, gs::rgb4(1, 1, 1), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_CUE, {0, gs::rgb4(13, 9, 4), gs::rgb4(7, 3, 1), gs::rgb4(15, 14, 11), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_WHITE, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15), gs::rgb4(6, 7, 8)});
    setPal(vdp, PAL_OBJECT, {0, gs::rgb4(12, 2, 2), gs::rgb4(15, 8, 6), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_WORD, {0, gs::rgb4(15, 14, 8), gs::rgb4(4, 3, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 8), gs::rgb4(1, 5, 2)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 5, 4), gs::rgb4(5, 1, 1)});
    setPal(vdp, PAL_METER, {0, gs::rgb4(3, 4, 5), gs::rgb4(8, 15, 7), gs::rgb4(15, 13, 5), gs::rgb4(15, 6, 3)});
    setPal(vdp, PAL_SHADOW, {0, gs::rgb4(0, 3, 1)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(6, 12, 15), gs::rgb4(2, 4, 8)});

    loadFont(vdp, art);
    art.ball = gs::uploadMipped(vdp, ballArt());
    art.cue = gs::uploadMipped(vdp, cueArt());
    gs::Bitmap one(1, 1);
    one.set(0, 0, 1);
    art.blot = gs::uploadImage(vdp, one);
    gs::Bitmap sh(10, 4);
    sh.ellipse(5, 2, 4.4f, 1.6f, 1);
    art.shadow = gs::uploadImage(vdp, sh);
}

}  // namespace cueseven
