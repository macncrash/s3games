#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace jugglechime {
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
    gs::Bitmap b(40, 72);
    b.ellipse(20.f, 10.f, 7.f, 7.f, 1);
    b.ellipse(17.5f, 9.5f, 1.1f, 1.2f, 2);
    b.ellipse(22.5f, 9.5f, 1.1f, 1.2f, 2);
    b.rect(16.f, 16.f, 8.f, 4.f, 1);
    b.rect(12.f, 20.f, 16.f, 22.f, 3);
    b.rect(12.f, 20.f, 16.f, 4.f, 4);
    b.rect(14.f, 42.f, 5.f, 22.f, 5);
    b.rect(21.f, 42.f, 5.f, 22.f, 5);
    b.rect(12.f, 62.f, 9.f, 4.f, 6);
    b.rect(19.f, 62.f, 9.f, 4.f, 6);
    b.line(12.f, 24.f, 3.f, 40.f, 1, 2.6f);
    b.line(28.f, 24.f, 37.f, 40.f, 1, 2.6f);
    return b;
}

gs::Bitmap gloveArt() {
    gs::Bitmap b(16, 12);
    b.ellipse(8.f, 7.f, 6.5f, 4.2f, 1);
    b.rect(2.f, 1.f, 2.4f, 5.f, 1);
    b.rect(6.5f, 0.5f, 2.4f, 5.5f, 1);
    b.rect(11.f, 1.f, 2.4f, 5.f, 1);
    b.rect(2.f, 6.f, 12.f, 1.6f, 2);
    return b;
}

gs::Bitmap ballArt(bool gold) {
    gs::Bitmap b(16, 16);
    b.ellipse(8.f, 8.f, 7.f, 7.f, 3);
    b.ellipse(8.f, 8.f, 5.4f, 5.4f, 1);
    b.ellipse(6.2f, 5.8f, 1.8f, 1.2f, 2);
    if (gold) {
        b.line(8.f, 4.2f, 8.f, 11.8f, 4, 1.3f);
        b.line(4.2f, 8.f, 11.8f, 8.f, 4, 1.3f);
    } else {
        b.ellipse(10.f, 10.f, 1.6f, 1.f, 4);
    }
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(28, 32);
    b.rect(13.f, 1.f, 2.f, 6.f, 2);
    b.ellipse(14.f, 16.f, 12.f, 10.f, 1);
    b.rect(2.f, 16.f, 24.f, 10.f, 1);
    b.rect(1.f, 24.f, 26.f, 3.f, 3);
    b.ellipse(14.f, 20.f, 3.f, 4.f, 4);
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(54, 54);
    b.ellipse(27.f, 27.f, 25.f, 25.f, 1);
    b.ellipse(27.f, 27.f, 21.f, 21.f, 2);
    b.ellipse(27.f, 27.f, 2.f, 2.f, 3);
    for (int i = 0; i < 12; i++) {
        float a = float(i) * 3.14159265f / 6.f;
        float x = 27.f + std::cos(a) * 18.f;
        float y = 27.f + std::sin(a) * 18.f;
        b.ellipse(x, y, i % 3 == 0 ? 1.6f : 0.9f, i % 3 == 0 ? 1.6f : 0.9f, 3);
    }
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(4, 4);
    b.ellipse(2.f, 2.f, 1.6f, 1.6f, 1);
    return b;
}

gs::Bitmap floorArt() {
    gs::Bitmap b(200, 22);
    b.rect(0.f, 0.f, 200.f, 6.f, 1);
    b.rect(0.f, 6.f, 200.f, 16.f, 2);
    for (int i = 0; i < 10; i++) b.rect(float(i * 20), 6.f, 2.f, 16.f, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_BODY, {0, gs::rgb4(12, 8, 6), gs::rgb4(2, 1, 1), gs::rgb4(4, 6, 12), gs::rgb4(8, 10, 14),
                           gs::rgb4(2, 2, 6), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_GLOVE, {0, gs::rgb4(13, 4, 4), gs::rgb4(8, 2, 2)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(14, 12, 8), gs::rgb4(15, 15, 12), gs::rgb4(8, 6, 3), gs::rgb4(10, 8, 5)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 15, 8), gs::rgb4(10, 7, 1), gs::rgb4(6, 3, 1)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(12, 11, 6), gs::rgb4(6, 5, 3), gs::rgb4(15, 14, 8), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(10, 8, 5), gs::rgb4(14, 13, 10), gs::rgb4(2, 1, 1), gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_STAGE, {0, gs::rgb4(6, 3, 2), gs::rgb4(3, 2, 2), gs::rgb4(8, 5, 3)});
    textPal(vdp, PAL_INK, gs::rgb4(14, 13, 12));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 13, 4));
    textPal(vdp, PAL_TITLE, gs::rgb4(12, 14, 15));
    loadFont(vdp, art);
    art.juggler = gs::uploadImage(vdp, jugglerArt());
    art.glove = gs::uploadImage(vdp, gloveArt());
    art.cream = gs::uploadImage(vdp, ballArt(false));
    art.gold = gs::uploadImage(vdp, ballArt(true));
    art.bell = gs::uploadImage(vdp, bellArt());
    art.face = gs::uploadImage(vdp, faceArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    art.floor = gs::uploadImage(vdp, floorArt());
}

}  // namespace jugglechime
