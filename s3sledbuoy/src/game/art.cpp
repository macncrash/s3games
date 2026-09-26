#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace sled {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(2, 2, 3));
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    return {cx + ly * c + lx * s, cy - (ly * s - lx * c)};
}

Bitmap paintSled(float heading) {
    Bitmap b(128, 128);
    const float cx = 64.f, cy = 64.f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        w.reserve(local.size());
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, c, s));
        b.poly(w, col);
    };
    auto blob = [&](float lx, float ly, float rx, float ry, int col) {
        Pt p = spin(cx, cy, lx, ly, c, s);
        b.ellipse(p.first, p.second, rx, ry, col);
    };
    auto stroke = [&](float ax, float ay, float bx, float by, int col, float th) {
        Pt A = spin(cx, cy, ax, ay, c, s);
        Pt B = spin(cx, cy, bx, by, c, s);
        b.line(A.first, A.second, B.first, B.second, col, th);
    };
    auto dog = [&](float x, int body, int dark) {
        stroke(x - 3.f, 26.f, x - 6.f, 20.f, dark, 1.7f);
        stroke(x + 3.f, 26.f, x + 6.f, 20.f, dark, 1.7f);
        stroke(x + 2.f, 26.f, x + 7.f, 23.f, dark, 1.5f);
        blob(x, 34.f, 6.2f, 10.f, body);
        blob(x, 45.f, 4.4f, 4.6f, body);
        blob(x - 2.4f, 49.f, 1.6f, 2.2f, dark);
        blob(x + 2.4f, 49.f, 1.6f, 2.2f, dark);
        blob(x, 46.2f, 1.1f, 1.1f, 13);
        blob(x + 1.6f, 47.f, 0.6f, 0.6f, 10);
        stroke(x - 1.f, 25.f, x - 6.f, 22.f, dark, 1.6f);
    };

    stroke(-16.f, 22.f, -16.f, -30.f, 1, 3.4f);
    stroke(16.f, 22.f, 16.f, -30.f, 1, 3.4f);
    stroke(-16.f, 22.f, -7.f, 32.f, 1, 3.2f);
    stroke(16.f, 22.f, 7.f, 32.f, 1, 3.2f);
    stroke(-16.f, -24.f, 16.f, -24.f, 1, 3.1f);
    stroke(-16.f, -24.f, -16.f, -10.f, 14, 2.2f);
    stroke(16.f, -24.f, 16.f, -10.f, 14, 2.2f);
    stroke(-14.f, 6.f, 14.f, -4.f, 14, 1.5f);
    stroke(-14.f, -4.f, 14.f, 6.f, 14, 1.5f);

    poly({{-14.f, 16.f}, {14.f, 16.f}, {13.f, -8.f}, {-13.f, -8.f}}, 2);
    poly({{-11.f, 13.f}, {11.f, 13.f}, {10.f, -4.f}, {-10.f, -4.f}}, 3);
    blob(0.f, 3.f, 9.f, 6.f, 4);
    blob(0.f, 7.f, 7.f, 2.6f, 12);
    stroke(-8.f, 8.f, 8.f, -1.f, 9, 1.4f);
    stroke(-8.f, -1.f, 8.f, 8.f, 9, 1.4f);

    blob(0.f, -8.f, 6.2f, 8.f, 5);
    blob(0.f, -2.f, 5.f, 4.2f, 6);
    blob(0.f, 2.5f, 3.1f, 3.2f, 7);
    blob(-1.2f, 3.4f, 0.6f, 0.6f, 10);
    blob(1.2f, 3.4f, 0.6f, 0.6f, 10);
    stroke(-7.f, -12.f, -15.f, -22.f, 5, 2.3f);
    stroke(7.f, -12.f, 15.f, -22.f, 5, 2.3f);

    stroke(0.f, 15.f, -9.f, 24.f, 9, 1.6f);
    stroke(0.f, 15.f, 9.f, 24.f, 9, 1.6f);
    dog(-9.f, 7, 8);
    dog(9.f, 8, 7);
    b.outline(10, false);
    return b;
}

