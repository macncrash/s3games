#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace scullbox {
namespace {

constexpr float kPi = 3.14159265f;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    return {cx + lx * c - ly * s, cy - lx * s - ly * c};
}

Bitmap paintShell(float heading, int phase) {
    Bitmap b(80, 80);
    const float cx = 40.f, cy = 40.f;
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
    // Local +Y is the bow. Catch reaches forward; the finish pulls the blades aft.
    const float shaft = phase ? 8.f : -16.f;
    const float blade = phase ? 14.f : -22.f;
    stroke(-5.f, -2.f, -34.f, shaft, 4, 2.4f);
    stroke(5.f, -2.f, 34.f, shaft, 4, 2.4f);
    poly({{-36.f, blade - 2.f}, {-28.f, blade - 2.f}, {-28.f, blade + 6.f}, {-36.f, blade + 5.f}}, 5);
    poly({{36.f, blade - 2.f}, {28.f, blade - 2.f}, {28.f, blade + 6.f}, {36.f, blade + 5.f}}, 5);
    poly({{0.f, 34.f}, {4.2f, 24.f}, {4.6f, -18.f}, {2.6f, -30.f}, {-2.6f, -30.f}, {-4.6f, -18.f}, {-4.2f, 24.f}}, 1);
    poly({{0.f, 28.f}, {2.2f, 16.f}, {2.2f, -16.f}, {1.2f, -24.f}, {-1.2f, -24.f}, {-2.2f, -16.f}, {-2.2f, 16.f}}, 2);
    stroke(-6.f, -6.f, 6.f, -6.f, 3, 2.2f);
    poly({{-2.4f, -10.f}, {2.4f, -10.f}, {2.0f, -2.f}, {-2.0f, -2.f}}, 6);
    b.ellipse(cx + (0 * c - (-6.f) * s), cy - (0 * s + (-6.f) * c), 1.8f, 1.8f, 7);
    b.outline(8, false);
    return b;
}

Bitmap paintPost() {
    Bitmap b(16, 28);
    b.rect(6, 4, 4, 20, 1);
    b.rect(4, 2, 8, 5, 2);
    b.rect(5, 22, 6, 3, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintDash() {
    Bitmap b(18, 8);
    b.rect(1, 2, 16, 4, 1);
    b.rect(1, 2, 16, 1, 2);
    return b;
}

Bitmap paintDock() {
    Bitmap b(88, 40);
    b.rect(4, 8, 80, 24, 1);
    for (int x = 8; x < 80; x += 10) b.rect(x, 8, 3, 24, 2);
    b.rect(36, 14, 16, 12, 3);
    b.rect(2, 16, 6, 4, 4);
    b.rect(80, 16, 6, 4, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintReed() {
    Bitmap b(24, 32);
    b.ellipse(12, 26, 8, 3.2f, 1);
    b.line(12, 24, 6, 6, 2, 2.f);
    b.line(12, 24, 12, 3, 3, 2.f);
    b.line(12, 24, 18, 7, 2, 2.f);
    b.line(8, 14, 3, 10, 4, 1.5f);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(14, 10);
    b.ellipse(7, 5, 6, 3, 1);
    b.ellipse(7, 5, 2.6f, 1.3f, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 2);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 13, 5), gs::rgb4(8, 14, 12), gs::rgb4(15, 6, 4),
                          gs::rgb4(7, 9, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 4)});
    setPal(vdp, PAL_SHELL, {0, gs::rgb4(14, 11, 6), gs::rgb4(10, 6, 3), gs::rgb4(4, 3, 3), gs::rgb4(8, 7, 6),
                            gs::rgb4(15, 15, 13), gs::rgb4(12, 4, 3), gs::rgb4(13, 9, 6), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_BOX, {0, gs::rgb4(15, 15, 14), gs::rgb4(14, 3, 3), gs::rgb4(6, 5, 4), gs::rgb4(3, 2, 2),
                          gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(9, 7, 4), gs::rgb4(6, 4, 2), gs::rgb4(12, 10, 6), gs::rgb4(13, 12, 8),
                           gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_REED, {0, gs::rgb4(2, 6, 2), gs::rgb4(3, 9, 3), gs::rgb4(6, 12, 4), gs::rgb4(9, 13, 5)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(12, 15, 15), gs::rgb4(7, 12, 14)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 5, 10), gs::rgb4(3, 7, 12), gs::rgb4(4, 9, 14), gs::rgb4(1, 4, 8)});

    uint8_t water[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int c = 1;
            if (((x * 3 + y) % 9) == 0) c = 2;
            if (((x + y * 2) % 13) == 0) c = 3;
            water[y * 8 + x] = uint8_t(c);
        }
    vdp.loadTile(1, water);
    art.waterTile = 1;

    for (int h = 0; h < 8; h++) {
        float a = h * (kPi * 2.f / 8.f);
        art.shell[h * 2] = gs::uploadMipped(vdp, paintShell(a, 0));
        art.shell[h * 2 + 1] = gs::uploadMipped(vdp, paintShell(a, 1));
    }
    art.post = gs::uploadMipped(vdp, paintPost());
    art.dash = gs::uploadMipped(vdp, paintDash());
    art.dock = gs::uploadMipped(vdp, paintDock());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    loadFont(vdp, art);
}

}  // namespace scullbox
