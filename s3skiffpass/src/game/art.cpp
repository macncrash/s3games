#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace skiffpass {
namespace {

constexpr float kPi = 3.14159265f;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    float wx = lx * c + ly * s;
    float wy = -lx * s + ly * c;
    return {cx + wx, cy - wy};
}

// Nose is +ly. Heading 0 points up the bitmap; positive heading turns clockwise.
Bitmap paintSkiff(float heading) {
    Bitmap b(96, 96);
    const float cx = 48.f, cy = 48.f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        w.reserve(local.size());
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, c, s));
        b.poly(w, col);
    };
    auto stroke = [&](float ax, float ay, float bx, float by, int col, float th) {
        Pt A = spin(cx, cy, ax, ay, c, s);
        Pt B = spin(cx, cy, bx, by, c, s);
        b.line(A.first, A.second, B.first, B.second, col, th);
    };
    auto blob = [&](float lx, float ly, float rx, float ry, int col) {
        Pt p = spin(cx, cy, lx, ly, c, s);
        b.ellipse(p.first, p.second, rx, ry, col);
    };
    // Lapstrake hull, transom stern, nose at +30 and stern at -30 (HULL_PX).
    poly({{0.f, 30.f}, {8.f, 18.f}, {11.f, 4.f}, {10.f, -16.f}, {8.f, -30.f}, {-8.f, -30.f}, {-10.f, -16.f}, {-11.f, 4.f},
          {-8.f, 18.f}},
         3);
    poly({{0.f, 26.f}, {6.5f, 16.f}, {8.f, 2.f}, {7.2f, -16.f}, {5.6f, -26.f}, {-5.6f, -26.f}, {-7.2f, -16.f}, {-8.f, 2.f},
          {-6.5f, 16.f}},
         2);
    poly({{0.f, 22.f}, {4.2f, 12.f}, {4.6f, -2.f}, {4.f, -18.f}, {3.f, -24.f}, {-3.f, -24.f}, {-4.f, -18.f}, {-4.6f, -2.f},
          {-4.2f, 12.f}},
         1);
    poly({{0.f, 16.f}, {3.2f, 8.f}, {3.4f, -14.f}, {2.2f, -22.f}, {-2.2f, -22.f}, {-3.4f, -14.f}, {-3.2f, 8.f}}, 4);
    stroke(-9.2f, 12.f, -8.2f, -22.f, 13, 1.5f);
    stroke(9.2f, 12.f, 8.2f, -22.f, 13, 1.5f);
    stroke(-7.4f, 10.f, 7.4f, 10.f, 5, 1.5f);
    stroke(-6.6f, -8.f, 6.6f, -8.f, 5, 1.5f);
    stroke(-10.f, 6.f, -8.6f, -18.f, 14, 1.4f);
    stroke(10.f, 6.f, 8.6f, -18.f, 14, 1.4f);
    poly({{-4.6f, 12.f}, {4.6f, 12.f}, {4.2f, 2.f}, {-4.2f, 2.f}}, 6);
    poly({{-3.4f, 11.f}, {3.4f, 11.f}, {3.f, 4.f}, {-3.f, 4.f}}, 7);
    blob(0.f, -2.f, 2.5f, 2.7f, 8);
    blob(0.f, 0.4f, 1.3f, 1.3f, 9);
    blob(-1.5f, -1.2f, 0.7f, 1.5f, 8);
    blob(1.5f, -1.2f, 0.7f, 1.5f, 8);
    poly({{-2.4f, -22.f}, {2.4f, -22.f}, {2.8f, -29.f}, {-2.8f, -29.f}}, 10);
    blob(0.f, -26.f, 1.5f, 2.2f, 11);
    stroke(-6.f, -28.f, 6.f, -28.f, 12, 1.6f);
    b.outline(15, false);
    return b;
}

Bitmap paintCliff() {
    Bitmap b(44, 40);
    b.rect(0, 0, 44, 40, 1);
    b.rect(0, 0, 44, 7, 8);
    b.rect(0, 7, 44, 3, 9);
    b.rect(28, 0, 16, 40, 2);
    b.rect(36, 0, 8, 40, 3);
    for (int x = 4; x < 40; x += 7) b.rect(x, 10, 1, 28, 4);
    b.rect(0, 32, 44, 8, 7);
    b.ellipse(12, 18, 4.5f, 3.f, 5);
    b.ellipse(20, 26, 3.2f, 2.2f, 6);
    b.rect(36, 30, 8, 8, 11);
    return b;
}

