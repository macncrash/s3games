#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace sledkilo {
namespace {

constexpr float kTau = 6.2831853f;
constexpr float kPi = 3.14159265f;

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

void poly(Bitmap& b, std::initializer_list<Pt> pts, int c) { b.poly(std::vector<Pt>(pts), c); }

void spin(float cx, float cy, float h, float lx, float ly, float& ox, float& oy) {
    const float c = std::cos(h), s = std::sin(h);
    ox = cx + ly * c + lx * s;
    oy = cy - (ly * s - lx * c);
}

void stroke(Bitmap& b, float cx, float cy, float h, float ax, float ay, float bx, float by, int col, float th) {
    float x0, y0, x1, y1;
    spin(cx, cy, h, ax, ay, x0, y0);
    spin(cx, cy, h, bx, by, x1, y1);
    b.line(x0, y0, x1, y1, col, th);
}

void blob(Bitmap& b, float cx, float cy, float h, float lx, float ly, float rx, float ry, int col) {
    float x, y;
    spin(cx, cy, h, lx, ly, x, y);
    b.ellipse(x, y, rx, ry, col);
}

void quad(Bitmap& b, float cx, float cy, float h, std::initializer_list<Pt> local, int col) {
    std::vector<Pt> w;
    w.reserve(local.size());
    for (const Pt& p : local) {
        float x, y;
        spin(cx, cy, h, p.first, p.second, x, y);
        w.push_back({x, y});
    }
    b.poly(w, col);
}

Bitmap paintSled(float heading) {
    Bitmap b(96, 96);
    const float cx = 47.5f, cy = 47.5f;
    auto s = [&](float ax, float ay, float bx, float by, int col, float th) {
        stroke(b, cx, cy, heading, ax, ay, bx, by, col, th);
    };
    auto e = [&](float lx, float ly, float rx, float ry, int col) { blob(b, cx, cy, heading, lx, ly, rx, ry, col); };
    auto q = [&](std::initializer_list<Pt> pts, int col) { quad(b, cx, cy, heading, pts, col); };

    s(-9.f, -30.f, -9.f, 22.f, 1, 3.6f);
    s(9.f, -30.f, 9.f, 22.f, 1, 3.6f);
    s(-9.f, 22.f, -4.f, 30.f, 2, 3.2f);
    s(9.f, 22.f, 4.f, 30.f, 2, 3.2f);
    s(-9.f, -30.f, 9.f, -30.f, 1, 3.f);
    s(-9.f, -8.f, 9.f, -8.f, 14, 2.2f);
    s(-9.f, 8.f, 9.f, 8.f, 14, 2.2f);
    s(-9.f, 18.f, 9.f, 18.f, 14, 2.4f);
    s(-11.f, 16.f, 11.f, 16.f, 2, 2.f);

    q({{-7.f, -6.f}, {7.f, -6.f}, {6.f, 14.f}, {-6.f, 14.f}}, 3);
    q({{-5.5f, -4.f}, {5.5f, -4.f}, {5.f, 12.f}, {-5.f, 12.f}}, 4);

    e(0.f, 2.f, 4.6f, 7.2f, 5);
    e(0.f, 1.f, 3.2f, 5.f, 6);
    e(0.f, 10.f, 3.3f, 3.6f, 7);
    e(-1.1f, 10.6f, 0.55f, 0.55f, 12);
    e(1.1f, 10.6f, 0.55f, 0.55f, 12);
    e(0.f, 14.2f, 3.6f, 2.4f, 8);
    e(0.f, 16.6f, 1.5f, 1.5f, 9);
    s(-3.2f, 6.f, -8.f, 16.f, 11, 2.2f);
    s(3.2f, 6.f, 8.f, 16.f, 11, 2.2f);
    s(-2.f, -4.f, -8.f, -16.f, 10, 2.4f);
    s(2.2f, -2.f, 7.f, -18.f, 10, 2.2f);
    e(-8.f, -16.f, 1.7f, 1.5f, 10);
    e(7.f, -18.f, 1.8f, 1.4f, 13);
    e(-2.f, 24.f, 1.5f, 1.5f, 14);
    e(2.f, 24.f, 1.5f, 1.5f, 14);
    b.outline(15, false);
    return b;
}

Bitmap paintWheel(int frame, int style) {
    Bitmap b(42, 42);
    const float cx = 20.5f, cy = 20.5f;
    const float ang = frame * (kPi * 0.5f) / 8.f;
    const bool bike = style == 2;
    const float outer = bike ? 15.5f : 18.f;
    const float inner = bike ? 13.2f : 14.2f;
    b.ellipse(cx, cy, outer, outer, 1);
    b.ellipse(cx, cy, inner, inner, 0);
    b.ellipse(cx, cy, inner - 1.2f, inner - 1.2f, bike ? 2 : 10);
    b.ellipse(cx, cy, inner - 2.6f, inner - 2.6f, 0);
    const int spokes = bike ? 8 : 4;
    const int spokeCol = bike ? 2 : 3;
    const float reach = inner - 3.2f;
    for (int i = 0; i < spokes; i++) {
        float a = ang + i * kTau / float(spokes);
        b.line(cx, cy, cx + std::cos(a) * reach, cy + std::sin(a) * reach, spokeCol, bike ? 1.15f : 2.1f);
    }
    if (style == 1) b.ellipse(cx, cy, outer - 1.3f, outer - 1.3f, 9);
    b.ellipse(cx, cy, bike ? 2.4f : 3.6f, bike ? 2.4f : 3.6f, 4);
    b.ellipse(cx, cy, 1.3f, 1.3f, 1);
    return b;
}

Bitmap paintHay() {
    Bitmap b(48, 92);
    b.rect(4, 8, 40, 76, 5);
    b.rect(7, 12, 34, 68, 6);
    b.ellipse(24, 40, 14, 22, 7);
    b.ellipse(16, 34, 7, 10, 8);
    b.ellipse(30, 48, 6, 9, 8);
    b.rect(6, 18, 36, 3, 10);
    b.rect(6, 70, 36, 3, 10);
    b.line(10, 14, 38, 78, 9, 1.4f);
    b.line(38, 14, 10, 78, 9, 1.4f);
    b.outline(15, false);
    return b;
}

Bitmap paintCart() {
    Bitmap b(40, 64);
    b.rect(4, 6, 32, 52, 5);
    b.rect(7, 10, 26, 44, 6);
    b.rect(10, 28, 20, 16, 11);
    b.rect(12, 30, 16, 4, 8);
    b.ellipse(20, 18, 6, 5, 9);
    b.rect(6, 14, 28, 2, 10);
    b.rect(6, 46, 28, 2, 10);
    b.outline(15, false);
    return b;
}

Bitmap paintBike() {
    Bitmap b(28, 78);
    b.line(14, 8, 14, 70, 4, 2.4f);
    b.line(8, 28, 20, 28, 5, 2.f);
    b.line(8, 28, 14, 48, 4, 2.f);
    b.line(20, 28, 14, 48, 4, 2.f);
    b.line(14, 48, 14, 62, 5, 2.2f);
    b.ellipse(14, 36, 5.5f, 7.f, 6);
    b.ellipse(14, 24, 3.4f, 3.6f, 7);
    b.ellipse(14, 20, 3.8f, 2.2f, 8);
    b.ellipse(11, 25, 0.7f, 0.7f, 11);
    b.ellipse(17, 25, 0.7f, 0.7f, 11);
    b.line(8, 40, 4, 52, 10, 2.f);
    b.line(20, 40, 24, 52, 10, 2.f);
    b.ellipse(4, 54, 2.f, 1.4f, 9);
    b.ellipse(24, 54, 2.f, 1.4f, 9);
    b.outline(15, false);
    return b;
}

Bitmap paintDray() {
    Bitmap b(44, 90);
    b.rect(3, 6, 38, 78, 5);
    b.rect(6, 10, 32, 70, 12);
    b.rect(6, 10, 32, 8, 13);
    b.rect(8, 28, 12, 16, 6);
    b.rect(24, 28, 12, 16, 6);
    b.rect(8, 52, 28, 14, 11);
    b.line(6, 22, 38, 22, 14, 1.6f);
    b.line(6, 70, 38, 70, 14, 1.6f);
    b.rect(18, 4, 8, 8, 10);
    b.outline(15, false);
    return b;
}

Bitmap paintPine() {
    Bitmap b(40, 58);
    b.rect(17, 40, 6, 14, 4);
    poly(b, {{20, 2}, {38, 24}, {2, 24}}, 1);
    poly(b, {{20, 14}, {36, 36}, {4, 36}}, 2);
    poly(b, {{20, 26}, {32, 46}, {8, 46}}, 1);
    b.ellipse(20, 16, 8, 2.2f, 3);
    b.ellipse(20, 30, 9, 2.2f, 3);
    b.outline(15, false);
    return b;
}

Bitmap paintCabin() {
    Bitmap b(64, 48);
    poly(b, {{2, 22}, {32, 4}, {62, 22}}, 6);
    b.rect(6, 20, 52, 24, 5);
    b.rect(8, 20, 48, 5, 9);
    b.rect(28, 28, 12, 16, 8);
    b.rect(12, 26, 10, 8, 7);
    b.rect(44, 26, 10, 8, 7);
    b.rect(30, 8, 3, 10, 4);
    b.outline(15, false);
    return b;
}

Bitmap paintClock(int step) {
    Bitmap b(40, 56);
    b.rect(17, 36, 6, 16, 6);
    b.ellipse(20, 22, 16, 16, 1);
    b.ellipse(20, 22, 13, 13, 2);
    b.ellipse(20, 22, 11.5f, 11.5f, 3);
    for (int i = 0; i < 12; i++) {
        float a = i * kTau / 12.f - kPi * 0.5f;
        float rad = (i % 3 == 0) ? 1.35f : 0.7f;
        b.ellipse(20.f + std::cos(a) * 8.6f, 22.f + std::sin(a) * 8.6f, rad, rad, 5);
    }
    float a = step * kTau / 8.f - kPi * 0.5f;
    b.line(20, 22, 20.f + std::cos(a) * 7.4f, 22.f + std::sin(a) * 7.4f, 4, 2.f);
    b.ellipse(20, 22, 1.7f, 1.7f, 4);
    b.outline(15, false);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(16, 32);
    b.rect(7, 12, 2, 16, 7);
    b.ellipse(8, 8, 6, 6, 5);
    b.ellipse(8, 8, 2.6f, 2.6f, 6);
    b.ellipse(8, 28, 5, 2, 2);
    return b;
}

Bitmap paintPost() {
    Bitmap b(18, 40);
    b.rect(7, 10, 4, 26, 1);
    b.rect(8, 12, 2, 18, 2);
    b.rect(11, 8, 6, 8, 3);
    b.rect(11, 8, 6, 3, 4);
    b.ellipse(9, 36, 6, 2.2f, 2);
    b.outline(15, false);
    return b;
}

Bitmap paintRibbon() {
    Bitmap b(96, 12);
    for (int i = 0; i < 8; i++) b.rect(float(i * 12), 1, 12, 10, (i & 1) ? 4 : 3);
    b.rect(0, 0, 96, 2, 3);
    b.rect(0, 10, 96, 2, 3);
    return b;
}

Bitmap paintStart() {
    Bitmap b(72, 8);
    b.rect(0, 2, 72, 4, 4);
    b.rect(32, 0, 8, 8, 3);
    return b;
}

Bitmap paintSpray() {
    Bitmap b(16, 10);
    b.ellipse(8, 5, 7, 3.4f, 1);
    b.ellipse(4, 5, 2.4f, 1.6f, 2);
    b.ellipse(12, 4, 2.f, 1.4f, 3);
    return b;
}

Bitmap paintBird(bool up) {
    Bitmap b(26, 14);
    b.ellipse(13, 8, 3.2f, 2.f, 8);
    float tip = up ? 2.f : 12.f;
    b.line(13, 7, 2, tip, 9, 1.6f);
    b.line(13, 7, 24, tip, 9, 1.6f);
    b.set(16, 7, 10);
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
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

void setRoad(gs::VDP& vdp) {
    const int b = PAL_ROAD * 16;
    const uint16_t c[16] = {
        0,
        gs::rgb4(8, 10, 13),
        gs::rgb4(12, 14, 15),
        gs::rgb4(15, 15, 15),
        gs::rgb4(7, 9, 12),
        gs::rgb4(10, 12, 14),
        gs::rgb4(13, 14, 15),
        gs::rgb4(11, 13, 15),
        gs::rgb4(6, 8, 11),
        gs::rgb4(14, 15, 15),
        gs::rgb4(9, 11, 14),
        gs::rgb4(8, 10, 13),
        gs::rgb4(7, 9, 12),
        gs::rgb4(10, 12, 14),
        gs::rgb4(15, 15, 15),
        gs::rgb4(13, 15, 15),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(b + i, c[i]);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(2, 2, 4);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 13, 15), gs::rgb4(15, 13, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SLED,
           {0, gs::rgb4(4, 5, 7), gs::rgb4(11, 13, 15), gs::rgb4(6, 4, 2), gs::rgb4(11, 7, 4), gs::rgb4(13, 2, 2),
            gs::rgb4(8, 1, 1), gs::rgb4(13, 9, 6), gs::rgb4(12, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(3, 3, 4),
            gs::rgb4(9, 2, 2), gs::rgb4(1, 1, 2), gs::rgb4(14, 13, 13), gs::rgb4(12, 9, 3), shadow});
    setPal(vdp, PAL_CREW,
           {0, gs::rgb4(4, 5, 7), gs::rgb4(11, 13, 15), gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 4), gs::rgb4(2, 4, 12),
            gs::rgb4(1, 2, 7), gs::rgb4(13, 9, 6), gs::rgb4(1, 2, 6), gs::rgb4(15, 15, 15), gs::rgb4(3, 3, 4),
            gs::rgb4(2, 3, 8), gs::rgb4(1, 1, 2), gs::rgb4(12, 13, 15), gs::rgb4(12, 9, 3), shadow});
    setPal(vdp, PAL_WAGON,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 7), gs::rgb4(9, 7, 4), gs::rgb4(13, 10, 4), gs::rgb4(7, 4, 2),
            gs::rgb4(12, 8, 4), gs::rgb4(12, 10, 3), gs::rgb4(14, 12, 6), gs::rgb4(12, 2, 2), gs::rgb4(3, 3, 4),
            gs::rgb4(9, 8, 5), gs::rgb4(11, 12, 13), gs::rgb4(6, 7, 9), gs::rgb4(8, 3, 2), shadow});
    setPal(vdp, PAL_BIKE,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(10, 11, 12), gs::rgb4(12, 10, 4), gs::rgb4(3, 4, 6), gs::rgb4(8, 9, 11),
            gs::rgb4(13, 8, 2), gs::rgb4(13, 9, 6), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 3), gs::rgb4(2, 3, 6),
            gs::rgb4(1, 1, 2), gs::rgb4(12, 4, 2), 0, 0, shadow});
    setPal(vdp, PAL_PINE,
           {0, gs::rgb4(1, 5, 2), gs::rgb4(3, 8, 3), gs::rgb4(15, 15, 15), gs::rgb4(6, 4, 2), gs::rgb4(8, 5, 3),
            gs::rgb4(5, 2, 2), gs::rgb4(15, 12, 4), gs::rgb4(3, 2, 2), gs::rgb4(12, 13, 14), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CLOCK,
           {0, gs::rgb4(9, 7, 3), gs::rgb4(14, 12, 7), gs::rgb4(15, 14, 11), gs::rgb4(2, 2, 3), gs::rgb4(6, 5, 4),
            gs::rgb4(5, 4, 3), gs::rgb4(12, 11, 9), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15), gs::rgb4(14, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 5), gs::rgb4(3, 3, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 8), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 8, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_POST,
           {0, gs::rgb4(7, 5, 3), gs::rgb4(14, 15, 15), gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(15, 12, 4),
            gs::rgb4(15, 15, 12), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3), gs::rgb4(7, 7, 8), gs::rgb4(12, 8, 3), 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TAG, {0, gs::rgb4(2, 3, 6), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setRoad(vdp);
    vdp.setFogColor(gs::rgb4(12, 14, 15));

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.sled[i] = gs::uploadMipped(vdp, paintSled(i * kTau / 16.f));
    for (int s = 0; s < 4; s++)
        for (int f = 0; f < 8; f++) art.wheel[s][f] = gs::uploadMipped(vdp, paintWheel(f, s));
    art.hay = gs::uploadMipped(vdp, paintHay());
    art.cart = gs::uploadMipped(vdp, paintCart());
    art.bike = gs::uploadMipped(vdp, paintBike());
    art.dray = gs::uploadMipped(vdp, paintDray());
    art.pine = gs::uploadMipped(vdp, paintPine());
    art.cabin = gs::uploadMipped(vdp, paintCabin());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.ribbon = gs::uploadMipped(vdp, paintRibbon());
    art.startLine = gs::uploadMipped(vdp, paintStart());
    art.spray = gs::uploadMipped(vdp, paintSpray());
    Bitmap dot(6, 6);
    dot.ellipse(3, 3, 2.1f, 2.1f, 1);
    art.dot = gs::uploadMipped(vdp, dot);
    for (int i = 0; i < 8; i++) art.clock[i] = gs::uploadMipped(vdp, paintClock(i));
    art.bird[0] = gs::uploadMipped(vdp, paintBird(true));
    art.bird[1] = gs::uploadMipped(vdp, paintBird(false));
    art.title = words(vdp, "SLED KILO", 3);
    art.paused = words(vdp, "PAUSED", 3);
    art.kilometer = words(vdp, "KILOMETER", 2);
    art.clean = words(vdp, "WHEELS CLEAN", 2);
    art.touched = words(vdp, "TOUCHED A WHEEL", 2);
    art.leftSnow = words(vdp, "LEFT THE SNOW", 2);
    art.crewTook = words(vdp, "CREW TOOK IT", 2);
    art.m250 = words(vdp, "250", 2);
    art.m500 = words(vdp, "500", 2);
    art.m750 = words(vdp, "750", 2);
    art.m1000 = words(vdp, "1000", 2);
    art.crewTag = words(vdp, "CREW", 2);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
}

}  // namespace sledkilo
