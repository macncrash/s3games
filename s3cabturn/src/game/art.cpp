#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace cabturn {
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

Bitmap paintCab(float heading) {
    Bitmap b(64, 64);
    const float cx = 32.f, cy = 32.f;
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
    poly({{0.f, 24.f}, {10.f, 16.f}, {11.f, 2.f}, {10.f, -14.f}, {6.f, -20.f}, {-6.f, -20.f}, {-10.f, -14.f}, {-11.f, 2.f}, {-10.f, 16.f}}, 8);
    poly({{0.f, 21.f}, {8.f, 14.f}, {8.5f, 2.f}, {7.5f, -12.f}, {4.f, -17.f}, {-4.f, -17.f}, {-7.5f, -12.f}, {-8.5f, 2.f}, {-8.f, 14.f}}, 1);
    poly({{-6.f, 8.f}, {6.f, 8.f}, {6.f, -6.f}, {-6.f, -6.f}}, 6);
    poly({{-4.5f, 6.f}, {-0.6f, 6.f}, {-0.6f, -4.f}, {-4.5f, -4.f}}, 5);
    poly({{0.6f, 6.f}, {4.5f, 6.f}, {4.5f, -4.f}, {0.6f, -4.f}}, 5);
    blob(0.f, 16.f, 2.2f, 1.6f, 12);
    blob(-9.f, 6.f, 2.1f, 2.6f, 3);
    blob(9.f, 6.f, 2.1f, 2.6f, 3);
    blob(-9.f, -8.f, 2.1f, 2.6f, 3);
    blob(9.f, -8.f, 2.1f, 2.6f, 3);
    blob(0.f, 1.f, 1.5f, 1.5f, 13);
    b.outline(15, false);
    return b;
}

Bitmap paintRoad() {
    Bitmap b(28, 28);
    b.rect(0, 0, 28, 28, 1);
    b.rect(1, 1, 26, 26, 2);
    b.rect(12, 4, 4, 8, 4);
    b.rect(12, 16, 4, 8, 4);
    return b;
}

Bitmap paintDash() {
    Bitmap b(16, 8);
    b.rect(1, 2, 14, 4, 1);
    return b;
}

Bitmap paintBlock() {
    Bitmap b(40, 32);
    b.rect(1, 1, 38, 30, 1);
    b.rect(4, 4, 10, 8, 3);
    b.rect(18, 4, 10, 8, 2);
    b.rect(4, 16, 10, 8, 2);
    b.rect(18, 16, 10, 8, 3);
    b.rect(30, 6, 6, 18, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(12, 28);
    b.rect(5, 8, 2, 18, 1);
    b.ellipse(6, 6, 4, 4, 2);
    b.ellipse(6, 6, 2, 2, 3);
    return b;
}

Bitmap paintCone() {
    Bitmap b(14, 16);
    b.poly({{7.f, 1.f}, {12.f, 14.f}, {2.f, 14.f}}, 1);
    b.rect(4, 8, 6, 2, 2);
    b.outline(3, false);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(18, 22);
    b.rect(3, 2, 2, 18, 1);
    b.poly({{5.f, 3.f}, {16.f, 7.f}, {5.f, 12.f}}, 2);
    return b;
}

Bitmap paintWheel() {
    Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
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
    const uint16_t line = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 9, 8), gs::rgb4(15, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3), line});
    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(15, 13, 2), gs::rgb4(12, 8, 1), gs::rgb4(2, 2, 2), gs::rgb4(14, 14, 12), gs::rgb4(7, 11, 13),
            gs::rgb4(3, 4, 6), gs::rgb4(15, 8, 1), gs::rgb4(3, 3, 3), gs::rgb4(14, 15, 15), gs::rgb4(8, 3, 2),
            gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(15, 14, 6), gs::rgb4(4, 4, 4), line});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(4, 4, 5), gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 6), gs::rgb4(14, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line});
    setPal(vdp, PAL_BLOCK,
           {0, gs::rgb4(7, 6, 6), gs::rgb4(5, 8, 9), gs::rgb4(10, 9, 6), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, line});
    setPal(vdp, PAL_WIN, {0, ink, gs::rgb4(4, 13, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 2), line});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 13, 12), gs::rgb4(15, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 1), line});
    setPal(vdp, PAL_BANNER, {0, ink, gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 4), line});
    setPal(vdp, PAL_CREW,
           {0, gs::rgb4(9, 10, 12), gs::rgb4(4, 5, 7), gs::rgb4(2, 2, 3), gs::rgb4(12, 12, 13), gs::rgb4(5, 7, 9),
            gs::rgb4(2, 3, 4), 0, gs::rgb4(3, 3, 4), 0, 0, gs::rgb4(8, 8, 9), 0, gs::rgb4(6, 6, 7), line});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(14, 12, 3), gs::rgb4(14, 4, 2), gs::rgb4(15, 15, 12), line, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line});
    vdp.setFogColor(gs::rgb4(3, 4, 5));

    for (int i = 0; i < 8; i++) art.cab[i] = gs::uploadMipped(vdp, paintCab(i * kTau / 8.f));
    art.road = gs::uploadMipped(vdp, paintRoad());
    art.dash = gs::uploadMipped(vdp, paintDash());
    art.block = gs::uploadMipped(vdp, paintBlock());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.cone = gs::uploadMipped(vdp, paintCone());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.wheel = gs::uploadMipped(vdp, paintWheel());
    art.title = word(vdp, "CAB TURN", 3);
    art.cleared = word(vdp, "THREE TURNS CLEAR", 2);
    art.tipped = word(vdp, "TIPPED", 3);
    art.crew = word(vdp, "OTHER CREW", 2);
    art.street = word(vdp, "LEFT THE STREET", 2);
    art.paused = word(vdp, "PAUSED", 3);
    loadFont(vdp, art);
    (void)kTau;
}

}  // namespace cabturn
