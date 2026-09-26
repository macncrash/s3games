#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace sledbox {
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
    Bitmap b(112, 112);
    const float cx = 55.5f, cy = 55.5f;
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
    auto dog = [&](float y, float sc) {
        float body = 6.2f * sc;
        stroke(-2.4f * sc, y - 1.f, -4.2f * sc, y - body * 0.55f, 8, 1.7f);
        stroke(2.4f * sc, y - 1.f, 4.2f * sc, y - body * 0.55f, 8, 1.7f);
        stroke(-1.5f * sc, y + 1.5f, -3.6f * sc, y - body * 0.2f, 8, 1.6f);
        stroke(1.5f * sc, y + 1.5f, 3.6f * sc, y - body * 0.2f, 8, 1.6f);
        blob(0.f, y, 3.5f * sc, body, 7);
        blob(0.f, y + body * 0.15f, 2.2f * sc, 2.4f * sc, 12);
        blob(0.f, y + body * 0.62f, 2.7f * sc, 3.1f * sc, 7);
        blob(-1.4f * sc, y + body * 0.85f, 1.15f * sc, 1.7f * sc, 8);
        blob(1.4f * sc, y + body * 0.85f, 1.15f * sc, 1.7f * sc, 8);
        blob(0.f, y + body * 0.95f, 0.7f * sc, 0.7f * sc, 13);
        blob(-0.9f * sc, y + body * 0.62f, 0.45f, 0.45f, 10);
        blob(0.9f * sc, y + body * 0.62f, 0.45f, 0.45f, 10);
        stroke(0.4f * sc, y - body * 0.75f, 3.2f * sc, y - body * 1.05f, 8, 1.6f);
    };

    stroke(-12.f, -28.f, -12.f, 8.f, 1, 3.4f);
    stroke(12.f, -28.f, 12.f, 8.f, 1, 3.4f);
    stroke(-12.f, 8.f, -7.f, 16.f, 1, 3.2f);
    stroke(12.f, 8.f, 7.f, 16.f, 1, 3.2f);
    stroke(-7.f, 16.f, 7.f, 16.f, 1, 3.2f);
    stroke(-12.f, -27.f, -12.f, -16.f, 14, 1.8f);
    stroke(12.f, -27.f, 12.f, -16.f, 14, 1.8f);
    stroke(-12.f, -6.f, 12.f, -6.f, 14, 2.2f);
    stroke(-12.f, 4.f, 12.f, 4.f, 14, 2.f);
    stroke(-11.f, -28.f, 11.f, -28.f, 1, 3.f);

    poly({{-10.f, -16.f}, {10.f, -16.f}, {9.f, 3.f}, {-9.f, 3.f}}, 2);
    poly({{-8.f, -13.f}, {8.f, -13.f}, {7.f, 0.f}, {-7.f, 0.f}}, 3);
    poly({{-7.f, -11.f}, {7.f, -11.f}, {6.f, -2.f}, {-6.f, -2.f}}, 4);
    stroke(-4.f, -9.f, 4.f, -9.f, 9, 1.4f);
    stroke(-4.f, -9.f, -4.f, -4.f, 9, 1.4f);
    stroke(4.f, -9.f, 4.f, -4.f, 9, 1.4f);
    stroke(-4.f, -4.f, 4.f, -4.f, 9, 1.4f);

    blob(0.f, -21.f, 5.2f, 7.2f, 5);
    blob(0.f, -15.f, 3.4f, 3.6f, 6);
    blob(-1.1f, -14.6f, 0.55f, 0.55f, 10);
    blob(1.1f, -14.6f, 0.55f, 0.55f, 10);
    stroke(-6.f, -24.f, -11.f, -28.f, 5, 2.4f);
    stroke(6.f, -24.f, 11.f, -28.f, 5, 2.4f);
    blob(-8.f, -27.f, 1.6f, 1.6f, 11);
    blob(8.f, -27.f, 1.6f, 1.6f, 11);

    stroke(0.f, 3.f, 0.f, 10.f, 9, 1.6f);
    stroke(0.f, 18.f, 0.f, 22.f, 9, 1.5f);
    dog(13.f, 1.f);
    dog(25.f, 0.86f);
    b.outline(15, false);
    return b;
}

