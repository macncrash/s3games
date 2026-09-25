#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace ferry {
namespace {

constexpr float kPi = 3.14159265f;
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
    // Green hull, white deck, cream cabin, yellow bow ramp. Bow is +local Y.
    poly({{0.f, 40.f}, {9.f, 32.f}, {13.f, 14.f}, {13.f, -18.f}, {9.f, -32.f}, {-9.f, -32.f}, {-13.f, -18.f}, {-13.f, 14.f}, {-9.f, 32.f}}, 2);
    poly({{0.f, 34.f}, {9.f, 26.f}, {9.f, -22.f}, {6.f, -28.f}, {-6.f, -28.f}, {-9.f, -22.f}, {-9.f, 26.f}}, 1);
    poly({{-10.f, 18.f}, {10.f, 18.f}, {9.f, 28.f}, {-9.f, 28.f}}, 9);
    poly({{-4.f, 20.f}, {-1.f, 20.f}, {-1.f, 26.f}, {-4.f, 26.f}}, 10);
    poly({{-0.8f, 20.f}, {2.2f, 20.f}, {2.2f, 26.f}, {-0.8f, 26.f}}, 10);
    poly({{3.4f, 20.f}, {6.4f, 20.f}, {6.4f, 26.f}, {3.4f, 26.f}}, 10);
    poly({{-9.f, -2.f}, {9.f, -2.f}, {9.f, 14.f}, {-9.f, 14.f}}, 3);
    poly({{-7.f, 2.f}, {7.f, 2.f}, {7.f, 6.f}, {-7.f, 6.f}}, 6);
    poly({{-7.f, 8.f}, {7.f, 8.f}, {7.f, 12.f}, {-7.f, 12.f}}, 6);
    poly({{-3.2f, -20.f}, {3.2f, -20.f}, {2.6f, -6.f}, {-2.6f, -6.f}}, 4);
    poly({{-3.2f, -20.f}, {3.2f, -20.f}, {2.8f, -16.f}, {-2.8f, -16.f}}, 7);
    poly({{-7.f, 30.f}, {7.f, 30.f}, {6.f, 38.f}, {-6.f, 38.f}}, 5);
    stroke(-11.f, 4.f, -11.f, -12.f, 8, 2.2f);
    stroke(11.f, 4.f, 11.f, -12.f, 8, 2.2f);
    stroke(0.f, 14.f, 0.f, 22.f, 7, 1.4f);
    b.outline(7, false);
    return b;
}

Bitmap pierArt() {
    Bitmap b(46, 132);
    b.rect(4, 0, 38, 132, 1);
    for (int y = 2; y < 128; y += 8) {
        b.rect(0, y, 8, 4, 2);
        b.rect(38, y, 8, 4, 2);
        b.rect(12, y + 2, 22, 3, 3);
    }
    for (int y = 18; y < 120; y += 26) {
        b.ellipse(14, y, 3.2f, 3.2f, 4);
        b.ellipse(32, y, 3.2f, 3.2f, 4);
    }
    b.outline(5, false);
    return b;
}

Bitmap apronArt() {
    Bitmap b(160, 28);
    b.rect(0, 4, 160, 20, 1);
    for (int x = 6; x < 154; x += 18) b.rect(x, 8, 8, 12, 2);
    b.rect(70, 6, 20, 16, 3);
    b.rect(18, 10, 10, 6, 6);
    b.rect(132, 10, 10, 6, 6);
    b.outline(5, false);
    return b;
}

Bitmap shedArt() {
    Bitmap b(100, 52);
    b.rect(10, 20, 80, 28, 1);
    b.poly({{8.f, 20.f}, {50.f, 4.f}, {92.f, 20.f}}, 2);
    b.rect(42, 28, 16, 20, 3);
    b.rect(18, 26, 14, 10, 4);
    b.rect(68, 26, 14, 10, 4);
    b.rect(46, 32, 8, 10, 6);
    b.outline(6, false);
    return b;
}

Bitmap bridgeArt() {
    Bitmap b(48, 18);
    b.rect(2, 2, 44, 14, 1);
    for (int x = 6; x < 42; x += 8) b.rect(x, 4, 4, 10, 2);
    b.outline(3, false);
    return b;
}

