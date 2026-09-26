#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace sledgrass {
namespace {

constexpr float kTau = 6.2831853f;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(2, 2, 3));
}

void setRoad(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    return {cx + ly * c + lx * s, cy - (ly * s - lx * c)};
}

Bitmap paintTeam(float heading) {
    Bitmap b(128, 128);
    const float cx = 64.f, cy = 64.f;
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
    auto stroke = [&](float ax, float ay, float bx, float by, int col, float th) {
        Pt A = spin(cx, cy, ax, ay, c, s);
        Pt B = spin(cx, cy, bx, by, c, s);
        b.line(A.first, A.second, B.first, B.second, col, th);
    };
    auto dog = [&](float lx, float ly, int body, int dark) {
        stroke(lx, ly - 6.f, lx + 3.2f, ly - 10.f, dark, 1.5f);
        stroke(lx - 2.2f, ly - 1.f, lx - 5.f, ly - 6.f, dark, 1.4f);
        stroke(lx + 2.2f, ly - 1.f, lx + 5.f, ly - 6.f, dark, 1.4f);
        stroke(lx - 1.6f, ly + 4.f, lx - 4.4f, ly + 1.2f, dark, 1.3f);
        stroke(lx + 1.6f, ly + 4.f, lx + 4.4f, ly + 1.2f, dark, 1.3f);
        poly({{lx - 3.4f, ly - 5.f}, {lx + 3.4f, ly - 5.f}, {lx + 2.6f, ly + 6.5f}, {lx - 2.6f, ly + 6.5f}}, body);
        blob(lx, ly + 8.6f, 3.2f, 3.2f, body);
        blob(lx - 1.7f, ly + 11.f, 1.3f, 1.6f, dark);
        blob(lx + 1.7f, ly + 11.f, 1.3f, 1.6f, dark);
        blob(lx + 0.4f, ly + 9.6f, 0.8f, 0.7f, 10);
        blob(lx - 1.1f, ly + 9.2f, 0.45f, 0.45f, 12);
        blob(lx + 1.5f, ly + 9.2f, 0.45f, 0.45f, 12);
    };

    stroke(-14.f, 14.f, -14.f, -24.f, 1, 3.2f);
    stroke(14.f, 14.f, 14.f, -24.f, 1, 3.2f);
    stroke(-14.f, 14.f, -6.f, 22.f, 1, 3.0f);
    stroke(14.f, 14.f, 6.f, 22.f, 1, 3.0f);
    stroke(-14.f, -18.f, 14.f, -18.f, 2, 2.6f);
    stroke(-12.f, -8.f, 12.f, -20.f, 14, 1.3f);
    stroke(-12.f, -20.f, 12.f, -8.f, 14, 1.3f);
    stroke(-14.f, -16.f, -14.f, -6.f, 12, 2.0f);
    stroke(14.f, -16.f, 14.f, -6.f, 12, 2.0f);

    poly({{-12.f, 12.f}, {12.f, 12.f}, {11.f, -12.f}, {-11.f, -12.f}}, 4);
    poly({{-9.f, 9.f}, {9.f, 9.f}, {8.f, -8.f}, {-8.f, -8.f}}, 3);
    stroke(-7.f, 6.f, 7.f, -4.f, 9, 1.3f);
    stroke(-7.f, -4.f, 7.f, 6.f, 9, 1.3f);
    blob(0.f, 1.f, 5.f, 3.4f, 6);

    blob(0.f, -4.f, 4.6f, 6.2f, 5);
    blob(0.f, 2.4f, 3.2f, 3.4f, 7);
    blob(-1.6f, 5.2f, 2.2f, 1.4f, 13);
    blob(1.6f, 5.2f, 2.2f, 1.4f, 13);
    blob(0.f, 3.2f, 0.7f, 0.6f, 10);
    stroke(-5.f, -8.f, -12.f, -16.f, 5, 2.0f);
    stroke(5.f, -8.f, 12.f, -16.f, 5, 2.0f);

    stroke(0.f, 12.f, 0.f, 20.f, 14, 1.5f);
    stroke(0.f, 20.f, -8.f, 24.f, 14, 1.4f);
    stroke(0.f, 20.f, 8.f, 24.f, 14, 1.4f);
    stroke(0.f, 20.f, 0.f, 30.f, 14, 1.4f);
    dog(-8.f, 28.f, 7, 8);
    dog(8.f, 28.f, 8, 7);
    dog(0.f, 42.f, 11, 8);
    b.outline(10, false);
    return b;
}

