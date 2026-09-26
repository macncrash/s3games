#include "art.h"

#include <initializer_list>
#include <vector>

namespace sledlane {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t shade) {
    const uint16_t edge = gs::rgb4(1, 1, 2);
    setPal(vdp, pal, {0, ink, shade, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, edge});
}

// Rear view of a dogsled. bank -1, 0, 1 leans the musher over the bar.
Bitmap paintSled(int bank) {
    Bitmap b(86, 116);
    const float roll = float(bank) * 9.f;
    auto X = [&](float x, float y) { return x + roll * (1.f - y / 116.f); };
    auto line = [&](float x0, float y0, float x1, float y1, int c, float th) {
        b.line(X(x0, y0), y0, X(x1, y1), y1, c, th);
    };
    auto ell = [&](float x, float y, float rx, float ry, int c) { b.ellipse(X(x, y), y, rx, ry, c); };
    auto poly = [&](std::initializer_list<Pt> pts, int c) {
        std::vector<Pt> w;
        w.reserve(pts.size());
        for (const Pt& p : pts) w.push_back({X(p.first, p.second), p.second});
        b.poly(w, c);
    };

    line(28, 78, 18, 112, 5, 3.4f);
    line(58, 78, 68, 112, 5, 3.4f);
    line(30, 82, 56, 82, 5, 2.6f);
    poly({{30, 56}, {56, 56}, {64, 94}, {22, 94}}, 1);
    poly({{34, 60}, {52, 60}, {56, 88}, {30, 88}}, 7);
    line(32, 70, 54, 70, 6, 1.8f);
    line(31, 78, 55, 78, 6, 1.8f);
    line(32, 56, 26, 82, 2, 2.4f);
    line(54, 56, 60, 82, 2, 2.4f);
    poly({{22, 48}, {64, 48}, {62, 56}, {24, 56}}, 2);
    ell(22, 52, 3.4f, 3.4f, 9);
    ell(64, 52, 3.4f, 3.4f, 9);
    poly({{35, 26}, {51, 26}, {58, 58}, {28, 58}}, 3);
    poly({{38, 32}, {48, 32}, {51, 50}, {35, 50}}, 10);
    ell(43, 20, 10, 8.5f, 4);
    ell(43, 22, 6.5f, 5.6f, 3);
    ell(43, 16, 4.2f, 2.6f, 8);
    line(36, 38, 22, 50, 3, 4.6f);
    line(50, 38, 64, 50, 3, 4.6f);
    ell(22, 51, 3.2f, 3.2f, 3);
    ell(64, 51, 3.2f, 3.2f, 3);
    poly({{20, 100}, {30, 100}, {31, 106}, {19, 106}}, 2);
    poly({{56, 100}, {66, 100}, {67, 106}, {55, 106}}, 2);
    b.outline(15, false);
    return b;
}

Bitmap paintDog(int step) {
    Bitmap b(42, 40);
    const float kick = step ? 3.f : -2.f;
    b.ellipse(21, 7, 3.2f, 4.2f, 2);
    b.ellipse(21, 5, 1.8f, 2.2f, 1);
    b.poly({{14, 16}, {17, 5}, {20, 16}}, 2);
    b.poly({{22, 16}, {25, 5}, {28, 16}}, 2);
    b.ellipse(21, 16, 7.2f, 5.4f, 1);
    b.ellipse(21, 15, 4.6f, 3.2f, 5);
    b.ellipse(21, 26, 10.5f, 7.2f, 2);
    b.ellipse(21, 25, 7.f, 4.4f, 1);
    b.rect(12, 23, 18, 2, 4);
    b.rect(12, 30, 3, 8, 2);
    b.rect(17, 30 + kick * 0.15f, 3, 8, 3);
    b.rect(23, 30 - kick * 0.15f, 3, 8, 2);
    b.rect(28, 30, 3, 8, 3);
    b.outline(15, false);
    return b;
}

Bitmap paintStake(bool blue) {
    Bitmap b(12, 46);
    b.rect(5, 8, 2, 34, 1);
    b.rect(3, 4, 6, 6, blue ? 3 : 2);
    b.poly({{8, 8}, {12, 12}, {8, 16}}, blue ? 3 : 2);
    b.ellipse(6, 43, 4, 1.6f, 4);
    return b;
}

