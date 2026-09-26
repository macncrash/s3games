#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace sledmark {
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

// Dogs toward local +y, basket and musher toward local -y. Heading 0 points the team east.
Bitmap paintSled(float heading) {
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
    auto dog = [&](float x, float y) {
        blob(x, y - 3.2f, 4.4f, 3.1f, 2);
        blob(x, y + 1.2f, 6.6f, 4.1f, 1);
        blob(x, y + 7.4f, 3.7f, 3.2f, 1);
        blob(x - 1.7f, y + 9.6f, 1.6f, 1.9f, 12);
        blob(x + 1.5f, y + 8.8f, 0.9f, 0.9f, 9);
        stroke(x, y - 5.2f, x + 1.6f, y - 9.4f, 2, 2.1f);
        stroke(x - 2.3f, y - 1.f, x - 3.6f, y - 6.2f, 3, 1.8f);
        stroke(x + 2.3f, y - 1.f, x + 3.6f, y - 6.2f, 3, 1.8f);
    };

    stroke(-12.f, 18.f, -12.f, -38.f, 8, 4.2f);
    stroke(12.f, 18.f, 12.f, -38.f, 8, 4.2f);
    stroke(-12.f, 18.f, -12.f, -38.f, 7, 2.1f);
    stroke(12.f, 18.f, 12.f, -38.f, 7, 2.1f);
    stroke(-12.f, 16.f, 0.f, 26.f, 8, 3.6f);
    stroke(12.f, 16.f, 0.f, 26.f, 8, 3.6f);

    poly({{-11.f, 12.f}, {11.f, 12.f}, {10.f, -24.f}, {-10.f, -24.f}}, 5);
    poly({{-8.f, 8.f}, {8.f, 8.f}, {7.f, -18.f}, {-7.f, -18.f}}, 6);
    blob(-2.f, -1.f, 4.6f, 3.2f, 2);
    blob(3.f, -6.f, 3.4f, 2.4f, 11);

    stroke(-10.f, -28.f, 10.f, -28.f, 3, 3.3f);
    stroke(-8.f, -18.f, -8.f, -28.f, 3, 2.3f);
    stroke(8.f, -18.f, 8.f, -28.f, 3, 2.3f);

    poly({{-6.2f, -8.f}, {6.2f, -8.f}, {7.2f, -22.f}, {4.f, -26.f}, {-4.f, -26.f}, {-7.2f, -22.f}}, 4);
    blob(0.f, -6.f, 4.4f, 3.7f, 12);
    blob(0.f, -5.2f, 2.3f, 2.1f, 11);
    stroke(-5.f, -16.f, -10.f, -27.f, 4, 2.5f);
    stroke(5.f, -16.f, 10.f, -27.f, 4, 2.5f);
    blob(-10.f, -28.f, 2.1f, 1.7f, 10);
    blob(10.f, -28.f, 2.1f, 1.7f, 10);

    stroke(-8.f, 40.f, 0.f, 16.f, 13, 1.9f);
    stroke(8.f, 40.f, 0.f, 16.f, 13, 1.9f);
    stroke(-15.f, 24.f, 0.f, 14.f, 13, 1.9f);
    stroke(15.f, 24.f, 0.f, 14.f, 13, 1.9f);
    stroke(0.f, 16.f, 0.f, 10.f, 13, 2.3f);
    dog(-8.f, 46.f);
    dog(8.f, 46.f);
    dog(-15.f, 30.f);
    dog(15.f, 30.f);
    blob(0.f, 24.f, 2.3f, 2.3f, 14);

    b.outline(15, false);
    return b;
}