Bitmap paintPine() {
    Bitmap b(30, 42);
    b.rect(13, 28, 4, 12, 4);
    b.poly({{15, 2}, {28, 32}, {2, 32}}, 1);
    b.poly({{15, 10}, {24, 30}, {6, 30}}, 2);
    b.poly({{15, 16}, {20, 28}, {10, 28}}, 3);
    b.outline(5, false);
    return b;
}

Bitmap paintSkerry() {
    Bitmap b(42, 36);
    b.poly({{6, 24}, {12, 10}, {22, 5}, {34, 12}, {38, 22}, {30, 32}, {14, 32}}, 2);
    b.poly({{14, 22}, {18, 13}, {26, 12}, {32, 20}, {27, 28}, {16, 28}}, 3);
    b.ellipse(20, 16, 3.2f, 2.f, 5);
    b.ellipse(18, 22, 2.f, 1.4f, 4);
    b.ellipse(22, 31, 14, 3.2f, 6);
    b.outline(1, false);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(22, 30);
    b.rect(10, 16, 3, 12, 3);
    b.ellipse(11, 11, 8, 8, 1);
    b.rect(4, 9, 14, 4, 2);
    b.ellipse(11, 8, 2.f, 2.f, 2);
    b.outline(4, false);
    return b;
}

Bitmap paintBeacon() {
    Bitmap b(16, 40);
    b.rect(6, 12, 4, 22, 3);
    b.rect(4, 32, 8, 6, 4);
    b.rect(3, 4, 10, 10, 1);
    b.rect(5, 6, 6, 6, 2);
    b.outline(4, false);
    return b;
}

Bitmap paintHut() {
    Bitmap b(42, 34);
    b.rect(8, 16, 26, 14, 1);
    b.rect(8, 16, 26, 4, 2);
    b.poly({{4, 18}, {21, 5}, {38, 18}}, 3);
    b.poly({{10, 16}, {21, 8}, {32, 16}}, 4);
    b.rect(18, 20, 7, 10, 6);
    b.rect(11, 20, 5, 4, 5);
    b.rect(27, 20, 5, 4, 5);
    b.rect(19, 2, 3, 6, 2);
    b.outline(8, false);
    return b;
}

Bitmap paintSign() {
    Bitmap b(52, 34);
    b.rect(1, 1, 50, 24, 1);
    b.rect(1, 1, 50, 4, 2);
    b.rect(23, 25, 6, 8, 3);
    auto vbar = [&](int x, int y, int h) { b.rect(float(x), float(y), 3, float(h), 4); };
    auto hbar = [&](int x, int y, int w) { b.rect(float(x), float(y), float(w), 3, 4); };
    vbar(5, 7, 14);
    hbar(5, 7, 9);
    hbar(5, 12, 9);
    vbar(11, 7, 8);
    vbar(16, 7, 14);
    vbar(22, 7, 14);
    hbar(16, 7, 9);
    hbar(16, 12, 9);
    hbar(27, 7, 9);
    vbar(27, 7, 8);
    hbar(27, 12, 9);
    vbar(33, 12, 9);
    hbar(27, 18, 9);
    hbar(38, 7, 9);
    vbar(38, 7, 8);
    hbar(38, 12, 9);
    vbar(44, 12, 9);
    hbar(38, 18, 9);
    b.outline(5, false);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(32, 16);
    b.ellipse(14, 9, 3.4f, 2.2f, 1);
    if (up) {
        b.line(14, 8, 2, 2, 2, 1.7f);
        b.line(14, 8, 30, 3, 2, 1.7f);
    } else {
        b.line(14, 9, 1, 9, 2, 1.8f);
        b.line(14, 9, 30, 8, 2, 1.8f);
    }
    b.line(17, 9, 23, 11, 3, 1.3f);
    b.outline(4, false);
    return b;
}

Bitmap paintRipple() {
    Bitmap b(24, 14);
    b.ellipse(12, 7, 10, 5, 1);
    b.ellipse(12, 7, 6, 3, 2);
    b.ellipse(12, 7, 3, 1.3f, 0);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(18, 12);
    b.ellipse(9, 6, 7, 4, 3);
    b.ellipse(6, 6, 3, 2, 2);
    b.ellipse(12, 5, 2.4f, 1.6f, 2);
    return b;
}

Bitmap paintShadow() {
    Bitmap b(36, 18);
    b.ellipse(18, 9, 15, 6, 1);
    return b;
}

