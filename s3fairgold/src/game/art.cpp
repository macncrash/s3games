#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace fairgold {
namespace {

constexpr float PI = 3.14159265f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap bottleArt() {
    gs::Bitmap b(18, 42);
    b.ellipse(9, 28, 7.4f, 11.2f, 2);
    b.ellipse(6.4f, 26, 2.6f, 7.2f, 1);
    b.ellipse(9, 37, 5.6f, 2.2f, 3);
    b.rect(6, 10, 6, 10, 2);
    b.rect(7, 11, 2, 7, 1);
    b.rect(5, 4, 8, 6, 4);
    b.rect(6, 5, 3, 2, 1);
    b.rect(4, 18, 10, 5, 5);
    b.rect(5, 19, 8, 1, 1);
    b.line(3, 16, 15, 16, 3, 1);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 9.2f, 9.2f, 3);
    b.ellipse(10, 10, 7.4f, 7.4f, 2);
    b.ellipse(10, 10, 5.8f, 5.8f, 1);
    b.ellipse(8.2f, 8.2f, 1.6f, 1.1f, 4);
    b.ellipse(10, 10, 3.6f, 3.6f, 0);
    return b;
}

gs::Bitmap kidArt(bool step) {
    gs::Bitmap b(16, 30);
    b.ellipse(8, 6, 4.1f, 4.2f, 1);
    b.rect(4, 1, 8, 3, 6);
    b.rect(3, 3, 3, 2, 4);
    b.rect(5, 10, 6, 9, 2);
    b.rect(3, 11, 2, 7, 1);
    b.rect(11, 12, 2, 6, 3);
    if (step) {
        b.rect(4, 19, 3, 8, 3);
        b.rect(9, 20, 3, 7, 3);
        b.rect(3, 26, 4, 2, 5);
        b.rect(9, 26, 4, 2, 5);
    } else {
        b.rect(5, 19, 3, 8, 3);
        b.rect(9, 19, 3, 8, 3);
        b.rect(4, 26, 4, 2, 5);
        b.rect(9, 26, 4, 2, 5);
    }
    b.set(6, 6, 3);
    b.set(10, 6, 3);
    return b;
}

gs::Bitmap pennantArt() {
    gs::Bitmap b(12, 14);
    b.poly({{6, 1}, {11, 12}, {1, 12}}, 2);
    b.poly({{6, 4}, {9, 11}, {3, 11}}, 1);
    b.rect(5, 0, 2, 2, 3);
    return b;
}

gs::Bitmap bulbArt() {
    gs::Bitmap b(8, 10);
    b.ellipse(4, 4, 3.1f, 3.2f, 2);
    b.ellipse(3.1f, 3.2f, 1.3f, 1.3f, 1);
    b.rect(3, 7, 2, 2, 3);
    b.set(2, 8, 4);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 48);
    b.rect(2, 0, 8, 48, 2);
    b.rect(3, 0, 3, 48, 1);
    b.rect(8, 0, 2, 48, 3);
    for (int y = 4; y < 46; y += 8) b.rect(2, y, 8, 2, 4);
    return b;
}

gs::Bitmap shelfArt() {
    gs::Bitmap b(24, 10);
    b.rect(0, 2, 24, 7, 2);
    b.rect(0, 0, 24, 2, 1);
    b.rect(0, 8, 24, 2, 3);
    for (int x = 3; x < 24; x += 6) b.set(x, 5, 4);
    return b;
}

gs::Bitmap awningArt() {
    gs::Bitmap b(48, 22);
    for (int x = 0; x < 48; x++) {
        int stripe = (x / 6) % 3;
        int c = stripe == 0 ? 1 : (stripe == 1 ? 2 : 3);
        for (int y = 0; y < 14; y++) b.set(x, y, c);
        int scallop = x % 8;
        int cut = scallop < 4 ? scallop : 7 - scallop;
        for (int y = 14; y < 14 + cut && y < 22; y++) b.set(x, y, c);
        if (cut > 0) b.set(x, 13 + cut, 4);
    }
    b.rect(0, 0, 48, 2, 4);
    return b;
}

gs::Bitmap clothArt() {
    gs::Bitmap b(16, 28);
    for (int x = 0; x < 16; x++) {
        int c = ((x / 4) & 1) ? 2 : 1;
        for (int y = 0; y < 28; y++) b.set(x, y, c);
    }
    b.rect(0, 0, 16, 2, 3);
    b.rect(0, 26, 16, 2, 3);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(40, 40);
    b.ellipse(20, 20, 18.5f, 18.5f, 1);
    b.ellipse(20, 20, 15.2f, 15.2f, 0);
    b.ellipse(20, 20, 14.2f, 14.2f, 2);
    b.ellipse(20, 20, 12.4f, 12.4f, 0);
    for (int i = 0; i < 8; i++) {
        float a = i * PI / 4.f;
        b.line(20, 20, 20 + std::cos(a) * 16.f, 20 + std::sin(a) * 16.f, 5, 1.1f);
    }
    b.ellipse(20, 20, 3.2f, 3.2f, 4);
    b.ellipse(20, 20, 1.5f, 1.5f, 1);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6.4f, 6.4f, 1);
    b.ellipse(11, 7, 5.1f, 5.1f, 0);
    b.set(5, 6, 2);
    b.set(6, 10, 2);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(11, 11);
    b.line(5, 0, 5, 10, 1, 1);
    b.line(0, 5, 10, 5, 1, 1);
    b.line(1, 1, 9, 9, 2, 1);
    b.line(9, 1, 1, 9, 2, 1);
    b.ellipse(5, 5, 1.5f, 1.5f, 1);
    return b;
}

