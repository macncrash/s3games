#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace cab {
namespace {

constexpr float kTau = 6.2831853f;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    return {cx + ly * c + lx * s, cy - (ly * s - lx * c)};
}

Bitmap paintCab(float heading) {
    Bitmap b(128, 128);
    const float cx = 64.f, cy = 64.f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, c, s));
        b.poly(w, col);
    };
    auto blob = [&](float lx, float ly, float r, int col) {
        Pt p = spin(cx, cy, lx, ly, c, s);
        b.ellipse(p.first, p.second, r, r, col);
    };
    auto stroke = [&](float ax, float ay, float bx, float by, int col, float th) {
        Pt A = spin(cx, cy, ax, ay, c, s);
        Pt B = spin(cx, cy, bx, by, c, s);
        b.line(A.first, A.second, B.first, B.second, col, th);
    };
    // Yellow water-taxi hull, bow north in local space.
    poly({{0.f, 46.f}, {14.f, 34.f}, {16.f, 8.f}, {14.f, -28.f}, {8.f, -44.f},
          {-8.f, -44.f}, {-14.f, -28.f}, {-16.f, 8.f}, {-14.f, 34.f}},
         1);
    poly({{0.f, 38.f}, {10.f, 26.f}, {11.f, 4.f}, {9.f, -24.f}, {5.f, -36.f},
          {-5.f, -36.f}, {-9.f, -24.f}, {-11.f, 4.f}, {-10.f, 26.f}},
         2);
    // Checker belt.
    for (int i = -3; i <= 3; i++) {
        float y0 = -8.f + i * 6.f;
        poly({{(i & 1) ? -12.f : -4.f, y0 + 5.f}, {(i & 1) ? -4.f : 4.f, y0 + 5.f},
              {(i & 1) ? -4.f : 4.f, y0}, {(i & 1) ? -12.f : -4.f, y0}},
             3);
        poly({{(i & 1) ? 4.f : -4.f, y0 + 5.f}, {(i & 1) ? 12.f : 4.f, y0 + 5.f},
              {(i & 1) ? 12.f : 4.f, y0}, {(i & 1) ? 4.f : -4.f, y0}},
             (i & 1) ? 3 : 4);
    }
    poly({{-9.f, 18.f}, {9.f, 18.f}, {8.f, 2.f}, {-8.f, 2.f}}, 5);
    poly({{-7.f, 16.f}, {7.f, 16.f}, {6.f, 6.f}, {-6.f, 6.f}}, 6);
    poly({{-8.f, -6.f}, {8.f, -6.f}, {7.f, -22.f}, {-7.f, -22.f}}, 7);
    blob(0.f, 28.f, 3.4f, 8);
    stroke(-6.f, 24.f, 6.f, 24.f, 8, 2.f);
    blob(-11.f, 30.f, 2.f, 9);
    blob(11.f, 30.f, 2.f, 10);
    b.outline(11, false);
    return b;
}

Bitmap nunArt(const char* num) {
    Bitmap b(42, 58);
    b.ellipse(21, 48, 14, 6, 2);
    b.poly({{21, 8}, {34, 42}, {8, 42}}, 1);
    b.rect(13, 26, 16, 8, 2);
    b.ellipse(21, 10, 3.2f, 3.2f, 4);
    b.outline(3, false);
    gs::TextStyle st{2, 5, 0, 0, 0};
    Bitmap t = gs::textBitmap(num, st);
    b.blit(t, 21 - t.w / 2, 24);
    return b;
}

Bitmap canArt(const char* num) {
    Bitmap b(42, 58);
    b.ellipse(21, 48, 14, 6, 2);
    b.rect(10, 18, 22, 28, 1);
    b.ellipse(21, 18, 11, 5, 1);
    b.rect(12, 26, 18, 8, 2);
    b.poly({{21, 4}, {29, 14}, {21, 16}, {13, 14}}, 3);
    b.ellipse(21, 6, 2.4f, 2.4f, 4);
    b.outline(3, false);
    gs::TextStyle st{2, 5, 0, 0, 0};
    Bitmap t = gs::textBitmap(num, st);
    b.blit(t, 21 - t.w / 2, 24);
    return b;
}