Bitmap pineArt() {
    Bitmap b(40, 56);
    b.rect(17, 36, 6, 16, 4);
    b.poly({{20, 2}, {37, 26}, {3, 26}}, 1);
    b.poly({{20, 14}, {34, 40}, {6, 40}}, 2);
    b.poly({{20, 4}, {30, 18}, {10, 18}}, 3);
    b.ellipse(20, 20, 9, 2.4f, 3);
    b.outline(6, false);
    return b;
}

Bitmap cabinArt() {
    Bitmap b(64, 52);
    b.rect(8, 24, 48, 22, 1);
    b.poly({{4, 26}, {32, 6}, {60, 26}}, 5);
    b.poly({{14, 24}, {32, 12}, {50, 24}}, 6);
    b.rect(27, 30, 10, 16, 3);
    b.rect(14, 30, 8, 7, 4);
    b.rect(42, 30, 8, 7, 4);
    b.rect(46, 8, 5, 12, 7);
    b.rect(44, 6, 9, 4, 2);
    b.outline(2, false);
    return b;
}

Bitmap tuftArt() {
    Bitmap b(20, 18);
    b.line(10, 16, 3, 4, 1, 1.6f);
    b.line(10, 16, 10, 2, 2, 1.8f);
    b.line(10, 16, 17, 5, 1, 1.6f);
    b.line(10, 16, 7, 3, 3, 1.4f);
    b.ellipse(10, 15, 4.2f, 2.2f, 1);
    return b;
}

Bitmap driftArt() {
    Bitmap b(48, 22);
    b.ellipse(24, 14, 20, 7, 1);
    b.ellipse(15, 12, 8, 3.5f, 2);
    b.ellipse(33, 15, 6, 3, 3);
    return b;
}

Bitmap flagArt() {
    Bitmap b(22, 32);
    b.rect(3, 4, 3, 26, 3);
    b.poly({{6, 4}, {20, 9}, {6, 16}}, 1);
    b.poly({{6, 4}, {16, 8}, {6, 11}}, 2);
    b.ellipse(4, 30, 3, 1.4f, 4);
    return b;
}

Bitmap postArt() {
    Bitmap b(12, 22);
    b.rect(4, 2, 4, 18, 1);
    b.rect(2, 2, 8, 3, 2);
    b.ellipse(6, 20, 4, 1.5f, 3);
    return b;
}

Bitmap railArt() {
    Bitmap b(40, 10);
    b.rect(0, 3, 40, 4, 1);
    b.rect(0, 3, 40, 1, 2);
    return b;
}

Bitmap ravenArt(bool up) {
    Bitmap b(28, 16);
    b.ellipse(14, 9, 3.6f, 2.2f, 1);
    float tip = up ? 2.f : 13.f;
    b.line(14, 8, 2, tip, 1, 1.7f);
    b.line(14, 8, 26, tip, 1, 1.7f);
    b.line(14, 8, 2, tip, 2, 0.8f);
    b.set(18, 8, 3);
    b.set(19, 8, 3);
    return b;
}

Bitmap sprayArt() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 6.5f, 3.f, 1);
    b.ellipse(5, 6, 2.6f, 1.4f, 2);
    return b;
}

Bitmap ringArt() {
    Bitmap b(64, 64);
    const float c = 31.5f, r = 26.f;
    for (int d = 0; d < 360; d += 14) {
        float a0 = d * kTau / 360.f;
        float a1 = (d + 6.f) * kTau / 360.f;
        b.line(c + std::cos(a0) * r, c + std::sin(a0) * r, c + std::cos(a1) * r, c + std::sin(a1) * r, 1, 2.4f);
    }
    return b;
}

Bitmap pinArt() {
    Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
    return b;
}

