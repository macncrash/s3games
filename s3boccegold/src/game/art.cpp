#include "game/art.h"

#include <initializer_list>

namespace boccegold {
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

gs::Bitmap courtArt() {
    gs::Bitmap b(240, 168);
    b.rect(0, 0, 240, 168, 1);
    b.rect(6, 6, 228, 156, 2);
    b.rect(6, 140, 228, 4, 3);
    b.line(16.f, 18.f, 224.f, 18.f, 4, 1);
    for (int i = 0; i < 8; i++) b.rect(16 + i * 28, 10, 3, 7, 5);
    b.ellipse(120.f, 36.f, 10.f, 6.f, 4);
    return b;
}

gs::Bitmap bowlArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8.f, 8.f, 7.f, 7.f, 1);
    b.ellipse(8.f, 8.f, 5.2f, 5.2f, 2);
    b.ellipse(5.6f, 5.6f, 1.5f, 1.3f, 3);
    return b;
}

gs::Bitmap jackArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5.f, 5.f, 4.f, 4.f, 1);
    b.ellipse(5.f, 5.f, 2.1f, 2.1f, 2);
    b.ellipse(3.6f, 3.6f, 0.8f, 0.8f, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_COURT,
           {0, gs::rgb4(5, 4, 2), gs::rgb4(11, 9, 5), gs::rgb4(14, 11, 6), gs::rgb4(15, 15, 11), gs::rgb4(2, 7, 3)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(10, 7, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(11, 9, 6), gs::rgb4(15, 13, 9), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_RIVAL, {0, gs::rgb4(2, 3, 8), gs::rgb4(4, 7, 13), gs::rgb4(12, 14, 15)});
    setPal(vdp, PAL_JACK, {0, gs::rgb4(12, 12, 12), gs::rgb4(15, 15, 15), gs::rgb4(8, 8, 8)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 4), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 11), gs::rgb4(1, 4, 1));
    textPal(vdp, PAL_HINT, gs::rgb4(9, 15, 11), gs::rgb4(1, 2, 2));
    textPal(vdp, PAL_AIM, gs::rgb4(15, 15, 15), gs::rgb4(3, 3, 3));

    loadFont(vdp, art);
    art.court = gs::uploadImage(vdp, courtArt());
    art.bowl = gs::uploadImage(vdp, bowlArt());
    art.jack = gs::uploadImage(vdp, jackArt());
    art.banner = gs::uploadImage(vdp, gs::textBitmap("GOLD DOUBLE", {2, 1, 2, 0, 1}));
}

}  // namespace boccegold