Bitmap quayArt(int variant) {
    Bitmap b(40, 34);
    for (int i = 0; i < 4; i++) b.rect(1, 16 + i * 4, 38, 3, (i & 1) ? 1 : 2);
    int bx = (variant & 1) ? 6 : 28;
    b.rect(float(bx), 18, 5, 5, 3);
    b.rect(18, 28, 4, 4, 4);
    return b;
}

Bitmap standArt() {
    Bitmap b(64, 48);
    b.rect(6, 20, 52, 22, 1);
    b.poly({{4, 22}, {32, 6}, {60, 22}}, 2);
    b.rect(28, 28, 8, 14, 3);
    b.rect(12, 26, 10, 6, 4);
    b.rect(42, 26, 10, 6, 4);
    b.rect(24, 10, 16, 6, 5);
    b.outline(6, false);
    return b;
}

Bitmap pileArt() {
    Bitmap b(14, 22);
    b.rect(5, 6, 4, 14, 1);
    b.ellipse(7, 6, 5, 3, 2);
    b.outline(3, false);
    return b;
}

Bitmap signArt() {
    Bitmap b(72, 18);
    b.rect(2, 3, 68, 12, 1);
    b.rect(4, 5, 64, 8, 2);
    gs::TextStyle st{1, 3, 0, 0, 0};
    Bitmap t = gs::textBitmap("TAXI", st);
    b.blit(t, 36 - t.w / 2, 5);
    b.outline(4, false);
    return b;
}

Bitmap lampPostArt() {
    Bitmap b(18, 40);
    b.rect(8, 14, 3, 24, 1);
    b.ellipse(9, 10, 6, 6, 2);
    b.ellipse(9, 10, 3, 3, 3);
    return b;
}

Bitmap ferryArt() {
    Bitmap b(96, 36);
    b.poly({{4, 22}, {16, 10}, {80, 10}, {92, 22}, {80, 28}, {16, 28}}, 1);
    b.rect(22, 6, 28, 12, 2);
    b.rect(54, 8, 16, 10, 3);
    b.rect(28, 9, 6, 5, 4);
    b.rect(38, 9, 6, 5, 4);
    b.outline(5, false);
    return b;
}

Bitmap dinghyArt() {
    Bitmap b(36, 16);
    b.poly({{2, 8}, {28, 2}, {34, 8}, {28, 14}}, 1);
    b.ellipse(18, 8, 3, 2, 2);
    b.outline(3, false);
    return b;
}

Bitmap rockArt() {
    Bitmap b(48, 36);
    b.ellipse(24, 20, 20, 12, 1);
    b.ellipse(18, 16, 8, 5, 2);
    b.ellipse(30, 22, 6, 4, 3);
    b.outline(4, false);
    return b;
}

Bitmap gullArt(bool up) {
    Bitmap b(28, 16);
    b.ellipse(14, 9, 3.2f, 2.f, 1);
    float tip = up ? 2.f : 13.f;
    b.line(14, 8, 2, tip, 1, 1.5f);
    b.line(14, 8, 26, tip, 2, 1.5f);
    return b;
}

Bitmap foamArt() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 7, 3, 1);
    b.ellipse(6, 6, 3, 1.5f, 2);
    return b;
}

Bitmap smokeArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 5, 1);
    b.ellipse(7, 7, 3, 2.2f, 2);
    return b;
}

Bitmap ringArt() {
    Bitmap b(72, 72);
    const float c = 35.5f, r = 32.f;
    for (int d = 0; d < 360; d += 12) {
        float a0 = d * kTau / 360.f;
        float a1 = (d + 6.f) * kTau / 360.f;
        b.line(c + std::cos(a0) * r, c + std::sin(a0) * r, c + std::cos(a1) * r, c + std::sin(a1) * r, 1, 2.2f);
    }
    return b;
}

Bitmap lampArt() {
    Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2.2f, 2.2f, 2);
    return b;
}

Bitmap pinArt() {
    Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
    return b;
}

