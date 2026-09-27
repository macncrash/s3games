#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace barge {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 2, 2));
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    return {cx + ly * c + lx * s, cy - (ly * s - lx * c)};
}

Bitmap paintHull(float heading) {
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

    poly({{-18.f, 28.f}, {18.f, 28.f}, {20.f, -22.f}, {10.f, -40.f}, {-10.f, -40.f}, {-20.f, -22.f}}, 2);
    poly({{-14.f, 22.f}, {14.f, 22.f}, {15.f, -16.f}, {8.f, -32.f}, {-8.f, -32.f}, {-15.f, -16.f}}, 3);
    poly({{-12.f, 8.f}, {12.f, 8.f}, {12.f, -6.f}, {-12.f, -6.f}}, 4);
    poly({{-10.f, 4.f}, {10.f, 4.f}, {10.f, -2.f}, {-10.f, -2.f}}, 5);
    blob(-6.f, 1.f, 2.2f, 2.2f, 6);
    blob(6.f, 1.f, 2.2f, 2.2f, 6);
    poly({{-9.f, 20.f}, {9.f, 20.f}, {8.f, 10.f}, {-8.f, 10.f}}, 7);
    stroke(0.f, 20.f, 0.f, 34.f, 8, 2.4f);
    stroke(0.f, 32.f, 8.f, 28.f, 9, 2.2f);
    blob(0.f, -28.f, 3.2f, 2.4f, 10);
    stroke(-16.f, 6.f, -16.f, -10.f, 11, 2.f);
    stroke(16.f, 6.f, 16.f, -10.f, 11, 2.f);
    stroke(-6.f, -34.f, 6.f, -34.f, 12, 2.f);
    b.outline(1, false);
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

Bitmap quayArt() {
    Bitmap b(96, 48);
    b.rect(0, 10, 96, 28, 1);
    b.rect(0, 10, 96, 6, 2);
    for (int i = 0; i < 6; i++) b.rect(6 + i * 16, 18, 4, 18, 3);
    b.rect(0, 36, 96, 4, 4);
    b.outline(5, false);
    return b;
}

Bitmap shedArt() {
    Bitmap b(48, 40);
    b.poly({{4, 22}, {24, 6}, {44, 22}}, 2);
    b.rect(8, 22, 32, 14, 1);
    b.rect(20, 26, 8, 10, 3);
    b.rect(12, 26, 5, 5, 4);
    b.outline(5, false);
    return b;
}

Bitmap pileArt() {
    Bitmap b(16, 36);
    b.rect(6, 4, 4, 28, 1);
    b.ellipse(8, 6, 3, 3, 2);
    b.outline(3, false);
    return b;
}

Bitmap flagArt() {
    Bitmap b(28, 36);
    b.rect(4, 2, 2, 32, 1);
    b.poly({{6, 4}, {24, 10}, {6, 16}}, 2);
    return b;
}

Bitmap reedArt() {
    Bitmap b(24, 32);
    b.line(6, 30, 4, 6, 1, 1.6f);
    b.line(12, 30, 14, 4, 2, 1.6f);
    b.line(18, 30, 16, 10, 1, 1.4f);
    b.ellipse(4, 6, 2.2f, 3.f, 3);
    b.ellipse(14, 4, 2.2f, 3.f, 3);
    return b;
}

Bitmap crateArt() {
    Bitmap b(28, 24);
    b.rect(2, 4, 24, 16, 1);
    b.line(2, 4, 26, 20, 2, 1.4f);
    b.line(26, 4, 2, 20, 2, 1.4f);
    b.outline(3, false);
    return b;
}

Bitmap birdArt(int flap) {
    Bitmap b(28, 16);
    if (flap) {
        b.line(2, 8, 14, 4, 1, 1.6f);
        b.line(14, 4, 26, 8, 1, 1.6f);
    } else {
        b.line(2, 4, 14, 8, 1, 1.6f);
        b.line(14, 8, 26, 4, 1, 1.6f);
    }
    b.ellipse(14, 8, 2.f, 2.f, 2);
    return b;
}

Bitmap wakeArt() {
    Bitmap b(16, 10);
    b.ellipse(8, 5, 7, 3, 1);
    b.ellipse(8, 5, 3, 1.4f, 2);
    return b;
}

Bitmap ringArt() {
    Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 8, 1);
    b.ellipse(10, 10, 5, 5, 0);
    return b;
}

