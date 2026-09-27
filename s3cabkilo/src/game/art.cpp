#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace cabkilo {
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

gs::Bitmap dashArt() {
    gs::Bitmap b(228, 70);
    b.poly({{0, 28}, {70, 8}, {158, 8}, {228, 28}, {228, 70}, {0, 70}}, 1);
    b.rect(8, 32, 212, 28, 2);
    b.rect(16, 36, 70, 16, 3);
    b.rect(142, 36, 70, 16, 3);
    b.rect(98, 34, 32, 12, 4);
    for (int i = 0; i < 6; i++) b.rect(104.f + (i % 3) * 8.f, 36.f + (i / 3) * 5.f, 6, 3, (i & 1) ? 5 : 6);
    b.rect(0, 58, 228, 12, 7);
    for (int i = 0; i < 11; i++) b.rect(4.f + i * 20.f, 60, 10, 6, i % 2 ? 8 : 1);
    b.rect(0, 22, 28, 8, 9);
    b.rect(200, 22, 28, 8, 9);
    return b;
}

gs::Bitmap helmArt(float ang) {
    gs::Bitmap b(48, 48);
    b.ellipse(24, 24, 20, 20, 1);
    b.ellipse(24, 24, 14, 14, 2);
    b.ellipse(24, 24, 4, 4, 3);
    for (int k = 0; k < 3; k++) {
        float a = ang + k * 2.0944f;
        float ca = std::cos(a), sa = std::sin(a);
        b.line(24 + ca * 4, 24 + sa * 4, 24 + ca * 14, 24 + sa * 14, 4, 2.2f);
    }
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(64, 64);
    b.ellipse(32, 32, 28, 28, 1);
    b.ellipse(32, 32, 20, 20, 2);
    b.ellipse(32, 32, 7, 7, 3);
    for (int k = 0; k < 5; k++) {
        float a = k * 1.2566f;
        b.line(32 + std::cos(a) * 7, 32 + std::sin(a) * 7, 32 + std::cos(a) * 20, 32 + std::sin(a) * 20, 4, 2.2f);
    }
    return b;
}

gs::Bitmap bodyArt() {
    gs::Bitmap b(72, 36);
    b.rect(4, 10, 64, 22, 1);
    b.poly({{8, 10}, {18, 2}, {54, 2}, {64, 10}}, 2);
    b.rect(20, 4, 14, 6, 3);
    b.rect(38, 4, 14, 6, 3);
    b.rect(30, 16, 12, 6, 4);
    b.rect(0, 18, 8, 10, 5);
    b.rect(64, 18, 8, 10, 5);
    return b;
}

gs::Bitmap blockArt() {
    gs::Bitmap b(40, 52);
    b.rect(0, 10, 40, 42, 1);
    b.rect(0, 0, 40, 12, 2);
    for (int row = 0; row < 3; row++)
        for (int col = 0; col < 2; col++)
            b.rect(4.f + col * 18.f, 16.f + row * 11.f, 12, 7, ((row + col) & 1) ? 3 : 4);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 40);
    b.rect(5, 12, 2, 24, 1);
    b.ellipse(6, 7, 5, 5, 2);
    b.rect(1, 34, 10, 4, 3);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(16, 64);
    b.rect(6, 8, 4, 56, 1);
    b.rect(2, 0, 12, 10, 2);
    b.rect(4, 2, 8, 6, 3);
    b.rect(3, 58, 10, 6, 1);
    return b;
}

gs::Bitmap bannerArt() {
    gs::TextStyle st{2, 1, 0, 0, 1};
    gs::Bitmap word = gs::textBitmap("1 KM", st);
    gs::Bitmap b(word.w + 12, word.h + 10);
    b.rect(0, 0, float(b.w), float(b.h), 2);
    b.rect(2, 2, float(b.w - 4), float(b.h - 4), 3);
    b.blit(word, 6, 5);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 15, 7));
    loadFont(vdp, art);

    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(14, 11, 2), gs::rgb4(4, 3, 2), gs::rgb4(2, 6, 5), gs::rgb4(1, 1, 2), gs::rgb4(12, 2, 2),
            gs::rgb4(15, 14, 8), gs::rgb4(3, 3, 2), gs::rgb4(8, 6, 2), gs::rgb4(10, 8, 3)});
    setPal(vdp, PAL_WHEEL,
           {0, gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 6), gs::rgb4(9, 8, 6), gs::rgb4(12, 11, 8)});
    setPal(vdp, PAL_BODY,
           {0, gs::rgb4(8, 3, 3), gs::rgb4(11, 5, 4), gs::rgb4(13, 14, 12), gs::rgb4(3, 3, 4), gs::rgb4(14, 12, 3)});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(5, 5, 6), gs::rgb4(8, 7, 6), gs::rgb4(3, 4, 6), gs::rgb4(9, 8, 7)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(2, 2, 3), gs::rgb4(14, 12, 3), gs::rgb4(3, 3, 2)});
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
    roadPal(PAL_ROAD, gs::rgb4(2, 5, 2), gs::rgb4(4, 4, 3), gs::rgb4(3, 3, 4), gs::rgb4(13, 12, 4));

    art.dash = gs::uploadMipped(vdp, dashArt());
    for (int i = 0; i < 3; i++) art.helm[i] = gs::uploadMipped(vdp, helmArt(i * 0.7f));
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.body = gs::uploadMipped(vdp, bodyArt());
    art.block = gs::uploadMipped(vdp, blockArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    vdp.setFogColor(gs::rgb4(4, 5, 8));
}

}  // namespace cabkilo
