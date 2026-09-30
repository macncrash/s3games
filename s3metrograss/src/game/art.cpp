#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace metrograss {
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

// Twin-hull water metro. Local +y is the bow.
Bitmap paintHull(float heading) {
    Bitmap b(96, 108);
    const float cx = 48.f, cy = 54.f;
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
    poly({{-22.f, 18.f}, {-14.f, 36.f}, {-8.f, 18.f}, {-10.f, -30.f}, {-20.f, -26.f}}, 3);
    poly({{22.f, 18.f}, {14.f, 36.f}, {8.f, 18.f}, {10.f, -30.f}, {20.f, -26.f}}, 3);
    poly({{-16.f, 10.f}, {16.f, 10.f}, {14.f, -16.f}, {-14.f, -16.f}}, 4);
    poly({{-12.f, 16.f}, {12.f, 16.f}, {10.f, -6.f}, {-10.f, -6.f}}, 1);
    poly({{-8.f, 12.f}, {8.f, 12.f}, {6.f, 2.f}, {-6.f, 2.f}}, 2);
    stroke(-14.f, 4.f, 14.f, 4.f, 5, 2.f);
    blob(0.f, 22.f, 3.f, 2.4f, 6);
    blob(-18.f, 8.f, 2.2f, 3.f, 7);
    blob(18.f, 8.f, 2.2f, 3.f, 7);
    stroke(0.f, 8.f, 0.f, 38.f, 8, 1.5f);
    stroke(-6.f, 36.f, 6.f, 36.f, 8, 1.6f);
    b.outline(12, false);
    return b;
}

Bitmap paintTuft() {
    Bitmap b(24, 24);
    b.ellipse(12, 16, 8, 5, 2);
    b.line(6, 16, 4, 6, 1, 1.3f);
    b.line(12, 16, 12, 4, 1, 1.3f);
    b.line(18, 16, 20, 6, 3, 1.3f);
    return b;
}

Bitmap paintPost() {
    Bitmap b(12, 30);
    b.rect(5, 8, 2, 18, 1);
    b.ellipse(6, 7, 4, 3, 2);
    b.rect(2, 24, 8, 3, 3);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(14, 28);
    b.rect(6, 10, 2, 14, 1);
    b.ellipse(7, 8, 4, 4, 2);
    b.ellipse(7, 8, 2, 2, 3);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(22, 32);
    b.rect(4, 8, 3, 20, 4);
    b.poly({{7, 8}, {18, 13}, {7, 18}}, 2);
    b.ellipse(5, 8, 2, 2, 1);
    return b;
}

Bitmap paintDash() {
    Bitmap b(16, 6);
    b.rect(0, 1, 16, 4, 1);
    return b;
}

Bitmap paintHalt() {
    Bitmap b(72, 40);
    b.rect(6, 16, 60, 14, 1);
    b.poly({{4, 18}, {36, 4}, {68, 18}}, 2);
    b.rect(28, 20, 16, 8, 5);
    b.rect(10, 20, 10, 6, 4);
    b.rect(52, 20, 10, 6, 4);
    b.outline(6, false);
    return b;
}

Bitmap paintWake() {
    Bitmap b(28, 16);
    b.line(14, 2, 4, 12, 1, 1.4f);
    b.line(14, 2, 24, 12, 1, 1.4f);
    b.line(14, 6, 8, 14, 2, 1.1f);
    b.line(14, 6, 20, 14, 2, 1.1f);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(28, 14);
    b.ellipse(14, 8, 3, 2, 1);
    float tip = up ? 2.f : 11.f;
    b.line(14, 7, 2, tip, 1, 1.3f);
    b.line(14, 7, 26, tip, 1, 1.3f);
    return b;
}