Bitmap buoyArt(bool red) {
    Bitmap b(24, 34);
    b.ellipse(12, 24, 8, 6, 1);
    b.ellipse(12, 23, 5, 3, 2);
    b.rect(10, 10, 4, 12, 2);
    b.poly({{12.f, 2.f}, {18.f, 12.f}, {6.f, 12.f}}, red ? 3 : 4);
    b.outline(5, false);
    return b;
}

Bitmap clockArt(float hand) {
    Bitmap b(40, 40);
    b.ellipse(20, 20, 18, 18, 5);
    b.ellipse(20, 20, 15, 15, 2);
    b.ellipse(20, 20, 13, 13, 1);
    for (int i = 0; i < 8; i++) {
        float a = i * kTau / 8.f - kPi * 0.5f;
        b.line(20 + std::cos(a) * 9.f, 20 + std::sin(a) * 9.f, 20 + std::cos(a) * 12.5f, 20 + std::sin(a) * 12.5f, 3, 1.4f);
    }
    b.line(20, 20, 20 + std::cos(hand) * 10.f, 20 + std::sin(hand) * 10.f, 4, 2.f);
    b.ellipse(20, 20, 2.1f, 2.1f, 4);
    b.outline(3, false);
    return b;
}

Bitmap foamArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 3.5f, 1);
    b.ellipse(8, 8, 3, 2, 2);
    return b;
}

Bitmap gullArt(bool up) {
    Bitmap b(28, 16);
    b.ellipse(14, 9, 3.5f, 2.f, 1);
    float tip = up ? 2.f : 13.f;
    b.line(14, 8, 2, tip, 1, 1.5f);
    b.line(14, 8, 26, tip, 2, 1.5f);
    b.set(18, 8, 3);
    return b;
}

Bitmap guideArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 2.6f, 2.6f, 1);
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

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int fill, int edge) {
    gs::TextStyle st{scale, fill, edge, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FERRY, {0, gs::rgb4(15, 15, 15), gs::rgb4(2, 10, 6), gs::rgb4(14, 13, 10), gs::rgb4(13, 2, 2),
                            gs::rgb4(15, 12, 2), gs::rgb4(3, 8, 14), gs::rgb4(1, 1, 2), gs::rgb4(14, 7, 2),
                            gs::rgb4(1, 6, 4), gs::rgb4(6, 7, 8), 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PIER, {0, gs::rgb4(9, 9, 10), gs::rgb4(8, 5, 2), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4),
                           gs::rgb4(2, 2, 3), gs::rgb4(12, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SHED, {0, gs::rgb4(12, 11, 9), gs::rgb4(10, 3, 3), gs::rgb4(4, 5, 7), gs::rgb4(6, 12, 14),
                           gs::rgb4(2, 2, 3), gs::rgb4(8, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(7, 8, 9), gs::rgb4(15, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(12, 3, 2), gs::rgb4(15, 15, 15), gs::rgb4(14, 2, 2), gs::rgb4(2, 12, 4),
                           gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(15, 14, 11), gs::rgb4(15, 15, 15), gs::rgb4(2, 2, 3), gs::rgb4(13, 2, 2),
                            gs::rgb4(4, 5, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GUIDE, {0, gs::rgb4(8, 15, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(4, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 8), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(10, 12, 13), gs::rgb4(2, 3, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.hull[i] = gs::uploadMipped(vdp, paintFerry(i * kTau / 16.f));
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.apron = gs::uploadMipped(vdp, apronArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.bridge = gs::uploadMipped(vdp, bridgeArt());
    art.buoy[0] = gs::uploadMipped(vdp, buoyArt(true));
    art.buoy[1] = gs::uploadMipped(vdp, buoyArt(false));
    for (int i = 0; i < 8; i++) {
        float u = i / 7.f;
        art.clock[i] = gs::uploadMipped(vdp, clockArt(-kPi * 0.5f + u * kPi));
    }
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(true));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(false));
    art.guide = gs::uploadMipped(vdp, guideArt());
    art.title = words(vdp, "S3 FERRY", 4, 1, 2);
    art.docked = words(vdp, "DOCKED", 4, 1, 2);
    art.tideOut = words(vdp, "TIDE OUT", 3, 1, 2);
    art.paused = words(vdp, "PAUSED", 4, 1, 2);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
}

}  // namespace ferry
