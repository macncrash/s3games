#include "game/art.h"

#include <initializer_list>

namespace boccebell {
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
    gs::Bitmap b(232, 156);
    b.rect(0, 0, 232, 156, 1);
    b.rect(8, 8, 216, 140, 2);
    b.rect(8, 118, 216, 3, 4);
    b.line(18.f, 22.f, 214.f, 22.f, 5, 1);
    for (int i = 0; i < 7; i++) b.rect(22 + i * 28, 12, 3, 8, 6);
    b.ellipse(116.f, 62.f, 22.f, 16.f, 7);
    b.ellipse(116.f, 62.f, 14.f, 10.f, 8);
    return b;
}

gs::Bitmap bowlArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8.f, 8.f, 7.f, 7.f, 1);
    b.ellipse(8.f, 8.f, 5.f, 5.f, 2);
    b.ellipse(5.4f, 5.4f, 1.6f, 1.2f, 3);
    return b;
}

gs::Bitmap pallinoArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4.f, 4.f, 3.2f, 3.2f, 1);
    b.ellipse(4.f, 4.f, 1.5f, 1.5f, 2);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(18, 16);
    b.rect(8, 0, 2, 3, 1);
    b.ellipse(9.f, 8.f, 7.f, 6.f, 2);
    b.rect(3, 8, 12, 5, 0);
    b.ellipse(9.f, 8.f, 5.f, 3.2f, 3);
    b.ellipse(9.f, 13.f, 1.2f, 1.2f, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_COURT,
           {0, gs::rgb4(4, 6, 3), gs::rgb4(12, 9, 5), gs::rgb4(8, 6, 3), gs::rgb4(15, 14, 10), gs::rgb4(14, 14, 12),
            gs::rgb4(3, 8, 4), gs::rgb4(13, 10, 3), gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_BOWL, {0, gs::rgb4(6, 2, 2), gs::rgb4(13, 4, 3), gs::rgb4(15, 12, 9)});
    setPal(vdp, PAL_PALLINO, {0, gs::rgb4(12, 12, 12), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(6, 5, 2), gs::rgb4(12, 9, 2), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 8)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 4), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 11), gs::rgb4(1, 4, 1));
    textPal(vdp, PAL_DEAD, gs::rgb4(15, 8, 6), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 15, 12), gs::rgb4(1, 2, 2));

    loadFont(vdp, art);
    art.court = gs::uploadImage(vdp, courtArt());
    art.bowl = gs::uploadImage(vdp, bowlArt());
    art.pallino = gs::uploadImage(vdp, pallinoArt());
    art.bell = gs::uploadImage(vdp, bellArt());
    art.banner = gs::uploadImage(vdp, gs::textBitmap("BOCCE BELL", {2, 1, 2, 0, 1}));
}

}  // namespace boccebell
