#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace skiffbox {
namespace {

constexpr float kTau = 6.2831853f;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i > 0 && i < 15) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    float wx = ly * c + lx * s;
    float wy = ly * s - lx * c;
    return {cx + wx, cy - wy};
}

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
    // White lapstrake skiff, blue cove, one hand on the tiller. Oars stay shipped.
    poly({{0.f, 36.f}, {9.f, 20.f}, {13.f, 4.f}, {12.f, -18.f}, {7.f, -33.f}, {-7.f, -33.f}, {-12.f, -18.f},
          {-13.f, 4.f}, {-9.f, 20.f}},
         1);
    poly({{0.f, 30.f}, {7.f, 16.f}, {8.f, 2.f}, {7.f, -16.f}, {4.f, -26.f}, {-4.f, -26.f}, {-7.f, -16.f}, {-8.f, 2.f},
          {-7.f, 16.f}},
         3);
    stroke(-10.f, 16.f, -9.f, -22.f, 5, 1.6f);
    stroke(10.f, 16.f, 9.f, -22.f, 5, 1.6f);
    stroke(-6.f, 18.f, -5.f, -20.f, 2, 1.3f);
    stroke(6.f, 18.f, 5.f, -20.f, 2, 1.3f);
    stroke(0.f, 32.f, 0.f, 22.f, 13, 1.2f);
    poly({{-5.f, 6.f}, {5.f, 6.f}, {4.f, -8.f}, {-4.f, -8.f}}, 4);
    blob(0.f, 1.f, 3.2f, 3.6f, 7);
    blob(0.f, 3.4f, 3.4f, 1.7f, 6);
    blob(-1.1f, 3.6f, 0.7f, 0.7f, 9);
    stroke(1.4f, -2.f, 2.6f, -14.f, 11, 1.4f);
    poly({{-3.2f, -24.f}, {3.2f, -24.f}, {2.6f, -33.f}, {-2.6f, -33.f}}, 8);
    blob(0.f, -31.f, 2.4f, 1.8f, 8);
    stroke(-11.f, 8.f, -4.f, 2.f, 11, 1.5f);
    stroke(11.f, 8.f, 4.f, 2.f, 11, 1.5f);
    b.outline(15, false);
    return b;
}

Bitmap paintHand(float ang) {
    Bitmap b(40, 40);
    float x0 = 20.f - std::sin(ang) * 5.f;
    float y0 = 20.f + std::cos(ang) * 5.f;
    float x1 = 20.f + std::sin(ang) * 15.f;
    float y1 = 20.f - std::cos(ang) * 15.f;
    b.line(x0, y0, x1, y1, 1, 3.4f);
    b.ellipse(20.f, 20.f, 2.1f, 2.1f, 2);
    return b;
}

Bitmap paintFace() {
    Bitmap b(48, 48);
    b.ellipse(24, 24, 20, 20, 2);
    b.ellipse(24, 24, 16, 16, 5);
    b.ellipse(24, 24, 14, 14, 4);
    for (int i = 0; i < 12; i++) {
        float a = i * kTau / 12.f;
        float r0 = (i % 3 == 0) ? 10.f : 12.f;
        b.line(24 + std::sin(a) * r0, 24 - std::cos(a) * r0, 24 + std::sin(a) * 14.5f, 24 - std::cos(a) * 14.5f, 5,
               (i % 3 == 0) ? 2.f : 1.2f);
    }
    b.poly({{24, 6}, {28, 12}, {20, 12}}, 6);
    b.ellipse(24, 24, 1.6f, 1.6f, 5);
    return b;
}

Bitmap paintTower() {
    Bitmap b(40, 56);
    b.rect(8, 18, 24, 32, 3);
    for (int y = 20; y < 48; y += 6) b.rect(8, y, 24, 2, 7);
    b.poly({{4, 20}, {20, 4}, {36, 20}}, 5);
    b.rect(18, 2, 4, 10, 2);
    b.rect(16, 34, 8, 16, 1);
    b.rect(12, 24, 5, 6, 4);
    b.rect(23, 24, 5, 6, 4);
    b.outline(7, false);
    return b;
}

Bitmap paintPost() {
    Bitmap b(16, 40);
    for (int i = 0; i < 6; i++) b.rect(5, 4 + i * 5, 6, 4, (i & 1) ? 1 : 2);
    b.poly({{8, 1}, {13, 8}, {3, 8}}, 1);
    b.ellipse(8, 34, 5, 2, 3);
    b.outline(2, false);
    return b;
}

Bitmap paintHdash() {
    Bitmap b(28, 8);
    b.rect(1, 2, 26, 4, 1);
    b.rect(1, 2, 26, 1, 2);
    return b;
}

Bitmap paintVdash() {
    Bitmap b(8, 28);
    b.rect(2, 1, 4, 26, 1);
    b.rect(2, 1, 1, 26, 2);
    return b;
}

