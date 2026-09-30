#include "game/art.h"

#include <initializer_list>

namespace bellbell {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
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

gs::Bitmap beamArt() {
    gs::Bitmap b(280, 18);
    b.rect(0, 6, 280, 8, 1);
    b.rect(0, 4, 280, 3, 2);
    for (int i = 0; i < 7; i++) b.rect(8 + i * 40, 0, 6, 18, 3);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(36, 44);
    b.rect(16, 0, 4, 6, 1);
    b.ellipse(18.f, 22.f, 16.f, 16.f, 2);
    b.rect(4, 20, 28, 16, 0);
    b.ellipse(18.f, 20.f, 11.f, 8.f, 3);
    b.rect(6, 30, 24, 6, 2);
    b.ellipse(18.f, 36.f, 4.f, 4.f, 4);
    b.rect(16, 32, 4, 8, 1);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(12, 16);
    b.ellipse(6.f, 10.f, 5.f, 5.f, 1);
    b.ellipse(6.f, 8.f, 3.f, 5.f, 2);
    b.ellipse(6.f, 6.f, 1.4f, 2.4f, 3);
    return b;
}

gs::Bitmap malletArt() {
    gs::Bitmap b(28, 14);
    b.rect(12, 2, 4, 10, 1);
    b.ellipse(8.f, 7.f, 7.f, 6.f, 2);
    b.ellipse(6.f, 6.f, 2.f, 1.4f, 3);
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(300, 28);
    b.rect(0, 0, 300, 28, 1);
    for (int i = 0; i < 10; i++) {
        b.rect(4 + i * 30, 4, 22, 8, 2);
        b.rect(10 + i * 30, 16, 18, 8, (i & 1) ? 3 : 2);
    }
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TOWER,
           {0, gs::rgb4(4, 3, 3), gs::rgb4(8, 6, 5), gs::rgb4(3, 2, 2), gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(5, 4, 2), gs::rgb4(12, 9, 3), gs::rgb4(15, 12, 5), gs::rgb4(15, 15, 9)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(12, 4, 1), gs::rgb4(15, 10, 2), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_MALLET, {0, gs::rgb4(6, 3, 1), gs::rgb4(10, 6, 3), gs::rgb4(14, 12, 8)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 4), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 11), gs::rgb4(1, 4, 1));
    textPal(vdp, PAL_DEAD, gs::rgb4(15, 8, 6), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 15, 12), gs::rgb4(1, 2, 2));

    loadFont(vdp, art);
    art.beam = gs::uploadImage(vdp, beamArt());
    art.bell = gs::uploadImage(vdp, bellArt());
    art.mark = gs::uploadImage(vdp, markArt());
    art.mallet = gs::uploadImage(vdp, malletArt());
    art.stone = gs::uploadImage(vdp, stoneArt());
}

}  // namespace bellbell
