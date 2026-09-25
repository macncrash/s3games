#include "game/art.h"

#include <cmath>
#include <cstdint>

namespace keel {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    float wx = ly * c + lx * s;
    float wy = ly * s - lx * c;
    return {cx + wx, cy - wy};
}

Bitmap paintBoat(float heading, int side) {
    Bitmap b(104, 104);
    const float cx = 52.f, cy = 52.f;
    const float c = std::cos(heading), s = std::sin(heading);
    const float sd = float(side);
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
    poly({{0.f, 24.f}, {sd * 28.f, -4.f}, {sd * 6.f, -18.f}, {0.f, -2.f}}, 4);
    poly({{0.f, 18.f}, {sd * 16.f, -2.f}, {0.f, 2.f}}, 5);
    poly({{0.f, 36.f}, {8.f, 22.f}, {12.f, 6.f}, {11.f, -22.f}, {0.f, -28.f}, {-11.f, -22.f}, {-12.f, 6.f}, {-8.f, 22.f}}, 1);
    poly({{0.f, 28.f}, {5.f, 16.f}, {7.f, 0.f}, {6.f, -16.f}, {0.f, -20.f}, {-6.f, -16.f}, {-7.f, 0.f}, {-5.f, 16.f}}, 2);
    poly({{0.f, 34.f}, {5.f, 24.f}, {-5.f, 24.f}}, 6);
    poly({{4.5f, -4.f}, {4.5f, -14.f}, {-4.5f, -14.f}, {-4.5f, -4.f}}, 3);
    poly({{1.4f, -26.f}, {1.4f, -34.f}, {-1.4f, -34.f}, {-1.4f, -26.f}}, 8);
    stroke(0.f, 4.f, sd * 22.f, -8.f, 8, 1.7f);
    Pt mast = spin(cx, cy, 0.f, 4.f, c, s);
    b.ellipse(mast.first, mast.second, 2.2f, 2.2f, 8);
    Pt crew = spin(cx, cy, 0.f, -8.f, c, s);
    b.ellipse(crew.first, crew.second, 2.6f, 2.6f, 9);
    b.outline(8, false);
    return b;
}

Bitmap buoyArt(const char* num) {
    Bitmap b(40, 52);
    b.ellipse(20, 32, 14, 10, 1);
    b.ellipse(20, 31, 10, 7, 2);
    b.ellipse(20, 30, 4, 3, 1);
    b.rect(18, 14, 4, 16, 3);
    b.poly({{20, 8}, {33, 16}, {20, 20}}, 4);
    b.outline(3, false);
    gs::TextStyle st{2, 5, 0, 0, 1};
    Bitmap t = gs::textBitmap(num, st);
    b.blit(t, 20 - t.w / 2, 24);
    return b;
}

Bitmap dockArt() {
    Bitmap b(56, 72);
    for (int i = 0; i < 8; i++) b.rect(12, 6 + i * 7, 32, 5, (i & 1) ? 1 : 2);
    b.rect(10, 8, 5, 5, 3);
    b.rect(41, 8, 5, 5, 3);
    b.rect(10, 40, 5, 5, 3);
    b.rect(41, 40, 5, 5, 3);
    b.rect(16, 48, 24, 16, 4);
    b.poly({{16, 48}, {28, 40}, {40, 48}}, 5);
    b.rect(25, 54, 6, 10, 6);
    b.line(44, 18, 50, 28, 7, 1.4f);
    b.outline(3, false);
    return b;
}

Bitmap foamArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 4, 1);
    b.ellipse(8, 8, 3, 2, 2);
    return b;
}

Bitmap gullArt(bool up) {
    Bitmap b(28, 16);
    b.ellipse(14, 9, 4, 2, 1);
    float tip = up ? 2.f : 13.f;
    b.line(14, 8, 2, tip, 1, 1.6f);
    b.line(14, 8, 26, tip, 2, 1.6f);
    b.line(14, 9, 4, (tip + 8.f) * 0.5f, 2, 1.2f);
    b.line(14, 9, 24, (tip + 8.f) * 0.5f, 1, 1.2f);
    b.set(17, 8, 3);
    b.set(18, 8, 3);
    return b;
}

Bitmap rockArt() {
    Bitmap b(48, 36);
    b.ellipse(24, 20, 20, 13, 1);
    b.ellipse(22, 18, 12, 8, 2);
    b.ellipse(16, 16, 5, 3, 3);
    b.ellipse(30, 22, 4, 2, 4);
    b.outline(2, false);
    return b;
}

Bitmap ringArt() {
    Bitmap b(80, 80);
    const float c = 39.5f, r = 36.f;
    for (int d = 0; d < 360; d += 8) {
        float a0 = d * 3.14159265f / 180.f;
        float a1 = (d + 4.2f) * 3.14159265f / 180.f;
        b.line(c + std::cos(a0) * r, c + std::sin(a0) * r, c + std::cos(a1) * r, c + std::sin(a1) * r, 1, 2.2f);
    }
    return b;
}

