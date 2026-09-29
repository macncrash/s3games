#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace mailkilo {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
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

gs::Bitmap hoodArt() {
    gs::Bitmap b(240, 78);
    b.poly({{0, 34}, {52, 6}, {188, 6}, {240, 34}, {240, 78}, {0, 78}}, 1);
    b.rect(10, 38, 220, 26, 2);
    b.rect(18, 42, 56, 14, 3);
    b.rect(166, 42, 56, 14, 3);
    b.rect(86, 16, 68, 16, 4);
    b.rect(90, 20, 60, 8, 5);
    b.rect(108, 44, 24, 10, 6);
    b.rect(0, 64, 240, 14, 7);
    for (int i = 0; i < 12; i++) b.rect(6.f + i * 19.f, 67, 10, 7, i % 2 ? 8 : 9);
    b.rect(4, 28, 22, 8, 10);
    b.rect(214, 28, 22, 8, 10);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(72, 72);
    b.ellipse(36, 36, 32, 32, 1);
    b.ellipse(36, 36, 24, 24, 2);
    b.ellipse(36, 36, 8, 8, 3);
    for (int k = 0; k < 6; k++) {
        float a = k * 1.0472f + 0.2f;
        b.line(36 + std::cos(a) * 8, 36 + std::sin(a) * 8, 36 + std::cos(a) * 24, 36 + std::sin(a) * 24, 4, 2.4f);
    }
    b.ellipse(36, 36, 30, 30, 5);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(40, 48);
    b.ellipse(20, 28, 16, 18, 1);
    b.poly({{12, 16}, {20, 2}, {28, 16}}, 2);
    b.rect(16, 8, 8, 8, 3);
    b.rect(14, 30, 12, 6, 4);
    return b;
}

gs::Bitmap boxArt() {
    gs::Bitmap b(36, 48);
    b.rect(2, 14, 32, 30, 1);
    b.rect(2, 6, 32, 10, 2);
    b.rect(8, 20, 8, 8, 3);
    b.rect(20, 20, 8, 8, 3);
    b.rect(8, 32, 20, 6, 4);
    b.rect(14, 0, 8, 8, 5);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 46);
    b.rect(6, 14, 2, 26, 1);
    b.ellipse(7, 8, 6, 6, 2);
    b.rect(2, 40, 10, 4, 3);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(18, 70);
    b.rect(7, 10, 4, 54, 1);
    b.rect(1, 0, 16, 12, 2);
    b.rect(3, 2, 12, 8, 3);
    b.rect(4, 62, 10, 6, 1);
    return b;
}

gs::Bitmap bannerArt() {
    gs::TextStyle st{2, 1, 0, 0, 1};
    gs::Bitmap word = gs::textBitmap("1 KM", st);
    gs::Bitmap b(word.w + 14, word.h + 12);
    b.rect(0, 0, float(b.w), float(b.h), 2);
    b.rect(2, 2, float(b.w - 4), float(b.h - 4), 3);
    b.blit(word, 7, 6);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 15, 7));
    loadFont(vdp, art);

    setPal(vdp, PAL_VAN,
           {0, gs::rgb4(14, 11, 2), gs::rgb4(6, 4, 2), gs::rgb4(2, 3, 5), gs::rgb4(12, 2, 2), gs::rgb4(15, 14, 8),
            gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 2), gs::rgb4(8, 2, 2), gs::rgb4(10, 8, 2), gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_WHEEL,
           {0, gs::rgb4(3, 2, 2), gs::rgb4(6, 5, 4), gs::rgb4(10, 8, 5), gs::rgb4(13, 11, 7), gs::rgb4(8, 3, 2)});
    setPal(vdp, PAL_SACK,
           {0, gs::rgb4(9, 7, 3), gs::rgb4(12, 9, 4), gs::rgb4(4, 3, 2), gs::rgb4(13, 4, 3)});
    setPal(vdp, PAL_BOX,
           {0, gs::rgb4(2, 5, 9), gs::rgb4(4, 7, 12), gs::rgb4(14, 13, 6), gs::rgb4(12, 3, 2), gs::rgb4(8, 8, 7)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(3, 3, 3), gs::rgb4(15, 13, 4), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(12, 12, 11), gs::rgb4(14, 3, 2), gs::rgb4(15, 14, 6)});

    auto roadPal = [&](int pal, uint16_t ground, uint16_t verge, uint16_t asphalt, uint16_t paint) {
        for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, asphalt);
        vdp.setColor(pal * 16 + 0, 0);
        vdp.setColor(pal * 16 + 1, ground);
        vdp.setColor(pal * 16 + 2, gs::rgb4(2, 4, 2));
        vdp.setColor(pal * 16 + 3, gs::rgb4(5, 6, 3));
        vdp.setColor(pal * 16 + 4, verge);
        vdp.setColor(pal * 16 + 5, gs::rgb4(4, 4, 3));
        vdp.setColor(pal * 16 + 6, asphalt);
        vdp.setColor(pal * 16 + 7, gs::rgb4(2, 2, 3));
        vdp.setColor(pal * 16 + 8, gs::rgb4(3, 3, 4));
        vdp.setColor(pal * 16 + 9, gs::rgb4(1, 1, 2));
        vdp.setColor(pal * 16 + 10, gs::rgb4(6, 5, 4));
        vdp.setColor(pal * 16 + 11, gs::rgb4(3, 4, 6));
        vdp.setColor(pal * 16 + 12, gs::rgb4(4, 6, 8));
        vdp.setColor(pal * 16 + 13, gs::rgb4(7, 8, 9));
        vdp.setColor(pal * 16 + 14, paint);
        vdp.setColor(pal * 16 + 15, gs::rgb4(8, 8, 7));
    };
    roadPal(PAL_ROAD, gs::rgb4(2, 5, 2), gs::rgb4(5, 5, 3), gs::rgb4(3, 3, 4), gs::rgb4(14, 12, 3));

    art.hood = gs::uploadMipped(vdp, hoodArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.box = gs::uploadMipped(vdp, boxArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    vdp.setFogColor(gs::rgb4(5, 5, 7));
}

}  // namespace mailkilo
