#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace skiffboom {
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

// Flat-bow work skiff, painted in local space so the bow follows `heading`.
// Indices: 1 deck, 2 deck hi, 3 hull, 4 boot, 5 cabin, 6 roof, 7 motor,
// 8 metal, 9 fender, 10 white, 11 stripe, 12 glass, 13 bitt, 15 outline.
Bitmap paintHull(float heading) {
    Bitmap b(112, 112);
    const float cx = 56.f, cy = 56.f;
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
    poly({{-13.f, 34.f}, {13.f, 34.f}, {16.f, 20.f}, {16.f, -16.f}, {11.f, -36.f}, {-11.f, -36.f}, {-16.f, -16.f}, {-16.f, 20.f}}, 3);
    poly({{-10.f, 30.f}, {10.f, 30.f}, {12.f, 16.f}, {12.f, -14.f}, {8.f, -30.f}, {-8.f, -30.f}, {-12.f, -14.f}, {-12.f, 16.f}}, 4);
    poly({{-8.f, 28.f}, {8.f, 28.f}, {9.f, 14.f}, {9.f, -12.f}, {6.f, -26.f}, {-6.f, -26.f}, {-9.f, -12.f}, {-9.f, 14.f}}, 1);
    poly({{-5.f, 22.f}, {5.f, 22.f}, {5.f, 8.f}, {-5.f, 8.f}}, 2);
    poly({{-7.f, 4.f}, {7.f, 4.f}, {7.f, -12.f}, {-7.f, -12.f}}, 5);
    poly({{-5.f, 2.f}, {5.f, 2.f}, {5.f, -8.f}, {-5.f, -8.f}}, 6);
    poly({{-4.f, 3.f}, {4.f, 3.f}, {4.f, 0.f}, {-4.f, 0.f}}, 12);
    poly({{-3.5f, -28.f}, {3.5f, -28.f}, {4.5f, -40.f}, {-4.5f, -40.f}}, 7);
    blob(0.f, -40.f, 3.2f, 2.2f, 8);
    stroke(-9.f, 16.f, -8.f, -18.f, 11, 1.6f);
    stroke(9.f, 16.f, 8.f, -18.f, 11, 1.6f);
    blob(15.f, 2.f, 2.6f, 4.2f, 9);
    blob(-15.f, 2.f, 2.6f, 4.2f, 9);
    blob(0.f, 26.f, 1.8f, 1.8f, 13);
    blob(0.f, -18.f, 1.5f, 1.5f, 10);
    b.outline(15, false);
    return b;
}

// The drive: three branded logs lashed as one bundle. Long axis is local +y.
Bitmap paintDrive(float heading) {
    Bitmap b(72, 72);
    const float cx = 36.f, cy = 36.f;
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
    auto cap = [&](float lx, float ly) {
        poly({{lx - 3.2f, ly + 2.f}, {lx + 3.2f, ly + 2.f}, {lx + 2.4f, ly - 2.f}, {lx - 2.4f, ly - 2.f}}, 4);
    };
    poly({{-3.2f, 22.f}, {3.2f, 22.f}, {4.f, 0.f}, {3.2f, -22.f}, {-3.2f, -22.f}, {-4.f, 0.f}}, 1);
    poly({{-9.f, 20.f}, {-4.2f, 20.f}, {-3.6f, 0.f}, {-4.2f, -20.f}, {-9.4f, -20.f}, {-10.f, 0.f}}, 2);
    poly({{4.2f, 20.f}, {9.f, 20.f}, {10.f, 0.f}, {9.4f, -20.f}, {4.2f, -20.f}, {3.6f, 0.f}}, 3);
    cap(0.f, 22.f);
    cap(-6.6f, 20.f);
    cap(6.6f, 20.f);
    stroke(-12.f, 8.f, 12.f, 8.f, 5, 2.1f);
    stroke(-12.f, -8.f, 12.f, -8.f, 5, 2.1f);
    stroke(0.f, 14.f, 0.f, -14.f, 6, 1.4f);
    b.outline(15, false);
    return b;
}

Bitmap paintLogV() {
    Bitmap b(22, 96);
    b.rect(4, 4, 14, 88, 1);
    b.rect(6, 6, 4, 84, 2);
    b.rect(14, 8, 3, 80, 3);
    for (int i = 0; i < 7; i++) b.rect(5, 10 + i * 12, 12, 2, 5);
    b.ellipse(11, 6, 8, 4, 4);
    b.ellipse(11, 90, 8, 4, 3);
    b.outline(15, false);
    return b;
}

Bitmap paintLogH() {
    Bitmap b(96, 22);
    b.rect(4, 4, 88, 14, 1);
    b.rect(6, 6, 84, 4, 2);
    b.rect(8, 14, 80, 3, 3);
    for (int i = 0; i < 7; i++) b.rect(10 + i * 12, 5, 2, 12, 5);
    b.ellipse(6, 11, 4, 8, 4);
    b.ellipse(90, 11, 4, 8, 3);
    b.outline(15, false);
    return b;
}