Bitmap paintSpruce() {
    Bitmap b(40, 72);
    b.rect(18, 56, 4, 14, 4);
    b.poly({{20, 2}, {36, 28}, {4, 28}}, 1);
    b.poly({{20, 16}, {38, 42}, {2, 42}}, 2);
    b.poly({{20, 28}, {40, 58}, {0, 58}}, 1);
    b.poly({{20, 6}, {28, 20}, {16, 18}}, 3);
    b.poly({{20, 22}, {30, 36}, {14, 34}}, 3);
    b.poly({{20, 36}, {32, 50}, {12, 48}}, 3);
    return b;
}

Bitmap paintHut() {
    Bitmap b(58, 46);
    b.poly({{4, 20}, {29, 4}, {54, 20}}, 3);
    b.rect(8, 18, 42, 24, 1);
    b.rect(10, 22, 38, 3, 2);
    b.rect(10, 28, 38, 3, 2);
    b.rect(24, 28, 10, 14, 4);
    b.rect(12, 26, 7, 6, 5);
    b.rect(38, 26, 7, 6, 5);
    b.rect(40, 6, 3, 12, 6);
    b.ellipse(41, 5, 3.2f, 2.f, 7);
    b.outline(15, false);
    return b;
}

Bitmap paintCache() {
    Bitmap b(32, 26);
    b.poly({{4, 10}, {16, 3}, {28, 10}}, 3);
    b.rect(4, 10, 24, 13, 1);
    b.rect(4, 10, 24, 3, 2);
    b.rect(7, 15, 7, 5, 4);
    b.line(14, 15, 14, 20, 2, 1.2f);
    b.outline(15, false);
    return b;
}

Bitmap paintPost() {
    Bitmap b(14, 70);
    b.rect(5, 6, 4, 58, 2);
    b.rect(3, 2, 8, 7, 1);
    b.rect(4, 18, 6, 3, 3);
    b.rect(4, 36, 6, 3, 3);
    b.ellipse(7, 66, 5, 2.f, 4);
    return b;
}

Bitmap paintBar() {
    Bitmap b(96, 10);
    b.rect(0, 3, 96, 4, 1);
    b.rect(0, 3, 96, 1, 5);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(18, 16);
    b.rect(2, 1, 2, 14, 2);
    b.poly({{4, 2}, {16, 6}, {4, 10}}, 1);
    return b;
}

Bitmap paintSpray() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 6.5f, 3.6f, 1);
    b.ellipse(4, 5, 2.2f, 1.6f, 2);
    b.ellipse(12, 7, 2.f, 1.4f, 2);
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

Bitmap paintBead() {
    Bitmap b(6, 6);
    b.ellipse(3, 3, 2.f, 2.f, 6);
    return b;
}

Bitmap paintShadow() {
    Bitmap b(28, 12);
    b.ellipse(14, 6, 12, 4, 1);
    return b;
}

Bitmap paintRaven(bool up) {
    Bitmap b(26, 14);
    b.ellipse(13, 8, 2.4f, 1.7f, 1);
    const float tip = up ? 2.f : 11.f;
    b.line(13, 7, 1, tip, 1, 1.8f);
    b.line(13, 7, 25, tip, 1, 1.8f);
    b.set(16, 7, 3);
    return b;
}

Bitmap paintSun() {
    Bitmap b(22, 22);
    b.ellipse(11, 11, 8, 8, 3);
    b.ellipse(11, 11, 4, 4, 4);
    return b;
}

Bitmap paintCloud() {
    Bitmap b(48, 20);
    b.ellipse(16, 12, 12, 6, 1);
    b.ellipse(30, 11, 11, 6, 1);
    b.ellipse(22, 8, 8, 5, 2);
    return b;
}