// Inner cream is the set circle. Height 36 maps a 22px radius to about 8 world units.
Bitmap paintDisc() {
    Bitmap b(96, 96);
    const float c = 47.5f;
    b.ellipse(c, c, 44, 44, 1);
    b.ellipse(c, c, 31, 31, 2);
    b.ellipse(c, c, 22, 22, 3);
    for (int d = 0; d < 360; d += 8) {
        float a0 = d * 3.14159265f / 180.f;
        float a1 = (d + 4.f) * 3.14159265f / 180.f;
        b.line(c + std::cos(a0) * 42.f, c + std::sin(a0) * 42.f, c + std::cos(a1) * 42.f, c + std::sin(a1) * 42.f, 5, 2.0f);
    }
    b.line(c, c - 14.f, c, c + 14.f, 4, 2.2f);
    b.line(c - 14.f, c, c + 14.f, c, 4, 2.2f);
    b.ellipse(c, c, 3.2f, 3.2f, 4);
    b.outline(6, false);
    return b;
}

Bitmap paintStake() {
    Bitmap b(28, 46);
    b.ellipse(14, 40, 8, 3, 3);
    b.rect(12, 14, 4, 26, 1);
    b.rect(13, 18, 2, 14, 2);
    b.poly({{16, 5}, {26, 12}, {16, 17}}, 4);
    b.poly({{16, 8}, {22, 12}, {16, 15}}, 5);
    b.outline(15, false);
    return b;
}

Bitmap paintPine() {
    Bitmap b(40, 54);
    b.rect(17, 40, 6, 12, 4);
    b.poly({{20, 3}, {37, 44}, {3, 44}}, 1);
    b.poly({{20, 12}, {31, 38}, {9, 38}}, 2);
    b.poly({{20, 18}, {26, 34}, {14, 34}}, 3);
    b.poly({{13, 16}, {20, 7}, {27, 16}, {20, 13}}, 5);
    b.outline(6, false);
    return b;
}

Bitmap paintCabin() {
    Bitmap b(52, 44);
    b.poly({{8, 32}, {44, 32}, {40, 16}, {12, 16}}, 1);
    b.poly({{4, 20}, {26, 4}, {48, 20}, {40, 18}, {26, 8}, {12, 18}}, 3);
    b.rect(22, 20, 8, 12, 7);
    b.rect(14, 20, 6, 5, 6);
    b.rect(32, 20, 6, 5, 6);
    b.outline(15, false);
    return b;
}

Bitmap paintRock() {
    Bitmap b(34, 26);
    b.ellipse(17, 15, 14, 8, 1);
    b.ellipse(16, 13, 10, 5, 2);
    b.ellipse(12, 11, 5, 2.4f, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintCrack() {
    Bitmap b(40, 22);
    b.line(2, 12, 14, 6, 2, 1.8f);
    b.line(14, 6, 24, 16, 2, 1.8f);
    b.line(24, 16, 38, 8, 2, 1.8f);
    b.line(14, 6, 18, 2, 1, 1.3f);
    b.line(24, 16, 28, 20, 1, 1.3f);
    return b;
}

Bitmap paintPuff() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 7, 4, 1);
    b.ellipse(6, 5, 3, 2, 2);
    return b;
}

Bitmap paintPin() {
    Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 3);
    return b;
}

Bitmap paintDot() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

