#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace ferry {
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

Bitmap paintFerry(float heading) {
    Bitmap b(128, 128);
    const float cx = 64.f, cy = 64.f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        w.reserve(local.size());
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, c, s));
        b.poly(w, col);
    };
    auto blob = [&](float lx, float ly, float r, int col) {
        Pt p = spin(cx, cy, lx, ly, c, s);
        b.ellipse(p.first, p.second, r, r, col);
    };
    auto stroke = [&](float ax, float ay, float bx, float by, int col, float th) {
        Pt A = spin(cx, cy, ax, ay, c, s);
        Pt B = spin(cx, cy, bx, by, c, s);
        b.line(A.first, A.second, B.first, B.second, col, th);
    };

    poly({{-20.f, -48.f}, {20.f, -48.f}, {22.f, -34.f}, {22.f, 34.f}, {20.f, 48.f}, {-20.f, 48.f}, {-22.f, 34.f}, {-22.f, -34.f}}, 1);
    poly({{-16.f, -44.f}, {16.f, -44.f}, {18.f, -30.f}, {18.f, 30.f}, {16.f, 44.f}, {-16.f, 44.f}, {-18.f, 30.f}, {-18.f, -30.f}}, 2);
    poly({{-14.f, -40.f}, {14.f, -40.f}, {15.f, -28.f}, {15.f, 28.f}, {14.f, 40.f}, {-14.f, 40.f}, {-15.f, 28.f}, {-15.f, -28.f}}, 10);
    poly({{-12.f, 36.f}, {12.f, 36.f}, {9.f, 47.f}, {-9.f, 47.f}}, 6);
    poly({{-12.f, -36.f}, {12.f, -36.f}, {9.f, -47.f}, {-9.f, -47.f}}, 6);
    stroke(-7.f, 41.f, 7.f, 41.f, 9, 1.3f);
    stroke(-7.f, -41.f, 7.f, -41.f, 9, 1.3f);
    poly({{-8.f, 30.f}, {-2.f, 30.f}, {-2.f, 24.f}, {-8.f, 24.f}}, 7);
    poly({{2.f, 30.f}, {8.f, 30.f}, {8.f, 24.f}, {2.f, 24.f}}, 8);
    poly({{-8.f, 22.f}, {-2.f, 22.f}, {-2.f, 16.f}, {-8.f, 16.f}}, 8);
    poly({{2.f, 22.f}, {8.f, 22.f}, {8.f, 16.f}, {2.f, 16.f}}, 7);
    poly({{-8.f, -18.f}, {-2.f, -18.f}, {-2.f, -24.f}, {-8.f, -24.f}}, 7);
    poly({{2.f, -18.f}, {8.f, -18.f}, {8.f, -24.f}, {2.f, -24.f}}, 8);
    poly({{-8.f, -26.f}, {-2.f, -26.f}, {-2.f, -32.f}, {-8.f, -32.f}}, 8);
    poly({{2.f, -26.f}, {8.f, -26.f}, {8.f, -32.f}, {2.f, -32.f}}, 7);
    poly({{-12.f, -6.f}, {12.f, -6.f}, {12.f, 16.f}, {-12.f, 16.f}}, 3);
    poly({{-16.f, 4.f}, {16.f, 4.f}, {16.f, 9.f}, {-16.f, 9.f}}, 3);
    poly({{-10.f, 12.f}, {10.f, 12.f}, {10.f, 7.f}, {-10.f, 7.f}}, 4);
    poly({{-10.f, 2.f}, {10.f, 2.f}, {10.f, -4.f}, {-10.f, -4.f}}, 4);
    poly({{-2.f, 16.f}, {2.f, 16.f}, {2.f, 10.f}, {-2.f, 10.f}}, 9);
    poly({{-5.f, -8.f}, {5.f, -8.f}, {4.f, -22.f}, {-4.f, -22.f}}, 5);
    poly({{-5.f, -18.f}, {5.f, -18.f}, {5.f, -22.f}, {-5.f, -22.f}}, 9);
    stroke(0.f, 16.f, 0.f, 28.f, 9, 1.5f);
    poly({{0.f, 28.f}, {8.f, 24.f}, {0.f, 23.f}}, 13);
    blob(18.f, 2.f, 3.3f, 11);
    blob(-18.f, 2.f, 3.3f, 11);
    for (float ly : {28.f, 12.f, -4.f, -20.f, -34.f}) {
        blob(21.f, ly, 1.7f, 9);
        blob(-21.f, ly, 1.7f, 9);
    }
    blob(-14.f, 42.f, 1.7f, 13);
    blob(14.f, 42.f, 1.7f, 12);
    blob(-14.f, -42.f, 1.7f, 12);
    blob(14.f, -42.f, 1.7f, 13);
    b.outline(9, false);
    return b;
}

Bitmap nunArt() {
    Bitmap b(40, 56);
    b.ellipse(20, 46, 12, 5, 2);
    b.poly({{20, 6}, {33, 40}, {7, 40}}, 1);
    b.rect(11, 22, 18, 8, 2);
    b.ellipse(20, 8, 3.f, 3.f, 4);
    b.outline(3, false);
    gs::TextStyle st{2, 5, 0, 0, 0};
    Bitmap t = gs::textBitmap("1", st);
    b.blit(t, 20 - t.w / 2, 20);
    return b;
}

