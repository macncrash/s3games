#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace loomgold {
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

gs::Bitmap postArt() {
    gs::Bitmap b(16, 150);
    b.rect(2, 0, 12, 150, 1);
    b.rect(2, 0, 4, 150, 2);
    b.rect(12, 0, 2, 150, 3);
    b.rect(0, 8, 16, 8, 2);
    b.rect(0, 132, 16, 10, 2);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(168, 12);
    b.rect(0, 2, 168, 8, 1);
    b.rect(0, 2, 168, 2, 2);
    b.rect(0, 8, 168, 2, 3);
    return b;
}

gs::Bitmap warpArt() {
    gs::Bitmap b(2, 96);
    b.rect(0, 0, 1, 96, 1);
    b.rect(1, 0, 1, 96, 2);
    return b;
}

gs::Bitmap weftArt(bool gold) {
    gs::Bitmap b(140, 8);
    int body = gold ? 1 : 1;
    int hi = gold ? 2 : 2;
    int lo = gold ? 3 : 3;
    b.rect(0, 1, 140, 6, body);
    b.rect(0, 1, 140, 2, hi);
    b.rect(0, 5, 140, 2, lo);
    for (int x = 4; x < 136; x += 8) b.rect(float(x), 2, 2, 4, hi);
    return b;
}

gs::Bitmap shuttleArt() {
    gs::Bitmap b(36, 12);
    b.ellipse(18.f, 6.f, 17.f, 5.f, 1);
    b.ellipse(18.f, 5.4f, 12.f, 2.6f, 2);
    b.rect(10, 4, 16, 4, 3);
    b.ellipse(14.f, 6.f, 3.f, 2.4f, 4);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(148, 8);
    b.rect(0, 0, 148, 2, 1);
    b.rect(0, 6, 148, 2, 1);
    for (int x = 3; x < 145; x += 5) b.rect(float(x), 1, 1, 6, 2);
    return b;
}

gs::Bitmap bobbinArt() {
    gs::Bitmap b(14, 18);
    b.ellipse(7.f, 9.f, 6.f, 7.f, 1);
    b.ellipse(7.f, 8.f, 3.f, 4.f, 2);
    b.rect(6, 2, 2, 14, 3);
    return b;
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
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 14, 12), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(4, 2, 0));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 7, 6), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_CREAM, gs::rgb4(14, 12, 9), gs::rgb4(8, 6, 4));
    vdp.setColor(PAL_CREAM * 16 + 3, gs::rgb4(6, 5, 3));
    vdp.setColor(PAL_GOLD * 16 + 3, gs::rgb4(8, 5, 1));
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(6, 3, 1), gs::rgb4(11, 7, 3), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_WARP, {0, gs::rgb4(13, 12, 10), gs::rgb4(8, 7, 6)});
    setPal(vdp, PAL_SHUTTLE, {0, gs::rgb4(5, 3, 1), gs::rgb4(12, 8, 3), gs::rgb4(14, 12, 7), gs::rgb4(15, 12, 3)});

    loadFont(vdp, art);
    art.post = gs::uploadImage(vdp, postArt());
    art.beam = gs::uploadImage(vdp, beamArt());
    art.warp = gs::uploadImage(vdp, warpArt());
    art.weftGold = gs::uploadImage(vdp, weftArt(true));
    art.weftCream = gs::uploadImage(vdp, weftArt(false));
    art.shuttle = gs::uploadImage(vdp, shuttleArt());
    art.reed = gs::uploadImage(vdp, reedArt());
    art.bobbin = gs::uploadImage(vdp, bobbinArt());
}

}  // namespace loomgold
