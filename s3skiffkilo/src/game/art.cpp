#include "art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace kilo {
namespace {

constexpr float kTau = 6.2831853f;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i > 0 && i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    vdp.setColor(pal * 16 + 0, 0);
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    float wx = ly * c + lx * s;
    float wy = ly * s - lx * c;
    return {cx + wx, cy - wy};
}

Bitmap paintSkiff(float heading) {
    Bitmap b(88, 88);
    const float cx = 44.f, cy = 44.f;
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
    // Ochre lapstrake skiff, blunt stern, pointed bow, oars shipped, one hand aft.
    poly({{0.f, 32.f}, {7.f, 20.f}, {10.f, 6.f}, {10.f, -8.f}, {8.f, -22.f}, {5.f, -28.f}, {-5.f, -28.f}, {-8.f, -22.f},
          {-10.f, -8.f}, {-10.f, 6.f}, {-7.f, 20.f}},
         1);
    poly({{0.f, 26.f}, {6.f, 16.f}, {7.f, 2.f}, {6.f, -16.f}, {3.f, -22.f}, {-3.f, -22.f}, {-6.f, -16.f}, {-7.f, 2.f},
          {-6.f, 16.f}},
         4);
    poly({{0.f, 22.f}, {4.5f, 12.f}, {5.f, 0.f}, {4.f, -14.f}, {2.f, -18.f}, {-2.f, -18.f}, {-4.f, -14.f}, {-5.f, 0.f},
          {-4.5f, 12.f}},
         2);
    stroke(-8.5f, 14.f, -7.5f, -18.f, 5, 1.7f);
    stroke(8.5f, 14.f, 7.5f, -18.f, 5, 1.7f);
    stroke(-6.f, 16.f, -5.f, -16.f, 11, 1.3f);
    stroke(6.f, 16.f, 5.f, -16.f, 11, 1.3f);
    stroke(-9.f, 8.f, -4.f, 4.f, 13, 1.4f);
    stroke(9.f, 8.f, 4.f, 4.f, 13, 1.4f);
    poly({{-4.f, -6.f}, {4.f, -6.f}, {3.2f, -16.f}, {-3.2f, -16.f}}, 11);
    blob(0.f, -10.f, 2.6f, 3.2f, 7);
    blob(0.f, -7.2f, 1.7f, 1.7f, 6);
    blob(-0.6f, -7.6f, 0.45f, 0.45f, 10);
    blob(0.f, -24.f, 2.4f, 2.8f, 12);
    stroke(0.f, -18.f, 2.2f, -12.f, 8, 1.3f);
    stroke(0.f, 28.f, 0.f, 20.f, 3, 1.2f);
    b.outline(15, false);
    return b;
}

Bitmap paintWheel(float ang, bool paddle) {
    Bitmap b(80, 80);
    const float cx = 40.f, cy = 40.f, R = 36.f;
    b.ellipse(cx, cy, R, R, paddle ? 1 : 2);
    b.ellipse(cx, cy, R - 7.f, R - 7.f, paddle ? 3 : 1);
    b.ellipse(cx, cy, 9.f, 9.f, 4);
    const int n = paddle ? 12 : 8;
    for (int i = 0; i < n; i++) {
        float a = ang + i * kTau / float(n);
        float ca = std::cos(a), sa = std::sin(a);
        b.line(cx, cy, cx + ca * (R - 5.f), cy + sa * (R - 5.f), paddle ? 2 : 3, paddle ? 2.f : 2.6f);
        float px = cx + ca * (R - 3.f);
        float py = cy + sa * (R - 3.f);
        if (paddle) b.rect(px - 3.2f, py - 2.2f, 6.4f, 4.4f, 6);
        else b.ellipse(px, py, 4.6f, 3.4f, 5);
    }
    b.ellipse(cx, cy, 4.2f, 4.2f, paddle ? 7 : 8);
    b.outline(15, false);
    return b;
}

Bitmap paintHand(float ang) {
    Bitmap b(32, 32);
    float x1 = 16.f + std::sin(ang) * 12.f;
    float y1 = 16.f - std::cos(ang) * 12.f;
    b.line(16.f, 16.f, x1, y1, 3, 2.2f);
    b.ellipse(16.f, 16.f, 2.2f, 2.2f, 5);
    return b;
}

