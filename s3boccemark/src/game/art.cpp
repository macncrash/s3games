#include "game/art.h"

#include <initializer_list>

namespace boccemark {
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
    gs::Bitmap b(248, 176);
    b.rect(0, 0, 248, 176, 1);
    b.rect(8, 8, 232, 160, 2);
    b.rect(8, 148, 232, 3, 3);
    b.line(20.f, 20.f, 228.f, 20.f, 4, 1);
    b.line(20.f, 156.f, 228.f, 156.f, 3, 1);
    for (int i = 0; i < 9; i++) b.rect(18 + i * 24, 14, 2, 6, 5);
    return b;
}

gs::Bitmap ballArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8.f, 8.f, 7.f, 7.f, 1);
    b.ellipse(8.f, 8.f, 5.f, 5.f, 2);
    b.ellipse(6.f, 6.f, 1.4f, 1.4f, 3);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5.f, 5.f, 4.2f, 4.2f, 1);
    b.ellipse(5.f, 5.f, 2.f, 2.f, 2);
    b.ellipse(3.8f, 3.8f, 0.8f, 0.8f, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_COURT,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(10, 8, 5), gs::rgb4(14, 12, 8), gs::rgb4(15, 15, 12), gs::rgb4(3, 6, 3)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(10, 2, 2), gs::rgb4(14, 5, 3), gs::rgb4(15, 12, 8)});
    setPal(vdp, PAL_THEM, {0, gs::rgb4(1, 3, 9), gs::rgb4(3, 6, 13), gs::rgb4(10, 13, 15)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(12, 9, 2), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 14, 8), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 12), gs::rgb4(1, 4, 1));
    textPal(vdp, PAL_HINT, gs::rgb4(8, 15, 10), gs::rgb4(1, 2, 2));
    textPal(vdp, PAL_AIM, gs::rgb4(15, 15, 15), gs::rgb4(4, 4, 4));

    loadFont(vdp, art);
    art.court = gs::uploadImage(vdp, courtArt());
    art.ball = gs::uploadImage(vdp, ballArt());
    art.mark = gs::uploadImage(vdp, markArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("S3 BOCCEMARK", {2, 1, 2, 0, 1}));
    art.win = gs::uploadImage(vdp, gs::textBitmap("FINISHED MARK", {2, 1, 2, 0, 1}));
}

}  // namespace boccemark
