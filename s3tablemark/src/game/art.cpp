#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace tablemark {
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

gs::Bitmap tableArt() {
    gs::Bitmap b(240, 160);
    b.rect(0, 0, 240, 160, 1);
    b.rect(6, 6, 228, 148, 2);
    b.rect(10, 78, 220, 2, 3);
    b.ellipse(120.f, 80.f, 18.f, 18.f, 3);
    float mx = 120.f;
    float my = 28.f;
    b.ellipse(mx, my, 16.f, 16.f, 4);
    b.ellipse(mx, my, 10.f, 10.f, 2);
    b.ellipse(mx, my, 3.f, 3.f, 5);
    b.line(mx - 18.f, my, mx - 12.f, my, 4, 1);
    b.line(mx + 12.f, my, mx + 18.f, my, 4, 1);
    b.line(mx, my - 18.f, mx, my - 12.f, 4, 1);
    b.line(mx, my + 12.f, mx, my + 18.f, 4, 1);
    return b;
}

gs::Bitmap puckArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6.f, 6.f, 5.2f, 5.2f, 1);
    b.ellipse(6.f, 6.f, 3.f, 3.f, 2);
    b.ellipse(4.4f, 4.4f, 1.1f, 1.1f, 3);
    return b;
}

gs::Bitmap malletArt() {
    gs::Bitmap b(26, 26);
    b.ellipse(13.f, 13.f, 12.f, 12.f, 1);
    b.ellipse(13.f, 13.f, 8.f, 8.f, 2);
    b.ellipse(13.f, 13.f, 3.f, 3.f, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TABLE,
           {0, gs::rgb4(6, 3, 1), gs::rgb4(2, 8, 10), gs::rgb4(8, 14, 15), gs::rgb4(14, 11, 3), gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_PUCK, {0, gs::rgb4(2, 2, 3), gs::rgb4(8, 8, 9), gs::rgb4(14, 14, 15)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(12, 2, 2), gs::rgb4(15, 6, 4), gs::rgb4(15, 12, 8)});
    setPal(vdp, PAL_THEM, {0, gs::rgb4(1, 3, 10), gs::rgb4(3, 7, 14), gs::rgb4(10, 13, 15)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(14, 11, 2)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 14, 8), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 12), gs::rgb4(1, 4, 1));
    textPal(vdp, PAL_HINT, gs::rgb4(6, 15, 10), gs::rgb4(1, 2, 2));

    loadFont(vdp, art);
    art.table = gs::uploadImage(vdp, tableArt());
    art.puck = gs::uploadImage(vdp, puckArt());
    art.mallet = gs::uploadImage(vdp, malletArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("S3 TABLEMARK", {2, 1, 2, 0, 1}));
    art.win = gs::uploadImage(vdp, gs::textBitmap("FINISHED MARK", {2, 1, 2, 0, 1}));
}

}  // namespace tablemark
