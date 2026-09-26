#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace fairmark {
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
    gs::Bitmap b(20, 44);
    b.ellipse(10, 30, 8.2f, 12.2f, 2);
    b.ellipse(7.2f, 28, 3.2f, 8.4f, 1);
    b.ellipse(10, 38, 6.4f, 2.4f, 3);
    b.rect(7, 8, 6, 12, 2);
    b.rect(8, 9, 2, 8, 1);
    b.rect(5, 2, 10, 5, 4);
    b.rect(7, 3, 3, 2, 1);
    b.rect(5, 22, 10, 6, 5);
    b.rect(6, 23, 8, 1, 1);
    b.line(4, 18, 16, 18, 3, 1);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 3);
    b.ellipse(11, 11, 8.2f, 8.2f, 2);
    b.ellipse(11, 11, 6.6f, 6.6f, 1);
    b.ellipse(10, 10, 2.2f, 1.4f, 4);
    b.ellipse(11, 11, 4.2f, 4.2f, 0);
    return b;
}

gs::Bitmap coinArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 3);
    b.ellipse(8, 8, 5.6f, 5.6f, 2);
    b.ellipse(8, 8, 4.2f, 4.2f, 1);
    b.rect(4, 7, 8, 2, 5);
    b.rect(7, 4, 2, 8, 5);
    b.set(5, 5, 4);
    return b;
}

gs::Bitmap handArt() {
    gs::Bitmap b(18, 16);
    b.ellipse(8, 10, 5.4f, 4.2f, 2);
    b.ellipse(8, 9, 4.2f, 3.1f, 1);
    b.ellipse(13, 7, 2.4f, 2.6f, 1);
    b.ellipse(13.2f, 6.6f, 1.2f, 1.3f, 2);
    b.rect(4, 12, 8, 3, 3);
    b.rect(5, 13, 6, 1, 4);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(11, 11);
    b.line(5, 0, 5, 10, 1, 1);
    b.line(0, 5, 10, 5, 1, 1);
    b.line(1, 1, 9, 9, 2, 1);
    b.line(9, 1, 1, 9, 2, 1);
    b.ellipse(5, 5, 1.6f, 1.6f, 1);
    return b;
}

gs::Bitmap bulbArt() {
    gs::Bitmap b(8, 10);
    b.ellipse(4, 4, 3.1f, 3.1f, 2);
    b.ellipse(3.2f, 3.2f, 1.4f, 1.4f, 1);
    b.rect(3, 7, 2, 2, 3);
    b.set(2, 8, 4);
    return b;
}

gs::Bitmap pennantArt() {
    gs::Bitmap b(12, 14);
    b.poly({{6, 1}, {11, 12}, {1, 12}}, 2);
    b.poly({{6, 4}, {9, 11}, {3, 11}}, 1);
    b.rect(5, 0, 2, 2, 3);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(4, 4);
    b.ellipse(2, 2, 1.6f, 1.6f, 1);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 40);
    b.rect(1, 0, 10, 40, 2);
    b.rect(2, 0, 3, 40, 1);
    b.rect(9, 0, 2, 40, 3);
    for (int y = 6; y < 38; y += 8) b.rect(1, y, 10, 1, 3);
    b.rect(4, 2, 2, 2, 4);
    b.rect(4, 34, 2, 2, 4);
    return b;
}

gs::Bitmap shelfArt() {
    gs::Bitmap b(24, 10);
    b.rect(0, 1, 24, 8, 2);
    b.rect(0, 0, 24, 2, 1);
    b.rect(0, 8, 24, 2, 3);
    for (int x = 3; x < 24; x += 6) b.set(x, 4, 4);
    return b;
}

gs::Bitmap awningArt() {
    gs::Bitmap b(48, 20);
    for (int x = 0; x < 48; x++) {
        int stripe = ((x / 6) & 1) ? 1 : 2;
        for (int y = 0; y < 14; y++) b.set(x, y, stripe);
        int scallop = (x % 8);
        int cut = scallop < 4 ? scallop : 7 - scallop;
        for (int y = 14; y < 14 + cut && y < 20; y++) b.set(x, y, stripe);
        if (cut > 0) b.set(x, 13 + cut, 3);
    }
    b.rect(0, 0, 48, 2, 3);
    return b;
}