Bitmap nunArt(const char* num) {
    Bitmap b(44, 64);
    b.ellipse(22, 54, 14, 5, 2);
    b.poly({{22, 8}, {36, 48}, {8, 48}}, 1);
    b.rect(13, 28, 18, 16, 2);
    b.ellipse(22, 12, 3.2f, 3.2f, 4);
    b.outline(3, false);
    gs::TextStyle st{2, 5, 0, 0, 0};
    Bitmap t = gs::textBitmap(num, st);
    b.blit(t, 22 - t.w / 2, 30);
    return b;
}

Bitmap canArt(const char* num) {
    Bitmap b(44, 64);
    b.ellipse(22, 54, 14, 5, 2);
    b.rect(10, 20, 24, 30, 1);
    b.ellipse(22, 20, 12, 5, 1);
    b.rect(12, 28, 20, 16, 2);
    b.ellipse(22, 16, 8, 3, 2);
    b.rect(20, 6, 4, 12, 3);
    b.ellipse(22, 6, 2.6f, 2.6f, 4);
    b.outline(3, false);
    gs::TextStyle st{2, 5, 0, 0, 0};
    Bitmap t = gs::textBitmap(num, st);
    b.blit(t, 22 - t.w / 2, 30);
    return b;
}

Bitmap quayArt(int variant) {
    Bitmap b(40, 34);
    for (int i = 0; i < 4; i++) b.rect(1, 14 + i * 4, 38, 3, (i & 1) ? 1 : 5);
    b.rect(0, 8, 40, 7, 2);
    int bx = (variant & 1) ? 6 : 26;
    b.rect(float(bx), 18, 6, 5, 3);
    return b;
}

Bitmap shedArt() {
    Bitmap b(58, 48);
    b.rect(8, 22, 42, 20, 1);
    b.poly({{4, 24}, {29, 8}, {54, 24}}, 6);
    b.ellipse(29, 16, 18, 4, 2);
    b.rect(24, 28, 10, 14, 3);
    b.rect(12, 26, 8, 7, 4);
    b.rect(38, 26, 8, 7, 4);
    b.rect(28, 4, 2, 10, 3);
    b.rect(30, 4, 9, 5, 7);
    b.outline(3, false);
    return b;
}

Bitmap pileArt() {
    Bitmap b(16, 28);
    b.rect(6, 10, 4, 14, 1);
    b.ellipse(8, 10, 6, 3, 2);
    b.outline(3, false);
    return b;
}

Bitmap flagArt() {
    Bitmap b(20, 28);
    b.rect(3, 6, 2, 20, 3);
    b.rect(5, 6, 11, 7, 7);
    b.rect(5, 6, 11, 2, 2);
    return b;
}

Bitmap treeArt() {
    Bitmap b(40, 56);
    b.rect(17, 36, 6, 16, 4);
    b.poly({{20, 4}, {36, 26}, {4, 26}}, 1);
    b.poly({{20, 16}, {34, 40}, {6, 40}}, 2);
    b.ellipse(20, 16, 8, 3, 3);
    b.ellipse(20, 30, 10, 3, 3);
    b.outline(1, false);
    return b;
}

Bitmap driftArt() {
    Bitmap b(48, 24);
    b.ellipse(24, 14, 22, 8, 1);
    b.ellipse(16, 12, 9, 4, 2);
    b.ellipse(32, 16, 4, 3, 3);
    return b;
}

Bitmap birdArt(bool up) {
    Bitmap b(26, 14);
    b.ellipse(13, 8, 3.2f, 2.f, 1);
    float tip = up ? 2.f : 12.f;
    b.line(13, 7, 2, tip, 2, 1.6f);
    b.line(13, 7, 24, tip, 2, 1.6f);
    b.set(16, 7, 3);
    return b;
}

Bitmap sprayArt() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 7, 3, 1);
    b.ellipse(5, 6, 3, 1.5f, 2);
    return b;
}

Bitmap ringArt() {
    Bitmap b(64, 64);
    const float c = 31.5f, r = 28.f;
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
    Bitmap b(72, 78);
    b.rect(0, 0, 72, 78, 1);
    for (int x = 0; x < 72; x++) {
        b.set(x, 0, 2);
        b.set(x, 77, 2);
    }
    for (int y = 0; y < 78; y++) {
        b.set(0, y, 2);
        b.set(71, y, 2);
    }
    return b;
}

