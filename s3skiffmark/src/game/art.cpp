#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace skiffmark {
namespace {

constexpr float kTau = 6.2831853f;

using gs::Bitmap;
using gs::Pt;

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

// Lapstrake skiff, bow toward local +y. A cuddy and an outboard, not a bare hull.
Bitmap paintSkiff(float heading) {
    Bitmap b(88, 96);
    const float cx = 44.f, cy = 48.f;
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
    poly({{0.f, 40.f}, {14.f, 22.f}, {15.f, 2.f}, {13.f, -16.f}, {9.f, -28.f}, {-9.f, -28.f}, {-13.f, -16.f}, {-15.f, 2.f}, {-14.f, 22.f}}, 3);
    poly({{0.f, 34.f}, {10.f, 18.f}, {11.f, 2.f}, {9.f, -12.f}, {6.f, -22.f}, {-6.f, -22.f}, {-9.f, -12.f}, {-11.f, 2.f}, {-10.f, 18.f}}, 1);
    poly({{0.f, 28.f}, {7.f, 14.f}, {7.f, 0.f}, {5.f, -14.f}, {-5.f, -14.f}, {-7.f, 0.f}, {-7.f, 14.f}}, 2);
    stroke(-9.f, 16.f, -8.f, -18.f, 5, 2.4f);
    stroke(9.f, 16.f, 8.f, -18.f, 5, 2.4f);
    poly({{-6.f, 8.f}, {6.f, 8.f}, {6.f, -6.f}, {4.f, -10.f}, {-4.f, -10.f}, {-6.f, -6.f}}, 4);
    poly({{-4.f, 6.f}, {4.f, 6.f}, {4.f, -2.f}, {-4.f, -2.f}}, 6);
    stroke(-5.f, 4.f, 5.f, 4.f, 9, 1.3f);
    blob(0.f, 30.f, 2.2f, 2.4f, 7);
    poly({{0.f, 36.f}, {4.f, 26.f}, {0.f, 22.f}, {-4.f, 26.f}}, 8);
    poly({{-4.f, -20.f}, {4.f, -20.f}, {5.f, -32.f}, {-5.f, -32.f}}, 10);
    blob(0.f, -33.f, 4.2f, 3.0f, 11);
    blob(-8.f, -4.f, 2.4f, 3.6f, 7);
    blob(8.f, -4.f, 2.4f, 3.6f, 7);
    b.outline(12, false);
    return b;
}

Bitmap paintDisc() {
    Bitmap b(96, 96);
    const float c = 47.5f;
    b.ellipse(c, c, 44, 44, 1);
    b.ellipse(c, c, 30, 30, 2);
    b.ellipse(c, c, 16, 16, 4);
    for (int d = 0; d < 360; d += 6) {
        float a0 = d * 3.14159265f / 180.f;
        float a1 = (d + 4.f) * 3.14159265f / 180.f;
        b.line(c + std::cos(a0) * 42.f, c + std::sin(a0) * 42.f, c + std::cos(a1) * 42.f, c + std::sin(a1) * 42.f, 3, 2.2f);
        b.line(c + std::cos(a0) * 22.f, c + std::sin(a0) * 22.f, c + std::cos(a1) * 22.f, c + std::sin(a1) * 22.f, 5, 1.8f);
    }
    b.line(c, c - 12.f, c, c + 12.f, 5, 1.6f);
    b.line(c - 12.f, c, c + 12.f, c, 5, 1.6f);
    b.outline(6, false);
    return b;
}

Bitmap paintRing() {
    Bitmap b(64, 64);
    const float c = 31.5f, r = 26.f;
    for (int d = 0; d < 360; d += 8) {
        float a0 = d * 3.14159265f / 180.f;
        float a1 = (d + 5.f) * 3.14159265f / 180.f;
        b.line(c + std::cos(a0) * r, c + std::sin(a0) * r, c + std::cos(a1) * r, c + std::sin(a1) * r, 1, 2.4f);
    }
    return b;
}

Bitmap paintSpar() {
    Bitmap b(28, 48);
    b.ellipse(14, 40, 8, 4, 3);
    b.rect(12, 14, 4, 26, 1);
    b.rect(13, 18, 2, 16, 2);
    b.poly({{16, 12}, {26, 18}, {16, 24}}, 4);
    b.poly({{16, 16}, {23, 18}, {16, 21}}, 5);
    b.ellipse(14, 12, 3, 3, 4);
    b.outline(6, false);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(22, 26);
    b.ellipse(11, 16, 7, 7, 1);
    b.ellipse(11, 16, 7, 3, 2);
    b.rect(10, 6, 2, 6, 3);
    b.ellipse(11, 6, 3, 2, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintReed() {
    Bitmap b(32, 36);
    b.ellipse(16, 26, 12, 6, 2);
    for (int i = 0; i < 5; i++) {
        float x = 6.f + i * 5.f;
        b.line(x, 26, x + ((i & 1) ? 3.f : -3.f), 5.f + (i % 3) * 2.f, 1, 1.5f);
    }
    b.ellipse(16, 24, 5, 2, 3);
    return b;
}

Bitmap paintPost() {
    Bitmap b(14, 28);
    b.ellipse(7, 22, 5, 3, 2);
    b.rect(5, 4, 4, 20, 1);
    b.rect(6, 8, 2, 10, 3);
    b.ellipse(7, 4, 3, 2, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintPlank() {
    Bitmap b(72, 18);
    b.rect(2, 4, 68, 10, 1);
    b.line(8, 5, 8, 13, 2, 1.2f);
    b.line(24, 5, 24, 13, 2, 1.2f);
    b.line(40, 5, 40, 13, 2, 1.2f);
    b.line(56, 5, 56, 13, 2, 1.2f);
    b.outline(3, false);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(32, 16);
    b.ellipse(16, 9, 3, 2, 1);
    float tip = up ? 2.f : 12.f;
    b.line(16, 8, 2, tip, 1, 1.5f);
    b.line(16, 8, 30, tip, 1, 1.5f);
    b.set(20, 8, 2);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(16, 10);
    b.ellipse(8, 5, 6, 3, 1);
    b.ellipse(8, 5, 3, 2, 2);
    return b;
}

Bitmap paintDash() {
    Bitmap b(18, 8);
    b.rect(1, 2, 16, 4, 1);
    return b;
}

Bitmap paintDot() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

Bitmap paintPin() {
    Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
    return b;
}

Bitmap paintPanel() {
    Bitmap b(52, 86);
    b.rect(1, 1, 50, 84, 1);
    b.rect(3, 3, 46, 80, 2);
    b.outline(3, false);
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

void shorePal(gs::VDP& vdp, int pal, bool basin) {
    uint16_t c[16] = {};
    c[1] = basin ? gs::rgb4(11, 10, 6) : gs::rgb4(9, 8, 5);
    c[2] = basin ? gs::rgb4(8, 8, 4) : gs::rgb4(6, 6, 3);
    c[3] = gs::rgb4(5, 8, 3);
    c[4] = gs::rgb4(7, 7, 4);
    c[5] = gs::rgb4(5, 6, 3);
    c[6] = gs::rgb4(4, 7, 9);
    c[7] = gs::rgb4(2, 5, 8);
    c[8] = gs::rgb4(8, 7, 4);
    c[11] = basin ? gs::rgb4(4, 10, 12) : gs::rgb4(2, 7, 10);
    c[12] = basin ? gs::rgb4(2, 8, 11) : gs::rgb4(1, 5, 8);
    c[13] = gs::rgb4(8, 13, 13);
    c[14] = gs::rgb4(14, 13, 8);
    c[15] = gs::rgb4(6, 9, 10);
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t dim = gs::rgb4(8, 9, 8);
    const uint16_t line = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, dim, gs::rgb4(15, 14, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(14, 13, 10), gs::rgb4(10, 9, 6), gs::rgb4(5, 4, 3), gs::rgb4(3, 8, 9), gs::rgb4(2, 12, 10),
            gs::rgb4(8, 13, 14), gs::rgb4(15, 15, 14), gs::rgb4(13, 3, 2), gs::rgb4(11, 8, 4), gs::rgb4(3, 3, 3),
            gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), gs::rgb4(6, 5, 4), gs::rgb4(15, 13, 4), line});
    setPal(vdp, PAL_CREW,
           {0, gs::rgb4(7, 8, 9), gs::rgb4(4, 5, 6), gs::rgb4(2, 2, 3), gs::rgb4(8, 4, 2), gs::rgb4(14, 8, 2),
            gs::rgb4(10, 12, 12), gs::rgb4(13, 13, 12), gs::rgb4(12, 10, 4), gs::rgb4(6, 5, 4), gs::rgb4(2, 2, 2),
            gs::rgb4(1, 1, 2), gs::rgb4(1, 1, 1), gs::rgb4(5, 4, 3), gs::rgb4(15, 12, 3), line});
    setPal(vdp, PAL_REED, {0, gs::rgb4(7, 12, 4), gs::rgb4(4, 7, 3), gs::rgb4(9, 11, 5), gs::rgb4(2, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(11, 8, 4), gs::rgb4(7, 5, 3), gs::rgb4(3, 2, 1), gs::rgb4(14, 13, 9), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_MARK,
           {0, gs::rgb4(14, 12, 5), gs::rgb4(11, 9, 3), gs::rgb4(15, 15, 13), gs::rgb4(15, 8, 2), gs::rgb4(15, 14, 8),
            gs::rgb4(4, 3, 1), gs::rgb4(8, 6, 2), 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 13, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(14, 7, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(14, 4, 2), gs::rgb4(15, 12, 6), gs::rgb4(3, 3, 3), gs::rgb4(15, 14, 8), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(8, 14, 13), gs::rgb4(2, 5, 8), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(10, 15, 6), gs::rgb4(4, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 1), line});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1), line});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 8), gs::rgb4(8, 7, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 1), line});
    shorePal(vdp, PAL_SHORE, false);
    shorePal(vdp, PAL_BASIN, true);

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.hull[i] = gs::uploadMipped(vdp, paintSkiff(i * kTau / 16.f));
    art.disc = gs::uploadMipped(vdp, paintDisc());
    art.ring = gs::uploadMipped(vdp, paintRing());
    art.spar = gs::uploadMipped(vdp, paintSpar());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.plank = gs::uploadMipped(vdp, paintPlank());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(true));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(false));
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.dash = gs::uploadMipped(vdp, paintDash());
    art.dot = gs::uploadMipped(vdp, paintDot());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.panel = gs::uploadMipped(vdp, paintPanel());
    art.title = word(vdp, "SKIFF MARK", 3);
    art.setDown = word(vdp, "SET DOWN", 3);
    art.onMark = word(vdp, "ON THE MARK", 2);
    art.crewTook = word(vdp, "CREW TOOK IT", 2);
    art.missed = word(vdp, "MISSED THE MARK", 2);
    art.grounded = word(vdp, "GROUNDED", 2);
    art.paused = word(vdp, "PAUSED", 3);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.setFogColor(gs::rgb4(1, 4, 7));
}

}  // namespace skiffmark