Bitmap canArt() {
    Bitmap b(40, 56);
    b.ellipse(20, 46, 13, 5, 2);
    b.rect(9, 16, 22, 28, 1);
    b.ellipse(20, 16, 11, 5, 1);
    b.rect(11, 24, 18, 8, 2);
    b.rect(16, 6, 8, 12, 3);
    b.ellipse(20, 6, 2.4f, 2.4f, 4);
    b.outline(5, false);
    gs::TextStyle st{2, 5, 0, 0, 0};
    Bitmap t = gs::textBitmap("2", st);
    b.blit(t, 20 - t.w / 2, 22);
    return b;
}

Bitmap specialArt() {
    Bitmap b(40, 56);
    b.ellipse(20, 46, 13, 5, 2);
    b.rect(11, 16, 18, 28, 1);
    b.ellipse(20, 16, 9, 5, 1);
    b.rect(12, 24, 16, 8, 2);
    b.line(14, 8, 26, 18, 3, 2.f);
    b.line(26, 8, 14, 18, 3, 2.f);
    b.ellipse(20, 6, 2.2f, 2.2f, 4);
    b.outline(5, false);
    gs::TextStyle st{2, 5, 0, 0, 0};
    Bitmap t = gs::textBitmap("3", st);
    b.blit(t, 20 - t.w / 2, 26);
    return b;
}

Bitmap quayArt() {
    Bitmap b(36, 28);
    for (int i = 0; i < 4; i++) b.rect(1, 8 + i * 4, 34, 3, (i & 1) ? 1 : 2);
    b.rect(6, 10, 5, 5, 7);
    b.rect(24, 18, 4, 4, 4);
    return b;
}

Bitmap shedArt(bool crew) {
    Bitmap b(52, 40);
    b.rect(8, 16, 36, 18, crew ? 3 : 1);
    b.poly({{6, 18}, {26, 6}, {46, 18}}, crew ? 1 : 3);
    b.rect(22, 22, 8, 12, crew ? 9 : 4);
    b.rect(12, 20, 7, 5, 4);
    b.rect(33, 20, 7, 5, 4);
    b.outline(crew ? 9 : 5, false);
    return b;
}

Bitmap pileArt() {
    Bitmap b(12, 22);
    b.rect(4, 4, 4, 16, 1);
    b.ellipse(6, 5, 4, 3, 2);
    b.outline(5, false);
    return b;
}

Bitmap rampArt() {
    Bitmap b(64, 18);
    b.rect(2, 4, 60, 10, 6);
    b.rect(2, 4, 60, 3, 2);
    for (int i = 0; i < 5; i++) b.rect(8 + i * 11, 7, 2, 6, 5);
    b.outline(5, false);
    return b;
}

Bitmap craneArt() {
    Bitmap b(56, 44);
    b.rect(22, 26, 16, 14, 1);
    b.rect(25, 20, 10, 8, 2);
    b.line(30, 24, 6, 8, 6, 2.2f);
    b.line(10, 12, 10, 22, 5, 1.4f);
    b.outline(5, false);
    return b;
}

Bitmap carArt() {
    Bitmap b(22, 14);
    b.rect(2, 5, 16, 5, 7);
    b.rect(5, 2, 8, 4, 4);
    b.ellipse(6, 10, 2.2f, 2.2f, 9);
    b.ellipse(15, 10, 2.2f, 2.2f, 9);
    return b;
}

Bitmap rockArt() {
    Bitmap b(40, 30);
    b.ellipse(20, 16, 16, 10, 1);
    b.ellipse(16, 14, 8, 5, 2);
    b.ellipse(26, 18, 5, 3, 3);
    b.outline(2, false);
    return b;
}

Bitmap gullArt(bool up) {
    Bitmap b(26, 14);
    b.ellipse(13, 8, 3.2f, 2.f, 1);
    float tip = up ? 2.f : 12.f;
    b.line(13, 7, 2, tip, 1, 1.5f);
    b.line(13, 7, 24, tip, 2, 1.5f);
    b.set(16, 7, 3);
    return b;
}

Bitmap foamArt() {
    Bitmap b(16, 10);
    b.ellipse(8, 5, 7, 3, 1);
    b.ellipse(6, 5, 3, 1.5f, 2);
    return b;
}

Bitmap smokeArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 9, 6, 5, 1);
    b.ellipse(7, 8, 3, 2.2f, 2);
    return b;
}

Bitmap ringArt() {
    Bitmap b(64, 64);
    const float c = 31.5f, r = 28.f;
    for (int d = 0; d < 360; d += 12) {
        float a0 = d * kTau / 360.f;
        float a1 = (d + 6.f) * kTau / 360.f;
        b.line(c + std::cos(a0) * r, c + std::sin(a0) * r, c + std::cos(a1) * r, c + std::sin(a1) * r, 1, 2.f);
    }
    return b;
}

