#include "game/art.h"

#include <initializer_list>

namespace juggleseven {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
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

gs::Bitmap jugglerArt() {
    gs::Bitmap b(44, 80);
    b.ellipse(22.f, 11.f, 8.f, 8.f, 1);
    b.ellipse(19.f, 10.f, 1.2f, 1.3f, 2);
    b.ellipse(25.f, 10.f, 1.2f, 1.3f, 2);
    b.rect(17.f, 18.f, 10.f, 4.f, 1);
    b.rect(12.f, 22.f, 20.f, 24.f, 3);
    b.rect(12.f, 22.f, 20.f, 4.f, 4);
    b.rect(14.f, 30.f, 5.f, 7.f, 5);
    b.rect(25.f, 30.f, 5.f, 7.f, 5);
    b.rect(14.f, 46.f, 6.f, 24.f, 6);
    b.rect(24.f, 46.f, 6.f, 24.f, 6);
    b.rect(12.f, 68.f, 10.f, 5.f, 7);
    b.rect(22.f, 68.f, 10.f, 5.f, 7);
    b.line(12.f, 26.f, 3.f, 42.f, 1, 3.f);
    b.line(32.f, 26.f, 41.f, 42.f, 1, 3.f);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(28, 48);
    b.ellipse(14.f, 8.f, 5.f, 5.f, 1);
    b.rect(8.f, 13.f, 12.f, 16.f, 3);
    b.rect(8.f, 13.f, 12.f, 3.f, 4);
    b.rect(9.f, 29.f, 4.f, 14.f, 5);
    b.rect(15.f, 29.f, 4.f, 14.f, 5);
    b.line(8.f, 16.f, 2.f, 26.f, 1, 2.2f);
    b.line(20.f, 16.f, 26.f, 26.f, 1, 2.2f);
    return b;
}

gs::Bitmap gloveArt() {
    gs::Bitmap b(18, 14);
    b.ellipse(9.f, 8.f, 7.f, 4.5f, 1);
    b.rect(2.f, 2.f, 3.f, 6.f, 1);
    b.rect(7.f, 1.f, 3.f, 6.f, 1);
    b.rect(12.f, 2.f, 3.f, 6.f, 1);
    b.rect(2.f, 7.f, 13.f, 2.f, 2);
    return b;
}

gs::Bitmap bean(int stripe) {
    gs::Bitmap b(16, 16);
    b.ellipse(8.f, 8.f, 7.f, 7.f, 3);
    b.ellipse(8.f, 8.f, 5.4f, 5.4f, 1);
    b.ellipse(6.f, 6.f, 1.8f, 1.2f, 2);
    b.rect(7.f, 3.f, 2.f, 10.f, stripe);
    return b;
}

gs::Bitmap floorArt() {
    gs::Bitmap b(240, 24);
    b.rect(0.f, 0.f, 240.f, 6.f, 1);
    b.rect(0.f, 6.f, 240.f, 18.f, 2);
    for (int i = 0; i < 12; i++) b.rect(float(i * 20), 6.f, 2.f, 18.f, 3);
    return b;
}

gs::Bitmap boothArt() {
    gs::Bitmap b(70, 90);
    b.rect(4.f, 8.f, 62.f, 82.f, 1);
    b.rect(10.f, 16.f, 50.f, 60.f, 2);
    b.rect(0.f, 0.f, 70.f, 10.f, 3);
    b.rect(28.f, 76.f, 14.f, 14.f, 4);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 36);
    b.rect(7.f, 12.f, 2.f, 24.f, 3);
    b.ellipse(8.f, 8.f, 6.f, 6.f, 1);
    b.ellipse(6.f, 6.f, 2.f, 1.4f, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_BODY,
           {0, gs::rgb4(14, 11, 8), gs::rgb4(3, 2, 2), gs::rgb4(12, 3, 4), gs::rgb4(15, 12, 6), gs::rgb4(8, 2, 3),
            gs::rgb4(4, 3, 8), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_GLOVE, {0, gs::rgb4(15, 14, 10), gs::rgb4(8, 6, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 12, 10), gs::rgb4(6, 1, 1), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_BLUE, {0, gs::rgb4(4, 8, 15), gs::rgb4(12, 14, 15), gs::rgb4(1, 2, 6), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_STAGE, {0, gs::rgb4(10, 7, 4), gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(9, 8, 12), gs::rgb4(3, 2, 5), gs::rgb4(5, 3, 8), gs::rgb4(12, 10, 6), gs::rgb4(2, 2, 4)});
    textPal(vdp, PAL_INK, gs::rgb4(14, 13, 12));
    textPal(vdp, PAL_MARK, gs::rgb4(15, 12, 5));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4));
    textPal(vdp, PAL_TITLE, gs::rgb4(12, 14, 15));
    textPal(vdp, PAL_WIN, gs::rgb4(8, 15, 8));
    loadFont(vdp, art);
    art.juggler = gs::uploadImage(vdp, jugglerArt());
    art.rival = gs::uploadImage(vdp, rivalArt());
    art.glove = gs::uploadImage(vdp, gloveArt());
    art.red = gs::uploadImage(vdp, bean(4));
    art.blue = gs::uploadImage(vdp, bean(4));
    art.floor = gs::uploadImage(vdp, floorArt());
    art.booth = gs::uploadImage(vdp, boothArt());
    art.lamp = gs::uploadImage(vdp, lampArt());
}

}  // namespace juggleseven
