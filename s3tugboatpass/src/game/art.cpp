#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace tugpass {
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
    return {cx + lx * c + ly * s, cy - ly * c + lx * s};
}

// Nose is +ly. Heading 0 points up the bitmap; positive heading turns the nose east.
Bitmap paintTug(float heading) {
    Bitmap b(104, 104);
    const float cx = 52.f, cy = 52.f;
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

    poly({{0.f, 36.f}, {12.f, 26.f}, {16.f, 10.f}, {16.f, -18.f}, {11.f, -34.f}, {-11.f, -34.f}, {-16.f, -18.f},
          {-16.f, 10.f}, {-12.f, 26.f}},
         1);
    poly({{0.f, 30.f}, {9.f, 20.f}, {11.f, 6.f}, {11.f, -16.f}, {7.f, -28.f}, {-7.f, -28.f}, {-11.f, -16.f}, {-11.f, 6.f},
          {-9.f, 20.f}},
         2);
    stroke(-15.2f, 18.f, -14.2f, -28.f, 3, 2.6f);
    stroke(15.2f, 18.f, 14.2f, -28.f, 3, 2.6f);
    for (float y : {16.f, 4.f, -8.f, -20.f}) {
        blob(-16.6f, y, 3.1f, 2.6f, 12);
        blob(16.6f, y, 3.1f, 2.6f, 12);
        blob(-16.6f, y, 1.5f, 1.3f, 9);
        blob(16.6f, y, 1.5f, 1.3f, 9);
    }
    blob(0.f, 27.f, 7.2f, 2.4f, 12);
    poly({{-9.2f, 20.f}, {9.2f, 20.f}, {8.4f, 4.f}, {-8.4f, 4.f}}, 4);
    poly({{-7.4f, 18.f}, {7.4f, 18.f}, {6.8f, 6.f}, {-6.8f, 6.f}}, 5);
    poly({{-5.6f, 16.f}, {-1.2f, 16.f}, {-1.2f, 9.f}, {-5.6f, 9.f}}, 6);
    poly({{1.2f, 16.f}, {5.6f, 16.f}, {5.6f, 9.f}, {1.2f, 9.f}}, 6);
    poly({{-3.6f, 2.f}, {3.6f, 2.f}, {3.1f, -12.f}, {-3.1f, -12.f}}, 7);
    poly({{-3.8f, -2.f}, {3.8f, -2.f}, {3.4f, -7.f}, {-3.4f, -7.f}}, 8);
    stroke(0.f, 22.f, 0.f, 33.f, 11, 1.5f);
    stroke(-4.2f, 31.f, 4.2f, 31.f, 11, 1.2f);
    poly({{-7.f, -18.f}, {7.f, -18.f}, {6.f, -30.f}, {-6.f, -30.f}}, 10);
    blob(-3.2f, -24.f, 1.7f, 1.7f, 8);
    blob(3.2f, -24.f, 1.7f, 1.7f, 8);
    blob(-6.4f, 24.f, 1.3f, 1.3f, 13);
    blob(6.4f, 24.f, 1.3f, 1.3f, 14);
    b.outline(15, false);
    return b;
}

Bitmap paintFace() {
    Bitmap b(48, 36);
    b.rect(0, 0, 48, 36, 1);
    for (int row = 0; row < 6; row++) {
        int y = 2 + row * 6;
        int off = (row & 1) ? 8 : 0;
        for (int x = off; x < 48; x += 16) b.rect(x, y, 14, 4, (row + x / 8) & 1 ? 2 : 3);
    }
    b.rect(34, 0, 14, 36, 6);
    for (int i = 0; i < 7; i++) b.rect(4 + (i * 5) % 28, 6 + (i * 7) % 24, 3, 2, 5);
    b.outline(15, false);
    return b;
}

Bitmap paintRidge() {
    Bitmap b(48, 36);
    b.rect(0, 0, 48, 36, 2);
    b.rect(0, 0, 18, 36, 4);
    b.rect(14, 0, 6, 36, 3);
    for (int row = 0; row < 5; row++) b.rect(20, 3 + row * 7, 26, 2, row & 1 ? 1 : 7);
    for (int i = 0; i < 5; i++) b.ellipse(float(6 + (i * 7) % 12), float(4 + i * 6), 2.2f, 1.4f, 4);
    b.rect(36, 0, 12, 36, 6);
    b.outline(15, false);
    return b;
}