Bitmap crackArt() {
    Bitmap b(10, 40);
    b.line(5, 2, 2, 18, 3, 1.5f);
    b.line(2, 18, 7, 38, 3, 1.5f);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
    uint8_t a[64] = {};
    uint8_t w[64] = {};
    a[2 * 8 + 2] = 1;
    a[4 * 8 + 5] = 2;
    a[6 * 8 + 1] = 1;
    w[1 * 8 + 6] = 2;
    w[3 * 8 + 3] = 1;
    w[5 * 8 + 4] = 2;
    int t0 = tiles.alloc(1);
    int t1 = tiles.alloc(1);
    vdp.loadTile(t0, a);
    vdp.loadTile(t1, w);
    for (int cy = 0; cy < vdp.B.h; cy++)
        for (int cx = 0; cx < vdp.B.w; cx++) vdp.B.set(cx, cy, gs::entry(((cx + cy) & 1) ? t1 : t0, PAL_SNOW));
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(2, 2, 3);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(9, 11, 13), gs::rgb4(15, 13, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SLED,
           {0, gs::rgb4(4, 5, 7), gs::rgb4(10, 7, 4), gs::rgb4(13, 10, 6), gs::rgb4(13, 2, 2), gs::rgb4(4, 7, 11),
            gs::rgb4(9, 13, 15), gs::rgb4(12, 9, 5), gs::rgb4(6, 4, 3), gs::rgb4(8, 5, 2), gs::rgb4(2, 2, 2),
            gs::rgb4(13, 7, 5), gs::rgb4(15, 15, 15), gs::rgb4(3, 1, 1), gs::rgb4(12, 13, 14), shadow});
    setPal(vdp, PAL_NUN, {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(4, 1, 1), gs::rgb4(15, 12, 3),
                          gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CAN, {0, gs::rgb4(2, 11, 4), gs::rgb4(15, 15, 15), gs::rgb4(1, 4, 2), gs::rgb4(14, 12, 3),
                          gs::rgb4(1, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(11, 8, 5), gs::rgb4(14, 15, 15), gs::rgb4(5, 3, 2), gs::rgb4(12, 9, 4),
                           gs::rgb4(7, 5, 3), gs::rgb4(9, 3, 3), gs::rgb4(3, 11, 5), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 7), gs::rgb4(12, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 6, 3), gs::rgb4(4, 9, 5), gs::rgb4(14, 15, 15), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DRIFT, {0, gs::rgb4(14, 15, 15), gs::rgb4(9, 11, 13), gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_OTHER, {0, gs::rgb4(8, 5, 3), gs::rgb4(13, 14, 15), gs::rgb4(4, 2, 2), gs::rgb4(12, 9, 4),
                            gs::rgb4(6, 4, 3), gs::rgb4(10, 4, 2), gs::rgb4(13, 8, 2), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 10), gs::rgb4(3, 4, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 7), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(13, 15, 15), gs::rgb4(8, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(1, 3, 6), gs::rgb4(9, 13, 15), gs::rgb4(3, 5, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.sled[i] = gs::uploadMipped(vdp, paintSled(i * kTau / 16.f));
    art.nun = gs::uploadMipped(vdp, nunArt("1"));
    art.nun3 = gs::uploadMipped(vdp, nunArt("3"));
    art.can = gs::uploadMipped(vdp, canArt("2"));
    art.quay = gs::uploadMipped(vdp, quayArt(0));
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.drift = gs::uploadMipped(vdp, driftArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(true));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(false));
    art.spray = gs::uploadMipped(vdp, sprayArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.pin = gs::uploadMipped(vdp, pinArt());
    Bitmap dot(6, 6);
    dot.ellipse(3, 3, 2.2f, 2.2f, 1);
    art.dot = gs::uploadMipped(vdp, dot);
    art.panel = gs::uploadMipped(vdp, panelArt());
    art.crack = gs::uploadMipped(vdp, crackArt());
    art.title = words(vdp, "SLED BUOY", 3);
    art.round = words(vdp, "ROUND THE BUOYS", 2);
    art.same = words(vdp, "SAME DOCK", 3);
    art.made = words(vdp, "THE LEG IS MADE", 2);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.wrong = words(vdp, "WRONG DOCK", 2);
    art.paused = words(vdp, "PAUSED", 3);

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
}

}  // namespace sled