Bitmap pileArt() {
    Bitmap b(12, 18);
    b.rect(4, 4, 4, 12, 1);
    b.ellipse(6, 4, 5, 3, 2);
    b.outline(3, false);
    return b;
}

Bitmap windArt() {
    Bitmap b(16, 26);
    b.poly({{8, 24}, {2, 12}, {6, 12}, {6, 2}, {10, 2}, {10, 12}, {14, 12}}, 1);
    b.outline(2, false);
    return b;
}

Bitmap pinArt() {
    Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
    return b;
}

Bitmap panelArt() {
    Bitmap b(78, 60);
    b.rect(0, 0, 78, 60, 1);
    for (int x = 0; x < 78; x++) {
        b.set(x, 0, 2);
        b.set(x, 59, 2);
    }
    for (int y = 0; y < 60; y++) {
        b.set(0, y, 2);
        b.set(77, y, 2);
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
    uint8_t a[64] = {};
    uint8_t w[64] = {};
    a[2 * 8 + 1] = 1;
    a[3 * 8 + 2] = 2;
    a[4 * 8 + 3] = 1;
    a[5 * 8 + 5] = 2;
    a[6 * 8 + 6] = 1;
    w[1 * 8 + 6] = 2;
    w[3 * 8 + 4] = 1;
    w[4 * 8 + 1] = 2;
    w[6 * 8 + 3] = 1;
    int t0 = tiles.alloc(1);
    int t1 = tiles.alloc(1);
    vdp.loadTile(t0, a);
    vdp.loadTile(t1, w);
    for (int cy = 0; cy < vdp.B.h; cy++) {
        for (int cx = 0; cx < vdp.B.w; cx++) {
            int tile = ((cx + cy) & 1) ? t1 : t0;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_WAVE));
        }
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 9, 11), gs::rgb4(15, 14, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOAT, {0, gs::rgb4(14, 15, 15), gs::rgb4(6, 8, 10), gs::rgb4(2, 3, 5), gs::rgb4(15, 14, 12),
                           gs::rgb4(11, 12, 13), gs::rgb4(13, 2, 2), gs::rgb4(9, 7, 4), gs::rgb4(2, 2, 3),
                           gs::rgb4(14, 6, 2), 0, 0, 0, 0, 0, shadow});
    auto mark = [&](int pal, uint16_t band, uint16_t flag) {
        setPal(vdp, pal, {0, band, gs::rgb4(15, 15, 15), gs::rgb4(3, 2, 2), flag, gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    };
    mark(PAL_MARK0, gs::rgb4(14, 2, 2), gs::rgb4(15, 8, 2));
    mark(PAL_MARK1, gs::rgb4(15, 11, 2), gs::rgb4(15, 6, 1));
    mark(PAL_MARK2, gs::rgb4(3, 12, 4), gs::rgb4(8, 15, 6));
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(4, 3, 2), gs::rgb4(10, 4, 3),
                           gs::rgb4(6, 2, 2), gs::rgb4(3, 2, 3), gs::rgb4(13, 12, 8), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 9, 10), gs::rgb4(15, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(1, 2, 6), gs::rgb4(7, 11, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5), gs::rgb4(4, 8, 3), gs::rgb4(10, 10, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WAVE, {0, gs::rgb4(8, 13, 14), gs::rgb4(13, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 8), gs::rgb4(3, 2, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) {
        float h = i * kTau / 16.f;
        art.boat[i][0] = gs::uploadMipped(vdp, paintBoat(h, -1));
        art.boat[i][1] = gs::uploadMipped(vdp, paintBoat(h, 1));
    }
    art.buoy[0] = gs::uploadMipped(vdp, buoyArt("1"));
    art.buoy[1] = gs::uploadMipped(vdp, buoyArt("2"));
    art.buoy[2] = gs::uploadMipped(vdp, buoyArt("3"));
    art.dock = gs::uploadMipped(vdp, dockArt());
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(true));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(false));
    art.rock = gs::uploadMipped(vdp, rockArt());
    Bitmap dot(6, 6);
    dot.ellipse(3, 3, 2.4f, 2.4f, 1);
    art.dot = gs::uploadMipped(vdp, dot);
    art.pin = gs::uploadMipped(vdp, pinArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.wind = gs::uploadMipped(vdp, windArt());
    art.panel = gs::uploadMipped(vdp, panelArt());
    art.title = words(vdp, "S3 KEEL", 4);
    art.docked = words(vdp, "DOCKED", 4);
    art.closed = words(vdp, "TRIANGLE CLOSED", 2);
    art.paused = words(vdp, "PAUSED", 4);

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
}

}  // namespace keel
