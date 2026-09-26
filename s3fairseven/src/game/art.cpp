#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace fairseven {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 0, 2));
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

gs::Image phrase(gs::VDP& vdp, const char* s, int scale) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, 1, 2, 0, 1}));
}

gs::Bitmap bottleArt() {
    gs::Bitmap b(18, 44);
    b.rect(6, 1, 6, 3, 4);
    b.rect(7, 4, 4, 7, 2);
    b.rect(5, 10, 8, 3, 1);
    b.ellipse(9, 28, 8.2f, 13.2f, 2);
    b.ellipse(6.2f, 24, 3.1f, 8.f, 1);
    b.rect(5, 18, 8, 7, 1);
    b.rect(6, 20, 6, 2, 4);
    b.ellipse(9, 38, 5.4f, 2.6f, 3);
    b.rect(4, 36, 10, 2, 5);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8.2f, 8.2f, 3);
    b.ellipse(9, 9, 6.4f, 6.4f, 2);
    b.ellipse(9, 9, 4.8f, 4.8f, 1);
    b.ellipse(6.6f, 6.6f, 1.5f, 1.1f, 4);
    b.ellipse(9, 9, 3.1f, 3.1f, 0);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(16, 14);
    b.rect(7, 0, 2, 3, 3);
    b.ellipse(8, 7, 6.2f, 4.6f, 2);
    b.ellipse(6, 6, 2.2f, 2.f, 1);
    b.rect(2, 10, 12, 3, 4);
    b.rect(3, 10, 10, 1, 1);
    b.ellipse(8, 8, 1.2f, 1.6f, 5);
    return b;
}

gs::Bitmap headArt() {
    gs::Bitmap b(18, 28);
    b.rect(3, 1, 11, 4, 4);
    b.rect(2, 4, 13, 3, 4);
    b.rect(12, 6, 5, 2, 4);
    b.ellipse(8, 13, 5.2f, 5.4f, 1);
    b.set(10, 12, 5);
    b.set(11, 12, 5);
    b.set(12, 14, 3);
    b.rect(4, 19, 10, 7, 2);
    b.rect(2, 20, 3, 5, 2);
    b.rect(13, 20, 3, 5, 1);
    b.rect(5, 25, 3, 2, 5);
    b.rect(10, 25, 3, 2, 5);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4.4f, 4.4f, 3);
    b.ellipse(5, 5, 3.2f, 3.2f, 2);
    b.ellipse(4.2f, 4.f, 1.5f, 1.3f, 1);
    return b;
}

gs::Bitmap bulbArt() {
    gs::Bitmap b(8, 10);
    b.ellipse(4, 4, 3.1f, 3.2f, 10);
    b.ellipse(3.1f, 3.2f, 1.2f, 1.2f, 9);
    b.rect(3, 7, 2, 2, 3);
    return b;
}

gs::Bitmap pennantArt() {
    gs::Bitmap b(12, 14);
    b.poly({{6, 1}, {11, 12}, {1, 12}}, 1);
    b.poly({{6, 4}, {9, 11}, {3, 11}}, 2);
    b.rect(5, 0, 2, 2, 2);
    return b;
}

gs::Bitmap shelfArt() {
    gs::Bitmap b(32, 10);
    b.rect(0, 2, 32, 6, 2);
    b.rect(0, 0, 32, 2, 1);
    b.rect(0, 8, 32, 2, 3);
    for (int x = 3; x < 32; x += 6) b.set(x, 5, 14);
    return b;
}

gs::Bitmap awningArt() {
    gs::Bitmap b(48, 20);
    for (int x = 0; x < 48; x++) {
        int stripe = (x / 6) % 3;
        int c = stripe == 0 ? 6 : (stripe == 1 ? 7 : 8);
        for (int y = 0; y < 13; y++) b.set(x, y, c);
        int scallop = x % 8;
        int cut = scallop < 4 ? scallop : 7 - scallop;
        for (int y = 13; y < 13 + cut && y < 20; y++) b.set(x, y, c);
    }
    b.rect(0, 0, 48, 2, 3);
    return b;
}

gs::Bitmap clothArt() {
    gs::Bitmap b(16, 28);
    for (int y = 0; y < 28; y++) {
        for (int x = 0; x < 16; x++) b.set(x, y, ((x / 4) & 1) ? 5 : 4);
    }
    b.rect(0, 0, 16, 2, 3);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 48);
    b.rect(2, 0, 6, 48, 2);
    b.rect(3, 0, 2, 48, 1);
    b.rect(6, 0, 2, 48, 3);
    for (int y = 6; y < 46; y += 10) b.rect(2, y, 6, 2, 14);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(40, 40);
    b.ellipse(20, 20, 18.4f, 18.4f, 11);
    b.ellipse(20, 20, 15.2f, 15.2f, 0);
    b.ellipse(20, 20, 14.2f, 14.2f, 12);
    b.ellipse(20, 20, 12.2f, 12.2f, 0);
    for (int i = 0; i < 8; i++) {
        float a = i * 3.14159265f / 4.f;
        b.line(20, 20, 20 + std::cos(a) * 15.f, 20 + std::sin(a) * 15.f, 11, 1.1f);
    }
    b.ellipse(20, 20, 2.6f, 2.6f, 10);
    return b;
}