Bitmap paintCloud() {
    Bitmap b(52, 28);
    b.ellipse(18, 16, 14, 8, 1);
    b.ellipse(32, 15, 16, 9, 2);
    b.ellipse(26, 12, 9, 6, 3);
    return b;
}

Bitmap paintRain() {
    Bitmap b(4, 16);
    b.line(1, 0, 2, 15, 4, 1.2f);
    return b;
}

Bitmap paintBolt() {
    Bitmap b(22, 64);
    b.line(12, 1, 7, 22, 6, 2.4f);
    b.line(7, 22, 15, 26, 6, 2.2f);
    b.line(15, 26, 8, 48, 5, 2.2f);
    b.line(8, 48, 13, 62, 6, 2.f);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 9, 10), gs::rgb4(15, 15, 15), gs::rgb4(15, 5, 3), gs::rgb4(5, 14, 6),
                          gs::rgb4(15, 12, 3), gs::rgb4(6, 9, 13), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 14), gs::rgb4(8, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 3, 2), gs::rgb4(15, 14, 12), gs::rgb4(3, 2, 2), gs::rgb4(1, 1, 1), 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, gs::rgb4(2, 0, 0)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(5, 15, 7), gs::rgb4(13, 15, 13), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOAT, {0, gs::rgb4(13, 10, 6), gs::rgb4(10, 7, 4), gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2),
                           gs::rgb4(12, 9, 5), gs::rgb4(14, 6, 1), gs::rgb4(15, 10, 3), gs::rgb4(2, 3, 6),
                           gs::rgb4(12, 8, 5), gs::rgb4(2, 2, 2), gs::rgb4(9, 9, 10), gs::rgb4(14, 14, 12),
                           gs::rgb4(3, 2, 1), gs::rgb4(11, 8, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CLIFF, {0, gs::rgb4(3, 3, 4), gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(2, 2, 2),
                            gs::rgb4(2, 5, 2), gs::rgb4(7, 8, 5), gs::rgb4(2, 3, 4), gs::rgb4(3, 6, 2),
                            gs::rgb4(2, 4, 1), 0, gs::rgb4(10, 12, 12), 0, 0, 0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(3, 3, 3), gs::rgb4(7, 7, 6), gs::rgb4(11, 11, 9), gs::rgb4(2, 5, 2),
                           gs::rgb4(12, 12, 10), gs::rgb4(13, 15, 15), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(1, 4, 1), gs::rgb4(2, 6, 2), gs::rgb4(4, 8, 3), gs::rgb4(5, 3, 1),
                           gs::rgb4(1, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(3, 8, 10), gs::rgb4(6, 12, 13), gs::rgb4(13, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, gs::rgb4(1, 2, 2)});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(14, 14, 15), gs::rgb4(6, 6, 7), gs::rgb4(12, 8, 2), gs::rgb4(1, 1, 2), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STORM, {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 6), gs::rgb4(7, 7, 8), gs::rgb4(8, 9, 10),
                            gs::rgb4(13, 14, 15), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_HUT, {0, gs::rgb4(6, 6, 5), gs::rgb4(3, 3, 3), gs::rgb4(5, 2, 1), gs::rgb4(3, 1, 1),
                          gs::rgb4(13, 10, 4), gs::rgb4(2, 1, 1), 0, gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(8, 6, 3), gs::rgb4(5, 4, 2), gs::rgb4(4, 3, 2), gs::rgb4(2, 1, 1),
                           gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(14, 7, 1), gs::rgb4(15, 15, 14), gs::rgb4(2, 1, 1), gs::rgb4(1, 1, 1), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, shadow});

    for (int i = 0; i < YAWS; i++) {
        float a = float(i) * (2.f * kPi / float(YAWS));
        art.skiff[i] = gs::uploadMipped(vdp, paintSkiff(a));
    }
    art.cliff = gs::uploadMipped(vdp, paintCliff());
    art.pine = gs::uploadMipped(vdp, paintPine());
    art.skerry = gs::uploadMipped(vdp, paintSkerry());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.beacon = gs::uploadMipped(vdp, paintBeacon());
    art.hut = gs::uploadMipped(vdp, paintHut());
    art.sign = gs::uploadMipped(vdp, paintSign());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(false));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(true));
    art.ripple = gs::uploadMipped(vdp, paintRipple());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.cloud = gs::uploadMipped(vdp, paintCloud());
    art.rain = gs::uploadMipped(vdp, paintRain());
    art.bolt = gs::uploadMipped(vdp, paintBolt());
    loadFont(vdp, art);
}

}  // namespace skiffpass
