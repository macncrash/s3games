#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace railgrass {
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

// Boxy rail car. Local +y is the nose that runs up the grass.
Bitmap paintCar(float heading) {
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
    poly({{-16.f, 30.f}, {16.f, 30.f}, {16.f, -28.f}, {-16.f, -28.f}}, 3);
    poly({{-12.f, 26.f}, {12.f, 26.f}, {12.f, -22.f}, {-12.f, -22.f}}, 1);
    poly({{-8.f, 18.f}, {8.f, 18.f}, {8.f, -8.f}, {-8.f, -8.f}}, 2);
    stroke(-16.f, 8.f, 16.f, 8.f, 4, 2.2f);
    stroke(-16.f, -10.f, 16.f, -10.f, 4, 2.2f);
    blob(-13.f, 20.f, 3.2f, 4.4f, 5);
    blob(13.f, 20.f, 3.2f, 4.4f, 5);
    blob(-13.f, -16.f, 3.2f, 4.4f, 5);
    blob(13.f, -16.f, 3.2f, 4.4f, 5);
    stroke(0.f, 6.f, 0.f, 34.f, 6, 1.6f);
    stroke(-7.f, 34.f, 7.f, 34.f, 6, 1.8f);
    blob(0.f, 22.f, 2.4f, 2.0f, 14);
    poly({{-6.f, 28.f}, {6.f, 28.f}, {4.f, 24.f}, {-4.f, 24.f}}, 7);
    stroke(-10.f, -24.f, 10.f, -24.f, 8, 2.0f);
    b.outline(12, false);
    return b;
}

Bitmap paintTuft() {
    Bitmap b(26, 26);
    b.ellipse(13, 16, 9, 6, 2);
    b.ellipse(9, 13, 4, 5, 1);
    b.ellipse(17, 12, 4, 5, 3);
    b.line(7, 17, 5, 7, 1, 1.3f);
    b.line(13, 17, 13, 5, 1, 1.3f);
    b.line(19, 17, 21, 7, 3, 1.3f);
    return b;
}

Bitmap paintSleeper() {
    Bitmap b(28, 10);
    b.rect(1, 2, 26, 6, 1);
    b.rect(3, 3, 4, 4, 2);
    b.rect(21, 3, 4, 4, 2);
    return b;
}

