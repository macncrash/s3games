#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace loomseven {
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
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 1);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 78);
    b.rect(2, 0, 8, 78, 1);
    b.rect(2, 0, 3, 78, 2);
    b.rect(8, 0, 2, 78, 3);
    b.rect(0, 2, 12, 6, 2);
    b.rect(0, 68, 12, 8, 2);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(150, 8);
    b.rect(0, 1, 150, 6, 1);
    b.rect(0, 1, 150, 2, 2);
    b.rect(0, 5, 150, 2, 3);
    return b;
}

gs::Bitmap warpArt() {
    gs::Bitmap b(2, 52);
    b.rect(0, 0, 1, 52, 1);
    b.rect(1, 0, 1, 52, 2);
    return b;
}

gs::Bitmap weftArt() {
    gs::Bitmap b(128, 6);
    b.rect(0, 0, 128, 6, 1);
    b.rect(0, 0, 128, 2, 2);
    b.rect(0, 4, 128, 2, 3);
    for (int x = 4; x < 124; x += 8) b.rect(float(x), 1, 2, 4, 2);
    return b;
}

gs::Bitmap shuttleArt() {
    gs::Bitmap b(28, 10);
    b.ellipse(14.f, 5.f, 13.f, 4.2f, 1);
    b.ellipse(14.f, 4.6f, 8.f, 2.f, 2);
    b.rect(8, 3, 12, 4, 3);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(132, 6);
    b.rect(0, 0, 132, 1, 1);
    b.rect(0, 5, 132, 1, 1);
    for (int x = 2; x < 130; x += 4) b.rect(float(x), 1, 1, 4, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 14, 12), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_YOU, gs::rgb4(8, 12, 15), gs::rgb4(1, 2, 5));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 7, 6), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_HOUSE, gs::rgb4(15, 10, 5), gs::rgb4(5, 2, 0));
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(6, 3, 1), gs::rgb4(11, 7, 3), gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_WARP, {0, gs::rgb4(13, 12, 10), gs::rgb4(7, 6, 5)});
    setPal(vdp, PAL_SHUTTLE, {0, gs::rgb4(4, 3, 2), gs::rgb4(12, 9, 5), gs::rgb4(15, 13, 8)});
    // Weft body lives in the bench palettes (index 1..3).
    vdp.setColor(PAL_YOU * 16 + 1, gs::rgb4(4, 7, 14));
    vdp.setColor(PAL_YOU * 16 + 2, gs::rgb4(9, 13, 15));
    vdp.setColor(PAL_YOU * 16 + 3, gs::rgb4(2, 3, 8));
    vdp.setColor(PAL_HOUSE * 16 + 1, gs::rgb4(12, 6, 2));
    vdp.setColor(PAL_HOUSE * 16 + 2, gs::rgb4(15, 11, 5));
    vdp.setColor(PAL_HOUSE * 16 + 3, gs::rgb4(6, 2, 1));

    loadFont(vdp, art);
    art.post = gs::uploadImage(vdp, postArt());
    art.beam = gs::uploadImage(vdp, beamArt());
    art.warp = gs::uploadImage(vdp, warpArt());
    art.weft = gs::uploadImage(vdp, weftArt());
    art.shuttle = gs::uploadImage(vdp, shuttleArt());
    art.reed = gs::uploadImage(vdp, reedArt());
}

}  // namespace loomseven