Bitmap paintPanel() {
    Bitmap b(56, 88);
    b.rect(0, 0, 56, 88, 1);
    b.rect(3, 3, 50, 82, 2);
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

void snowRoad(gs::VDP& vdp, int pal, bool ice) {
    uint16_t c[16] = {};
    c[1] = gs::rgb4(12, 14, 15);
    c[2] = gs::rgb4(9, 11, 14);
    c[3] = gs::rgb4(14, 15, 15);
    c[4] = gs::rgb4(11, 13, 15);
    c[5] = gs::rgb4(8, 10, 13);
    if (ice) {
        c[6] = gs::rgb4(13, 15, 15);
        c[7] = gs::rgb4(10, 14, 15);
        c[8] = gs::rgb4(14, 15, 15);
        c[9] = gs::rgb4(6, 12, 15);
        c[14] = gs::rgb4(15, 15, 15);
        c[15] = gs::rgb4(8, 13, 15);
    } else {
        c[6] = gs::rgb4(15, 15, 15);
        c[7] = gs::rgb4(13, 14, 15);
        c[8] = gs::rgb4(10, 12, 14);
        c[9] = gs::rgb4(8, 11, 14);
        c[14] = gs::rgb4(7, 10, 13);
        c[15] = gs::rgb4(15, 15, 15);
    }
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t dim = gs::rgb4(9, 10, 12);
    const uint16_t line = gs::rgb4(2, 3, 5);
    setPal(vdp, PAL_HUD, {0, ink, dim, gs::rgb4(15, 14, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_TEAM,
           {0, gs::rgb4(15, 14, 11), gs::rgb4(9, 8, 6), gs::rgb4(3, 2, 2), gs::rgb4(14, 1, 1), gs::rgb4(7, 4, 2),
            gs::rgb4(12, 8, 4), gs::rgb4(12, 14, 15), gs::rgb4(4, 5, 6), gs::rgb4(2, 1, 1), gs::rgb4(13, 9, 5),
            gs::rgb4(15, 15, 14), gs::rgb4(15, 13, 10), gs::rgb4(2, 2, 2), gs::rgb4(15, 12, 3), line});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(12, 13, 14), gs::rgb4(6, 7, 8), gs::rgb4(2, 2, 3), gs::rgb4(2, 5, 13), gs::rgb4(5, 4, 3),
            gs::rgb4(8, 7, 5), gs::rgb4(9, 12, 14), gs::rgb4(3, 4, 5), gs::rgb4(1, 1, 2), gs::rgb4(4, 7, 10),
            gs::rgb4(14, 15, 15), gs::rgb4(11, 13, 15), gs::rgb4(2, 2, 3), gs::rgb4(8, 13, 15), line});
    setPal(vdp, PAL_PINE,
           {0, gs::rgb4(1, 4, 2), gs::rgb4(2, 7, 3), gs::rgb4(4, 9, 5), gs::rgb4(6, 4, 2), gs::rgb4(15, 15, 15),
            gs::rgb4(1, 2, 2), 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 2), gs::rgb4(15, 15, 15), gs::rgb4(13, 2, 2), gs::rgb4(15, 8, 4),
            gs::rgb4(15, 12, 4), gs::rgb4(3, 2, 2), gs::rgb4(12, 13, 15), 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_MARK,
           {0, gs::rgb4(13, 9, 2), gs::rgb4(8, 5, 1), gs::rgb4(15, 14, 9), gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 13),
            gs::rgb4(4, 3, 1), 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 14, 15), gs::rgb4(8, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(11, 15, 8), gs::rgb4(4, 8, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 2), line});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1), line});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 8), gs::rgb4(8, 7, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 1), line});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(2, 4, 7), gs::rgb4(3, 6, 9), gs::rgb4(8, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(7, 8, 9), gs::rgb4(4, 5, 6), gs::rgb4(15, 15, 15), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    snowRoad(vdp, PAL_SNOW, false);
    snowRoad(vdp, PAL_ICE, true);

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.sled[i] = gs::uploadMipped(vdp, paintSled(i * kTau / 16.f));
    art.disc = gs::uploadMipped(vdp, paintDisc());
    art.stake = gs::uploadMipped(vdp, paintStake());
    art.pine = gs::uploadMipped(vdp, paintPine());
    art.cabin = gs::uploadMipped(vdp, paintCabin());
    art.rock = gs::uploadMipped(vdp, paintRock());
    art.crack = gs::uploadMipped(vdp, paintCrack());
    art.puff = gs::uploadMipped(vdp, paintPuff());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.dot = gs::uploadMipped(vdp, paintDot());
    art.panel = gs::uploadMipped(vdp, paintPanel());
    art.title = word(vdp, "SLED MARK", 3);
    art.setDown = word(vdp, "SET DOWN", 3);
    art.onMark = word(vdp, "ON THE MARK", 2);
    art.crewTook = word(vdp, "CREW TOOK IT", 2);
    art.missed = word(vdp, "MISSED THE MARK", 2);
    art.buried = word(vdp, "BURIED", 2);
    art.paused = word(vdp, "PAUSED", 3);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.setFogColor(gs::rgb4(8, 10, 13));
}

}  // namespace sledmark
