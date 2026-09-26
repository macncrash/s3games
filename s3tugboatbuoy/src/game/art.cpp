#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace tug {
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
    float wx = ly * c + lx * s;
    float wy = ly * s - lx * c;
    return {cx + wx, cy - wy};
}

Bitmap paintTug(float heading) {
    Bitmap b(128, 128);
    const float cx = 64.f, cy = 64.f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        w.reserve(local.size());
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

    poly({{0.f, 48.f}, {8.f, 40.f}, {15.f, 28.f}, {17.f, 10.f}, {16.f, -12.f}, {14.f, -30.f}, {9.f, -46.f},
          {-9.f, -46.f}, {-14.f, -30.f}, {-16.f, -12.f}, {-17.f, 10.f}, {-15.f, 28.f}, {-8.f, 40.f}},
         1);
    poly({{0.f, 40.f}, {6.f, 32.f}, {11.f, 16.f}, {12.f, 0.f}, {11.f, -18.f}, {8.f, -36.f}, {-8.f, -36.f},
          {-11.f, -18.f}, {-12.f, 0.f}, {-11.f, 16.f}, {-6.f, 32.f}},
         11);
    poly({{0.f, 52.f}, {9.f, 44.f}, {0.f, 40.f}, {-9.f, 44.f}}, 6);
    poly({{0.f, 50.f}, {6.f, 45.f}, {0.f, 42.f}, {-6.f, 45.f}}, 7);
    poly({{0.f, 36.f}, {4.f, 28.f}, {-4.f, 28.f}}, 9);
    blob(16.f, 22.f, 3.2f, 6);
    blob(-16.f, 22.f, 3.2f, 6);
    blob(16.f, 2.f, 3.2f, 6);
    blob(-16.f, 2.f, 3.2f, 6);
    blob(15.f, -18.f, 3.2f, 6);
    blob(-15.f, -18.f, 3.2f, 6);
    poly({{-4.f, 26.f}, {4.f, 26.f}, {4.f, 18.f}, {-4.f, 18.f}}, 10);
    blob(0.f, 22.f, 2.1f, 9);
    poly({{-10.f, 4.f}, {10.f, 4.f}, {10.f, -16.f}, {-10.f, -16.f}}, 3);
    poly({{-9.f, -4.f}, {9.f, -4.f}, {9.f, -15.f}, {-9.f, -15.f}}, 12);
    poly({{-7.f, 2.f}, {7.f, 2.f}, {7.f, -2.f}, {-7.f, -2.f}}, 4);
    blob(-6.f, -8.f, 2.3f, 7);
    poly({{-5.f, -16.f}, {5.f, -16.f}, {5.f, -30.f}, {-5.f, -30.f}}, 5);
    poly({{-5.f, -20.f}, {5.f, -20.f}, {5.f, -23.f}, {-5.f, -23.f}}, 8);
    poly({{-4.f, -28.f}, {4.f, -28.f}, {4.f, -33.f}, {-4.f, -33.f}}, 6);
    stroke(0.f, 8.f, 0.f, 22.f, 9, 1.6f);
    poly({{0.f, 21.f}, {8.f, 16.f}, {0.f, 15.f}}, 8);
    stroke(-7.f, -34.f, 7.f, -34.f, 10, 2.2f);
    stroke(-7.f, -30.f, -7.f, -40.f, 10, 2.2f);
    stroke(7.f, -30.f, 7.f, -40.f, 10, 2.2f);
    blob(-14.f, 30.f, 1.8f, 8);
    blob(14.f, 30.f, 1.8f, 2);
    b.outline(6, false);
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

Bitmap shedArt() {
    Bitmap b(56, 44);
    b.rect(8, 16, 40, 22, 1);
    b.poly({{6, 18}, {28, 6}, {50, 18}}, 2);
    b.rect(24, 26, 8, 12, 3);
    b.rect(12, 22, 8, 6, 4);
    b.rect(36, 22, 8, 6, 4);
    b.outline(5, false);
    return b;
}

Bitmap pileArt() {
    Bitmap b(14, 22);
    b.rect(5, 6, 4, 14, 1);
    b.ellipse(7, 6, 5, 3, 2);
    b.outline(3, false);
    return b;
}

Bitmap beamArt() {
    Bitmap b(72, 16);
    b.rect(2, 4, 68, 8, 1);
    b.rect(2, 4, 68, 3, 2);
    b.rect(6, 6, 4, 4, 3);
    b.rect(62, 6, 4, 4, 3);
    b.outline(3, false);
    return b;
}

Bitmap craneArt() {
    Bitmap b(64, 52);
    b.rect(26, 32, 18, 14, 1);
    b.rect(29, 26, 12, 10, 2);
    b.line(35, 30, 8, 8, 3, 2.4f);
    b.line(12, 12, 12, 24, 4, 1.3f);
    b.ellipse(12, 25, 3, 2, 4);
    b.outline(5, false);
    return b;
}

Bitmap bargeArt() {
    Bitmap b(90, 38);
    b.rect(4, 8, 82, 22, 1);
    b.rect(8, 12, 16, 14, 3);
    b.rect(28, 12, 16, 14, 3);
    b.rect(48, 12, 12, 14, 3);
    b.rect(64, 6, 20, 26, 2);
    b.rect(70, 12, 8, 6, 4);
    b.outline(5, false);
    return b;
}

Bitmap skiffArt() {
    Bitmap b(40, 18);
    b.poly({{2, 9}, {30, 3}, {36, 9}, {30, 15}}, 1);
    b.rect(12, 6, 8, 6, 2);
    b.ellipse(22, 9, 2, 2, 3);
    b.outline(4, false);
    return b;
}

Bitmap rockArt() {
    Bitmap b(48, 36);
    b.ellipse(24, 20, 20, 12, 1);
    b.ellipse(20, 17, 10, 7, 2);
    b.ellipse(30, 22, 6, 4, 3);
    b.ellipse(16, 22, 4, 3, 4);
    b.outline(2, false);
    return b;
}

Bitmap gullArt(bool up) {
    Bitmap b(28, 16);
    b.ellipse(14, 9, 3.5f, 2.f, 1);
    float tip = up ? 2.f : 13.f;
    b.line(14, 8, 2, tip, 1, 1.6f);
    b.line(14, 8, 26, tip, 2, 1.6f);
    b.set(18, 8, 3);
    return b;
}

Bitmap foamArt() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 7, 3, 1);
    b.ellipse(6, 6, 3, 1.6f, 2);
    return b;
}