gs::Bitmap gondolaArt() {
    gs::Bitmap b(8, 7);
    b.rect(1, 2, 6, 4, 7);
    b.rect(2, 3, 4, 2, 1);
    b.rect(3, 0, 2, 2, 11);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6.f, 6.f, 13);
    b.ellipse(9.4f, 6.2f, 4.6f, 4.6f, 0);
    b.set(5, 5, 10);
    b.set(6, 9, 10);
    return b;
}

gs::Bitmap shadeArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7.f, 2.f, 14);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(4, 4);
    b.ellipse(2, 2, 1.4f, 1.4f, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 15, 14), gs::rgb4(4, 3, 6)});
    setPal(vdp, PAL_TITLE, {0, gs::rgb4(15, 13, 5), gs::rgb4(6, 2, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(10, 15, 8), gs::rgb4(1, 4, 2)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 6, 5), gs::rgb4(6, 1, 2)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(15, 14, 10), gs::rgb4(8, 2, 2)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 14, 11), gs::rgb4(12, 10, 7), gs::rgb4(7, 5, 3), gs::rgb4(3, 5, 10),
                            gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 14, 8), gs::rgb4(14, 10, 2), gs::rgb4(8, 5, 1), gs::rgb4(12, 2, 2),
                           gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(15, 13, 8), gs::rgb4(13, 9, 3), gs::rgb4(7, 4, 1), gs::rgb4(14, 3, 2),
                           gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(15, 15, 12), gs::rgb4(14, 11, 3), gs::rgb4(7, 4, 1), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_THEM, {0, gs::rgb4(12, 15, 15), gs::rgb4(3, 7, 14), gs::rgb4(1, 2, 6), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_HEADY, {0, gs::rgb4(15, 12, 9), gs::rgb4(13, 3, 3), gs::rgb4(4, 2, 1), gs::rgb4(14, 10, 2),
                            gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_HEADT, {0, gs::rgb4(15, 12, 9), gs::rgb4(3, 5, 13), gs::rgb4(3, 2, 1), gs::rgb4(6, 6, 8),
                            gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_PIPY, {0, gs::rgb4(15, 14, 6), gs::rgb4(14, 8, 1), gs::rgb4(6, 2, 1)});
    setPal(vdp, PAL_PIPT, {0, gs::rgb4(11, 15, 15), gs::rgb4(2, 6, 13), gs::rgb4(1, 2, 5)});
    setPal(vdp, PAL_PIPD, {0, gs::rgb4(5, 4, 6), gs::rgb4(3, 2, 4), gs::rgb4(2, 1, 3)});
    setPal(vdp, PAL_BOOTH,
           {0, gs::rgb4(14, 10, 5), gs::rgb4(9, 6, 3), gs::rgb4(4, 2, 1), gs::rgb4(12, 2, 3), gs::rgb4(6, 1, 2),
            gs::rgb4(13, 2, 3), gs::rgb4(15, 13, 9), gs::rgb4(15, 11, 3), gs::rgb4(15, 15, 14), gs::rgb4(15, 13, 4),
            gs::rgb4(10, 11, 13), gs::rgb4(5, 5, 8), gs::rgb4(14, 14, 10), gs::rgb4(2, 1, 2)});

    loadFont(vdp, art);
    art.bottle = gs::uploadMipped(vdp, bottleArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.head = gs::uploadMipped(vdp, headArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.bulb = gs::uploadMipped(vdp, bulbArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.shelf = gs::uploadMipped(vdp, shelfArt());
    art.awning = gs::uploadMipped(vdp, awningArt());
    art.cloth = gs::uploadMipped(vdp, clothArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.gondola = gs::uploadMipped(vdp, gondolaArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.fair = phrase(vdp, "FAIR", 3);
    art.seven = phrase(vdp, "SEVEN", 3);
    art.late = phrase(vdp, "LATE", 3);
    art.sign = phrase(vdp, "TO SEVEN", 2);
    art.one = phrase(vdp, "1", 2);
    art.two = phrase(vdp, "2", 2);
    art.three = phrase(vdp, "3", 2);
}

}  // namespace fairseven
