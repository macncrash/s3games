#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace cabslip {
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

// Yellow water cab. Local +ly is the bow. Heading 0 faces east.
Bitmap paintCab(float heading) {
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
    poly({{0.f, 36.f}, {14.f, 22.f}, {16.f, 6.f}, {15.f, -18.f}, {10.f, -28.f}, {-10.f, -28.f}, {-15.f, -18.f}, {-16.f, 6.f}, {-14.f, 22.f}}, 8);
    poly({{0.f, 32.f}, {12.f, 20.f}, {13.f, 4.f}, {12.f, -16.f}, {8.f, -24.f}, {-8.f, -24.f}, {-12.f, -16.f}, {-13.f, 4.f}, {-12.f, 20.f}}, 1);
    poly({{0.f, 28.f}, {9.f, 16.f}, {10.f, 2.f}, {9.f, -12.f}, {5.f, -20.f}, {-5.f, -20.f}, {-9.f, -12.f}, {-10.f, 2.f}, {-9.f, 16.f}}, 2);
    poly({{-8.f, 8.f}, {8.f, 8.f}, {8.f, -10.f}, {-8.f, -10.f}}, 6);
    poly({{-6.f, 6.f}, {-1.f, 6.f}, {-1.f, -8.f}, {-6.f, -8.f}}, 5);
    poly({{1.f, 6.f}, {6.f, 6.f}, {6.f, -8.f}, {1.f, -8.f}}, 5);
    stroke(-13.f, 14.f, -12.f, -14.f, 4, 2.2f);
    stroke(13.f, 14.f, 12.f, -14.f, 3, 2.2f);
    stroke(-11.f, 10.f, -10.f, -10.f, 11, 1.6f);
    stroke(11.f, 10.f, 10.f, -10.f, 11, 1.6f);
    blob(0.f, 4.f, 2.2f, 2.2f, 13);
    blob(0.f, 4.f, 1.1f, 1.1f, 7);
    poly({{-3.f, -22.f}, {3.f, -22.f}, {4.f, -32.f}, {-4.f, -32.f}}, 12);
    blob(-14.f, 0.f, 2.4f, 3.2f, 10);
    blob(14.f, 0.f, 2.4f, 3.2f, 10);
    b.outline(15, false);
    return b;
}

Bitmap paintPierH() {
    Bitmap b(120, 40);
    b.rect(2, 4, 116, 32, 1);
    for (int i = 0; i < 10; i++) b.rect(4 + i * 11, 6, 9, 28, (i & 1) ? 2 : 1);
    b.rect(2, 4, 116, 3, 4);
    b.rect(2, 33, 116, 3, 3);
    b.outline(5, false);
    return b;
}

Bitmap paintPierV() {
    Bitmap b(28, 140);
    b.rect(4, 2, 20, 136, 1);
    for (int i = 0; i < 12; i++) b.rect(6, 4 + i * 11, 16, 9, (i & 1) ? 2 : 1);
    b.rect(4, 2, 3, 136, 4);
    b.rect(21, 2, 3, 136, 3);
    b.outline(5, false);
    return b;
}

Bitmap paintCleat() {
    Bitmap b(22, 12);
    b.rect(2, 4, 18, 4, 1);
    b.rect(4, 2, 3, 8, 2);
    b.rect(15, 2, 3, 8, 2);
    b.outline(3, false);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(16, 36);
    b.rect(7, 10, 2, 22, 1);
    b.ellipse(8, 8, 5, 5, 2);
    b.ellipse(8, 7, 2, 2, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(18, 28);
    b.ellipse(9, 16, 7, 8, 1);
    b.rect(7, 6, 4, 8, 2);
    b.ellipse(9, 8, 3, 3, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(20, 28);
    b.rect(3, 2, 2, 24, 1);
    b.poly({{5, 3}, {17, 8}, {5, 13}}, 2);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(28, 12);
    b.ellipse(8, 6, 6, 3, 1);
    b.ellipse(18, 6, 7, 4, 1);
    b.ellipse(13, 5, 4, 2, 2);
    return b;
}

Bitmap paintPin() {
    Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
    return b;
}

Bitmap paintDot() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

Bitmap paintPanel() {
    Bitmap b(64, 88);
    b.rect(1, 1, 62, 86, 1);
    b.rect(4, 4, 56, 80, 2);
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 13);
    const uint16_t line = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 9, 8), gs::rgb4(15, 12, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3), line});
    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(15, 13, 2), gs::rgb4(12, 9, 1), gs::rgb4(2, 2, 2), gs::rgb4(14, 14, 13), gs::rgb4(6, 10, 13),
            gs::rgb4(4, 4, 5), gs::rgb4(15, 8, 1), gs::rgb4(2, 5, 4), gs::rgb4(14, 15, 15), gs::rgb4(8, 3, 2),
            gs::rgb4(13, 2, 2), gs::rgb4(10, 10, 11), gs::rgb4(15, 14, 6), gs::rgb4(3, 3, 3), line});
    setPal(vdp, PAL_PIER,
           {0, gs::rgb4(11, 9, 6), gs::rgb4(8, 7, 5), gs::rgb4(4, 3, 2), gs::rgb4(6, 5, 3), gs::rgb4(2, 2, 2),
            0, 0, 0, 0, 0, 0, 0, 0, 0, line});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(14, 12, 4), gs::rgb4(15, 4, 3), gs::rgb4(3, 3, 3), line, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line});
    setPal(vdp, PAL_WIN, {0, ink, gs::rgb4(4, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 2), line});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 13, 12), gs::rgb4(15, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 1), line});
    setPal(vdp, PAL_BANNER, {0, ink, gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 4), line});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(5, 5, 6), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 12), line, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(2, 4, 6), gs::rgb4(1, 2, 4), gs::rgb4(10, 12, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line});
    setPal(vdp, PAL_SLIP, {0, gs::rgb4(3, 8, 10), gs::rgb4(5, 11, 12), gs::rgb4(8, 12, 11), gs::rgb4(2, 5, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SHORE, {0, gs::rgb4(8, 7, 4), gs::rgb4(6, 5, 3), gs::rgb4(10, 9, 6), gs::rgb4(4, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    vdp.setFogColor(gs::rgb4(2, 4, 6));

    for (int i = 0; i < 8; i++) art.cab[i] = gs::uploadMipped(vdp, paintCab(i * kTau / 8.f));
    art.pierH = gs::uploadMipped(vdp, paintPierH());
    art.pierV = gs::uploadMipped(vdp, paintPierV());
    art.cleat = gs::uploadMipped(vdp, paintCleat());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.dot = gs::uploadMipped(vdp, paintDot());
    art.panel = gs::uploadMipped(vdp, paintPanel());
    art.title = word(vdp, "CAB SLIP", 3);
    art.berthed = word(vdp, "BERTHED", 3);
    art.inSlip = word(vdp, "IN THE SLIP", 2);
    art.tide = word(vdp, "TIDE TURNED", 2);
    art.scraped = word(vdp, "SCRAPED THE PIER", 2);
    art.missed = word(vdp, "MISSED THE END", 2);
    art.leg = word(vdp, "LEG FAILED", 2);
    art.paused = word(vdp, "PAUSED", 3);
    loadFont(vdp, art);
}

}  // namespace cabslip