Bitmap paintRing() {
    Bitmap b(48, 48);
    const float c = 23.5f, r = 18.f;
    for (int d = 0; d < 360; d += 12) {
        float a0 = d * 3.14159265f / 180.f;
        float a1 = (d + 6.f) * 3.14159265f / 180.f;
        b.line(c + std::cos(a0) * r, c + std::sin(a0) * r, c + std::cos(a1) * r, c + std::sin(a1) * r, 1, 2.f);
    }
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
    Bitmap b(48, 70);
    b.rect(2, 2, 44, 66, 1);
    b.rect(4, 4, 40, 62, 2);
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

void surfacePal(gs::VDP& vdp, int pal, bool endBand, bool water) {
    uint16_t c[16] = {};
    if (water) {
        c[1] = gs::rgb4(4, 6, 8);
        c[2] = gs::rgb4(3, 5, 7);
        c[3] = gs::rgb4(6, 8, 9);
        c[4] = gs::rgb4(8, 8, 7);
        c[5] = gs::rgb4(6, 6, 5);
        c[6] = gs::rgb4(5, 8, 10);
        c[7] = gs::rgb4(3, 6, 8);
        c[11] = gs::rgb4(3, 8, 12);
        c[12] = gs::rgb4(2, 6, 10);
        c[13] = gs::rgb4(8, 13, 15);
    } else {
        c[1] = endBand ? gs::rgb4(10, 14, 6) : gs::rgb4(5, 11, 3);
        c[2] = endBand ? gs::rgb4(6, 11, 3) : gs::rgb4(3, 7, 2);
        c[3] = gs::rgb4(10, 12, 5);
        c[4] = gs::rgb4(9, 10, 5);
        c[5] = gs::rgb4(6, 8, 3);
        c[6] = endBand ? gs::rgb4(13, 15, 8) : gs::rgb4(7, 13, 4);
        c[7] = endBand ? gs::rgb4(8, 13, 4) : gs::rgb4(3, 8, 2);
        c[8] = gs::rgb4(2, 5, 2);
        c[10] = endBand ? gs::rgb4(15, 15, 12) : gs::rgb4(12, 14, 7);
        c[11] = gs::rgb4(3, 7, 10);
        c[12] = gs::rgb4(2, 5, 8);
        c[13] = gs::rgb4(8, 12, 14);
        c[15] = gs::rgb4(8, 12, 5);
    }
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t dim = gs::rgb4(8, 9, 10);
    const uint16_t line = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, dim, gs::rgb4(12, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(14, 14, 13), gs::rgb4(6, 10, 14), gs::rgb4(4, 5, 6), gs::rgb4(9, 8, 7), gs::rgb4(14, 10, 2),
            gs::rgb4(15, 14, 6), gs::rgb4(3, 8, 12), gs::rgb4(15, 15, 14), gs::rgb4(2, 3, 4), gs::rgb4(8, 9, 10),
            gs::rgb4(12, 6, 3), gs::rgb4(1, 1, 1), gs::rgb4(7, 8, 9), gs::rgb4(11, 13, 15), line});
    setPal(vdp, PAL_WAKE, {0, gs::rgb4(12, 15, 15), gs::rgb4(6, 10, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_POST, {0, gs::rgb4(7, 7, 8), gs::rgb4(14, 12, 4), gs::rgb4(4, 4, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(11, 8, 4), gs::rgb4(8, 12, 14), gs::rgb4(5, 4, 2), gs::rgb4(14, 13, 9), gs::rgb4(3, 6, 8),
            gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 3, 2), gs::rgb4(15, 13, 3), gs::rgb4(6, 5, 4), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(5, 5, 6), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_TUFT, {0, gs::rgb4(8, 14, 4), gs::rgb4(4, 9, 2), gs::rgb4(12, 15, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(6, 10, 12), gs::rgb4(3, 5, 4), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(10, 15, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 1), line});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1), line});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 1), line});
    surfacePal(vdp, PAL_FIELD, false, false);
    surfacePal(vdp, PAL_ENDF, true, false);
    surfacePal(vdp, PAL_WATER, false, true);

    loadFont(vdp, art);
    for (int i = 0; i < 8; i++) art.hull[i] = gs::uploadMipped(vdp, paintHull(i * kTau / 8.f));
    art.tuft = gs::uploadMipped(vdp, paintTuft());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.dash = gs::uploadMipped(vdp, paintDash());
    art.halt = gs::uploadMipped(vdp, paintHalt());
    art.wake = gs::uploadMipped(vdp, paintWake());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(true));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(false));
    art.ring = gs::uploadMipped(vdp, paintRing());
    art.dot = gs::uploadMipped(vdp, paintDot());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.panel = gs::uploadMipped(vdp, paintPanel());
    art.title = word(vdp, "METRO GRASS", 3);
    art.fullStop = word(vdp, "FULL STOP", 3);
    art.onGrass = word(vdp, "ON THE GRASS", 2);
    art.missed = word(vdp, "MISSED THE END", 2);
    art.legFail = word(vdp, "LEG FAILS", 2);
    art.paused = word(vdp, "PAUSED", 3);
    (void)kTau;
}

}  // namespace metrograss