gs::Bitmap clothArt() {
    gs::Bitmap b(16, 24);
    b.rect(0, 0, 16, 24, 3);
    b.rect(3, 0, 2, 24, 2);
    b.rect(10, 0, 2, 24, 2);
    b.rect(0, 0, 16, 2, 2);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(46, 46);
    b.ellipse(23, 23, 21, 21, 1);
    b.ellipse(23, 23, 17, 17, 0);
    for (int i = 0; i < 8; i++) {
        float a = i * PI / 4.f;
        b.line(23, 23, 23 + std::cos(a) * 19.f, 23 + std::sin(a) * 19.f, 5, 1.2f);
    }
    b.ellipse(23, 23, 4.2f, 4.2f, 4);
    b.ellipse(23, 23, 2.f, 2.f, 1);
    b.rect(20, 2, 6, 5, 3);
    b.rect(20, 39, 6, 5, 3);
    b.rect(2, 20, 5, 6, 3);
    b.rect(39, 20, 5, 6, 3);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6.4f, 6.4f, 1);
    b.ellipse(11, 7, 5.2f, 5.2f, 0);
    b.set(5, 6, 2);
    b.set(6, 10, 2);
    return b;
}

gs::Bitmap kidArt() {
    gs::Bitmap b(16, 28);
    b.ellipse(8, 6, 4.2f, 4.2f, 1);
    b.rect(4, 2, 8, 3, 4);
    b.rect(3, 4, 3, 2, 5);
    b.rect(6, 10, 5, 9, 2);
    b.rect(4, 11, 2, 7, 1);
    b.rect(11, 12, 2, 6, 3);
    b.rect(5, 19, 3, 8, 3);
    b.rect(9, 19, 3, 8, 3);
    b.set(6, 6, 3);
    b.set(10, 6, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7, 2.2f, 1);
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
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_INK, {0, ink, gs::rgb4(8, 8, 10), gs::rgb4(5, 5, 7)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(15, 12, 4), gs::rgb4(5, 3, 1), gs::rgb4(10, 8, 2)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 7), gs::rgb4(1, 5, 2), gs::rgb4(4, 10, 4)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 5, 4), gs::rgb4(6, 1, 1), gs::rgb4(10, 3, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(13, 9, 4), gs::rgb4(8, 5, 2), gs::rgb4(4, 2, 1), gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 12, 12), gs::rgb4(13, 2, 3), gs::rgb4(6, 1, 1), gs::rgb4(14, 12, 8),
                          gs::rgb4(15, 14, 12)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 12, 3), gs::rgb4(8, 6, 1), gs::rgb4(14, 10, 4),
                           gs::rgb4(13, 2, 2)});
    setPal(vdp, PAL_BLUE, {0, gs::rgb4(12, 14, 15), gs::rgb4(3, 6, 14), gs::rgb4(1, 2, 6), gs::rgb4(14, 12, 8),
                           gs::rgb4(15, 14, 12)});
    setPal(vdp, PAL_RING, {0, gs::rgb4(15, 14, 8), gs::rgb4(12, 9, 3), gs::rgb4(6, 4, 1), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(15, 13, 11), gs::rgb4(12, 8, 6), gs::rgb4(6, 3, 2), gs::rgb4(8, 2, 2)});
    setPal(vdp, PAL_BULB, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 4), gs::rgb4(6, 4, 2), gs::rgb4(15, 8, 2)});
    setPal(vdp, PAL_AWN, {0, gs::rgb4(14, 2, 3), gs::rgb4(15, 13, 8), gs::rgb4(5, 1, 1), gs::rgb4(10, 6, 2)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(7, 8, 12), gs::rgb4(4, 3, 8), gs::rgb4(12, 3, 4), gs::rgb4(15, 12, 5),
                            gs::rgb4(11, 11, 14)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(15, 14, 10), gs::rgb4(6, 2, 2), gs::rgb4(10, 8, 5)});
    setPal(vdp, PAL_KID, {0, gs::rgb4(15, 12, 9), gs::rgb4(12, 2, 3), gs::rgb4(2, 2, 6), gs::rgb4(8, 3, 2),
                          gs::rgb4(3, 2, 1)});

    loadFont(vdp, art);
    art.bottle = gs::uploadMipped(vdp, bottleArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.coin = gs::uploadMipped(vdp, coinArt());
    art.hand = gs::uploadMipped(vdp, handArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.bulb = gs::uploadMipped(vdp, bulbArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.shelf = gs::uploadMipped(vdp, shelfArt());
    art.awning = gs::uploadMipped(vdp, awningArt());
    art.cloth = gs::uploadMipped(vdp, clothArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.kid = gs::uploadMipped(vdp, kidArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.logo = phrase(vdp, "S3 FAIRMARK");
    art.finished = phrase(vdp, "FINISHED MARK");
    art.open = phrase(vdp, "STILL OPEN");
    art.sign = phrase(vdp, "RING TOSS");
}

}  // namespace fairmark