Bitmap stakeArt() {
    Bitmap b(22, 46);
    b.ellipse(11, 40, 8, 3.2f, 5);
    b.rect(9, 14, 4, 26, 1);
    b.rect(10, 16, 2, 20, 2);
    b.rect(13, 8, 8, 8, 3);
    b.rect(13, 8, 8, 3, 4);
    b.outline(15, false);
    return b;
}

Bitmap pineArt() {
    Bitmap b(44, 58);
    b.rect(19, 40, 6, 14, 4);
    b.poly({{22, 2}, {40, 24}, {4, 24}}, 1);
    b.poly({{22, 14}, {38, 36}, {6, 36}}, 2);
    b.poly({{22, 26}, {34, 46}, {10, 46}}, 1);
    b.ellipse(22, 16, 8, 2.4f, 3);
    b.ellipse(22, 30, 9, 2.4f, 3);
    b.ellipse(22, 42, 7, 2.f, 3);
    b.outline(15, false);
    return b;
}

Bitmap cabinArt() {
    Bitmap b(70, 52);
    b.poly({{4, 24}, {35, 6}, {66, 24}}, 3);
    b.rect(8, 22, 54, 24, 1);
    b.rect(10, 22, 50, 5, 2);
    b.rect(30, 30, 12, 16, 5);
    b.rect(14, 28, 10, 8, 4);
    b.rect(46, 28, 10, 8, 4);
    b.rect(33, 8, 3, 10, 6);
    b.rect(36, 6, 7, 4, 7);
    b.outline(15, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 1);
    b.ellipse(9, 9, 3.2f, 3.2f, 2);
    return b;
}

Bitmap driftArt() {
    Bitmap b(52, 24);
    b.ellipse(26, 14, 22, 8, 5);
    b.ellipse(16, 12, 10, 5, 4);
    b.ellipse(34, 15, 8, 4, 1);
    return b;
}

Bitmap sprayArt() {
    Bitmap b(18, 12);
    b.ellipse(9, 6, 8, 4, 1);
    b.ellipse(5, 6, 3, 2, 2);
    b.ellipse(13, 5, 2.2f, 1.5f, 2);
    return b;
}

Bitmap pinArt() {
    Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
    return b;
}

Bitmap boxArt() {
    Bitmap b(72, 136);
    b.rect(0, 0, 72, 136, 3);
    b.rect(2, 2, 68, 132, 4);
    b.rect(6, 6, 60, 124, 1);
    b.rect(12, 16, 48, 104, 2);
    b.poly({{36, 48}, {52, 68}, {36, 88}, {20, 68}}, 5);
    b.poly({{36, 56}, {44, 68}, {36, 80}, {28, 68}}, 6);
    for (int i = 0; i < 8; i++) {
        b.rect(8, 10.f + i * 15.f, 3, 6, 3);
        b.rect(61, 10.f + i * 15.f, 3, 6, 3);
    }
    return b;
}

Bitmap clockArt(int step) {
    Bitmap b(40, 44);
    b.rect(16, 1, 8, 5, 1);
    b.ellipse(20, 24, 16, 16, 1);
    b.ellipse(20, 24, 13, 13, 2);
    b.ellipse(20, 24, 12, 12, 5);
    for (int i = 0; i < 12; i++) {
        float a = i * kTau / 12.f - 1.5707963f;
        float rad = (i % 3 == 0) ? 1.5f : 0.8f;
        b.ellipse(20.f + std::cos(a) * 9.5f, 24.f + std::sin(a) * 9.5f, rad, rad, 4);
    }
    float a = step * kTau / 12.f - 1.5707963f;
    b.line(20, 24, 20.f + std::cos(a) * 8.5f, 24.f + std::sin(a) * 8.5f, 3, 2.1f);
    b.ellipse(20, 24, 1.8f, 1.8f, 3);
    b.outline(15, false);
    return b;
}

