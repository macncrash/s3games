#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace skiff {
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
    // Oars ship along the gunwales. The outboard, not the oars, drives the skiff.
    poly({{14.f, 4.f}, {20.f, -2.f}, {18.f, -10.f}, {12.f, -4.f}}, 10);
    poly({{-14.f, 4.f}, {-20.f, -2.f}, {-18.f, -10.f}, {-12.f, -4.f}}, 10);
    poly({{0.f, 34.f}, {10.f, 16.f}, {12.f, 0.f}, {11.f, -12.f}, {7.f, -20.f}, {-7.f, -20.f}, {-11.f, -12.f}, {-12.f, 0.f}, {-10.f, 16.f}}, 1);
    poly({{0.f, 28.f}, {7.f, 14.f}, {8.f, 0.f}, {6.f, -12.f}, {0.f, -16.f}, {-6.f, -12.f}, {-8.f, 0.f}, {-7.f, 14.f}}, 4);
    poly({{0.f, 22.f}, {4.f, 10.f}, {4.f, -6.f}, {0.f, -10.f}, {-4.f, -6.f}, {-4.f, 10.f}}, 5);
    stroke(-8.f, 12.f, -8.f, -12.f, 2, 1.4f);
    stroke(8.f, 12.f, 8.f, -12.f, 2, 1.4f);
    stroke(0.f, 30.f, 0.f, 16.f, 2, 1.2f);
    poly({{-3.5f, -18.f}, {3.5f, -18.f}, {3.f, -30.f}, {-3.f, -30.f}}, 6);
    blob(0.f, -32.f, 3.2f, 2.4f, 6);
    blob(-5.f, -16.f, 3.4f, 2.6f, 9);
    blob(0.f, -1.f, 4.2f, 5.4f, 7);
    blob(0.f, 5.f, 2.5f, 2.6f, 8);
    blob(0.f, 7.2f, 2.2f, 1.3f, 11);
    stroke(1.2f, -4.f, 0.4f, -16.f, 10, 1.3f);
    blob(0.f, 18.f, 2.2f, 1.6f, 10);
    b.outline(11, false);
    return b;
}

Bitmap paintCan() {
    Bitmap b(36, 52);
    b.ellipse(18, 38, 12, 6, 3);
    b.rect(8, 16, 20, 22, 1);
    b.rect(8, 22, 20, 4, 2);
    b.rect(8, 30, 20, 3, 2);
    b.ellipse(18, 16, 10, 5, 1);
    b.ellipse(18, 15, 6, 3, 6);
    b.rect(16, 8, 4, 8, 3);
    b.poly({{18, 2}, {28, 10}, {8, 10}}, 4);
    b.outline(3, false);
    gs::TextStyle st{2, 5, 0, 0, 1};
    Bitmap t = gs::textBitmap("1", st);
    b.blit(t, 18 - t.w / 2, 24);
    return b;
}

Bitmap paintNun() {
    Bitmap b(36, 52);
    b.ellipse(18, 40, 11, 5, 3);
    b.poly({{18, 8}, {30, 38}, {6, 38}}, 1);
    b.poly({{18, 14}, {26, 36}, {10, 36}}, 6);
    b.poly({{18, 20}, {24, 28}, {12, 28}}, 2);
    b.rect(16, 4, 4, 8, 3);
    b.outline(3, false);
    gs::TextStyle st{2, 5, 0, 0, 1};
    Bitmap t = gs::textBitmap("2", st);
    b.blit(t, 18 - t.w / 2, 28);
    return b;
}

Bitmap paintBall() {
    Bitmap b(40, 40);
    b.ellipse(20, 22, 14, 14, 1);
    b.ellipse(20, 22, 14, 5, 2);
    b.ellipse(16, 16, 5, 3, 6);
    b.rect(18, 4, 4, 6, 3);
    b.outline(3, false);
    gs::TextStyle st{2, 5, 0, 0, 1};
    Bitmap t = gs::textBitmap("3", st);
    b.blit(t, 20 - t.w / 2, 18);
    return b;
}