Bitmap paintSignal() {
    Bitmap b(18, 36);
    b.rect(8, 12, 3, 20, 1);
    b.ellipse(9, 10, 6, 6, 2);
    b.ellipse(9, 10, 3, 3, 3);
    b.ellipse(9, 28, 4, 2, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(22, 32);
    b.rect(4, 8, 3, 20, 4);
    b.poly({{7, 8}, {18, 13}, {7, 18}}, 2);
    b.poly({{7, 11}, {15, 14}, {7, 16}}, 3);
    b.ellipse(5, 8, 2, 2, 1);
    return b;
}

Bitmap paintDash() {
    Bitmap b(18, 6);
    b.rect(0, 1, 18, 4, 1);
    return b;
}

Bitmap paintHalt() {
    Bitmap b(70, 44);
    b.rect(8, 18, 54, 16, 1);
    b.poly({{4, 20}, {35, 6}, {66, 20}}, 2);
    b.rect(30, 22, 12, 12, 5);
    b.rect(14, 22, 10, 7, 4);
    b.rect(46, 22, 10, 7, 4);
    b.rect(6, 30, 58, 4, 3);
    b.outline(6, false);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(14, 28);
    b.rect(6, 10, 2, 14, 1);
    b.ellipse(7, 8, 4, 4, 2);
    b.ellipse(7, 8, 2, 2, 3);
    return b;
}

Bitmap paintBird(bool up) {
    Bitmap b(28, 14);
    b.ellipse(14, 8, 3, 2, 1);
    float tip = up ? 2.f : 11.f;
    b.line(14, 7, 2, tip, 1, 1.3f);
    b.line(14, 7, 26, tip, 1, 1.3f);
    return b;
}

Bitmap paintSpark() {
    Bitmap b(12, 12);
    b.line(2, 10, 6, 4, 1, 1.4f);
    b.line(6, 4, 5, 8, 2, 1.2f);
    b.line(5, 8, 10, 2, 1, 1.3f);
    return b;
}

Bitmap paintRing() {
    Bitmap b(64, 64);
    const float c = 31.5f, r = 26.f;
    for (int d = 0; d < 360; d += 10) {
        float a0 = d * 3.14159265f / 180.f;
        float a1 = (d + 5.f) * 3.14159265f / 180.f;
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
    Bitmap b(52, 74);
    b.rect(2, 2, 48, 70, 1);
    b.rect(4, 4, 44, 66, 2);
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
    uint16_t c[16] = {};
    c[1] = endBand ? gs::rgb4(9, 14, 6) : gs::rgb4(5, 11, 3);
    c[2] = endBand ? gs::rgb4(6, 11, 3) : gs::rgb4(3, 7, 2);
    c[3] = gs::rgb4(10, 12, 5);
    c[4] = gs::rgb4(11, 12, 5);
    c[5] = gs::rgb4(6, 8, 3);
    c[6] = endBand ? gs::rgb4(13, 15, 8) : gs::rgb4(7, 13, 4);
    c[7] = endBand ? gs::rgb4(8, 13, 4) : gs::rgb4(3, 8, 2);
    c[8] = gs::rgb4(2, 5, 2);
    c[9] = gs::rgb4(4, 7, 2);
    c[10] = endBand ? gs::rgb4(15, 15, 12) : gs::rgb4(12, 14, 7);
    c[11] = gs::rgb4(6, 5, 3);
    c[12] = gs::rgb4(4, 4, 2);
    c[13] = gs::rgb4(8, 7, 4);
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
    setPal(vdp, PAL_CAR,
           {0, gs::rgb4(14, 12, 8), gs::rgb4(11, 4, 3), gs::rgb4(6, 5, 4), gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 2),
            gs::rgb4(9, 9, 10), gs::rgb4(15, 13, 4), gs::rgb4(12, 3, 2), gs::rgb4(8, 7, 5), gs::rgb4(15, 15, 13),
            gs::rgb4(4, 8, 12), gs::rgb4(1, 1, 1), gs::rgb4(7, 6, 4), gs::rgb4(15, 14, 6), line});
    setPal(vdp, PAL_TUFT, {0, gs::rgb4(8, 14, 4), gs::rgb4(4, 9, 2), gs::rgb4(12, 15, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_RAIL,
           {0, gs::rgb4(7, 6, 5), gs::rgb4(12, 11, 9), gs::rgb4(3, 3, 3), gs::rgb4(9, 4, 2), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(11, 8, 4), gs::rgb4(13, 5, 3), gs::rgb4(5, 4, 2), gs::rgb4(14, 13, 9), gs::rgb4(3, 2, 1),
            gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 15, 14), gs::rgb4(14, 3, 2), gs::rgb4(15, 13, 3), gs::rgb4(8, 6, 3), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_SPARK, {0, gs::rgb4(15, 15, 10), gs::rgb4(14, 10, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(15, 15, 15), gs::rgb4(4, 4, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(5, 5, 5), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(8, 12, 7), gs::rgb4(3, 5, 3), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(10, 15, 6), gs::rgb4(4, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 1), line});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1), line});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 8), gs::rgb4(8, 7, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 1), line});
    fieldPal(vdp, PAL_FIELD, false);
    fieldPal(vdp, PAL_ENDF, true);

    loadFont(vdp, art);
    for (int i = 0; i < 8; i++) art.car[i] = gs::uploadMipped(vdp, paintCar(i * kTau / 8.f));
    art.tuft = gs::uploadMipped(vdp, paintTuft());
    art.sleeper = gs::uploadMipped(vdp, paintSleeper());
    art.signal = gs::uploadMipped(vdp, paintSignal());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.dash = gs::uploadMipped(vdp, paintDash());
    art.halt = gs::uploadMipped(vdp, paintHalt());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.bird[0] = gs::uploadMipped(vdp, paintBird(true));
    art.bird[1] = gs::uploadMipped(vdp, paintBird(false));
    art.spark = gs::uploadMipped(vdp, paintSpark());
    art.ring = gs::uploadMipped(vdp, paintRing());
    art.dot = gs::uploadMipped(vdp, paintDot());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.panel = gs::uploadMipped(vdp, paintPanel());
    art.title = word(vdp, "RAIL GRASS", 3);
    art.fullStop = word(vdp, "FULL STOP", 3);
    art.onGrass = word(vdp, "ON THE GRASS", 2);
    art.missed = word(vdp, "MISSED THE END", 2);
    art.legFail = word(vdp, "THE LEG FAILS", 2);
    art.paused = word(vdp, "PAUSED", 3);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.setFogColor(gs::rgb4(3, 5, 3));
}

}  // namespace railgrass