Bitmap paintBank() {
    Bitmap b(64, 24);
    b.rect(0, 0, 64, 24, 1);
    b.rect(0, 0, 10, 24, 3);
    b.rect(10, 0, 8, 24, 5);
    for (int i = 0; i < 6; i++) {
        b.line(22.f + i * 7.f, 2.f, 24.f + i * 7.f, 20.f, 2, 1.2f);
        b.ellipse(30.f + (i % 3) * 9.f, 6.f + (i % 2) * 10.f, 1.4f, 2.2f, 4);
    }
    return b;
}

Bitmap paintReed() {
    Bitmap b(16, 28);
    b.line(8, 26, 5, 6, 1, 1.4f);
    b.line(8, 26, 11, 4, 2, 1.4f);
    b.line(8, 24, 8, 8, 3, 1.3f);
    b.ellipse(6, 6, 2.2f, 1.4f, 4);
    b.ellipse(11, 5, 2.f, 1.3f, 4);
    return b;
}

Bitmap paintHouse() {
    Bitmap b(48, 56);
    b.rect(8, 24, 32, 28, 1);
    b.poly({{4, 26}, {24, 6}, {44, 26}}, 2);
    b.rect(22, 4, 4, 10, 3);
    b.rect(20, 34, 8, 18, 4);
    b.rect(12, 30, 7, 8, 5);
    b.rect(30, 30, 7, 8, 5);
    b.ellipse(14, 40, 5, 5, 6);
    b.ellipse(14, 40, 2, 2, 3);
    b.outline(7, false);
    return b;
}

Bitmap paintClock() {
    Bitmap b(40, 56);
    b.rect(10, 18, 20, 32, 1);
    b.poly({{6, 20}, {20, 4}, {34, 20}}, 4);
    b.rect(18, 2, 4, 8, 5);
    b.ellipse(20, 30, 10, 10, 2);
    b.ellipse(20, 30, 8, 8, 6);
    for (int i = 0; i < 4; i++) {
        float a = i * kTau / 4.f;
        b.line(20 + std::sin(a) * 4.f, 30 - std::cos(a) * 4.f, 20 + std::sin(a) * 7.f, 30 - std::cos(a) * 7.f, 1, 1.2f);
    }
    b.rect(16, 40, 8, 10, 3);
    b.outline(7, false);
    return b;
}