Bitmap paintPile() {
    Bitmap b(14, 24);
    b.rect(5, 6, 4, 14, 1);
    b.ellipse(7, 6, 5, 3, 2);
    b.rect(6, 10, 2, 8, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintDay() {
    Bitmap b(28, 40);
    b.rect(12, 16, 4, 20, 3);
    b.poly({{14, 2}, {26, 16}, {14, 30}, {2, 16}}, 1);
    b.poly({{14, 7}, {21, 16}, {14, 25}, {7, 16}}, 2);
    b.outline(4, false);
    return b;
}

Bitmap paintFinger() {
    Bitmap b(18, 84);
    for (int i = 0; i < 10; i++) b.rect(2, 4 + i * 8, 14, 6, (i & 1) ? 1 : 2);
    b.rect(1, 2, 3, 78, 3);
    b.rect(14, 2, 3, 78, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintShed() {
    Bitmap b(72, 46);
    b.rect(8, 18, 56, 22, 1);
    b.poly({{4, 20}, {36, 4}, {68, 20}}, 2);
    b.rect(30, 24, 12, 16, 5);
    b.rect(14, 24, 10, 8, 6);
    b.rect(48, 24, 10, 8, 6);
    b.rect(34, 6, 3, 14, 3);
    b.poly({{44, 8}, {52, 12}, {44, 16}}, 7);
    b.outline(4, false);
    return b;
}

Bitmap paintRock() {
    Bitmap b(44, 30);
    b.ellipse(22, 16, 18, 11, 1);
    b.ellipse(18, 14, 8, 5, 2);
    b.ellipse(28, 18, 5, 3, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(32, 16);
    b.ellipse(16, 9, 3, 2, 1);
    float tip = up ? 2.f : 12.f;
    b.line(16, 8, 2, tip, 1, 1.5f);
    b.line(16, 8, 30, tip, 1, 1.5f);
    b.line(16, 9, 6, (tip + 9.f) * 0.5f, 2, 1.1f);
    b.line(16, 9, 26, (tip + 9.f) * 0.5f, 2, 1.1f);
    b.set(19, 8, 3);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 3, 1);
    b.ellipse(8, 8, 3, 2, 2);
    return b;
}

Bitmap paintRing() {
    Bitmap b(72, 72);
    const float c = 35.5f, r = 32.f;
    for (int d = 0; d < 360; d += 10) {
        float a0 = d * 3.14159265f / 180.f;
        float a1 = (d + 5.f) * 3.14159265f / 180.f;
        b.line(c + std::cos(a0) * r, c + std::sin(a0) * r, c + std::cos(a1) * r, c + std::sin(a1) * r, 1, 2.f);
    }
    return b;
}

Bitmap paintPin() {
    Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
    return b;
}

Bitmap paintDash() {
    Bitmap b(64, 10);
    for (int i = 0; i < 8; i++) b.rect(2 + i * 8, 2, 5, 6, (i & 1) ? 1 : 2);
    return b;
}

Bitmap paintPanel() {
    Bitmap b(86, 104);
    b.rect(0, 0, 86, 104, 1);
    for (int x = 0; x < 86; x++) {
        b.set(x, 0, 2);
        b.set(x, 103, 2);
    }
    for (int y = 0; y < 104; y++) {
        b.set(0, y, 2);
        b.set(85, y, 2);
    }
    return b;
}

Bitmap paintFlag() {
    Bitmap b(16, 20);
    b.rect(2, 2, 2, 16, 3);
    b.poly({{4, 3}, {14, 6}, {4, 10}}, 1);
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
    a[4 * 8 + 4] = 1;
    a[6 * 8 + 6] = 2;
    w[1 * 8 + 5] = 1;
    w[3 * 8 + 2] = 2;
    w[5 * 8 + 6] = 1;
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
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 9, 10), gs::rgb4(14, 12, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_HULL, {0, gs::rgb4(14, 13, 11), gs::rgb4(15, 15, 14), gs::rgb4(5, 8, 9), gs::rgb4(11, 8, 4),
                           gs::rgb4(4, 5, 6), gs::rgb4(2, 2, 3), gs::rgb4(14, 6, 2), gs::rgb4(13, 9, 6),
                           gs::rgb4(12, 2, 2), gs::rgb4(8, 5, 2), gs::rgb4(1, 2, 3), 0, 0, 0, ink});
    setPal(vdp, PAL_CAN, {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(6, 1, 1), gs::rgb4(15, 8, 2),
                          gs::rgb4(1, 1, 2), gs::rgb4(15, 12, 10), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_NUN, {0, gs::rgb4(2, 11, 4), gs::rgb4(15, 15, 14), gs::rgb4(1, 5, 2), gs::rgb4(8, 14, 6),
                          gs::rgb4(1, 1, 2), gs::rgb4(10, 14, 8), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(14, 12, 2), gs::rgb4(15, 15, 14), gs::rgb4(8, 6, 1), gs::rgb4(15, 10, 2),
                           gs::rgb4(1, 1, 2), gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 2), gs::rgb4(3, 2, 2),
                           gs::rgb4(2, 2, 3), gs::rgb4(6, 8, 9), gs::rgb4(10, 12, 13), gs::rgb4(13, 3, 2), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(9, 10, 11), gs::rgb4(14, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_END, {0, gs::rgb4(15, 8, 2), gs::rgb4(15, 15, 14), gs::rgb4(6, 4, 2), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(1, 3, 6), gs::rgb4(8, 12, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WAVE, {0, gs::rgb4(8, 13, 14), gs::rgb4(13, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 8), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, gs::rgb4(9, 8, 5));
    vdp.setColor(PAL_ROAD * 16 + 0, 0);
    vdp.setColor(PAL_ROAD * 16 + 1, gs::rgb4(12, 10, 6));
    vdp.setColor(PAL_ROAD * 16 + 2, gs::rgb4(9, 8, 5));
    vdp.setColor(PAL_ROAD * 16 + 3, gs::rgb4(7, 8, 4));
    vdp.setColor(PAL_ROAD * 16 + 4, gs::rgb4(11, 8, 4));
    vdp.setColor(PAL_ROAD * 16 + 5, gs::rgb4(7, 5, 3));
    vdp.setColor(PAL_ROAD * 16 + 8, gs::rgb4(8, 7, 5));
    vdp.setColor(PAL_ROAD * 16 + 11, gs::rgb4(3, 11, 12));
    vdp.setColor(PAL_ROAD * 16 + 12, gs::rgb4(2, 8, 10));
    vdp.setColor(PAL_ROAD * 16 + 13, gs::rgb4(8, 14, 14));
    vdp.setColor(PAL_ROAD * 16 + 14, gs::rgb4(14, 12, 8));

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.hull[i] = gs::uploadMipped(vdp, paintSkiff(i * kTau / 16.f));
    art.can = gs::uploadMipped(vdp, paintCan());
    art.nun = gs::uploadMipped(vdp, paintNun());
    art.ball = gs::uploadMipped(vdp, paintBall());
    art.pile = gs::uploadMipped(vdp, paintPile());
    art.day = gs::uploadMipped(vdp, paintDay());
    art.finger = gs::uploadMipped(vdp, paintFinger());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.rock = gs::uploadMipped(vdp, paintRock());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(true));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(false));
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.ring = gs::uploadMipped(vdp, paintRing());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.dash = gs::uploadMipped(vdp, paintDash());
    art.panel = gs::uploadMipped(vdp, paintPanel());
    Bitmap dot(8, 8);
    dot.ellipse(4, 4, 3.f, 3.f, 1);
    art.dot = gs::uploadMipped(vdp, dot);
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.title = words(vdp, "S3 SKIFF BUOY", 2);
    art.missed = words(vdp, "MISSED THE END", 3);
    art.legFail = words(vdp, "THE LEG FAILS", 2);
    art.sameDock = words(vdp, "SAME DOCK", 3);
    art.endHeld = words(vdp, "THE END HELD", 2);
    art.paused = words(vdp, "PAUSED", 3);

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(1, 5, 8));
}

}  // namespace skiff
