#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace cablock {
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
    gs::Bitmap b(220, 64);
    b.poly({{0, 26}, {78, 6}, {142, 6}, {220, 26}, {220, 64}, {0, 64}}, 1);
    b.rect(10, 30, 200, 26, 2);
    b.rect(18, 34, 78, 14, 3);
    b.rect(124, 34, 76, 14, 3);
    b.rect(96, 32, 28, 10, 4);
    b.rect(102, 34, 16, 6, 5);
    b.rect(0, 54, 220, 10, 6);
    for (int i = 0; i < 9; i++) b.rect(8.f + i * 24.f, 56, 12, 4, i % 2 ? 7 : 8);
    b.rect(0, 22, 36, 8, 9);
    b.rect(184, 22, 36, 8, 9);
    return b;
}

gs::Bitmap wheelArt(float ang) {
    gs::Bitmap b(52, 52);
    b.ellipse(26, 26, 22, 22, 1);
    b.ellipse(26, 26, 16, 16, 2);
    b.ellipse(26, 26, 5, 5, 3);
    for (int k = 0; k < 3; k++) {
        float a = ang + k * 2.0944f;
        float ca = std::cos(a), sa = std::sin(a);
        b.line(26 + ca * 5, 26 + sa * 5, 26 + ca * 16, 26 + sa * 16, 1, 2.4f);
    }
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(36, 72);
    b.rect(2, 0, 32, 72, 1);
    b.rect(6, 4, 24, 64, 2);
    for (int i = 0; i < 6; i++) {
        float y = 6.f + i * 10.f;
        b.poly({{8, y + 8}, {28, y}, {28, y + 3}, {8, y + 11}}, (i % 2) ? 3 : 4);
    }
    b.rect(0, 0, 4, 72, 5);
    b.rect(32, 0, 4, 72, 5);
    b.rect(14, 30, 8, 12, 6);
    return b;
}

gs::Bitmap wallArt() {
    gs::Bitmap b(28, 40);
    b.rect(0, 6, 28, 34, 1);
    b.rect(0, 0, 28, 8, 2);
    for (int row = 0; row < 3; row++)
        for (int col = 0; col < 2; col++) {
            int c = ((row + col) % 2) ? 3 : 4;
            b.rect(3.f + col * 12.f, 12.f + row * 8.f, 9, 6, c);
        }
    return b;
}

gs::Bitmap houseArt() {
    gs::Bitmap b(48, 56);
    b.poly({{0, 20}, {24, 2}, {48, 20}}, 1);
    b.rect(4, 18, 40, 38, 2);
    b.rect(8, 24, 12, 14, 3);
    b.rect(28, 24, 12, 14, 3);
    b.rect(18, 40, 12, 16, 4);
    b.rect(20, 8, 8, 8, 5);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(40, 28);
    b.rect(2, 8, 36, 16, 1);
    b.rect(6, 2, 28, 10, 2);
    b.rect(8, 4, 10, 6, 3);
    b.rect(22, 4, 10, 6, 3);
    b.ellipse(8, 22, 4, 4, 4);
    b.ellipse(32, 22, 4, 4, 4);
    b.rect(16, 12, 8, 4, 5);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 32);
    b.rect(4, 10, 2, 22, 1);
    b.ellipse(5, 6, 4, 4, 2);
    b.rect(1, 28, 8, 3, 3);
    return b;
}

gs::Bitmap signArt() {
    gs::TextStyle st{2, 1, 0, 0, 1};
    gs::Bitmap word = gs::textBitmap("LOCK", st);
    gs::Bitmap b(word.w + 10, word.h + 8);
    b.rect(0, 0, float(b.w), float(b.h), 2);
    b.rect(2, 2, float(b.w - 4), float(b.h - 4), 3);
    b.blit(word, 5, 4);
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
           {0, gs::rgb4(13, 10, 2), gs::rgb4(5, 4, 3), gs::rgb4(2, 5, 4), gs::rgb4(1, 1, 2), gs::rgb4(9, 12, 8),
            gs::rgb4(3, 3, 2), gs::rgb4(14, 12, 4), gs::rgb4(8, 3, 2), gs::rgb4(15, 14, 9)});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(6, 6, 7), gs::rgb4(8, 7, 6), gs::rgb4(4, 4, 5), gs::rgb4(9, 9, 8)});
    setPal(vdp, PAL_GATE,
           {0, gs::rgb4(5, 6, 7), gs::rgb4(8, 9, 10), gs::rgb4(14, 12, 2), gs::rgb4(2, 2, 2), gs::rgb4(3, 3, 4),
            gs::rgb4(12, 4, 3)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(10, 3, 3), gs::rgb4(6, 2, 2), gs::rgb4(12, 14, 15), gs::rgb4(2, 2, 2), gs::rgb4(14, 12, 4)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(1, 1, 2), gs::rgb4(12, 3, 2), gs::rgb4(14, 12, 3)});
    setPal(vdp, 9, {0, gs::rgb4(3, 3, 4), gs::rgb4(14, 13, 6)});

    auto roadPal = [&](int pal, uint16_t ground, uint16_t verge, uint16_t asphalt, uint16_t paint) {
        for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, asphalt);
        vdp.setColor(pal * 16 + 0, 0);
        vdp.setColor(pal * 16 + 1, ground);
        vdp.setColor(pal * 16 + 2, gs::rgb4(3, 5, 3));
        vdp.setColor(pal * 16 + 3, gs::rgb4(6, 7, 4));
        vdp.setColor(pal * 16 + 4, verge);
        vdp.setColor(pal * 16 + 5, gs::rgb4(5, 5, 4));
        vdp.setColor(pal * 16 + 6, asphalt);
        vdp.setColor(pal * 16 + 7, gs::rgb4(3, 3, 4));
        vdp.setColor(pal * 16 + 8, gs::rgb4(4, 4, 4));
        vdp.setColor(pal * 16 + 9, gs::rgb4(2, 2, 3));
        vdp.setColor(pal * 16 + 10, gs::rgb4(7, 6, 5));
        vdp.setColor(pal * 16 + 11, gs::rgb4(3, 5, 8));
        vdp.setColor(pal * 16 + 12, gs::rgb4(4, 7, 10));
        vdp.setColor(pal * 16 + 13, gs::rgb4(8, 10, 12));
        vdp.setColor(pal * 16 + 14, paint);
        vdp.setColor(pal * 16 + 15, gs::rgb4(9, 9, 8));
    };
    roadPal(PAL_ROAD, gs::rgb4(3, 6, 3), gs::rgb4(5, 5, 4), gs::rgb4(4, 4, 5), gs::rgb4(13, 12, 5));
    roadPal(PAL_LOCK, gs::rgb4(3, 5, 7), gs::rgb4(5, 5, 5), gs::rgb4(5, 5, 6), gs::rgb4(14, 13, 4));

    art.dash = gs::uploadMipped(vdp, dashArt());
    for (int i = 0; i < 4; i++) art.wheel[i] = gs::uploadMipped(vdp, wheelArt(i * 0.55f));
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.wall = gs::uploadMipped(vdp, wallArt());
    art.house = gs::uploadMipped(vdp, houseArt());
    art.rival = gs::uploadMipped(vdp, rivalArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    vdp.setFogColor(gs::rgb4(7, 8, 10));
}

}  // namespace cablock
