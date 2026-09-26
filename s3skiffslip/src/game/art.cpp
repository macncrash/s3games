#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace skiffslip {
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

// Pointed harbor skiff: cream deck, blue bottom, outboard, hanging fenders.
Bitmap paintSkiff(float heading) {
    Bitmap b(128, 128);
    const float cx = 64.f, cy = 64.f;
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
    poly({{0.f, 42.f}, {16.f, 26.f}, {18.f, 4.f}, {16.f, -16.f}, {11.f, -30.f}, {-11.f, -30.f}, {-16.f, -16.f}, {-18.f, 4.f}, {-16.f, 26.f}}, 3);
    poly({{0.f, 36.f}, {13.f, 22.f}, {14.f, 4.f}, {12.f, -14.f}, {8.f, -26.f}, {-8.f, -26.f}, {-12.f, -14.f}, {-14.f, 4.f}, {-13.f, 22.f}}, 1);
    poly({{0.f, 30.f}, {9.f, 18.f}, {9.f, 2.f}, {7.f, -12.f}, {4.f, -22.f}, {-4.f, -22.f}, {-7.f, -12.f}, {-9.f, 2.f}, {-9.f, 18.f}}, 2);
    poly({{0.f, 16.f}, {7.f, 8.f}, {7.f, -10.f}, {4.f, -18.f}, {-4.f, -18.f}, {-7.f, -10.f}, {-7.f, 8.f}}, 5);
    stroke(-11.f, 18.f, -10.f, -16.f, 11, 2.0f);
    stroke(11.f, 18.f, 10.f, -16.f, 11, 2.0f);
    stroke(-7.f, 6.f, 7.f, 6.f, 6, 1.8f);
    stroke(-6.f, -6.f, 6.f, -6.f, 6, 1.6f);
    poly({{-4.f, -26.f}, {4.f, -26.f}, {5.f, -40.f}, {-5.f, -40.f}}, 7);
    blob(0.f, -40.f, 3.6f, 2.4f, 8);
    stroke(0.f, -24.f, 3.f, -16.f, 12, 1.5f);
    blob(16.f, 2.f, 3.4f, 4.4f, 9);
    blob(-16.f, 2.f, 3.4f, 4.4f, 9);
    blob(15.f, -14.f, 3.0f, 4.0f, 10);
    blob(-15.f, -14.f, 3.0f, 4.0f, 10);
    blob(0.f, 32.f, 1.8f, 1.8f, 13);
    b.outline(15, false);
    return b;
}