Bitmap panelArt() {
    Bitmap b(78, 64);
    b.rect(0, 0, 78, 64, 1);
    for (int x = 0; x < 78; x++) {
        b.set(x, 0, 2);
        b.set(x, 63, 2);
    }
    for (int y = 0; y < 64; y++) {
        b.set(0, y, 2);
        b.set(77, y, 2);
    }
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
    uint8_t a[64] = {};
    uint8_t w[64] = {};
    a[2 * 8 + 2] = 1;
    a[4 * 8 + 4] = 2;
    a[6 * 8 + 1] = 1;
    w[1 * 8 + 5] = 2;
    w[3 * 8 + 2] = 1;
    w[5 * 8 + 6] = 2;
    int t0 = tiles.alloc(1);
    int t1 = tiles.alloc(1);
    vdp.loadTile(t0, a);
    vdp.loadTile(t1, w);
    for (int cy = 0; cy < vdp.B.h; cy++) {
        for (int cx = 0; cx < vdp.B.w; cx++) {
            int tile = ((cx + cy) & 1) ? t1 : t0;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_WAVE));
        }
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 9, 11), gs::rgb4(15, 13, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(15, 13, 1), gs::rgb4(14, 11, 2), gs::rgb4(1, 1, 2), gs::rgb4(15, 15, 14),
            gs::rgb4(4, 8, 12), gs::rgb4(10, 14, 15), gs::rgb4(3, 3, 4), gs::rgb4(15, 4, 2),
            gs::rgb4(15, 15, 12), gs::rgb4(2, 6, 15), gs::rgb4(2, 2, 3), 0, 0, 0, shadow});
    setPal(vdp, PAL_NUN, {0, gs::rgb4(14, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(3, 1, 1), gs::rgb4(15, 10, 2),
                          gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CAN, {0, gs::rgb4(2, 12, 4), gs::rgb4(15, 15, 15), gs::rgb4(1, 5, 2), gs::rgb4(15, 14, 4),
                          gs::rgb4(1, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(11, 8, 5), gs::rgb4(7, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(6, 9, 11),
                           gs::rgb4(3, 2, 2), gs::rgb4(14, 12, 8), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 9, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10), gs::rgb4(4, 5, 4), gs::rgb4(3, 3, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WAVE, {0, gs::rgb4(6, 12, 14), gs::rgb4(12, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STAND, {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 3, 2), gs::rgb4(3, 2, 2), gs::rgb4(10, 13, 15),
                            gs::rgb4(15, 12, 2), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 3), gs::rgb4(3, 2, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(8, 8, 9), gs::rgb4(13, 13, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(1, 2, 5), gs::rgb4(9, 10, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 3), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.cab[i] = gs::uploadMipped(vdp, paintCab(i * kTau / 16.f));
    art.buoy[0] = gs::uploadMipped(vdp, nunArt("1"));
    art.buoy[1] = gs::uploadMipped(vdp, canArt("2"));
    art.buoy[2] = gs::uploadMipped(vdp, nunArt("3"));
    art.quay = gs::uploadMipped(vdp, quayArt(0));
    art.quayB = gs::uploadMipped(vdp, quayArt(1));
    art.stand = gs::uploadMipped(vdp, standArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    art.lampPost = gs::uploadMipped(vdp, lampPostArt());
    art.ferry = gs::uploadMipped(vdp, ferryArt());
    art.dinghy = gs::uploadMipped(vdp, dinghyArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(true));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(false));
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.smoke = gs::uploadMipped(vdp, smokeArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.pin = gs::uploadMipped(vdp, pinArt());
    Bitmap dot(6, 6);
    dot.ellipse(3, 3, 2.2f, 2.2f, 1);
    art.dot = gs::uploadMipped(vdp, dot);
    art.panel = gs::uploadMipped(vdp, panelArt());
    art.title = words(vdp, "CAB BUOY", 3);
    art.sub = words(vdp, "ROUND THE BUOYS", 2);
    art.same = words(vdp, "SAME DOCK", 3);
    art.paused = words(vdp, "PAUSED", 3);

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
}

}  // namespace cab