Bitmap paintPine() {
    Bitmap b(22, 30);
    b.poly({{11.f, 1.f}, {20.f, 16.f}, {2.f, 16.f}}, 1);
    b.poly({{11.f, 8.f}, {21.f, 26.f}, {1.f, 26.f}}, 2);
    b.poly({{11.f, 6.f}, {16.f, 14.f}, {6.f, 14.f}}, 3);
    b.rect(9, 24, 4, 6, 4);
    b.outline(15, false);
    return b;
}

Bitmap paintHut() {
    Bitmap b(36, 30);
    b.rect(6, 12, 24, 14, 1);
    b.poly({{3.f, 14.f}, {18.f, 3.f}, {33.f, 14.f}}, 2);
    b.rect(15, 17, 6, 9, 3);
    b.rect(8, 16, 5, 4, 4);
    b.rect(23, 16, 5, 4, 4);
    b.rect(16, 4, 3, 6, 5);
    b.outline(15, false);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(16, 24);
    b.ellipse(8, 14, 6.2f, 6.2f, 1);
    b.ellipse(8, 12, 3.4f, 2.2f, 4);
    b.rect(7, 3, 2, 8, 2);
    b.ellipse(8, 3, 1.8f, 1.8f, 4);
    b.outline(15, false);
    return b;
}

Bitmap paintBeacon() {
    Bitmap b(18, 32);
    b.rect(6, 12, 6, 16, 2);
    b.poly({{3.f, 14.f}, {9.f, 4.f}, {15.f, 14.f}}, 3);
    b.ellipse(9, 8, 3.2f, 3.2f, 1);
    b.ellipse(9, 8, 1.5f, 1.5f, 4);
    b.rect(8, 26, 2, 5, 2);
    b.outline(15, false);
    return b;
}

Bitmap paintDaymark() {
    Bitmap b(18, 28);
    b.poly({{9.f, 2.f}, {16.f, 12.f}, {9.f, 22.f}, {2.f, 12.f}}, 1);
    b.rect(8, 20, 2, 7, 2);
    b.outline(15, false);
    return b;
}

Bitmap paintGull(bool flap) {
    Bitmap b(28, 14);
    if (flap) {
        b.line(2, 11, 14, 5, 1, 1.7f);
        b.line(14, 5, 26, 11, 1, 1.7f);
    } else {
        b.line(2, 6, 14, 8, 1, 1.7f);
        b.line(14, 8, 26, 6, 1, 1.7f);
    }
    b.ellipse(14, 8, 2.2f, 1.5f, 2);
    return b;
}

Bitmap paintSmoke() {
    Bitmap b(16, 16);
    b.ellipse(8, 9, 6.2f, 5.f, 3);
    b.ellipse(7, 8, 3.2f, 2.5f, 1);
    return b;
}

Bitmap paintWake() {
    Bitmap b(28, 12);
    b.ellipse(14, 6, 12.f, 4.5f, 2);
    b.ellipse(14, 6, 7.f, 2.4f, 1);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 6.5f, 4.f, 1);
    b.ellipse(8, 6, 3.f, 1.8f, 2);
    return b;
}

Bitmap paintShadow() {
    Bitmap b(36, 16);
    b.ellipse(18, 8, 16.f, 6.f, 1);
    return b;
}

Bitmap paintCloud() {
    Bitmap b(40, 18);
    b.ellipse(14, 10, 10.f, 6.f, 1);
    b.ellipse(26, 9, 11.f, 6.5f, 2);
    b.ellipse(20, 8, 7.f, 4.f, 1);
    return b;
}

Bitmap paintRain() {
    Bitmap b(6, 18);
    b.line(4, 1, 1, 16, 3, 1.3f);
    return b;
}

