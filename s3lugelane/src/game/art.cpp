#include "art.h"

#include <initializer_list>

namespace lugelane {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t shade) {
    setPal(vdp, pal, {0, ink, shade, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
}

Bitmap paintRider(int bank) {
    Bitmap b(72, 96);
    const float roll = float(bank) * 7.f;
    auto X = [&](float x, float y) { return x + roll * (1.f - y / 96.f); };
    auto ell = [&](float x, float y, float rx, float ry, int c) { b.ellipse(X(x, y), y, rx, ry, c); };
    ell(36, 18, 12, 10, 2);
    ell(36, 16, 8, 6, 3);
    ell(32, 16, 2.2f, 2.2f, 8);
    b.ellipse(X(40, 16), 16, 2.2f, 2.2f, 8);
    b.poly({{X(22, 28), 28}, {X(50, 28), 28}, {X(58, 62), 62}, {X(14, 62), 62}}, 1);
    b.poly({{X(26, 34), 34}, {X(46, 34), 34}, {X(50, 56), 56}, {X(22, 56), 56}}, 4);
    b.line(X(20, 40), 40, X(8, 58), 58, 1, 5.f);
    b.line(X(52, 40), 40, X(64, 58), 58, 1, 5.f);
    ell(8, 60, 4, 3.2f, 5);
    ell(64, 60, 4, 3.2f, 5);
    b.poly({{X(18, 64), 64}, {X(54, 64), 64}, {X(60, 86), 86}, {X(12, 86), 86}}, 6);
    b.line(X(16, 78), 78, X(56, 78), 78, 7, 2.f);
    ell(24, 90, 6, 3, 9);
    ell(48, 90, 6, 3, 9);
    b.outline(15, false);
    return b;
}

Bitmap paintPod() {
    Bitmap b(48, 22);
    b.poly({{4, 6}, {44, 6}, {46, 16}, {2, 16}}, 1);
    b.rect(8, 8, 32, 4, 2);
    b.ellipse(8, 18, 5, 2.4f, 3);
    b.ellipse(40, 18, 5, 2.4f, 3);
    b.outline(15, false);
    return b;
}

Bitmap paintStake() {
    Bitmap b(14, 52);
    b.rect(6, 8, 2, 40, 1);
    b.rect(2, 4, 10, 6, 2);
    b.poly({{12, 6}, {14, 10}, {12, 14}}, 3);
    b.ellipse(7, 49, 4, 1.6f, 4);
    return b;
}

Bitmap paintRock() {
    Bitmap b(36, 48);
    b.poly({{18, 4}, {34, 20}, {30, 44}, {6, 42}, {2, 18}}, 1);
    b.poly({{16, 12}, {26, 22}, {22, 36}, {10, 32}}, 2);
    b.ellipse(12, 18, 3, 2, 3);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(16, 40);
    b.rect(7, 12, 2, 26, 1);
    b.ellipse(8, 8, 5, 5, 2);
    b.ellipse(8, 8, 2.4f, 2.4f, 3);
    return b;
}

Bitmap paintPost() {
    Bitmap b(12, 64);
    b.rect(4, 4, 4, 56, 1);
    b.rect(2, 2, 8, 6, 2);
    b.rect(3, 22, 6, 3, 3);
    return b;
}

Bitmap paintBar() {
    Bitmap b(88, 8);
    b.rect(0, 2, 88, 4, 1);
    b.rect(0, 2, 88, 1, 2);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(16, 14);
    b.rect(1, 1, 2, 12, 2);
    b.poly({{3, 2}, {15, 6}, {3, 10}}, 1);
    return b;
}

Bitmap paintSpray() {
    Bitmap b(14, 10);
    b.ellipse(7, 5, 6, 3, 1);
    b.ellipse(3, 4, 2.f, 1.4f, 2);
    return b;
}

Bitmap paintFlake() {
    Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 1, 1);
    b.set(2, 2, 1);
    b.set(2, 3, 1);
    b.set(2, 4, 1);
    b.set(0, 2, 1);
    b.set(1, 2, 1);
    b.set(3, 2, 1);
    b.set(4, 2, 1);
    return b;
}

Bitmap paintShadow() {
    Bitmap b(40, 12);
    b.ellipse(20, 6, 16, 4, 1);
    return b;
}

Bitmap paintSun() {
    Bitmap b(18, 18);
    b.ellipse(9, 9, 6, 6, 1);
    b.ellipse(9, 9, 3, 3, 2);
    return b;
}

Bitmap paintCloud() {
    Bitmap b(40, 16);
    b.ellipse(14, 9, 8, 5, 1);
    b.ellipse(24, 8, 10, 6, 1);
    b.ellipse(30, 10, 6, 4, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp, 1);
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
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14), gs::rgb4(5, 7, 10));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1));
    textPal(vdp, PAL_WIN, gs::rgb4(8, 15, 10), gs::rgb4(1, 5, 3));
    textPal(vdp, PAL_BANNER, gs::rgb4(14, 12, 15), gs::rgb4(4, 3, 8));
    textPal(vdp, PAL_TAG, gs::rgb4(12, 15, 15), gs::rgb4(2, 6, 8));

    setPal(vdp, PAL_RIDER,
           {0, gs::rgb4(12, 2, 3), gs::rgb4(4, 5, 8), gs::rgb4(14, 12, 8), gs::rgb4(15, 4, 4), gs::rgb4(8, 9, 12),
            gs::rgb4(9, 9, 11), gs::rgb4(6, 6, 8), gs::rgb4(2, 2, 3), gs::rgb4(11, 12, 14), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WALL,
           {0, gs::rgb4(10, 12, 14), gs::rgb4(14, 3, 3), gs::rgb4(15, 14, 8), gs::rgb4(13, 15, 15), 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, ink});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10), gs::rgb4(4, 4, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           ink});
    setPal(vdp, PAL_ICE, {0, gs::rgb4(14, 15, 15), gs::rgb4(10, 13, 15), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, ink});
    setPal(vdp, PAL_GATE,
           {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 14, 8), gs::rgb4(8, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SKY,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(14, 15, 15), gs::rgb4(15, 13, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_END,
           {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SPARE, {0});
    setPal(vdp, PAL_LANE,
           {gs::rgb4(3, 5, 8), gs::rgb4(14, 15, 15), gs::rgb4(11, 13, 15), gs::rgb4(8, 10, 13), gs::rgb4(9, 12, 14),
            gs::rgb4(6, 9, 12), gs::rgb4(12, 15, 15), gs::rgb4(9, 13, 15), gs::rgb4(7, 10, 13), gs::rgb4(13, 15, 15),
            gs::rgb4(15, 15, 15), gs::rgb4(4, 7, 11), gs::rgb4(3, 6, 10), gs::rgb4(10, 14, 15), gs::rgb4(15, 15, 15),
            gs::rgb4(12, 14, 15)});

    loadFont(vdp, art);
    for (int i = 0; i < 3; i++) art.rider[i] = gs::uploadMipped(vdp, paintRider(i - 1));
    art.pod = gs::uploadMipped(vdp, paintPod());
    art.stake = gs::uploadMipped(vdp, paintStake());
    art.rock = gs::uploadMipped(vdp, paintRock());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.bar = gs::uploadMipped(vdp, paintBar());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.spray = gs::uploadMipped(vdp, paintSpray());
    art.flake = gs::uploadMipped(vdp, paintFlake());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.sun = gs::uploadMipped(vdp, paintSun());
    art.cloud = gs::uploadMipped(vdp, paintCloud());
    art.title = words(vdp, "LUGE LANE", 3);
    art.held = words(vdp, "HELD THE LANE", 2);
    art.whole = words(vdp, "THE WHOLE LEG", 2);
    art.left = words(vdp, "LEFT THE LANE", 2);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.paused = words(vdp, "PAUSED", 3);
    art.end = words(vdp, "END", 2);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(12, 14, 15));
}

}  // namespace lugelane
