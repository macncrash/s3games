#include "art.h"

#include <initializer_list>

namespace kartlane {
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

Bitmap paintKart(int bank) {
    Bitmap b(80, 96);
    const float lean = float(bank) * 5.f;
    auto X = [&](float x, float y) { return x + lean * (y / 96.f); };
    auto ell = [&](float x, float y, float rx, float ry, int c) { b.ellipse(X(x, y), y, rx, ry, c); };
    ell(16, 22, 7, 11, 2);
    ell(64, 22, 7, 11, 2);
    ell(18, 78, 8, 12, 2);
    ell(62, 78, 8, 12, 2);
    b.poly({{X(28, 18), 18}, {X(52, 18), 18}, {X(58, 84), 84}, {X(22, 84), 84}}, 1);
    b.rect(int(X(30, 28)), 30, 20, 16, 4);
    ell(40, 36, 8, 7, 3);
    b.ellipse(X(36, 35), 35, 2, 2, 8);
    b.ellipse(X(44, 35), 35, 2, 2, 8);
    b.rect(int(X(32, 52)), 52, 16, 10, 5);
    b.line(X(24, 46), 46, X(56, 46), 46, 6, 2.f);
    b.line(X(26, 70), 70, X(54, 70), 70, 6, 2.f);
    b.outline(15, false);
    return b;
}

Bitmap paintWheel() {
    Bitmap b(16, 28);
    b.ellipse(8, 14, 6, 12, 1);
    b.ellipse(8, 14, 3, 6, 2);
    return b;
}

Bitmap paintCone() {
    Bitmap b(22, 40);
    b.poly({{11, 3}, {19, 34}, {3, 34}}, 1);
    b.rect(4, 12, 14, 4, 2);
    b.rect(5, 22, 12, 3, 2);
    b.ellipse(11, 36, 8, 2.4f, 3);
    return b;
}

Bitmap paintBale() {
    Bitmap b(36, 24);
    b.rect(2, 4, 32, 16, 1);
    b.line(6, 6, 6, 18, 2, 2.f);
    b.line(18, 6, 18, 18, 2, 2.f);
    b.line(30, 6, 30, 18, 2, 2.f);
    b.rect(2, 4, 32, 3, 3);
    return b;
}

Bitmap paintPole() {
    Bitmap b(10, 48);
    b.rect(4, 6, 2, 40, 1);
    b.rect(1, 2, 8, 6, 2);
    return b;
}

Bitmap paintPost() {
    Bitmap b(12, 64);
    b.rect(4, 4, 4, 56, 1);
    b.rect(2, 2, 8, 6, 2);
    return b;
}

Bitmap paintBar() {
    Bitmap b(96, 10);
    b.rect(0, 2, 96, 6, 1);
    for (int i = 0; i < 8; i++) b.rect(i * 12, 2, 6, 6, 2);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(28, 18);
    b.rect(0, 2, 28, 14, 1);
    for (int y = 0; y < 3; y++)
        for (int x = 0; x < 4; x++)
            if ((x + y) & 1) b.rect(x * 7, 2 + y * 4, 7, 5, 2);
    return b;
}

Bitmap paintDust() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 4, 1);
    b.ellipse(5, 9, 3, 2, 2);
    return b;
}

Bitmap paintShadow() {
    Bitmap b(40, 14);
    b.ellipse(20, 7, 18, 5, 1);
    return b;
}

Bitmap paintSun() {
    Bitmap b(20, 20);
    b.ellipse(10, 10, 7, 7, 1);
    b.ellipse(10, 10, 3, 3, 2);
    return b;
}

Bitmap paintCloud() {
    Bitmap b(44, 18);
    b.ellipse(14, 10, 9, 5, 1);
    b.ellipse(26, 8, 11, 6, 1);
    b.ellipse(34, 11, 7, 4, 2);
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
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14), gs::rgb4(4, 5, 6));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1));
    textPal(vdp, PAL_WIN, gs::rgb4(8, 15, 9), gs::rgb4(1, 5, 2));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 12, 4), gs::rgb4(6, 3, 1));
    textPal(vdp, PAL_TAG, gs::rgb4(12, 14, 15), gs::rgb4(2, 4, 6));

    setPal(vdp, PAL_KART,
           {0, gs::rgb4(14, 3, 2), gs::rgb4(2, 2, 3), gs::rgb4(14, 11, 6), gs::rgb4(3, 8, 13), gs::rgb4(8, 8, 9),
            gs::rgb4(15, 14, 6), gs::rgb4(6, 1, 1), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CONE,
           {0, gs::rgb4(15, 8, 1), gs::rgb4(15, 15, 15), gs::rgb4(4, 4, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BALE,
           {0, gs::rgb4(12, 9, 3), gs::rgb4(8, 6, 2), gs::rgb4(14, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_POLE,
           {0, gs::rgb4(10, 11, 12), gs::rgb4(14, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GATE,
           {0, gs::rgb4(12, 12, 13), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(10, 9, 7), gs::rgb4(13, 12, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SKY,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), gs::rgb4(15, 13, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_END,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SPARE, {0});
    setPal(vdp, PAL_LANE,
           {gs::rgb4(2, 6, 2), gs::rgb4(4, 9, 3), gs::rgb4(3, 7, 2), gs::rgb4(6, 8, 3), gs::rgb4(5, 5, 5),
            gs::rgb4(3, 3, 3), gs::rgb4(4, 4, 5), gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 6), gs::rgb4(2, 2, 3),
            gs::rgb4(7, 7, 7), gs::rgb4(2, 5, 8), gs::rgb4(3, 6, 9), gs::rgb4(8, 9, 10), gs::rgb4(15, 15, 12),
            gs::rgb4(7, 7, 8)});

    loadFont(vdp, art);
    for (int i = 0; i < 3; i++) art.kart[i] = gs::uploadMipped(vdp, paintKart(i - 1));
    art.wheel = gs::uploadMipped(vdp, paintWheel());
    art.cone = gs::uploadMipped(vdp, paintCone());
    art.bale = gs::uploadMipped(vdp, paintBale());
    art.pole = gs::uploadMipped(vdp, paintPole());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.bar = gs::uploadMipped(vdp, paintBar());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.dust = gs::uploadMipped(vdp, paintDust());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.sun = gs::uploadMipped(vdp, paintSun());
    art.cloud = gs::uploadMipped(vdp, paintCloud());
    art.title = words(vdp, "KART LANE", 3);
    art.held = words(vdp, "HELD THE LANE", 2);
    art.whole = words(vdp, "THE WHOLE LEG", 2);
    art.left = words(vdp, "LEFT THE LANE", 2);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.crew = words(vdp, "OTHER CREW", 2);
    art.paused = words(vdp, "PAUSED", 3);
    art.end = words(vdp, "END", 2);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(10, 12, 13));
}

}  // namespace kartlane
