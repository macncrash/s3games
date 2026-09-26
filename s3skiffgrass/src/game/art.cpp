#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace skiffgrass {
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

// Blunt-bow landing skiff. Local +y is the bow that goes up the grass.
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
    poly({{0.f, 32.f}, {18.f, 18.f}, {18.f, -8.f}, {14.f, -22.f}, {-14.f, -22.f}, {-18.f, -8.f}, {-18.f, 18.f}}, 3);
    poly({{0.f, 26.f}, {13.f, 14.f}, {13.f, -6.f}, {10.f, -16.f}, {-10.f, -16.f}, {-13.f, -6.f}, {-13.f, 14.f}}, 1);
    poly({{0.f, 22.f}, {8.f, 12.f}, {8.f, -4.f}, {5.f, -12.f}, {-5.f, -12.f}, {-8.f, -4.f}, {-8.f, 12.f}}, 2);
    stroke(-11.f, 14.f, -11.f, -14.f, 13, 2.2f);
    stroke(11.f, 14.f, 11.f, -14.f, 13, 2.2f);
    stroke(-12.f, 6.f, 12.f, 6.f, 4, 1.6f);
    stroke(-10.f, -8.f, 10.f, -8.f, 9, 1.5f);
    poly({{0.f, 28.f}, {7.f, 20.f}, {0.f, 16.f}, {-7.f, 20.f}}, 5);
    blob(0.f, 24.f, 2.2f, 2.2f, 14);
    poly({{-5.f, -16.f}, {5.f, -16.f}, {4.f, -30.f}, {-4.f, -30.f}}, 6);
    blob(0.f, -31.f, 3.4f, 2.2f, 7);
    blob(-6.f, 2.f, 3.0f, 4.2f, 8);
    blob(6.f, 2.f, 3.0f, 4.2f, 8);
    stroke(0.f, -14.f, 2.f, -24.f, 10, 1.3f);
    b.outline(12, false);
    return b;
}

Bitmap paintTuft() {
    Bitmap b(28, 28);
    b.ellipse(14, 16, 10, 7, 2);
    b.ellipse(10, 13, 5, 6, 1);
    b.ellipse(18, 12, 4, 5, 3);
    b.line(8, 18, 6, 8, 1, 1.4f);
    b.line(14, 18, 14, 6, 1, 1.4f);
    b.line(20, 18, 22, 8, 3, 1.4f);
    return b;
}

Bitmap paintReed() {
    Bitmap b(36, 40);
    b.ellipse(18, 26, 14, 8, 2);
    b.ellipse(18, 24, 8, 4, 3);
    for (int i = 0; i < 5; i++) {
        float x = 8.f + i * 5.f;
        b.line(x, 26, x + ((i & 1) ? 2.f : -2.f), 6.f + (i % 3), 1, 1.6f);
    }
    b.outline(4, false);
    return b;
}

Bitmap paintPost() {
    Bitmap b(16, 22);
    b.ellipse(8, 16, 6, 4, 2);
    b.rect(6, 4, 4, 14, 1);
    b.rect(7, 6, 2, 8, 3);
    b.ellipse(8, 4, 3, 2, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(22, 32);
    b.rect(10, 10, 3, 18, 4);
    b.poly({{13, 8}, {20, 12}, {13, 18}}, 2);
    b.poly({{13, 12}, {18, 14}, {13, 16}}, 3);
    b.ellipse(11, 8, 2, 2, 1);
    b.outline(5, false);
    return b;
}

Bitmap paintDash() {
    Bitmap b(20, 8);
    b.rect(1, 2, 18, 4, 1);
    return b;
}

Bitmap paintShed() {
    Bitmap b(64, 48);
    b.ellipse(32, 30, 26, 12, 3);
    b.rect(10, 16, 44, 18, 1);
    b.poly({{8, 18}, {32, 6}, {56, 18}}, 2);
    b.rect(28, 20, 10, 12, 5);
    b.rect(16, 20, 8, 6, 4);
    b.rect(42, 20, 8, 6, 4);
    b.outline(6, false);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(24, 24);
    b.ellipse(12, 13, 8, 8, 1);
    b.ellipse(12, 13, 8, 3, 2);
    b.ellipse(9, 10, 3, 2, 3);
    b.rect(11, 3, 2, 5, 4);
    b.outline(4, false);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(30, 16);
    b.ellipse(15, 9, 3, 2, 1);
    float tip = up ? 2.f : 12.f;
    b.line(15, 8, 2, tip, 1, 1.4f);
    b.line(15, 8, 28, tip, 1, 1.4f);
    b.set(18, 8, 2);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 6, 3, 1);
    b.ellipse(8, 6, 3, 2, 2);
    return b;
}