Bitmap lampArt() {
    Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2.f, 2.f, 2);
    return b;
}

Bitmap pinArt() {
    Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
    return b;
}

Bitmap panelArt() {
    Bitmap b(72, 78);
    b.rect(0, 0, 72, 78, 1);
    for (int x = 0; x < 72; x++) {
        b.set(x, 0, 2);
        b.set(x, 77, 2);
    }
    for (int y = 0; y < 78; y++) {
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
    a[2 * 8 + 2] = 1;
    a[3 * 8 + 4] = 2;
    a[5 * 8 + 6] = 1;
    w[1 * 8 + 1] = 2;
    w[4 * 8 + 5] = 1;
    w[6 * 8 + 3] = 2;
    int t0 = tiles.alloc(1);
    int t1 = tiles.alloc(1);
    vdp.loadTile(t0, a);
    vdp.loadTile(t1, w);
    for (int cy = 0; cy < vdp.B.h; cy++) {
        for (int cx = 0; cx < vdp.B.w; cx++) vdp.B.set(cx, cy, gs::entry(((cx / 3 + cy) & 3) == 0 ? t1 : t0, PAL_WAVE));
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 10, 12), gs::rgb4(15, 13, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FERRY,
           {0, gs::rgb4(1, 5, 2), gs::rgb4(3, 12, 5), gs::rgb4(15, 15, 15), gs::rgb4(3, 8, 13), gs::rgb4(12, 7, 3),
            gs::rgb4(14, 12, 2), gs::rgb4(13, 2, 2), gs::rgb4(2, 4, 11), gs::rgb4(1, 1, 2), gs::rgb4(13, 11, 8),
            gs::rgb4(14, 6, 2), gs::rgb4(2, 13, 4), gs::rgb4(13, 2, 3), gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_NUN, {0, gs::rgb4(14, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(4, 1, 1), gs::rgb4(15, 12, 3), gs::rgb4(3, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CAN, {0, gs::rgb4(2, 11, 4), gs::rgb4(15, 15, 15), gs::rgb4(1, 4, 2), gs::rgb4(15, 14, 5), gs::rgb4(1, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_YEL, {0, gs::rgb4(14, 12, 2), gs::rgb4(15, 15, 14), gs::rgb4(6, 5, 1), gs::rgb4(15, 10, 2), gs::rgb4(3, 2, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DOCK,
           {0, gs::rgb4(11, 8, 5), gs::rgb4(7, 5, 3), gs::rgb4(3, 8, 4), gs::rgb4(4, 7, 9), gs::rgb4(2, 2, 2),
            gs::rgb4(9, 9, 8), gs::rgb4(12, 10, 3), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 9, 10), gs::rgb4(14, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WAVE, {0, gs::rgb4(8, 13, 14), gs::rgb4(14, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CREW,
           {0, gs::rgb4(10, 2, 2), gs::rgb4(14, 4, 3), gs::rgb4(15, 15, 15), gs::rgb4(3, 7, 12), gs::rgb4(3, 2, 2),
            gs::rgb4(14, 12, 3), gs::rgb4(6, 6, 7), gs::rgb4(2, 3, 8), gs::rgb4(1, 1, 2), gs::rgb4(12, 10, 8),
            gs::rgb4(14, 6, 2), gs::rgb4(2, 12, 4), gs::rgb4(13, 2, 2), gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 8), gs::rgb4(3, 2, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(7, 8, 9), gs::rgb4(13, 13, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(1, 3, 6), gs::rgb4(9, 12, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5), gs::rgb4(5, 8, 4), gs::rgb4(12, 12, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.ferry[i] = gs::uploadMipped(vdp, paintFerry(i * kTau / 16.f));
    art.buoy[0] = gs::uploadMipped(vdp, nunArt());
    art.buoy[1] = gs::uploadMipped(vdp, canArt());
    art.buoy[2] = gs::uploadMipped(vdp, specialArt());
    art.quay = gs::uploadMipped(vdp, quayArt());
    art.shed = gs::uploadMipped(vdp, shedArt(false));
    art.crewShed = gs::uploadMipped(vdp, shedArt(true));
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.ramp = gs::uploadMipped(vdp, rampArt());
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.car = gs::uploadMipped(vdp, carArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(true));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(false));
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.smoke = gs::uploadMipped(vdp, smokeArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.pin = gs::uploadMipped(vdp, pinArt());
    Bitmap dot(6, 6);
    dot.ellipse(3, 3, 2.2f, 2.2f, 1);
    art.dot = gs::uploadMipped(vdp, dot);
    art.panel = gs::uploadMipped(vdp, panelArt());
    art.title = words(vdp, "FERRY BUOY", 3);
    art.sub = words(vdp, "ROUND THE BUOYS", 2);
    art.same = words(vdp, "SAME DOCK", 3);
    art.ahead = words(vdp, "AHEAD OF THE CREW", 2);
    art.crewTook = words(vdp, "CREW TOOK IT", 2);
    art.wrong = words(vdp, "WRONG DOCK", 3);
    art.paused = words(vdp, "PAUSED", 3);

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
}

}  // namespace ferry