Bitmap birdArt(bool up) {
    Bitmap b(28, 16);
    b.ellipse(14, 9, 3.4f, 2.2f, 1);
    float tip = up ? 2.f : 13.f;
    b.line(14, 8, 2, tip, 2, 1.7f);
    b.line(14, 8, 26, tip, 2, 1.7f);
    b.set(17, 8, 3);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(9, 12, 14), gs::rgb4(15, 12, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SLED,
           {0, gs::rgb4(5, 6, 8), gs::rgb4(9, 6, 3), gs::rgb4(13, 9, 5), gs::rgb4(13, 2, 2), gs::rgb4(3, 6, 12),
            gs::rgb4(13, 11, 8), gs::rgb4(14, 14, 13), gs::rgb4(6, 6, 7), gs::rgb4(8, 5, 2), gs::rgb4(2, 2, 2),
            gs::rgb4(12, 3, 3), gs::rgb4(15, 15, 15), gs::rgb4(3, 1, 1), gs::rgb4(11, 13, 15), shadow});
    setPal(vdp, PAL_CREW,
           {0, gs::rgb4(3, 4, 6), gs::rgb4(8, 5, 3), gs::rgb4(11, 8, 5), gs::rgb4(2, 3, 8), gs::rgb4(13, 7, 2),
            gs::rgb4(12, 10, 8), gs::rgb4(5, 5, 6), gs::rgb4(2, 2, 3), gs::rgb4(12, 10, 2), gs::rgb4(1, 1, 2),
            gs::rgb4(13, 8, 3), gs::rgb4(10, 10, 11), gs::rgb4(4, 2, 2), gs::rgb4(8, 9, 11), shadow});
    setPal(vdp, PAL_STAKE, {0, gs::rgb4(7, 5, 3), gs::rgb4(11, 8, 5), gs::rgb4(13, 2, 2), gs::rgb4(15, 12, 10),
                            gs::rgb4(14, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 3), gs::rgb4(12, 10, 7), gs::rgb4(6, 3, 3), gs::rgb4(15, 12, 4),
                           gs::rgb4(4, 2, 2), gs::rgb4(5, 4, 3), gs::rgb4(12, 12, 13), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(2, 6, 3), gs::rgb4(4, 9, 4), gs::rgb4(14, 15, 15), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 10), gs::rgb4(3, 4, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 8), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 7, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(10, 8, 3), gs::rgb4(14, 13, 10), gs::rgb4(2, 2, 3), gs::rgb4(8, 2, 2),
                            gs::rgb4(15, 14, 9), gs::rgb4(12, 11, 8), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOX, {0, gs::rgb4(10, 13, 15), gs::rgb4(14, 15, 15), gs::rgb4(12, 2, 2), gs::rgb4(15, 8, 5),
                          gs::rgb4(4, 6, 9), gs::rgb4(7, 9, 12), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(2, 2, 3), gs::rgb4(8, 8, 9), gs::rgb4(12, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(14, 15, 15), gs::rgb4(9, 11, 13), gs::rgb4(6, 8, 11), gs::rgb4(12, 14, 15),
                           gs::rgb4(7, 9, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.sled[i] = gs::uploadMipped(vdp, paintSled(i * kTau / 16.f));
    for (int i = 0; i < 12; i++) art.clock[i] = gs::uploadMipped(vdp, clockArt(i));
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.cabin = gs::uploadMipped(vdp, cabinArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.drift = gs::uploadMipped(vdp, driftArt());
    art.spray = gs::uploadMipped(vdp, sprayArt());
    art.pin = gs::uploadMipped(vdp, pinArt());
    Bitmap dot(6, 6);
    dot.ellipse(3, 3, 2.2f, 2.2f, 1);
    art.dot = gs::uploadMipped(vdp, dot);
    art.box = gs::uploadMipped(vdp, boxArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(true));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(false));
    art.title = words(vdp, "SLED BOX", 3);
    art.stopIn = words(vdp, "STOP INSIDE", 2);
    art.crewIs = words(vdp, "THE CREW IS THE CLOCK", 2);
    art.stopped = words(vdp, "STOPPED INSIDE", 2);
    art.ahead = words(vdp, "AHEAD OF THE CREW", 2);
    art.paused = words(vdp, "PAUSED", 3);
    art.crewTook = words(vdp, "CREW TOOK THE BOX", 2);
    art.shortOf = words(vdp, "SHORT OF THE BOX", 2);
    art.outside = words(vdp, "OUTSIDE THE BOX", 2);
    art.slid = words(vdp, "SLID PAST THE BOX", 2);
    art.boxTag = words(vdp, "BOX", 2);
    art.crewTag = words(vdp, "CREW", 2);

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(8, 10, 12));
}

}  // namespace sledbox
