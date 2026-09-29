#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace subby {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(0, 1, 2));
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    float wx = ly * c + lx * s;
    float wy = ly * s - lx * c;
    return {cx + wx, cy - wy};
}

Bitmap paintSub(float heading) {
    Bitmap b(96, 96);
    const float cx = 48.f, cy = 48.f;
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
    poly({{0, 40}, {7, 32}, {9, 8}, {8, -18}, {6, -30}, {-6, -30}, {-8, -18}, {-9, 8}, {-7, 32}}, 1);
    poly({{0, 34}, {4, 26}, {5, 6}, {4, -16}, {-4, -16}, {-5, 6}, {-4, 26}}, 2);
    poly({{-7, -22}, {7, -22}, {5, -34}, {-5, -34}}, 6);
    blob(0, 4, 7.5f, 6.f, 3);
    blob(0, 6, 4.2f, 3.2f, 4);
    stroke(0, 10, 0, 22, 5, 1.8f);
    blob(0, 23, 2.2f, 2.2f, 9);
    stroke(-8, 18, 8, 18, 10, 2.f);
    blob(-8.5f, 12, 1.6f, 1.6f, 8);
    blob(8.5f, 12, 1.6f, 1.6f, 7);
    blob(-5, -8, 1.4f, 1.4f, 4);
    blob(0, -8, 1.4f, 1.4f, 4);
    blob(5, -8, 1.4f, 1.4f, 4);
    b.outline(11, false);
    return b;
}

Bitmap nunArt(const char* num, int body) {
    Bitmap b(40, 56);
    b.ellipse(20, 46, 13, 5, 2);
    b.poly({{20, 6}, {33, 40}, {7, 40}}, body);
    b.rect(12, 24, 16, 8, 2);
    b.ellipse(20, 8, 3.f, 3.f, 4);
    b.outline(3, false);
    gs::TextStyle st{2, 5, 0, 0, 0};
    Bitmap t = gs::textBitmap(num, st);
    b.blit(t, 20 - t.w / 2, 22);
    return b;
}

Bitmap canArt(const char* num) {
    Bitmap b(40, 56);
    b.ellipse(20, 46, 13, 5, 2);
    b.rect(9, 16, 22, 28, 1);
    b.ellipse(20, 16, 11, 5, 1);
    b.rect(11, 24, 18, 8, 2);
    b.poly({{20, 3}, {28, 13}, {20, 15}, {12, 13}}, 3);
    b.ellipse(20, 5, 2.2f, 2.2f, 4);
    b.outline(3, false);
    gs::TextStyle st{2, 5, 0, 0, 0};
    Bitmap t = gs::textBitmap(num, st);
    b.blit(t, 20 - t.w / 2, 22);
    return b;
}

Bitmap quayArt(int variant) {
    Bitmap b(36, 28);
    for (int i = 0; i < 4; i++) b.rect(1, 10 + i * 4, 34, 3, (i & 1) ? 1 : 2);
    b.rect(float((variant & 1) ? 4 : 24), 12, 6, 6, 3);
    b.outline(4, false);
    return b;
}

Bitmap shedArt() {
    Bitmap b(48, 36);
    b.poly({{2, 34}, {46, 34}, {40, 16}, {8, 16}}, 1);
    b.poly({{8, 16}, {40, 16}, {24, 4}}, 2);
    b.rect(18, 20, 10, 14, 3);
    b.rect(8, 20, 6, 5, 5);
    b.outline(4, false);
    return b;
}

Bitmap pileArt() {
    Bitmap b(12, 28);
    b.rect(3, 2, 6, 24, 1);
    b.rect(2, 2, 8, 4, 2);
    b.outline(3, false);
    return b;
}

Bitmap beamArt() {
    Bitmap b(64, 14);
    b.rect(0, 4, 64, 6, 1);
    for (int i = 0; i < 5; i++) b.rect(float(6 + i * 12), 2, 3, 10, 2);
    return b;
}

Bitmap craneArt() {
    Bitmap b(28, 48);
    b.rect(10, 18, 8, 28, 1);
    b.poly({{14, 18}, {14, 4}, {26, 10}, {14, 12}}, 2);
    b.line(22, 10, 22, 30, 3, 1.5f);
    b.ellipse(22, 32, 3, 3, 4);
    return b;
}

Bitmap wreckArt() {
    Bitmap b(52, 28);
    b.poly({{2, 20}, {18, 8}, {40, 10}, {48, 22}, {8, 24}}, 1);
    b.rect(16, 12, 10, 6, 2);
    b.line(6, 18, 44, 14, 3, 1.4f);
    b.outline(4, false);
    return b;
}

Bitmap kelpArt() {
    Bitmap b(16, 40);
    b.line(8, 38, 6, 24, 1, 2.2f);
    b.line(6, 24, 10, 12, 2, 2.f);
    b.line(10, 12, 7, 2, 1, 1.6f);
    b.ellipse(5, 16, 3, 2, 2);
    b.ellipse(11, 8, 3, 2, 1);
    return b;
}