Bitmap paintPier() {
    Bitmap b(64, 98);
    b.rect(4, 2, 56, 94, 1);
    for (int i = 0; i < 8; i++) b.rect(6, 4 + i * 12, 52, 9, (i & 1) ? 2 : 1);
    b.rect(8, 3, 2, 92, 3);
    b.rect(54, 3, 2, 92, 3);
    b.rect(2, 2, 5, 94, 4);
    b.rect(57, 2, 5, 94, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintHead() {
    Bitmap b(180, 28);
    b.rect(2, 6, 176, 16, 1);
    for (int i = 0; i < 14; i++) b.rect(6 + i * 12, 8, 9, 12, (i & 1) ? 2 : 1);
    b.rect(2, 4, 176, 4, 4);
    b.rect(2, 20, 176, 4, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintPile() {
    Bitmap b(22, 34);
    b.ellipse(11, 26, 8, 5, 2);
    b.rect(8, 8, 6, 18, 1);
    b.rect(9, 10, 2, 12, 5);
    b.ellipse(11, 8, 7, 4, 3);
    b.ellipse(11, 7, 3, 2, 4);
    b.outline(15, false);
    return b;
}

Bitmap paintCleat() {
    Bitmap b(18, 12);
    b.rect(2, 5, 14, 3, 1);
    b.rect(3, 2, 3, 8, 2);
    b.rect(12, 2, 3, 8, 2);
    b.ellipse(9, 6, 2, 2, 3);
    return b;
}

Bitmap paintLadder() {
    Bitmap b(16, 40);
    b.rect(2, 2, 3, 36, 1);
    b.rect(11, 2, 3, 36, 1);
    for (int i = 0; i < 6; i++) b.rect(2, 4 + i * 6, 12, 2, 2);
    return b;
}

Bitmap paintShed() {
    Bitmap b(72, 54);
    b.ellipse(36, 40, 28, 10, 3);
    b.rect(12, 22, 48, 20, 1);
    b.poly({{8, 24}, {36, 8}, {64, 24}}, 2);
    b.rect(32, 28, 10, 14, 4);
    b.rect(18, 28, 8, 7, 5);
    b.rect(46, 28, 8, 7, 5);
    b.outline(15, false);
    return b;
}

Bitmap paintTuft() {
    Bitmap b(26, 26);
    b.ellipse(13, 16, 9, 6, 7);
    b.ellipse(9, 13, 4, 5, 6);
    b.ellipse(17, 12, 4, 5, 10);
    b.line(8, 16, 6, 6, 6, 1.4f);
    b.line(13, 17, 13, 4, 6, 1.4f);
    b.line(18, 16, 20, 6, 10, 1.4f);
    return b;
}

Bitmap paintBuoy(bool red) {
    Bitmap b(22, 32);
    int body = red ? 1 : 2;
    b.ellipse(11, 18, 8, 8, body);
    b.ellipse(11, 18, 8, 3, 5);
    b.ellipse(8, 15, 2, 2, 3);
    b.rect(10, 4, 2, 8, 3);
    b.ellipse(11, 26, 3, 2, 4);
    b.outline(4, false);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(22, 36);
    b.rect(4, 10, 3, 22, 4);
    b.poly({{7, 8}, {19, 13}, {7, 20}}, 2);
    b.poly({{7, 12}, {15, 14}, {7, 17}}, 5);
    b.ellipse(5, 8, 2, 2, 3);
    b.outline(15, false);
    return b;
}

Bitmap paintStaff() {
    Bitmap b(14, 52);
    b.rect(6, 4, 3, 44, 3);
    for (int i = 0; i < 7; i++) b.rect(4, 6 + i * 6, 7, 3, (i & 1) ? 2 : 1);
    b.rect(3, 44, 8, 5, 4);
    return b;
}

Bitmap paintBobber() {
    Bitmap b(12, 12);
    b.poly({{6, 1}, {11, 6}, {6, 11}, {1, 6}}, 2);
    b.poly({{6, 4}, {8, 6}, {6, 8}, {4, 6}}, 1);
    return b;
}

Bitmap paintHeron(bool peck) {
    Bitmap b(28, 44);
    b.line(12, 40, 10, 28, 4, 1.4f);
    b.line(16, 40, 16, 28, 4, 1.4f);
    b.ellipse(14, 24, 6, 5, 2);
    float neck = peck ? 16.f : 10.f;
    b.line(14, 22, 18, neck, 2, 1.6f);
    b.ellipse(18, neck - 1.f, 2.2f, 2.2f, 2);
    b.line(18, neck - 1.f, peck ? 24.f : 22.f, neck - 1.f, 3, 1.3f);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(32, 16);
    b.ellipse(16, 9, 3, 2, 1);
    float tip = up ? 2.f : 12.f;
    b.line(16, 8, 2, tip, 1, 1.5f);
    b.line(16, 8, 30, tip, 1, 1.5f);
    b.set(20, 8, 3);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(18, 12);
    b.ellipse(9, 6, 7, 3, 1);
    b.ellipse(9, 6, 3, 2, 2);
    return b;
}

Bitmap paintDash() {
    Bitmap b(18, 6);
    b.rect(0, 1, 18, 4, 1);
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
    Bitmap b(68, 96);
    b.rect(1, 1, 66, 94, 1);
    b.rect(4, 4, 60, 88, 2);
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
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t line = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 9, 8), gs::rgb4(15, 13, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3), line});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(13, 12, 9), gs::rgb4(15, 14, 11), gs::rgb4(2, 5, 9), gs::rgb4(1, 3, 6), gs::rgb4(8, 5, 3),
            gs::rgb4(11, 8, 5), gs::rgb4(4, 4, 5), gs::rgb4(9, 9, 10), gs::rgb4(15, 15, 14), gs::rgb4(11, 12, 12),
            gs::rgb4(12, 3, 2), gs::rgb4(3, 3, 3), gs::rgb4(13, 11, 4), gs::rgb4(2, 2, 2), line});
    setPal(vdp, PAL_PIER,
           {0, gs::rgb4(12, 10, 7), gs::rgb4(9, 8, 6), gs::rgb4(5, 4, 3), gs::rgb4(7, 6, 4), gs::rgb4(2, 2, 2),
            gs::rgb4(14, 13, 10), 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_PILE,
           {0, gs::rgb4(8, 7, 5), gs::rgb4(4, 4, 3), gs::rgb4(14, 14, 12), gs::rgb4(15, 15, 14), gs::rgb4(5, 8, 4),
            0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_MARK,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(13, 3, 2), gs::rgb4(15, 12, 4), gs::rgb4(7, 5, 3), gs::rgb4(8, 2, 2),
            0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 12, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_BIRD,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(7, 8, 9), gs::rgb4(14, 11, 3), gs::rgb4(3, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_TIDE,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(13, 3, 2), gs::rgb4(2, 2, 2), gs::rgb4(8, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_BUOY,
           {0, gs::rgb4(14, 3, 2), gs::rgb4(3, 10, 4), gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 2), gs::rgb4(12, 10, 8),
            0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_SHED,
           {0, gs::rgb4(12, 11, 8), gs::rgb4(11, 4, 3), gs::rgb4(4, 4, 3), gs::rgb4(3, 2, 2), gs::rgb4(8, 12, 13),
            0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(12, 14, 13), gs::rgb4(2, 5, 8), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(10, 15, 6), gs::rgb4(4, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 1), line});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 1), line});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 8), gs::rgb4(8, 7, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 1), line});
    setPal(vdp, PAL_SLIP,
           {0, gs::rgb4(6, 7, 4), gs::rgb4(4, 5, 3), gs::rgb4(8, 8, 4), gs::rgb4(10, 9, 6), gs::rgb4(7, 6, 4),
            gs::rgb4(2, 5, 8), gs::rgb4(1, 4, 7), gs::rgb4(3, 6, 5), gs::rgb4(5, 8, 8), gs::rgb4(8, 10, 8),
            gs::rgb4(1, 5, 9), gs::rgb4(2, 7, 11), gs::rgb4(10, 13, 13), gs::rgb4(4, 6, 5), line});
    setPal(vdp, PAL_SHORE,
           {0, gs::rgb4(7, 7, 4), gs::rgb4(4, 5, 3), gs::rgb4(8, 8, 4), gs::rgb4(10, 9, 6), gs::rgb4(6, 6, 3),
            gs::rgb4(6, 8, 3), gs::rgb4(5, 7, 3), gs::rgb4(4, 5, 2), gs::rgb4(7, 8, 4), gs::rgb4(8, 9, 5),
            0, 0, 0, gs::rgb4(3, 3, 2), gs::rgb4(9, 10, 5)});
    vdp.setFogColor(gs::rgb4(3, 5, 8));

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.hull[i] = gs::uploadMipped(vdp, paintSkiff(i * kTau / 16.f));
    art.pier = gs::uploadMipped(vdp, paintPier());
    art.head = gs::uploadMipped(vdp, paintHead());
    art.pile = gs::uploadMipped(vdp, paintPile());
    art.cleat = gs::uploadMipped(vdp, paintCleat());
    art.ladder = gs::uploadMipped(vdp, paintLadder());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.tuft = gs::uploadMipped(vdp, paintTuft());
    art.buoyR = gs::uploadMipped(vdp, paintBuoy(true));
    art.buoyG = gs::uploadMipped(vdp, paintBuoy(false));
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.staff = gs::uploadMipped(vdp, paintStaff());
    art.bobber = gs::uploadMipped(vdp, paintBobber());
    art.heron[0] = gs::uploadMipped(vdp, paintHeron(false));
    art.heron[1] = gs::uploadMipped(vdp, paintHeron(true));
    art.gull[0] = gs::uploadMipped(vdp, paintGull(true));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(false));
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.dash = gs::uploadMipped(vdp, paintDash());
    art.dot = gs::uploadMipped(vdp, paintDot());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.panel = gs::uploadMipped(vdp, paintPanel());
    art.title = word(vdp, "SKIFF SLIP", 3);
    art.berthed = word(vdp, "BERTHED", 3);
    art.inSlip = word(vdp, "IN THE SLIP", 2);
    art.missed = word(vdp, "MISSED THE END", 2);
    art.tide = word(vdp, "TIDE TURNED", 2);
    art.scraped = word(vdp, "SCRAPED THE PIER", 2);
    art.leg = word(vdp, "LEG FAILED", 2);
    art.paused = word(vdp, "PAUSED", 3);
    art.endMark = word(vdp, "END", 2);
}

}  // namespace skiffslip
