#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace gbuoy {
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
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t edge) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 2, edge);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

// Local +ly is the nose. Heading 0 points east; screen +y is south.
Pt spin(float cx, float cy, float lx, float ly, float h) {
    float c = std::cos(h), s = std::sin(h);
    return {cx + ly * c + lx * s, cy - ly * s + lx * c};
}

Bitmap paintWing(float heading) {
    Bitmap b(120, 120);
    const float cx = 60.f, cy = 60.f;
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        w.reserve(local.size());
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, heading));
        b.poly(w, col);
    };
    auto stroke = [&](float ax, float ay, float bx, float by, int col, float th) {
        Pt A = spin(cx, cy, ax, ay, heading);
        Pt B = spin(cx, cy, bx, by, heading);
        b.line(A.first, A.second, B.first, B.second, col, th);
    };
    auto blob = [&](float lx, float ly, float rx, float ry, int col) {
        Pt p = spin(cx, cy, lx, ly, heading);
        b.ellipse(p.first, p.second, rx, ry, col);
    };
    // Long wing first, then the fuselage, so the sailplane reads at 32px.
    poly({{-46.f, 6.f}, {-44.f, -1.f}, {-8.f, 1.f}, {0.f, 7.f}, {8.f, 1.f}, {44.f, -1.f}, {46.f, 6.f}, {8.f, 9.f},
          {-8.f, 9.f}},
         1);
    poly({{-40.f, 5.f}, {-8.f, 3.f}, {0.f, 6.f}, {8.f, 3.f}, {40.f, 5.f}, {8.f, 8.f}, {-8.f, 8.f}}, 2);
    stroke(-42.f, 6.5f, -6.f, 8.f, 3, 2.2f);
    stroke(42.f, 6.5f, 6.f, 8.f, 3, 2.2f);
    poly({{-3.2f, 34.f}, {3.2f, 28.f}, {4.2f, 8.f}, {3.4f, -16.f}, {1.6f, -30.f}, {-1.6f, -30.f}, {-3.4f, -16.f},
          {-4.2f, 8.f}, {-3.2f, 28.f}},
         1);
    poly({{-1.6f, 30.f}, {1.6f, 26.f}, {2.2f, 6.f}, {1.4f, -18.f}, {-1.4f, -18.f}, {-2.2f, 6.f}, {-1.6f, 26.f}}, 2);
    poly({{-13.f, -20.f}, {13.f, -20.f}, {11.f, -25.f}, {-11.f, -25.f}}, 1);
    poly({{-9.f, -21.f}, {9.f, -21.f}, {8.f, -24.f}, {-8.f, -24.f}}, 2);
    blob(0.f, 16.f, 2.6f, 6.2f, 4);
    blob(-0.6f, 18.f, 1.1f, 2.4f, 6);
    poly({{0.f, 36.f}, {2.4f, 30.f}, {-2.4f, 30.f}}, 5);
    poly({{-2.2f, -27.f}, {2.2f, -27.f}, {1.2f, -33.f}, {-1.2f, -33.f}}, 5);
    stroke(0.f, 28.f, 0.f, -26.f, 3, 1.3f);
    b.outline(7, false);
    return b;
}

Bitmap paintSpar() {
    Bitmap b(40, 40);
    b.ellipse(20, 20, 15, 15, 1);
    b.ellipse(20, 20, 9, 9, 2);
    b.ellipse(20, 20, 3.2f, 3.2f, 1);
    b.line(20, 5, 20, 35, 5, 2.f);
    b.line(5, 20, 35, 20, 5, 2.f);
    b.poly({{20, 4}, {27, 12}, {13, 12}}, 4);
    b.outline(3, false);
    gs::TextStyle st{2, 6, 0, 0, 1};
    Bitmap t = gs::textBitmap("1", st);
    b.blit(t, 20 - t.w / 2, 16);
    return b;
}

Bitmap paintCone() {
    Bitmap b(40, 40);
    b.poly({{20, 3}, {36, 20}, {20, 37}, {4, 20}}, 1);
    b.poly({{20, 10}, {29, 20}, {20, 30}, {11, 20}}, 2);
    b.ellipse(20, 20, 4, 4, 6);
    b.outline(3, false);
    gs::TextStyle st{2, 5, 0, 0, 1};
    Bitmap t = gs::textBitmap("2", st);
    b.blit(t, 20 - t.w / 2, 15);
    return b;
}