Bitmap fishArt(int flap) {
    Bitmap b(22, 12);
    b.ellipse(10, 6, 7, 4, 1);
    if (flap) b.poly({{16, 6}, {21, 2}, {21, 10}}, 2);
    else b.poly({{16, 6}, {21, 4}, {21, 8}}, 2);
    b.ellipse(5, 5, 1.2f, 1.2f, 3);
    return b;
}

Bitmap bubbleArt() {
    Bitmap b(12, 12);
    b.ellipse(6, 6, 4.5f, 4.5f, 1);
    b.ellipse(6, 6, 2.4f, 2.4f, 0);
    b.set(4, 4, 2);
    return b;
}

Bitmap ringArt() {
    Bitmap b(40, 40);
    b.ellipse(20, 20, 18, 18, 1);
    b.ellipse(20, 20, 13, 13, 0);
    return b;
}

Bitmap lampArt() {
    Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

Bitmap pinArt() {
    Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
    return b;
}

Bitmap dotArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

Bitmap panelArt() {
    Bitmap b(72, 58);
    b.rect(0, 0, 72, 58, 1);
    for (int x = 0; x < 72; x++) {
        b.set(x, 0, 2);
        b.set(x, 57, 2);
    }
    for (int y = 0; y < 58; y++) {
        b.set(0, y, 2);
        b.set(71, y, 2);
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
    for (int i = 0; i < 8; i++) {
        a[i * 8 + ((i * 3) & 7)] = 1;
        w[i * 8 + ((i * 5 + 2) & 7)] = 2;
    }
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
    const uint16_t ink = gs::rgb4(0, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 15, 15), gs::rgb4(6, 9, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SUB, {0, gs::rgb4(2, 5, 4), gs::rgb4(5, 9, 7), gs::rgb4(3, 4, 5), gs::rgb4(8, 14, 15),
                          gs::rgb4(11, 12, 10), gs::rgb4(8, 8, 6), gs::rgb4(14, 3, 2), gs::rgb4(3, 13, 5),
                          gs::rgb4(15, 15, 14), gs::rgb4(14, 12, 3), gs::rgb4(1, 2, 2), 0, 0, 0, ink});
    setPal(vdp, PAL_NUN, {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(3, 1, 1), gs::rgb4(15, 11, 3),
                          gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CAN, {0, gs::rgb4(2, 11, 4), gs::rgb4(15, 15, 15), gs::rgb4(1, 4, 2), gs::rgb4(15, 14, 5),
                          gs::rgb4(1, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(9, 8, 6), gs::rgb4(6, 5, 4), gs::rgb4(3, 3, 3), gs::rgb4(4, 6, 8),
                           gs::rgb4(12, 11, 8), gs::rgb4(7, 9, 10), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BUB, {0, gs::rgb4(10, 14, 15), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_KELP, {0, gs::rgb4(1, 7, 3), gs::rgb4(3, 11, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WRECK, {0, gs::rgb4(6, 5, 4), gs::rgb4(3, 6, 7), gs::rgb4(10, 8, 5), gs::rgb4(2, 2, 2), 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WAVE, {0, gs::rgb4(2, 6, 9), gs::rgb4(4, 9, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FISH, {0, gs::rgb4(12, 10, 4), gs::rgb4(8, 6, 2), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(12, 14, 15), gs::rgb4(1, 3, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(6, 15, 8), gs::rgb4(1, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(4, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SONAR, {0, gs::rgb4(4, 15, 8), gs::rgb4(10, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(1, 2, 4), gs::rgb4(7, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    loadFont(vdp, art);
    for (int i = 0; i < 8; i++) art.hull[i] = gs::uploadMipped(vdp, paintSub(i * kTau / 8.f));
    art.buoy[0] = gs::uploadMipped(vdp, nunArt("1", 1));
    art.buoy[1] = gs::uploadMipped(vdp, canArt("2"));
    art.buoy[2] = gs::uploadMipped(vdp, nunArt("3", 1));
    art.quay = gs::uploadMipped(vdp, quayArt(0));
    art.quayB = gs::uploadMipped(vdp, quayArt(1));
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.wreck = gs::uploadMipped(vdp, wreckArt());
    art.kelp = gs::uploadMipped(vdp, kelpArt());
    art.fish[0] = gs::uploadMipped(vdp, fishArt(0));
    art.fish[1] = gs::uploadMipped(vdp, fishArt(1));
    art.bubble = gs::uploadMipped(vdp, bubbleArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.pin = gs::uploadMipped(vdp, pinArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.panel = gs::uploadMipped(vdp, panelArt());
    art.title = words(vdp, "SUB BUOY", 3);
    art.line = words(vdp, "ROUND THE BUOYS", 1);
    art.same = words(vdp, "SAME PEN", 2);
    art.paused = words(vdp, "HOLDING", 2);
}

}  // namespace subby
