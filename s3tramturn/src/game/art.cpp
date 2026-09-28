#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace tramturn {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float kTau = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    uint16_t c[16] = {};
    int i = 0;
    for (uint16_t v : cs)
        if (i < 16) c[i++] = v;
    for (int k = 0; k < 16; k++) vdp.setColor(pal * 16 + k, c[k]);
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    float wx = ly * c + lx * s;
    float wy = ly * s - lx * c;
    return {cx + wx, cy - wy};
}

Bitmap paintTram(float heading) {
    Bitmap b(72, 72);
    const float cx = 36.f, cy = 36.f;
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
    poly({{-9.f, 26.f}, {9.f, 26.f}, {10.f, -22.f}, {-10.f, -22.f}}, 2);
    poly({{-8.f, 24.f}, {8.f, 24.f}, {8.f, -20.f}, {-8.f, -20.f}}, 1);
    poly({{-6.f, 18.f}, {6.f, 18.f}, {6.f, 8.f}, {-6.f, 8.f}}, 5);
    poly({{-6.f, 4.f}, {6.f, 4.f}, {6.f, -6.f}, {-6.f, -6.f}}, 5);
    poly({{-6.f, -10.f}, {6.f, -10.f}, {6.f, -16.f}, {-6.f, -16.f}}, 4);
    blob(0.f, 22.f, 2.2f, 1.4f, 8);
    blob(-7.5f, 12.f, 1.6f, 2.4f, 3);
    blob(7.5f, 12.f, 1.6f, 2.4f, 3);
    blob(-7.5f, -8.f, 1.6f, 2.4f, 3);
    blob(7.5f, -8.f, 1.6f, 2.4f, 3);
    poly({{-1.2f, 6.f}, {1.2f, 6.f}, {1.2f, 30.f}, {0.f, 33.f}, {-1.2f, 30.f}}, 9);
    poly({{-7.f, 32.f}, {7.f, 32.f}, {7.f, 33.5f}, {-7.f, 33.5f}}, 9);
    b.outline(15, false);
    return b;
}

Bitmap paintRail() {
    Bitmap b(22, 22);
    b.rect(0, 0, 22, 22, 1);
    b.rect(2, 9, 18, 2, 3);
    b.rect(2, 13, 18, 2, 3);
    b.rect(9, 8, 4, 8, 4);
    return b;
}

Bitmap paintSleeper() {
    Bitmap b(16, 6);
    b.rect(0, 1, 16, 4, 1);
    return b;
}

Bitmap paintPole() {
    Bitmap b(10, 36);
    b.rect(4, 8, 2, 26, 1);
    b.rect(1, 4, 8, 4, 2);
    b.rect(3, 32, 4, 3, 1);
    return b;
}

Bitmap paintWire() {
    Bitmap b(40, 6);
    b.rect(0, 2, 40, 2, 1);
    return b;
}

Bitmap paintShed() {
    Bitmap b(48, 36);
    b.rect(2, 12, 44, 22, 1);
    b.poly({{0.f, 12.f}, {24.f, 1.f}, {48.f, 12.f}}, 2);
    b.rect(6, 16, 8, 8, 3);
    b.rect(18, 16, 8, 8, 4);
    b.rect(30, 16, 8, 8, 3);
    b.rect(18, 26, 12, 8, 5);
    b.outline(6, false);
    return b;
}

Bitmap paintClock() {
    Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 1);
    b.ellipse(11, 11, 8, 8, 2);
    b.line(11, 11, 11, 4, 3, 1.4f);
    b.line(11, 11, 16, 13, 3, 1.4f);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(18, 24);
    b.rect(2, 2, 2, 20, 1);
    b.poly({{4.f, 3.f}, {16.f, 8.f}, {4.f, 13.f}}, 2);
    return b;
}

gs::Mipped word(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st{scale, 1, 14, 0, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 13);
    const uint16_t line = gs::rgb4(2, 2, 3);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 10, 9), gs::rgb4(15, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 3), line});
    setPal(vdp, PAL_TRAM,
           {0, gs::rgb4(13, 3, 3), gs::rgb4(6, 1, 1), gs::rgb4(2, 2, 2), gs::rgb4(12, 14, 15), gs::rgb4(6, 10, 13),
            gs::rgb4(15, 13, 4), gs::rgb4(4, 4, 5), gs::rgb4(15, 15, 8), gs::rgb4(9, 9, 10), 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_RAIL,
           {0, gs::rgb4(5, 6, 5), gs::rgb4(3, 4, 3), gs::rgb4(11, 11, 9), gs::rgb4(7, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line});
    setPal(vdp, PAL_YARD,
           {0, gs::rgb4(8, 7, 6), gs::rgb4(5, 3, 3), gs::rgb4(13, 12, 8), gs::rgb4(4, 6, 8), gs::rgb4(3, 2, 2),
            gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, line});
    setPal(vdp, PAL_WIN, {0, ink, gs::rgb4(4, 13, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 2), line});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 13, 12), gs::rgb4(15, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 1), line});
    setPal(vdp, PAL_BANNER, {0, ink, gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 4), line});
    setPal(vdp, PAL_CREW,
           {0, gs::rgb4(3, 6, 12), gs::rgb4(1, 2, 6), gs::rgb4(2, 2, 2), gs::rgb4(10, 12, 14), gs::rgb4(5, 8, 11),
            gs::rgb4(14, 12, 3), gs::rgb4(4, 4, 5), gs::rgb4(12, 14, 15), gs::rgb4(8, 8, 9), 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WIRE, {0, gs::rgb4(6, 6, 7), gs::rgb4(12, 11, 6), gs::rgb4(14, 14, 12), gs::rgb4(8, 2, 2)});

    for (int i = 0; i < 8; i++) art.tram[i] = gs::uploadMipped(vdp, paintTram(float(i) * kTau / 8.f));
    art.rail = gs::uploadMipped(vdp, paintRail());
    art.sleeper = gs::uploadMipped(vdp, paintSleeper());
    art.pole = gs::uploadMipped(vdp, paintPole());
    art.wire = gs::uploadMipped(vdp, paintWire());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.clock = gs::uploadMipped(vdp, paintClock());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.title = word(vdp, "TRAM TURN", 3);
    art.cleared = word(vdp, "THREE TURNS", 2);
    art.tipped = word(vdp, "TIPPED", 2);
    art.crew = word(vdp, "CREW BEAT YOU", 2);
    art.rails = word(vdp, "LEFT THE RAILS", 2);
    art.paused = word(vdp, "HELD", 2);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(3, 4, 5));
}

}  // namespace tramturn