Bitmap paintMoon() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 5);
    b.ellipse(11, 7, 4.5f, 4.5f, 0);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
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
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14), gs::rgb4(6, 8, 10));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 7, 3), gs::rgb4(5, 1, 1));
    textPal(vdp, PAL_WIN, gs::rgb4(8, 15, 8), gs::rgb4(1, 5, 2));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 12, 6), gs::rgb4(6, 3, 1));
    textPal(vdp, PAL_TAG, gs::rgb4(12, 15, 15), gs::rgb4(3, 6, 8));

    setPal(vdp, PAL_SLED,
           {0, gs::rgb4(13, 9, 4), gs::rgb4(8, 5, 2), gs::rgb4(13, 2, 2), gs::rgb4(14, 12, 8), gs::rgb4(10, 12, 14),
            gs::rgb4(5, 3, 2), gs::rgb4(6, 4, 2), gs::rgb4(15, 15, 15), gs::rgb4(12, 8, 3), gs::rgb4(9, 2, 2), 0, 0, 0,
            0, ink});
    setPal(vdp, PAL_DOG,
           {0, gs::rgb4(14, 12, 9), gs::rgb4(7, 7, 8), gs::rgb4(3, 3, 4), gs::rgb4(12, 2, 2), gs::rgb4(15, 14, 12), 0,
            0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_STAKE,
           {0, gs::rgb4(9, 7, 4), gs::rgb4(13, 2, 2), gs::rgb4(3, 5, 12), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, ink});
    setPal(vdp, PAL_TREE,
           {0, gs::rgb4(1, 5, 2), gs::rgb4(2, 8, 3), gs::rgb4(14, 15, 15), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, ink});
    setPal(vdp, PAL_HUT,
           {0, gs::rgb4(10, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(15, 15, 15), gs::rgb4(3, 2, 2), gs::rgb4(14, 12, 5),
            gs::rgb4(4, 4, 5), gs::rgb4(12, 12, 13), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 7), gs::rgb4(12, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           ink});
    setPal(vdp, PAL_END,
           {0, gs::rgb4(13, 2, 2), gs::rgb4(8, 5, 2), gs::rgb4(15, 15, 14), gs::rgb4(14, 15, 15), gs::rgb4(13, 10, 4),
            0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SKY,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), gs::rgb4(15, 13, 7), gs::rgb4(15, 15, 12),
            gs::rgb4(12, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SPARE, {0});

    // Packed snow lane. 6/7 the trail, 9 the runner tracks, 14/15 the lip and fresh edge.
    // 1-3 are the ploughed snow wall beside the lane.
    setPal(vdp, PAL_LANE,
           {gs::rgb4(2, 3, 5), gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15), gs::rgb4(9, 11, 13), gs::rgb4(8, 9, 11),
            gs::rgb4(6, 8, 10), gs::rgb4(13, 15, 15), gs::rgb4(10, 13, 14), gs::rgb4(7, 9, 12), gs::rgb4(6, 9, 12),
            gs::rgb4(14, 15, 15), gs::rgb4(4, 7, 10), gs::rgb4(3, 6, 9), gs::rgb4(8, 12, 14), gs::rgb4(15, 15, 15),
            gs::rgb4(14, 15, 14)});

    loadFont(vdp, art);
    for (int i = 0; i < 3; i++) art.sled[i] = gs::uploadMipped(vdp, paintSled(i - 1));
    art.dog[0] = gs::uploadMipped(vdp, paintDog(0));
    art.dog[1] = gs::uploadMipped(vdp, paintDog(1));
    art.stakeRed = gs::uploadMipped(vdp, paintStake(false));
    art.stakeBlue = gs::uploadMipped(vdp, paintStake(true));
    art.spruce = gs::uploadMipped(vdp, paintSpruce());
    art.hut = gs::uploadMipped(vdp, paintHut());
    art.cache = gs::uploadMipped(vdp, paintCache());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.bar = gs::uploadMipped(vdp, paintBar());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.spray = gs::uploadMipped(vdp, paintSpray());
    art.flake = gs::uploadMipped(vdp, paintFlake());
    art.bead = gs::uploadMipped(vdp, paintBead());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.raven[0] = gs::uploadMipped(vdp, paintRaven(true));
    art.raven[1] = gs::uploadMipped(vdp, paintRaven(false));
    art.sun = gs::uploadMipped(vdp, paintSun());
    art.cloud = gs::uploadMipped(vdp, paintCloud());
    art.moon = gs::uploadMipped(vdp, paintMoon());
    art.title = words(vdp, "SLED LANE", 3);
    art.held = words(vdp, "HELD THE LANE", 2);
    art.whole = words(vdp, "THE WHOLE LEG", 2);
    art.left = words(vdp, "LEFT THE LANE", 2);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.paused = words(vdp, "PAUSED", 3);
    art.end = words(vdp, "END", 2);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(13, 14, 15));
}

}  // namespace sledlane