Bitmap paintRing() {
    Bitmap b(72, 72);
    const float c = 35.5f, r = 30.f;
    for (int d = 0; d < 360; d += 8) {
        float a0 = d * 3.14159265f / 180.f;
        float a1 = (d + 4.f) * 3.14159265f / 180.f;
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
    Bitmap b(56, 78);
    b.rect(2, 2, 52, 74, 1);
    b.rect(4, 4, 48, 70, 2);
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

void fieldPal(gs::VDP& vdp, int pal, bool endBand) {
    const uint16_t waterA = gs::rgb4(3, 8, 11);
    const uint16_t waterB = gs::rgb4(1, 5, 8);
    const uint16_t glint = gs::rgb4(8, 14, 14);
    uint16_t c[16] = {};
    c[1] = endBand ? gs::rgb4(8, 13, 5) : gs::rgb4(5, 10, 3);
    c[2] = endBand ? gs::rgb4(5, 10, 3) : gs::rgb4(3, 7, 2);
    c[3] = gs::rgb4(9, 12, 5);
    c[4] = gs::rgb4(10, 11, 4);
    c[5] = gs::rgb4(6, 8, 3);
    c[6] = endBand ? gs::rgb4(11, 15, 7) : gs::rgb4(6, 13, 4);
    c[7] = endBand ? gs::rgb4(7, 12, 4) : gs::rgb4(3, 8, 2);
    c[8] = gs::rgb4(2, 6, 2);
    c[9] = gs::rgb4(4, 7, 2);
    c[10] = endBand ? gs::rgb4(15, 15, 11) : gs::rgb4(12, 14, 7);
    c[11] = waterA;
    c[12] = waterB;
    c[13] = glint;
    c[14] = gs::rgb4(15, 15, 13);
    c[15] = gs::rgb4(8, 12, 5);
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t dim = gs::rgb4(8, 9, 8);
    const uint16_t line = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, dim, gs::rgb4(15, 14, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(13, 11, 7), gs::rgb4(9, 7, 4), gs::rgb4(5, 4, 3), gs::rgb4(15, 15, 13), gs::rgb4(4, 12, 3),
            gs::rgb4(2, 2, 2), gs::rgb4(8, 10, 11), gs::rgb4(13, 3, 2), gs::rgb4(11, 8, 4), gs::rgb4(6, 5, 3),
            gs::rgb4(14, 15, 15), gs::rgb4(1, 1, 1), gs::rgb4(3, 3, 4), gs::rgb4(15, 13, 3), line});
    setPal(vdp, PAL_TUFT, {0, gs::rgb4(8, 14, 4), gs::rgb4(4, 9, 2), gs::rgb4(11, 15, 6), gs::rgb4(2, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_REED, {0, gs::rgb4(6, 11, 3), gs::rgb4(3, 7, 2), gs::rgb4(2, 4, 2), gs::rgb4(1, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(10, 7, 3), gs::rgb4(13, 5, 3), gs::rgb4(5, 4, 2), gs::rgb4(14, 13, 10), gs::rgb4(3, 2, 1),
            gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 15, 14), gs::rgb4(14, 3, 2), gs::rgb4(15, 13, 3), gs::rgb4(8, 6, 3), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 13, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(14, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(15, 8, 2), gs::rgb4(12, 4, 1), gs::rgb4(15, 14, 8), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(8, 14, 13), gs::rgb4(2, 6, 9), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(10, 15, 6), gs::rgb4(4, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 1), line});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1), line});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 8), gs::rgb4(8, 7, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 1), line});
    fieldPal(vdp, PAL_FIELD, false);
    fieldPal(vdp, PAL_ENDF, true);

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.hull[i] = gs::uploadMipped(vdp, paintSkiff(i * kTau / 16.f));
    art.tuft = gs::uploadMipped(vdp, paintTuft());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.dash = gs::uploadMipped(vdp, paintDash());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(true));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(false));
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.ring = gs::uploadMipped(vdp, paintRing());
    art.dot = gs::uploadMipped(vdp, paintDot());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.panel = gs::uploadMipped(vdp, paintPanel());
    art.title = word(vdp, "SKIFF GRASS", 3);
    art.fullStop = word(vdp, "FULL STOP", 3);
    art.onGrass = word(vdp, "ON THE GRASS", 2);
    art.missed = word(vdp, "MISSED THE END", 2);
    art.offGrass = word(vdp, "OFF THE GRASS", 2);
    art.legFail = word(vdp, "THE LEG FAILS", 2);
    art.paused = word(vdp, "PAUSED", 3);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.setFogColor(gs::rgb4(2, 6, 8));
}

}  // namespace skiffgrass
