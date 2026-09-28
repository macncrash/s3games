#include "game/art.h"

#include <initializer_list>

namespace juggletape {
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
    gs::Bitmap b(48, 84);
    b.ellipse(24.f, 10.f, 9.f, 7.f, 2);
    b.rect(16.f, 4.f, 16.f, 5.f, 8);
    b.rect(14.f, 8.f, 20.f, 3.f, 8);
    b.ellipse(20.f, 12.f, 1.3f, 1.4f, 3);
    b.ellipse(28.f, 12.f, 1.3f, 1.4f, 3);
    b.rect(20.f, 16.f, 8.f, 2.f, 4);
    b.rect(16.f, 20.f, 16.f, 6.f, 2);
    b.rect(14.f, 26.f, 20.f, 22.f, 5);
    b.rect(14.f, 26.f, 20.f, 4.f, 6);
    b.rect(22.f, 30.f, 4.f, 16.f, 6);
    b.rect(16.f, 48.f, 7.f, 22.f, 7);
    b.rect(25.f, 48.f, 7.f, 22.f, 7);
    b.rect(14.f, 70.f, 11.f, 5.f, 9);
    b.rect(23.f, 70.f, 11.f, 5.f, 9);
    b.line(14.f, 30.f, 4.f, 46.f, 2, 3.2f);
    b.line(34.f, 30.f, 44.f, 46.f, 2, 3.2f);
    return b;
}

gs::Bitmap gloveArt() {
    gs::Bitmap b(20, 14);
    b.ellipse(10.f, 8.f, 8.f, 4.6f, 1);
    b.rect(2.f, 2.f, 3.f, 6.f, 1);
    b.rect(7.f, 1.f, 3.f, 7.f, 1);
    b.rect(12.f, 2.f, 3.f, 6.f, 1);
    b.rect(2.f, 7.f, 15.f, 2.f, 2);
    b.rect(16.f, 4.f, 3.f, 5.f, 3);
    return b;
}

gs::Bitmap ballArt(int body, int shine, int mark) {
    gs::Bitmap b(16, 16);
    b.ellipse(8.f, 8.f, 7.f, 7.f, body);
    b.ellipse(6.2f, 5.6f, 2.2f, 1.6f, shine);
    b.ellipse(10.f, 10.f, 2.4f, 1.8f, mark);
    return b;
}

gs::Bitmap floorArt() {
    gs::Bitmap b(240, 26);
    b.rect(0.f, 0.f, 240.f, 6.f, 1);
    b.rect(0.f, 6.f, 240.f, 20.f, 2);
    for (int i = 0; i < 12; i++) b.rect(float(i * 20), 6.f, 2.f, 20.f, 3);
    b.rect(0.f, 22.f, 240.f, 4.f, 4);
    return b;
}

gs::Bitmap reelArt() {
    gs::Bitmap b(36, 44);
    b.rect(16.f, 0.f, 4.f, 8.f, 4);
    b.ellipse(18.f, 22.f, 14.f, 14.f, 1);
    b.ellipse(18.f, 22.f, 8.f, 8.f, 2);
    b.ellipse(18.f, 22.f, 3.f, 3.f, 3);
    b.rect(4.f, 20.f, 28.f, 3.f, 5);
    b.rect(8.f, 36.f, 20.f, 4.f, 4);
    b.rect(14.f, 40.f, 8.f, 4.f, 4);
    return b;
}

gs::Bitmap drawerArt() {
    gs::Bitmap b(54, 36);
    b.rect(0.f, 0.f, 54.f, 36.f, 1);
    b.rect(3.f, 4.f, 48.f, 22.f, 2);
    b.rect(6.f, 7.f, 42.f, 4.f, 3);
    b.rect(6.f, 13.f, 42.f, 4.f, 4);
    b.rect(6.f, 19.f, 42.f, 4.f, 5);
    b.ellipse(27.f, 30.f, 6.f, 2.4f, 6);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(18, 32);
    b.rect(8.f, 0.f, 2.f, 8.f, 3);
    b.poly({{1.f, 10.f}, {17.f, 10.f}, {13.f, 22.f}, {5.f, 22.f}}, 1);
    b.ellipse(9.f, 13.f, 2.4f, 1.6f, 2);
    b.rect(7.f, 22.f, 4.f, 10.f, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_BODY,
           {0, gs::rgb4(0, 0, 0), gs::rgb4(14, 11, 8), gs::rgb4(2, 1, 1), gs::rgb4(12, 6, 6), gs::rgb4(7, 2, 6),
            gs::rgb4(13, 11, 4), gs::rgb4(3, 2, 5), gs::rgb4(2, 2, 3), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_GLOVE, {0, gs::rgb4(13, 4, 4), gs::rgb4(6, 1, 1), gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(14, 2, 3), gs::rgb4(15, 10, 10), gs::rgb4(6, 0, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 15, 9), gs::rgb4(8, 5, 0)});
    setPal(vdp, PAL_BLUE, {0, gs::rgb4(2, 6, 14), gs::rgb4(10, 13, 15), gs::rgb4(1, 2, 6)});
    setPal(vdp, PAL_STAGE, {0, gs::rgb4(8, 5, 3), gs::rgb4(5, 3, 2), gs::rgb4(10, 7, 4), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_REEL, {0, gs::rgb4(4, 4, 5), gs::rgb4(12, 12, 13), gs::rgb4(2, 2, 3), gs::rgb4(7, 6, 5),
                           gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_DRAWER,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 3), gs::rgb4(14, 3, 3), gs::rgb4(14, 11, 2), gs::rgb4(3, 5, 12),
            gs::rgb4(12, 10, 6)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(14, 12, 6), gs::rgb4(15, 15, 11), gs::rgb4(5, 4, 3)});
    textPal(vdp, PAL_INK, gs::rgb4(14, 13, 12));
    textPal(vdp, PAL_MARK, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 14, 8));
    textPal(vdp, PAL_WIN, gs::rgb4(8, 15, 9));
    loadFont(vdp, art);
    art.juggler = gs::uploadImage(vdp, jugglerArt());
    art.glove = gs::uploadImage(vdp, gloveArt());
    art.red = gs::uploadImage(vdp, ballArt(1, 2, 3));
    art.gold = gs::uploadImage(vdp, ballArt(1, 2, 3));
    art.blue = gs::uploadImage(vdp, ballArt(1, 2, 3));
    art.floor = gs::uploadImage(vdp, floorArt());
    art.reel = gs::uploadImage(vdp, reelArt());
    art.drawer = gs::uploadImage(vdp, drawerArt());
    art.lamp = gs::uploadImage(vdp, lampArt());
}

}  // namespace juggletape