Bitmap smokeArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 5, 1);
    b.ellipse(7, 7, 3, 2.4f, 2);
    return b;
}

Bitmap ringArt() {
    Bitmap b(72, 72);
    const float c = 35.5f, r = 32.f;
    for (int d = 0; d < 360; d += 10) {
        float a0 = d * kTau / 360.f;
        float a1 = (d + 5.f) * kTau / 360.f;
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
    a[2 * 8 + 1] = 1;
    a[3 * 8 + 2] = 2;
    a[4 * 8 + 3] = 1;
    a[5 * 8 + 5] = 2;
    a[6 * 8 + 6] = 1;
    w[1 * 8 + 6] = 2;
    w[3 * 8 + 4] = 1;
    w[4 * 8 + 1] = 2;
    w[6 * 8 + 3] = 1;
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 9, 11), gs::rgb4(15, 13, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TUG, {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 15, 6), gs::rgb4(15, 15, 15), gs::rgb4(6, 12, 15),
                          gs::rgb4(15, 12, 2), gs::rgb4(1, 1, 2), gs::rgb4(15, 7, 1), gs::rgb4(14, 2, 2),
                          gs::rgb4(15, 14, 11), gs::rgb4(10, 7, 3), gs::rgb4(2, 8, 4), gs::rgb4(12, 13, 14), 0, 0, shadow});
    setPal(vdp, PAL_NUN, {0, gs::rgb4(14, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(3, 1, 1), gs::rgb4(15, 10, 2),
                          gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CAN, {0, gs::rgb4(2, 12, 4), gs::rgb4(15, 15, 15), gs::rgb4(1, 5, 2), gs::rgb4(15, 14, 4),
                          gs::rgb4(1, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(12, 9, 6), gs::rgb4(8, 6, 4), gs::rgb4(4, 3, 2), gs::rgb4(5, 8, 10),
                           gs::rgb4(3, 2, 2), gs::rgb4(9, 8, 7), gs::rgb4(14, 12, 8), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 9, 10), gs::rgb4(15, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5), gs::rgb4(5, 8, 4), gs::rgb4(11, 11, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WAVE, {0, gs::rgb4(7, 13, 14), gs::rgb4(13, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BARGE, {0, gs::rgb4(11, 6, 3), gs::rgb4(6, 3, 2), gs::rgb4(8, 7, 5), gs::rgb4(4, 8, 10),
                            gs::rgb4(3, 2, 2), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 8), gs::rgb4(3, 2, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(8, 8, 9), gs::rgb4(13, 13, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(1, 2, 5), gs::rgb4(8, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 3), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.tug[i] = gs::uploadMipped(vdp, paintTug(i * kTau / 16.f));
    art.buoy[0] = gs::uploadMipped(vdp, nunArt("1"));
    art.buoy[1] = gs::uploadMipped(vdp, canArt("2"));
    art.buoy[2] = gs::uploadMipped(vdp, nunArt("3"));
    art.quay = gs::uploadMipped(vdp, quayArt(0));
    art.quayB = gs::uploadMipped(vdp, quayArt(1));
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.barge = gs::uploadMipped(vdp, bargeArt());
    art.skiff = gs::uploadMipped(vdp, skiffArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(true));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(false));
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.smoke = gs::uploadMipped(vdp, smokeArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.pin = gs::uploadMipped(vdp, pinArt());
    Bitmap dot(6, 6);
    dot.ellipse(3, 3, 2.3f, 2.3f, 1);
    art.dot = gs::uploadMipped(vdp, dot);
    art.panel = gs::uploadMipped(vdp, panelArt());
    art.title = words(vdp, "TUGBOAT BUOY", 3);
    art.sub = words(vdp, "ROUND THE BUOYS", 2);
    art.same = words(vdp, "SAME DOCK", 3);
    art.paused = words(vdp, "PAUSED", 3);

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
}

}  // namespace tug