Bitmap paintDrum() {
    Bitmap b(44, 44);
    b.ellipse(22, 22, 18, 16, 1);
    b.ellipse(22, 22, 18, 6, 2);
    b.ellipse(22, 22, 8, 8, 6);
    b.ellipse(22, 16, 5, 3, 4);
    b.outline(3, false);
    gs::TextStyle st{2, 5, 0, 0, 1};
    Bitmap t = gs::textBitmap("3", st);
    b.blit(t, 22 - t.w / 2, 17);
    return b;
}

Bitmap paintDock() {
    Bitmap b(96, 140);
    // North is the top of the bitmap. The T is the threshold you fly onto.
    b.rect(32, 6, 32, 128, 1);
    for (int y = 8; y < 132; y += 7) b.rect(33, y, 30, 2, 2);
    b.rect(32, 6, 32, 4, 3);
    b.rect(32, 126, 32, 6, 3);
    b.rect(30, 18, 4, 10, 4);
    b.rect(62, 18, 4, 10, 4);
    b.rect(30, 118, 4, 12, 4);
    b.rect(62, 118, 4, 12, 4);
    b.rect(36, 28, 24, 4, 5);
    b.rect(46, 28, 4, 36, 5);
    b.poly({{48, 70}, {56, 78}, {40, 78}}, 6);
    b.rect(44, 100, 8, 14, 7);
    return b;
}

Bitmap paintSock(int frame) {
    Bitmap b(28, 36);
    b.rect(12, 14, 3, 20, 5);
    b.ellipse(13, 14, 4, 3, 3);
    float wag = (frame - 1) * 3.f;
    b.poly({{14, 12}, {24.f + wag, 6.f}, {22.f + wag * 0.5f, 14.f}, {14, 16}}, 1);
    b.poly({{16, 12}, {22.f + wag, 8.f}, {20.f + wag * 0.4f, 13.f}}, 2);
    return b;
}

Bitmap paintHut() {
    Bitmap b(36, 28);
    b.rect(6, 12, 24, 12, 1);
    b.poly({{4, 12}, {18, 3}, {32, 12}}, 6);
    b.rect(16, 16, 6, 8, 3);
    b.rect(8, 15, 5, 4, 5);
    b.outline(3, false);
    return b;
}

Bitmap paintPost() {
    Bitmap b(14, 22);
    b.rect(6, 4, 3, 16, 4);
    b.ellipse(7, 4, 4, 3, 5);
    b.rect(5, 18, 5, 3, 3);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(16, 22);
    b.rect(3, 2, 2, 18, 4);
    b.poly({{5, 3}, {14, 7}, {5, 11}}, 6);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(32, 16);
    if (up) {
        b.poly({{2, 10}, {14, 6}, {16, 8}, {14, 7}, {2, 8}}, 1);
        b.poly({{30, 10}, {18, 6}, {16, 8}, {18, 7}, {30, 8}}, 1);
    } else {
        b.poly({{2, 4}, {14, 8}, {16, 7}, {14, 9}, {2, 8}}, 1);
        b.poly({{30, 4}, {18, 8}, {16, 7}, {18, 9}, {30, 8}}, 1);
    }
    b.ellipse(16, 8, 2.2f, 1.6f, 2);
    b.rect(17, 7, 3, 1, 3);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(24, 16);
    b.ellipse(8, 8, 6, 3, 1);
    b.ellipse(15, 7, 5, 3, 2);
    b.ellipse(12, 10, 4, 2, 1);
    return b;
}

Bitmap paintRing() {
    Bitmap b(64, 64);
    for (int i = 0; i < 48; i++) {
        float a = i * kTau / 48.f;
        b.line(32 + std::cos(a) * 22.f, 32 + std::sin(a) * 22.f, 32 + std::cos(a) * 28.f, 32 + std::sin(a) * 28.f, 1,
               2.4f);
    }
    return b;
}

Bitmap paintLift() {
    Bitmap b(48, 48);
    for (int i = 0; i < 28; i++) {
        float a = i * kTau / 28.f;
        b.ellipse(24 + std::cos(a) * 16.f, 24 + std::sin(a) * 16.f, 2.2f, 2.2f, (i & 1) ? 1 : 2);
    }
    return b;
}

Bitmap paintShade() {
    Bitmap b(48, 28);
    b.ellipse(24, 14, 20, 8, 1);
    return b;
}

Bitmap paintPin() {
    Bitmap b(16, 16);
    b.poly({{8, 1}, {14, 8}, {10, 8}, {10, 14}, {6, 14}, {6, 8}, {2, 8}}, 1);
    b.outline(2, false);
    return b;
}