Bitmap paintBolt() {
    Bitmap b(18, 48);
    b.line(10, 1, 6, 16, 4, 2.2f);
    b.line(6, 16, 13, 22, 5, 2.f);
    b.line(13, 22, 5, 36, 4, 2.2f);
    b.line(5, 36, 9, 47, 5, 1.8f);
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
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 10, 12), gs::rgb4(15, 15, 15), gs::rgb4(15, 6, 4), gs::rgb4(6, 14, 8),
                          gs::rgb4(15, 12, 4), gs::rgb4(5, 9, 12), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(12, 8, 2), ink, gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 3, 2), gs::rgb4(7, 6, 5), gs::rgb4(8, 2, 2), gs::rgb4(15, 14, 12), 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN,
           {0, gs::rgb4(4, 14, 6), gs::rgb4(7, 7, 6), gs::rgb4(2, 6, 3), gs::rgb4(14, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, shadow});
    setPal(vdp, PAL_TUG,
           {0, gs::rgb4(1, 2, 3), gs::rgb4(3, 5, 6), gs::rgb4(13, 2, 2), gs::rgb4(14, 14, 13), gs::rgb4(8, 9, 9),
            gs::rgb4(3, 7, 11), gs::rgb4(14, 11, 2), gs::rgb4(1, 1, 1), gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 4),
            gs::rgb4(12, 12, 11), gs::rgb4(13, 13, 12), gs::rgb4(15, 4, 3), gs::rgb4(4, 14, 6), gs::rgb4(0, 0, 1)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(1, 4, 3), gs::rgb4(2, 7, 5), gs::rgb4(12, 12, 10), gs::rgb4(14, 14, 12), gs::rgb4(8, 9, 8),
            gs::rgb4(3, 6, 9), gs::rgb4(13, 7, 2), gs::rgb4(1, 1, 1), gs::rgb4(2, 2, 2), gs::rgb4(4, 5, 4),
            gs::rgb4(11, 12, 11), gs::rgb4(12, 13, 12), gs::rgb4(15, 5, 3), gs::rgb4(8, 14, 8), gs::rgb4(0, 1, 1)});
    setPal(vdp, PAL_CLIFF,
           {0, gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10), gs::rgb4(13, 14, 15), gs::rgb4(2, 4, 3),
            gs::rgb4(2, 3, 4), gs::rgb4(5, 5, 6), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(1, 4, 2), gs::rgb4(2, 7, 3), gs::rgb4(4, 10, 4), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, gs::rgb4(0, 1, 1)});
    setPal(vdp, PAL_HUT, {0, gs::rgb4(12, 10, 7), gs::rgb4(6, 3, 2), gs::rgb4(3, 2, 2), gs::rgb4(14, 12, 4),
                          gs::rgb4(4, 4, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 12, 13), gs::rgb4(10, 11, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, shadow});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(15, 15, 15), gs::rgb4(7, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STORM,
           {0, gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 6), gs::rgb4(10, 11, 13), gs::rgb4(15, 15, 14), gs::rgb4(13, 13, 8), 0,
            0, 0, 0, 0, 0, 0, 0, 0, shadow});

    for (int i = 0; i < YAWS; i++) {
        float a = float(i) * (kPi * 2.f / float(YAWS));
        art.tug[i] = gs::uploadMipped(vdp, paintTug(a));
    }
    art.face = gs::uploadMipped(vdp, paintFace());
    art.ridge = gs::uploadMipped(vdp, paintRidge());
    art.pine = gs::uploadMipped(vdp, paintPine());
    art.hut = gs::uploadMipped(vdp, paintHut());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.beacon = gs::uploadMipped(vdp, paintBeacon());
    art.daymark = gs::uploadMipped(vdp, paintDaymark());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(false));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(true));
    art.smoke = gs::uploadMipped(vdp, paintSmoke());
    art.wake = gs::uploadMipped(vdp, paintWake());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.cloud = gs::uploadMipped(vdp, paintCloud());
    art.rain = gs::uploadMipped(vdp, paintRain());
    art.bolt = gs::uploadMipped(vdp, paintBolt());
    loadFont(vdp, art);
}

}  // namespace tugpass