Bitmap paintDock() {
    Bitmap b(96, 28);
    for (int i = 0; i < 7; i++) b.rect(4, 2 + i * 3, 88, 2, (i & 1) ? 1 : 2);
    b.rect(8, 4, 5, 22, 3);
    b.rect(84, 4, 5, 22, 3);
    b.rect(44, 6, 6, 18, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(16, 20);
    b.ellipse(8, 8, 6, 6, 1);
    b.rect(3, 6, 10, 3, 2);
    b.poly({{8, 14}, {4, 18}, {12, 18}}, 3);
    b.outline(3, false);
    return b;
}

Bitmap paintPost() {
    Bitmap b(14, 36);
    for (int i = 0; i < 6; i++) b.rect(4, 6 + i * 4, 6, 3, (i & 1) ? 1 : 2);
    b.poly({{7, 1}, {12, 8}, {2, 8}}, 3);
    b.ellipse(7, 32, 5, 2, 4);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(20, 14);
    b.ellipse(6, 7, 4, 2.4f, 1);
    b.ellipse(13, 6, 3.2f, 2.f, 2);
    b.ellipse(10, 9, 2.4f, 1.5f, 1);
    return b;
}

Bitmap paintLine() {
    Bitmap b(128, 8);
    for (int i = 0; i < 16; i++) b.rect(i * 8, 0, 8, 8, (i & 1) ? 2 : 1);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(28, 16);
    if (up) {
        b.line(2, 12, 14, 6, 1, 1.6f);
        b.line(26, 12, 14, 6, 1, 1.6f);
    } else {
        b.line(2, 4, 14, 8, 1, 1.6f);
        b.line(26, 4, 14, 8, 1, 1.6f);
    }
    b.ellipse(14, 8, 2.1f, 1.5f, 2);
    b.line(14, 8, 18, 10, 3, 1.1f);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(4, 5, 6), gs::rgb4(14, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, ink});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(14, 11, 6), gs::rgb4(9, 7, 4), gs::rgb4(2, 2, 2), gs::rgb4(2, 8, 5), gs::rgb4(13, 3, 2),
            gs::rgb4(14, 10, 7), gs::rgb4(3, 5, 12), gs::rgb4(13, 11, 4), gs::rgb4(15, 15, 14), gs::rgb4(3, 2, 1),
            gs::rgb4(8, 6, 3), gs::rgb4(3, 3, 4), gs::rgb4(6, 5, 3), gs::rgb4(10, 8, 5), ink});
    setPal(vdp, PAL_MILL, {0, gs::rgb4(13, 10, 5), gs::rgb4(8, 6, 3), gs::rgb4(5, 3, 2), gs::rgb4(4, 4, 5),
                           gs::rgb4(6, 7, 4), gs::rgb4(10, 8, 4), gs::rgb4(3, 3, 3), gs::rgb4(12, 11, 8), 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_PADDLE,
           {0, gs::rgb4(3, 4, 5), gs::rgb4(6, 7, 8), gs::rgb4(9, 6, 4), gs::rgb4(2, 2, 3), gs::rgb4(8, 8, 7),
            gs::rgb4(12, 10, 6), gs::rgb4(14, 13, 8), gs::rgb4(5, 5, 6), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(4, 10, 3), gs::rgb4(2, 6, 2), gs::rgb4(8, 6, 3), gs::rgb4(10, 12, 4),
                           gs::rgb4(7, 6, 4), gs::rgb4(5, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_HOUSE, {0, gs::rgb4(10, 9, 8), gs::rgb4(8, 3, 2), gs::rgb4(4, 4, 4), gs::rgb4(3, 2, 2),
                            gs::rgb4(6, 8, 10), gs::rgb4(7, 5, 3), ink, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(9, 7, 4), gs::rgb4(14, 13, 10), gs::rgb4(12, 2, 2), gs::rgb4(6, 2, 2),
                            gs::rgb4(13, 11, 4), gs::rgb4(15, 15, 13), ink, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 14, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 9, 10), gs::rgb4(14, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 7), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TAG, {0, gs::rgb4(15, 14, 8), gs::rgb4(5, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CREW,
           {0, gs::rgb4(3, 6, 4), gs::rgb4(2, 4, 3), gs::rgb4(1, 1, 2), gs::rgb4(1, 4, 3), gs::rgb4(8, 3, 2),
            gs::rgb4(10, 8, 6), gs::rgb4(2, 3, 6), gs::rgb4(8, 7, 3), gs::rgb4(12, 12, 11), gs::rgb4(2, 2, 1),
            gs::rgb4(5, 4, 2), gs::rgb4(2, 2, 3), gs::rgb4(4, 3, 2), gs::rgb4(6, 5, 3), ink});
    setPal(vdp, PAL_SPAR, {0, gs::rgb4(14, 12, 6), gs::rgb4(3, 3, 3), gs::rgb4(13, 3, 2), gs::rgb4(6, 5, 3), 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, ink});

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.hull[i] = gs::uploadMipped(vdp, paintSkiff(i * kTau / 16.f));
    for (int i = 0; i < 8; i++) {
        float a = i * kTau / 8.f;
        art.mill[i] = gs::uploadMipped(vdp, paintWheel(a, false));
        art.paddle[i] = gs::uploadMipped(vdp, paintWheel(a, true));
        art.hand[i] = gs::uploadMipped(vdp, paintHand(a));
    }
    art.bank = gs::uploadMipped(vdp, paintBank());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.house = gs::uploadMipped(vdp, paintHouse());
    art.clock = gs::uploadMipped(vdp, paintClock());
    art.dock = gs::uploadMipped(vdp, paintDock());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.line = gs::uploadMipped(vdp, paintLine());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(true));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(false));
    art.title = words(vdp, "S3 SKIFF KILO", 2);
    art.clean = words(vdp, "WHEELS CLEAN", 2);
    art.took = words(vdp, "CREW TOOK IT", 2);
    art.touched = words(vdp, "TOUCHED A WHEEL", 2);
    art.beached = words(vdp, "LEFT THE FAIRWAY", 2);
    art.paused = words(vdp, "PAUSED", 3);
    art.kilo = words(vdp, "ONE KILOMETER", 2);
    art.m250 = words(vdp, "250", 2);
    art.m500 = words(vdp, "500", 2);
    art.m750 = words(vdp, "750", 2);
    art.m1000 = words(vdp, "1000", 2);
    art.crewTag = words(vdp, "CREW", 2);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(1, 5, 7));
}

}  // namespace kilo