Bitmap lampArt() {
    Bitmap b(12, 20);
    b.rect(5, 8, 2, 10, 2);
    b.ellipse(6, 6, 4, 4, 1);
    return b;
}

Bitmap pinArt() {
    Bitmap b(10, 14);
    b.poly({{5, 1}, {9, 8}, {5, 13}, {1, 8}}, 1);
    return b;
}

Bitmap panelArt() {
    Bitmap b(72, 16);
    b.rect(0, 0, 72, 16, 1);
    b.rect(2, 2, 68, 12, 2);
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
        for (int cx = 0; cx < vdp.B.w; cx++) vdp.B.set(cx, cy, gs::entry(((cx + cy) & 1) ? t1 : t0, PAL_WATER));
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 2, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 10, 9), gs::rgb4(14, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(3, 2, 2), gs::rgb4(7, 4, 2), gs::rgb4(10, 6, 3), gs::rgb4(5, 4, 3), gs::rgb4(12, 9, 5),
            gs::rgb4(14, 12, 8), gs::rgb4(6, 3, 2), gs::rgb4(4, 3, 2), gs::rgb4(9, 7, 3), gs::rgb4(13, 11, 4),
            gs::rgb4(2, 2, 2), gs::rgb4(15, 14, 10), gs::rgb4(8, 2, 2), gs::rgb4(11, 8, 4), ink});
    setPal(vdp, PAL_NUN, {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(4, 1, 1), gs::rgb4(15, 12, 3),
                          gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CAN, {0, gs::rgb4(2, 11, 4), gs::rgb4(15, 15, 15), gs::rgb4(1, 4, 2), gs::rgb4(14, 12, 3),
                          gs::rgb4(1, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(9, 6, 3), gs::rgb4(13, 10, 6), gs::rgb4(5, 3, 2), gs::rgb4(12, 11, 8),
                           gs::rgb4(3, 2, 1), gs::rgb4(8, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WAKE, {0, gs::rgb4(12, 14, 13), gs::rgb4(8, 12, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(2, 2, 3), gs::rgb4(12, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_REED, {0, gs::rgb4(2, 7, 3), gs::rgb4(4, 10, 4), gs::rgb4(8, 12, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CRATE, {0, gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_OTHER, {0, gs::rgb4(7, 5, 4), gs::rgb4(11, 8, 6), gs::rgb4(4, 3, 2), gs::rgb4(13, 12, 10),
                            gs::rgb4(8, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 9), gs::rgb4(3, 4, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 7), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(6, 10, 9), gs::rgb4(4, 8, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(12, 10, 6), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    loadFont(vdp, art);
    for (int i = 0; i < 8; i++) art.hull[i] = gs::uploadMipped(vdp, paintHull(i * kTau / 8.f));
    art.nun = gs::uploadMipped(vdp, nunArt("1"));
    art.nun3 = gs::uploadMipped(vdp, nunArt("3"));
    art.can = gs::uploadMipped(vdp, canArt("2"));
    art.quay = gs::uploadMipped(vdp, quayArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(0));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(1));
    art.wake = gs::uploadMipped(vdp, wakeArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.pin = gs::uploadMipped(vdp, pinArt());
    art.panel = gs::uploadMipped(vdp, panelArt());
    art.title = words(vdp, "BARGE BUOY", 3);
    art.round = words(vdp, "ROUND TO PORT", 2);
    art.same = words(vdp, "SAME DOCK", 3);
    art.made = words(vdp, "LEG MADE", 2);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.wrong = words(vdp, "WRONG DOCK", 2);
    art.paused = words(vdp, "PAUSED", 3);
}

}  // namespace barge
