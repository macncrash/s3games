#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace van {
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

// Boxy postal launch: cream body, red band, mail sack on the cabin.
Bitmap paintVan(float heading) {
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
    poly({{-22.f, 30.f}, {22.f, 30.f}, {24.f, -8.f}, {16.f, -36.f}, {-16.f, -36.f}, {-24.f, -8.f}}, 2);
    poly({{-16.f, 22.f}, {16.f, 22.f}, {16.f, -18.f}, {-16.f, -18.f}}, 3);
    poly({{-16.f, 6.f}, {16.f, 6.f}, {16.f, 1.f}, {-16.f, 1.f}}, 8);
    poly({{-12.f, 18.f}, {12.f, 18.f}, {11.f, 8.f}, {-11.f, 8.f}}, 4);
    poly({{-8.f, 16.f}, {-1.f, 16.f}, {-1.f, 10.f}, {-8.f, 10.f}}, 5);
    poly({{1.f, 16.f}, {8.f, 16.f}, {8.f, 10.f}, {1.f, 10.f}}, 5);
    blob(0.f, 24.f, 5.f, 4.f, 9);
    stroke(0.f, 24.f, 0.f, 32.f, 10, 2.2f);
    blob(-10.f, -6.f, 3.2f, 2.2f, 6);
    blob(10.f, -6.f, 3.2f, 2.2f, 6);
    blob(0.f, -28.f, 2.4f, 2.4f, 7);
    stroke(-18.f, 4.f, -18.f, -12.f, 11, 2.f);
    stroke(18.f, 4.f, 18.f, -12.f, 11, 2.f);
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
    for (int x = 4; x < 96; x += 12) b.rect(x, 18, 3, 16, 3);
    b.rect(0, 36, 96, 4, 4);
    return b;
}

Bitmap shedArt() {
    Bitmap b(48, 40);
    b.poly({{2, 22}, {24, 6}, {46, 22}}, 6);
    b.rect(6, 20, 36, 16, 1);
    b.rect(18, 26, 10, 10, 3);
    b.rect(10, 24, 6, 5, 2);
    b.rect(32, 24, 6, 5, 2);
    return b;
}

Bitmap pileArt() {
    Bitmap b(16, 28);
    b.rect(6, 2, 4, 22, 1);
    b.ellipse(8, 24, 6, 3, 2);
    return b;
}

Bitmap flagArt() {
    Bitmap b(28, 36);
    b.rect(4, 4, 2, 28, 2);
    b.poly({{6, 4}, {24, 10}, {6, 16}}, 1);
    return b;
}

Bitmap reedArt() {
    Bitmap b(20, 32);
    b.line(4, 28, 6, 6, 1, 1.6f);
    b.line(10, 28, 8, 4, 2, 1.6f);
    b.line(15, 28, 13, 8, 3, 1.6f);
    return b;
}

Bitmap sackArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 16, 10, 9, 1);
    b.rect(10, 6, 8, 6, 2);
    b.line(8, 12, 20, 20, 3, 1.4f);
    return b;
}

Bitmap birdArt(int flap) {
    Bitmap b(24, 16);
    b.ellipse(12, 9, 3, 2, 1);
    float y = flap ? 4.f : 8.f;
    b.line(12, 9, 2, y, 2, 1.4f);
    b.line(12, 9, 22, y, 2, 1.4f);
    return b;
}

Bitmap wakeArt() {
    Bitmap b(20, 12);
    b.ellipse(10, 6, 8, 3, 1);
    b.ellipse(10, 6, 4, 1.5f, 2);
    return b;
}

Bitmap ringArt() {
    Bitmap b(24, 12);
    b.ellipse(12, 6, 10, 4, 1);
    b.ellipse(12, 6, 5, 2.f, 2);
    return b;
}

Bitmap lampArt() {
    Bitmap b(12, 24);
    b.rect(5, 8, 2, 14, 2);
    b.ellipse(6, 6, 4, 4, 1);
    return b;
}

Bitmap pinArt() {
    Bitmap b(12, 16);
    b.poly({{6, 2}, {11, 10}, {1, 10}}, 1);
    b.rect(5, 10, 2, 4, 2);
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
    setPal(vdp, PAL_VAN,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(12, 11, 8), gs::rgb4(15, 14, 11), gs::rgb4(6, 8, 10), gs::rgb4(10, 13, 15),
            gs::rgb4(3, 3, 4), gs::rgb4(15, 13, 4), gs::rgb4(13, 2, 2), gs::rgb4(8, 5, 2), gs::rgb4(4, 3, 2),
            gs::rgb4(7, 7, 8), gs::rgb4(15, 15, 14), gs::rgb4(9, 2, 2), gs::rgb4(5, 4, 3), ink});
    setPal(vdp, PAL_NUN, {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(4, 1, 1), gs::rgb4(15, 12, 3),
                          gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CAN, {0, gs::rgb4(2, 11, 4), gs::rgb4(15, 15, 15), gs::rgb4(1, 4, 2), gs::rgb4(14, 12, 3),
                          gs::rgb4(1, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(9, 6, 3), gs::rgb4(13, 10, 6), gs::rgb4(5, 3, 2), gs::rgb4(12, 11, 8),
                           gs::rgb4(3, 2, 1), gs::rgb4(12, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WAKE, {0, gs::rgb4(12, 14, 13), gs::rgb4(8, 12, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(2, 2, 3), gs::rgb4(12, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_REED, {0, gs::rgb4(2, 7, 3), gs::rgb4(4, 10, 4), gs::rgb4(8, 12, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SACK, {0, gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_OTHER, {0, gs::rgb4(7, 5, 4), gs::rgb4(11, 8, 6), gs::rgb4(4, 3, 2), gs::rgb4(13, 12, 10),
                            gs::rgb4(8, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 9), gs::rgb4(3, 4, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 7), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(6, 10, 9), gs::rgb4(4, 8, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(12, 10, 6), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    loadFont(vdp, art);
    for (int i = 0; i < 8; i++) art.hull[i] = gs::uploadMipped(vdp, paintVan(i * kTau / 8.f));
    art.nun = gs::uploadMipped(vdp, nunArt("1"));
    art.nun3 = gs::uploadMipped(vdp, nunArt("3"));
    art.can = gs::uploadMipped(vdp, canArt("2"));
    art.quay = gs::uploadMipped(vdp, quayArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(0));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(1));
    art.wake = gs::uploadMipped(vdp, wakeArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.pin = gs::uploadMipped(vdp, pinArt());
    art.title = words(vdp, "MAIL VAN", 3);
    art.round = words(vdp, "ROUND THE BUOYS", 2);
    art.same = words(vdp, "SAME DOCK", 3);
    art.made = words(vdp, "LEG MADE", 2);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.wrong = words(vdp, "WRONG DOCK", 2);
    art.paused = words(vdp, "PAUSED", 3);
}

}  // namespace van
