#include "game/art.h"

#include <initializer_list>

namespace boccechime {
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
    gs::Bitmap b(248, 168);
    b.rect(0, 0, 248, 168, 1);
    b.rect(6, 6, 236, 156, 2);
    b.rect(6, 128, 236, 3, 4);
    b.line(16.f, 18.f, 232.f, 18.f, 5, 1);
    for (int i = 0; i < 8; i++) b.rect(18 + i * 28, 8, 2, 8, 6);
    b.ellipse(124.f, 50.f, 22.f, 16.f, 7);
    b.ellipse(124.f, 50.f, 12.f, 8.f, 8);
    return b;
}

gs::Bitmap bowlArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8.f, 8.f, 7.f, 7.f, 1);
    b.ellipse(8.f, 8.f, 5.f, 5.f, 2);
    b.ellipse(5.2f, 5.2f, 1.5f, 1.1f, 3);
    return b;
}

gs::Bitmap pallinoArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4.f, 4.f, 3.1f, 3.1f, 1);
    b.ellipse(3.2f, 3.2f, 1.2f, 1.0f, 2);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(36, 44);
    b.rect(15, 28, 6, 16, 1);
    b.rect(8, 40, 20, 4, 1);
    b.ellipse(18.f, 16.f, 14.f, 14.f, 2);
    b.ellipse(18.f, 16.f, 11.f, 11.f, 3);
    b.rect(17, 6, 2, 6, 4);
    b.rect(17, 16, 7, 2, 4);
    b.ellipse(18.f, 4.f, 1.4f, 1.4f, 5);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_COURT,
           {0, gs::rgb4(3, 5, 3), gs::rgb4(11, 8, 4), gs::rgb4(7, 5, 3), gs::rgb4(14, 13, 9), gs::rgb4(13, 13, 11),
            gs::rgb4(2, 7, 3), gs::rgb4(12, 9, 2), gs::rgb4(14, 12, 5)});
    setPal(vdp, PAL_BOWL, {0, gs::rgb4(5, 2, 2), gs::rgb4(12, 4, 3), gs::rgb4(15, 11, 8)});
    setPal(vdp, PAL_PALL, {0, gs::rgb4(11, 12, 13), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(4, 4, 5), gs::rgb4(9, 8, 6), gs::rgb4(14, 13, 10), gs::rgb4(2, 2, 3),
                            gs::rgb4(15, 12, 4)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 5), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 11), gs::rgb4(1, 4, 1));
    textPal(vdp, PAL_DEAD, gs::rgb4(15, 8, 6), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 15, 12), gs::rgb4(1, 2, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 14, 6), gs::rgb4(4, 3, 0));

    loadFont(vdp, art);
    art.court = gs::uploadImage(vdp, courtArt());
    art.bowl = gs::uploadImage(vdp, bowlArt());
    art.pallino = gs::uploadImage(vdp, pallinoArt());
    art.clock = gs::uploadImage(vdp, clockArt());
    art.banner = gs::uploadImage(vdp, gs::textBitmap("BOCCE CHIME", {2, 1, 2, 0, 1}));
}

}  // namespace boccechime