gs::Bitmap shadeArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7.f, 2.1f, 3);
    return b;
}

gs::Bitmap balloonArt() {
    gs::Bitmap b(14, 18);
    b.ellipse(7, 7, 5.6f, 6.2f, 2);
    b.ellipse(5.2f, 5.4f, 2.f, 2.4f, 1);
    b.poly({{7, 12}, {9, 15}, {5, 15}}, 3);
    b.line(7, 15, 7, 17, 3, 1);
    return b;
}

gs::Bitmap gondolaArt() {
    gs::Bitmap b(10, 8);
    b.rect(1, 2, 8, 5, 3);
    b.rect(2, 3, 6, 3, 1);
    b.rect(4, 0, 2, 2, 5);
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(32, 8);
    for (int x = 0; x < 32; x++) {
        int c = ((x / 4) & 1) ? 2 : 1;
        for (int y = 1; y < 7; y++) b.set(x, y, c);
    }
    b.rect(0, 0, 32, 1, 4);
    b.rect(0, 7, 32, 1, 3);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(4, 4);
    b.ellipse(2, 2, 1.5f, 1.5f, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Image phrase(gs::VDP& vdp, const char* s) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {2, 1, 2, 0, 1}));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 15, 14), gs::rgb4(6, 6, 8), gs::rgb4(3, 3, 5)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 11, 2), gs::rgb4(8, 5, 1), gs::rgb4(12, 3, 2),
                           gs::rgb4(6, 3, 1)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 14, 11), gs::rgb4(12, 10, 7), gs::rgb4(6, 5, 4), gs::rgb4(3, 4, 8),
                            gs::rgb4(8, 7, 5)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(10, 15, 8), gs::rgb4(2, 6, 2), gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 6, 5), gs::rgb4(6, 1, 1), gs::rgb4(10, 3, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(14, 10, 5), gs::rgb4(9, 6, 3), gs::rgb4(4, 2, 1), gs::rgb4(15, 13, 9)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 12, 12), gs::rgb4(13, 2, 3), gs::rgb4(6, 1, 1), gs::rgb4(15, 14, 10)});
    setPal(vdp, PAL_BLUE, {0, gs::rgb4(13, 15, 15), gs::rgb4(3, 6, 14), gs::rgb4(1, 2, 6), gs::rgb4(15, 14, 10)});
    setPal(vdp, PAL_RING, {0, gs::rgb4(15, 15, 12), gs::rgb4(13, 10, 4), gs::rgb4(6, 4, 1), gs::rgb4(15, 12, 6)});
    setPal(vdp, PAL_KID, {0, gs::rgb4(15, 12, 9), gs::rgb4(12, 3, 4), gs::rgb4(2, 3, 7), gs::rgb4(5, 3, 1),
                          gs::rgb4(2, 1, 1), gs::rgb4(14, 10, 3)});
    setPal(vdp, PAL_BULB, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 13, 5), gs::rgb4(5, 4, 2), gs::rgb4(15, 8, 3)});
    setPal(vdp, PAL_AWN, {0, gs::rgb4(13, 2, 3), gs::rgb4(15, 13, 9), gs::rgb4(15, 11, 3), gs::rgb4(5, 1, 1)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(11, 12, 14), gs::rgb4(5, 5, 8), gs::rgb4(14, 11, 4), gs::rgb4(12, 3, 4),
                            gs::rgb4(8, 8, 12)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(15, 14, 10), gs::rgb4(7, 2, 2), gs::rgb4(10, 8, 5)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(15, 15, 14), gs::rgb4(13, 2, 3), gs::rgb4(4, 1, 1), gs::rgb4(14, 11, 4)});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 12, 4), gs::rgb4(4, 1, 2), gs::rgb4(8, 4, 1)});

    loadFont(vdp, art);
    art.bottle = gs::uploadMipped(vdp, bottleArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.kid[0] = gs::uploadMipped(vdp, kidArt(false));
    art.kid[1] = gs::uploadMipped(vdp, kidArt(true));
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.bulb = gs::uploadMipped(vdp, bulbArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.shelf = gs::uploadMipped(vdp, shelfArt());
    art.awning = gs::uploadMipped(vdp, awningArt());
    art.cloth = gs::uploadMipped(vdp, clothArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.balloon = gs::uploadMipped(vdp, balloonArt());
    art.gondola = gs::uploadMipped(vdp, gondolaArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.logo = phrase(vdp, "S3 FAIR GOLD");
    art.doubled = phrase(vdp, "DOUBLE");
    art.stay = phrase(vdp, "STAY");
    art.sign = phrase(vdp, "GOLD X2");
    art.one = phrase(vdp, "1");
    art.two = phrase(vdp, "2");
}

}  // namespace fairgold