Bitmap paintPier() {
    Bitmap b(78, 40);
    for (int i = 0; i < 5; i++) b.rect(4, 6 + i * 6, 70, 4, (i & 1) ? 1 : 2);
    b.rect(8, 4, 4, 32, 3);
    b.rect(36, 4, 4, 32, 3);
    b.rect(64, 4, 4, 32, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintShed() {
    Bitmap b(52, 36);
    b.rect(6, 14, 40, 18, 1);
    b.poly({{4, 16}, {26, 4}, {48, 16}}, 2);
    b.rect(22, 20, 10, 12, 5);
    b.rect(10, 18, 8, 6, 6);
    b.rect(34, 18, 8, 6, 6);
    b.outline(4, false);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(18, 26);
    b.rect(3, 4, 2, 20, 3);
    b.poly({{5, 4}, {16, 8}, {5, 13}}, 1);
    b.poly({{5, 8}, {12, 10}, {5, 12}}, 2);
    return b;
}

Bitmap paintTuft() {
    Bitmap b(18, 16);
    b.line(4, 14, 3, 3, 1, 1.4f);
    b.line(9, 14, 9, 2, 2, 1.6f);
    b.line(14, 14, 15, 4, 1, 1.4f);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(28, 14);
    b.ellipse(14, 8, 2.4f, 1.6f, 1);
    float tip = up ? 2.f : 11.f;
    b.line(14, 7, 2, tip, 1, 1.4f);
    b.line(14, 7, 26, tip, 1, 1.4f);
    b.line(14, 8, 6, (tip + 8.f) * 0.5f, 2, 1.1f);
    b.line(14, 8, 22, (tip + 8.f) * 0.5f, 2, 1.1f);
    b.set(16, 7, 3);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 6, 3, 1);
    b.ellipse(8, 6, 3, 1.6f, 2);
    return b;
}

Bitmap paintPin() {
    Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
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
    a[4 * 8 + 5] = 2;
    a[6 * 8 + 3] = 1;
    w[1 * 8 + 6] = 2;
    w[3 * 8 + 1] = 1;
    w[5 * 8 + 4] = 2;
    int t0 = tiles.alloc(1);
    int t1 = tiles.alloc(1);
    vdp.loadTile(t0, a);
    vdp.loadTile(t1, w);
    for (int cy = 0; cy < vdp.B.h; cy++) {
        for (int cx = 0; cx < vdp.B.w; cx++) {
            int tile = ((cx * 3 + cy * 5) & 3) == 0 ? t1 : t0;
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
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(7, 9, 10), gs::rgb4(14, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(3, 7, 14), gs::rgb4(11, 13, 14), gs::rgb4(12, 8, 4), gs::rgb4(1, 2, 6),
            gs::rgb4(13, 2, 2), gs::rgb4(14, 10, 7), gs::rgb4(2, 2, 3), gs::rgb4(4, 2, 2), gs::rgb4(15, 15, 15),
            gs::rgb4(8, 5, 2), gs::rgb4(14, 11, 4), gs::rgb4(6, 7, 8), 0, ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(15, 13, 2), gs::rgb4(2, 2, 2), gs::rgb4(8, 7, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CLOCK,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 10, 3), gs::rgb4(11, 4, 2), gs::rgb4(15, 15, 13), gs::rgb4(2, 2, 3),
            gs::rgb4(14, 3, 2), gs::rgb4(3, 2, 2), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(12, 9, 5), gs::rgb4(8, 6, 3), gs::rgb4(5, 4, 3), gs::rgb4(3, 2, 2), gs::rgb4(4, 3, 3),
            gs::rgb4(10, 12, 13), gs::rgb4(13, 3, 2), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 10, 12), gs::rgb4(14, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 7), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WAVE, {0, gs::rgb4(8, 14, 14), gs::rgb4(14, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TAG, {0, gs::rgb4(15, 14, 8), gs::rgb4(6, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    for (int i = 0; i < 16; i++) vdp.setColor(PAL_BOX * 16 + i, gs::rgb4(12, 11, 6));
    vdp.setColor(PAL_BOX * 16 + 0, 0);
    vdp.setColor(PAL_BOX * 16 + 4, gs::rgb4(13, 9, 3));
    vdp.setColor(PAL_BOX * 16 + 5, gs::rgb4(11, 8, 2));
    vdp.setColor(PAL_BOX * 16 + 6, gs::rgb4(14, 13, 8));
    vdp.setColor(PAL_BOX * 16 + 7, gs::rgb4(12, 11, 6));
    vdp.setColor(PAL_BOX * 16 + 14, gs::rgb4(15, 8, 2));
    vdp.setColor(PAL_BOX * 16 + 15, gs::rgb4(10, 9, 4));

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) {
        float a = i * kTau / 16.f;
        art.hull[i] = gs::uploadMipped(vdp, paintSkiff(a));
        art.hand[i] = gs::uploadMipped(vdp, paintHand(a));
    }
    art.face = gs::uploadMipped(vdp, paintFace());
    art.tower = gs::uploadMipped(vdp, paintTower());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.hdash = gs::uploadMipped(vdp, paintHdash());
    art.vdash = gs::uploadMipped(vdp, paintVdash());
    art.pier = gs::uploadMipped(vdp, paintPier());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.tuft = gs::uploadMipped(vdp, paintTuft());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(true));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(false));
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.title = words(vdp, "S3 SKIFF BOX", 2);
    art.inBox = words(vdp, "IN THE BOX", 3);
    art.ahead = words(vdp, "AHEAD OF THE CREW", 2);
    art.crewTook = words(vdp, "CREW HAS THE BOX", 2);
    art.paused = words(vdp, "PAUSED", 3);
    art.boxTag = words(vdp, "BOX", 2);
    art.crewTag = words(vdp, "CREW", 2);

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(1, 5, 8));
}

}  // namespace skiffbox