Bitmap panelArt() {
    Bitmap b(74, 86);
    b.rect(0, 0, 74, 86, 1);
    for (int x = 0; x < 74; x++) {
        b.set(x, 0, 2);
        b.set(x, 85, 2);
    }
    for (int y = 0; y < 86; y++) {
        b.set(0, y, 2);
        b.set(73, y, 2);
    }
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(2, 2, 3);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(9, 11, 13), gs::rgb4(15, 13, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TEAM,
           {0, gs::rgb4(8, 9, 11), gs::rgb4(5, 3, 2), gs::rgb4(11, 7, 4), gs::rgb4(13, 2, 2), gs::rgb4(2, 8, 4),
            gs::rgb4(6, 12, 7), gs::rgb4(14, 12, 9), gs::rgb4(5, 5, 6), gs::rgb4(7, 2, 2), gs::rgb4(2, 2, 3),
            gs::rgb4(12, 10, 8), gs::rgb4(15, 15, 15), gs::rgb4(12, 3, 3), gs::rgb4(9, 6, 3), shadow});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(1, 5, 2), gs::rgb4(3, 9, 4), gs::rgb4(14, 15, 15), gs::rgb4(6, 4, 2),
                           gs::rgb4(5, 11, 5), gs::rgb4(1, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(10, 7, 4), gs::rgb4(5, 3, 2), gs::rgb4(4, 2, 1), gs::rgb4(14, 12, 5),
                           gs::rgb4(14, 15, 15), gs::rgb4(8, 10, 12), gs::rgb4(8, 3, 2), gs::rgb4(3, 6, 3), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TUFT, {0, gs::rgb4(2, 7, 2), gs::rgb4(6, 12, 3), gs::rgb4(9, 13, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 2, 1), gs::rgb4(7, 5, 3), gs::rgb4(4, 3, 2),
                           gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RAVEN, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 7), gs::rgb4(12, 7, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DRIFT, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 14, 15), gs::rgb4(7, 9, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(1, 3, 6), gs::rgb4(10, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 7), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 10), gs::rgb4(3, 4, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    // Road banks. Index 0 stays empty; 1-3 ground, 4-5 verge, 6-7 surface, 8-10 wear, 14-15 specks.
    setRoad(vdp, PAL_SNOW,
            {0, gs::rgb4(12, 14, 15), gs::rgb4(8, 11, 13), gs::rgb4(14, 15, 15), gs::rgb4(10, 13, 14),
             gs::rgb4(7, 10, 12), gs::rgb4(14, 15, 15), gs::rgb4(11, 14, 15), gs::rgb4(9, 10, 12), gs::rgb4(12, 13, 14),
             gs::rgb4(15, 15, 15), gs::rgb4(6, 10, 12), gs::rgb4(13, 14, 15), gs::rgb4(8, 12, 14), gs::rgb4(15, 15, 15),
             gs::rgb4(13, 14, 15)});
    setRoad(vdp, PAL_FIELD,
            {0, gs::rgb4(2, 6, 2), gs::rgb4(1, 4, 1), gs::rgb4(4, 8, 3), gs::rgb4(5, 9, 3), gs::rgb4(3, 7, 2),
             gs::rgb4(3, 10, 3), gs::rgb4(2, 8, 2), gs::rgb4(6, 5, 2), gs::rgb4(4, 8, 2), gs::rgb4(7, 11, 4),
             gs::rgb4(1, 4, 1), gs::rgb4(2, 6, 2), gs::rgb4(3, 5, 2), gs::rgb4(8, 11, 4), gs::rgb4(2, 4, 1)});
    setRoad(vdp, PAL_END,
            {0, gs::rgb4(6, 9, 3), gs::rgb4(4, 7, 2), gs::rgb4(8, 10, 4), gs::rgb4(8, 11, 4), gs::rgb4(6, 9, 3),
             gs::rgb4(13, 14, 6), gs::rgb4(11, 13, 5), gs::rgb4(8, 7, 3), gs::rgb4(14, 14, 8), gs::rgb4(15, 15, 9),
             gs::rgb4(5, 8, 3), gs::rgb4(7, 10, 4), gs::rgb4(9, 11, 5), gs::rgb4(15, 15, 10), gs::rgb4(10, 12, 5)});

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.team[i] = gs::uploadMipped(vdp, paintTeam(i * kTau / 16.f));
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.cabin = gs::uploadMipped(vdp, cabinArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.drift = gs::uploadMipped(vdp, driftArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.rail = gs::uploadMipped(vdp, railArt());
    art.raven[0] = gs::uploadMipped(vdp, ravenArt(true));
    art.raven[1] = gs::uploadMipped(vdp, ravenArt(false));
    art.spray = gs::uploadMipped(vdp, sprayArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.pin = gs::uploadMipped(vdp, pinArt());
    Bitmap dot(6, 6);
    dot.ellipse(3, 3, 2.2f, 2.2f, 1);
    art.dot = gs::uploadMipped(vdp, dot);
    art.panel = gs::uploadMipped(vdp, panelArt());
    art.title = words(vdp, "SLED GRASS", 3);
    art.fullStop = words(vdp, "FULL STOP", 3);
    art.onGrass = words(vdp, "ON THE GRASS", 2);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.offGrass = words(vdp, "OFF THE GRASS", 2);
    art.timed = words(vdp, "TIMED OUT", 2);
    art.legFail = words(vdp, "LEG FAILS", 2);
    art.paused = words(vdp, "PAUSED", 3);
    art.endWord = words(vdp, "END", 2);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
}

}  // namespace sledgrass