Bitmap paintPanel() {
    Bitmap b(72, 80);
    b.rect(1, 1, 70, 78, 1);
    b.rect(3, 3, 66, 74, 2);
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
    a[4 * 8 + 4] = 2;
    a[6 * 8 + 6] = 1;
    w[1 * 8 + 5] = 1;
    w[3 * 8 + 2] = 2;
    w[5 * 8 + 7] = 1;
    int t0 = tiles.alloc(1);
    int t1 = tiles.alloc(1);
    vdp.loadTile(t0, a);
    vdp.loadTile(t1, w);
    for (int cy = 0; cy < vdp.B.h; cy++) {
        for (int cx = 0; cx < vdp.B.w; cx++) {
            int tile = ((cx * 3 + cy) & 3) == 0 ? t1 : t0;
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
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14), gs::rgb4(4, 6, 8));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 14, 10), gs::rgb4(4, 3, 2));
    textPal(vdp, PAL_WIN, gs::rgb4(8, 15, 7), gs::rgb4(1, 4, 2));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 3), gs::rgb4(4, 1, 1));
    textPal(vdp, PAL_CREW, gs::rgb4(15, 11, 2), gs::rgb4(4, 2, 1));

    setPal(vdp, PAL_SAIL,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(9, 11, 13), gs::rgb4(3, 7, 13), gs::rgb4(6, 12, 15),
            gs::rgb4(15, 8, 2), gs::rgb4(14, 14, 12), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_SPAR,
           {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(5, 1, 1), gs::rgb4(15, 7, 2), gs::rgb4(6, 5, 4),
            gs::rgb4(1, 1, 2), gs::rgb4(15, 12, 10)});
    setPal(vdp, PAL_CONE,
           {0, gs::rgb4(2, 12, 4), gs::rgb4(15, 15, 14), gs::rgb4(1, 5, 2), gs::rgb4(8, 14, 6), gs::rgb4(1, 1, 2),
            gs::rgb4(10, 15, 10)});
    setPal(vdp, PAL_DRUM,
           {0, gs::rgb4(14, 11, 2), gs::rgb4(15, 15, 13), gs::rgb4(7, 5, 1), gs::rgb4(15, 14, 8), gs::rgb4(1, 1, 2),
            gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_DOCK,
           {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 2), gs::rgb4(4, 3, 2), gs::rgb4(15, 15, 13),
            gs::rgb4(13, 3, 2), gs::rgb4(9, 6, 3), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 15, 15), gs::rgb4(7, 12, 13)});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(7, 8, 9), gs::rgb4(14, 6, 2)});
    setPal(vdp, PAL_LIFT, {0, gs::rgb4(10, 15, 15), gs::rgb4(14, 15, 15), gs::rgb4(5, 10, 12)});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(10, 14, 15), gs::rgb4(1, 4, 7), gs::rgb4(6, 10, 12)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(1, 2, 4)});
    setPal(vdp, PAL_WAVE, {0, gs::rgb4(8, 14, 15), gs::rgb4(13, 15, 15)});

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.wing[i] = gs::uploadMipped(vdp, paintWing(i * kTau / 16.f));
    art.spar = gs::uploadMipped(vdp, paintSpar());
    art.cone = gs::uploadMipped(vdp, paintCone());
    art.drum = gs::uploadMipped(vdp, paintDrum());
    art.dock = gs::uploadMipped(vdp, paintDock());
    for (int i = 0; i < 3; i++) art.sock[i] = gs::uploadMipped(vdp, paintSock(i));
    art.hut = gs::uploadMipped(vdp, paintHut());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(true));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(false));
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.ring = gs::uploadMipped(vdp, paintRing());
    art.lift = gs::uploadMipped(vdp, paintLift());
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.panel = gs::uploadMipped(vdp, paintPanel());
    Bitmap dot(8, 8);
    dot.ellipse(4, 4, 3.f, 3.f, 1);
    art.dot = gs::uploadMipped(vdp, dot);
    art.title = words(vdp, "S3 GLIDER BUOY", 2);
    art.paused = words(vdp, "PAUSED", 3);
    art.sameDock = words(vdp, "SAME DOCK", 3);
    art.ahead = words(vdp, "AHEAD OF THE CREW", 2);
    art.ditched = words(vdp, "DITCHED", 3);
    art.crewTook = words(vdp, "CREW TOOK IT", 2);

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(1, 4, 8));
}

}  // namespace gbuoy