Bitmap paintCrib() {
    Bitmap b(40, 40);
    b.rect(6, 6, 28, 28, 3);
    b.rect(4, 8, 32, 5, 1);
    b.rect(4, 27, 32, 5, 1);
    b.rect(8, 4, 5, 32, 2);
    b.rect(27, 4, 5, 32, 2);
    b.rect(14, 14, 12, 12, 4);
    b.rect(16, 16, 4, 4, 5);
    b.outline(15, false);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(20, 30);
    b.ellipse(10, 16, 7, 7, 1);
    b.ellipse(10, 16, 7, 3, 2);
    b.rect(9, 4, 2, 8, 3);
    b.ellipse(10, 4, 3, 2, 2);
    b.ellipse(10, 24, 3, 2, 4);
    b.outline(15, false);
    return b;
}

Bitmap paintPile() {
    Bitmap b(26, 30);
    b.ellipse(13, 22, 9, 5, 2);
    b.rect(6, 8, 4, 16, 1);
    b.rect(12, 6, 4, 18, 1);
    b.rect(17, 9, 3, 14, 3);
    b.ellipse(8, 8, 3, 2, 4);
    b.ellipse(14, 6, 3, 2, 4);
    b.outline(15, false);
    return b;
}

Bitmap paintReed() {
    Bitmap b(24, 28);
    b.ellipse(12, 20, 8, 5, 2);
    b.line(8, 20, 5, 6, 1, 1.5f);
    b.line(12, 21, 12, 3, 1, 1.6f);
    b.line(16, 20, 19, 7, 3, 1.5f);
    b.line(10, 18, 7, 10, 3, 1.3f);
    return b;
}

Bitmap paintShed() {
    Bitmap b(70, 52);
    b.ellipse(36, 40, 26, 8, 3);
    b.rect(14, 22, 44, 18, 1);
    b.poly({{10, 24}, {36, 8}, {62, 24}}, 2);
    b.rect(32, 28, 10, 12, 4);
    b.rect(18, 28, 8, 6, 5);
    b.rect(46, 28, 8, 6, 5);
    b.rect(30, 16, 12, 4, 6);
    b.outline(15, false);
    return b;
}

Bitmap paintPost() {
    Bitmap b(14, 40);
    b.rect(5, 4, 4, 32, 1);
    b.rect(3, 32, 8, 4, 2);
    b.rect(4, 6, 6, 3, 3);
    return b;
}

Bitmap paintTender(bool up) {
    Bitmap b(24, 32);
    b.ellipse(12, 8, 4, 4, 1);
    b.rect(8, 4, 8, 3, 4);
    b.rect(9, 12, 6, 9, 2);
    b.rect(8, 20, 3, 8, 3);
    b.rect(13, 20, 3, 8, 3);
    if (up) b.line(15, 14, 20, 6, 2, 1.6f);
    else b.line(15, 14, 21, 14, 2, 1.6f);
    b.line(9, 14, 4, 18, 2, 1.6f);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(30, 16);
    b.ellipse(15, 9, 3, 2, 1);
    float tip = up ? 2.f : 12.f;
    b.line(15, 8, 2, tip, 1, 1.5f);
    b.line(15, 8, 28, tip, 1, 1.5f);
    b.set(18, 8, 3);
    return b;
}

Bitmap paintHeron() {
    Bitmap b(28, 42);
    b.line(11, 38, 10, 26, 4, 1.4f);
    b.line(16, 38, 15, 26, 4, 1.4f);
    b.ellipse(13, 22, 6, 5, 2);
    b.line(14, 20, 20, 10, 2, 1.6f);
    b.ellipse(20, 9, 2.2f, 2.2f, 2);
    b.line(20, 9, 26, 8, 3, 1.3f);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(18, 12);
    b.ellipse(9, 6, 7, 3, 1);
    b.ellipse(9, 6, 3, 2, 2);
    return b;
}

Bitmap paintDash() {
    Bitmap b(16, 6);
    b.rect(0, 1, 16, 4, 1);
    return b;
}

Bitmap paintDot() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

Bitmap paintPanel() {
    Bitmap b(54, 86);
    b.rect(1, 1, 52, 84, 1);
    b.rect(3, 3, 48, 80, 2);
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
           {0, gs::rgb4(13, 11, 7), gs::rgb4(15, 14, 10), gs::rgb4(3, 6, 4), gs::rgb4(2, 4, 3), gs::rgb4(9, 8, 6),
            gs::rgb4(5, 5, 4), gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 5), gs::rgb4(12, 10, 6), gs::rgb4(15, 15, 14),
            gs::rgb4(14, 12, 3), gs::rgb4(6, 10, 12), gs::rgb4(10, 6, 3), gs::rgb4(2, 2, 2), line});
    setPal(vdp, PAL_DRIVE,
           {0, gs::rgb4(12, 8, 4), gs::rgb4(14, 11, 6), gs::rgb4(8, 5, 3), gs::rgb4(15, 8, 2), gs::rgb4(13, 12, 8),
            gs::rgb4(6, 6, 7), gs::rgb4(15, 13, 9), 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 1), line});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(10, 10, 9), gs::rgb4(13, 13, 12), gs::rgb4(12, 3, 2), gs::rgb4(6, 2, 2), gs::rgb4(8, 8, 7),
            gs::rgb4(3, 3, 3), gs::rgb4(5, 5, 6), gs::rgb4(4, 4, 5), gs::rgb4(8, 7, 6), gs::rgb4(15, 14, 6),
            gs::rgb4(14, 12, 3), gs::rgb4(8, 10, 12), gs::rgb4(12, 8, 3), gs::rgb4(2, 2, 2), line});
    setPal(vdp, PAL_BOOM,
           {0, gs::rgb4(11, 8, 5), gs::rgb4(14, 11, 7), gs::rgb4(6, 4, 3), gs::rgb4(13, 12, 9), gs::rgb4(4, 6, 3),
            0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 1), line});
    setPal(vdp, PAL_CRIB,
           {0, gs::rgb4(10, 8, 5), gs::rgb4(7, 6, 4), gs::rgb4(5, 5, 4), gs::rgb4(8, 8, 7), gs::rgb4(12, 12, 11),
            0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 2), line});
    setPal(vdp, PAL_SHED,
           {0, gs::rgb4(12, 11, 8), gs::rgb4(10, 4, 3), gs::rgb4(4, 4, 3), gs::rgb4(3, 2, 2), gs::rgb4(8, 12, 13),
            gs::rgb4(14, 12, 6), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 1), line});
    setPal(vdp, PAL_BIRD,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(7, 8, 9), gs::rgb4(14, 11, 3), gs::rgb4(3, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 12, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_MARK,
           {0, gs::rgb4(15, 14, 6), gs::rgb4(14, 3, 2), gs::rgb4(15, 15, 14), gs::rgb4(4, 4, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(10, 15, 6), gs::rgb4(4, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 1), line});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 1), line});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 8), gs::rgb4(8, 7, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 1), line});
    setPal(vdp, PAL_WATER,
           {0, gs::rgb4(6, 8, 3), gs::rgb4(3, 5, 2), gs::rgb4(8, 9, 4), gs::rgb4(8, 7, 4), gs::rgb4(5, 5, 3),
            gs::rgb4(2, 5, 8), gs::rgb4(1, 4, 7), gs::rgb4(7, 6, 4), 0, 0, gs::rgb4(4, 9, 12), gs::rgb4(2, 6, 9),
            gs::rgb4(11, 14, 14), gs::rgb4(9, 8, 5), line});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(12, 14, 12), gs::rgb4(2, 5, 7), gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_CREW,
           {0, gs::rgb4(13, 9, 6), gs::rgb4(8, 4, 3), gs::rgb4(3, 3, 5), gs::rgb4(14, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    vdp.setFogColor(gs::rgb4(4, 6, 7));

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) {
        float h = i * kTau / 16.f;
        art.hull[i] = gs::uploadMipped(vdp, paintHull(h));
        art.drive[i] = gs::uploadMipped(vdp, paintDrive(h));
    }
    art.logV = gs::uploadMipped(vdp, paintLogV());
    art.logH = gs::uploadMipped(vdp, paintLogH());
    art.crib = gs::uploadMipped(vdp, paintCrib());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.pile = gs::uploadMipped(vdp, paintPile());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.tender[0] = gs::uploadMipped(vdp, paintTender(false));
    art.tender[1] = gs::uploadMipped(vdp, paintTender(true));
    art.gull[0] = gs::uploadMipped(vdp, paintGull(true));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(false));
    art.heron = gs::uploadMipped(vdp, paintHeron());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.dash = gs::uploadMipped(vdp, paintDash());
    art.dot = gs::uploadMipped(vdp, paintDot());
    art.panel = gs::uploadMipped(vdp, paintPanel());
    art.title = word(vdp, "SKIFF BOOM", 3);
    art.delivered = word(vdp, "DELIVERED", 3);
    art.onBoom = word(vdp, "DRIVE ON THE BOOM", 2);
    art.crewTook = word(vdp, "CREW TOOK THE BOOM", 2);
    art.driveLost = word(vdp, "DRIVE WENT OVER", 2);
    art.broke = word(vdp, "BROKE THE BOOM", 2);
    art.missed = word(vdp, "MISSED THE BOOM", 2);
    art.grounded = word(vdp, "GROUNDED", 2);
    art.paused = word(vdp, "PAUSED", 3);
    art.boomSign = word(vdp, "BOOM", 2);
    art.crewTag = word(vdp, "CREW", 2);
}

}  // namespace skiffboom
