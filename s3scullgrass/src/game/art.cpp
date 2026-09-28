#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace scullgrass {
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
    Bitmap b(84, 84);
    const float cx = 42.f, cy = 42.f;
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
    // Single scull: two sculls, catch reaches forward, finish pulls aft.
    const float shaft = phase ? 6.f : -18.f;
    const float blade = phase ? 12.f : -24.f;
    stroke(-4.5f, 0.f, -36.f, shaft, 4, 2.2f);
    stroke(4.5f, 0.f, 36.f, shaft, 4, 2.2f);
    poly({{-38.f, blade - 1.f}, {-30.f, blade - 2.f}, {-30.f, blade + 7.f}, {-38.f, blade + 6.f}}, 5);
    poly({{38.f, blade - 1.f}, {30.f, blade - 2.f}, {30.f, blade + 7.f}, {38.f, blade + 6.f}}, 5);
    poly({{0.f, 36.f}, {3.6f, 26.f}, {4.0f, -20.f}, {2.2f, -32.f}, {-2.2f, -32.f}, {-4.0f, -20.f}, {-3.6f, 26.f}}, 1);
    poly({{0.f, 30.f}, {1.7f, 18.f}, {1.8f, -16.f}, {1.0f, -26.f}, {-1.0f, -26.f}, {-1.8f, -16.f}, {-1.7f, 18.f}}, 2);
    stroke(-7.f, -4.f, 7.f, -4.f, 3, 2.0f);
    poly({{-2.2f, -8.f}, {2.2f, -8.f}, {1.8f, 0.f}, {-1.8f, 0.f}}, 6);
    b.ellipse(cx - (-2.f) * s, cy - (-2.f) * c, 1.6f, 1.6f, 7);
    b.outline(8, false);
    return b;
}

Bitmap paintMat() {
    Bitmap b(48, 32);
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 48; x++) {
            int n = (x * 13 + y * 7 + (x / 3) * (y / 2)) % 11;
            int c = 1;
            if (n == 0) c = 3;
            else if (n < 3) c = 2;
            else if (n == 8) c = 4;
            if (y < 3) c = 5;
            b.set(x, y, c);
        }
    }
    return b;
}

Bitmap paintTuft() {
    Bitmap b(18, 16);
    b.line(9, 14, 4, 3, 2, 1.6f);
    b.line(9, 14, 9, 1, 3, 1.8f);
    b.line(9, 14, 14, 4, 2, 1.6f);
    b.line(7, 10, 3, 7, 4, 1.3f);
    b.ellipse(9, 14, 5, 2, 1);
    return b;
}

Bitmap paintReed() {
    Bitmap b(22, 30);
    b.ellipse(11, 25, 7, 2.6f, 1);
    b.line(11, 24, 5, 5, 2, 1.8f);
    b.line(11, 24, 11, 2, 3, 1.8f);
    b.line(11, 24, 17, 6, 2, 1.8f);
    return b;
}

Bitmap paintDock() {
    Bitmap b(72, 28);
    b.rect(2, 6, 68, 16, 1);
    for (int x = 6; x < 66; x += 8) b.rect(x, 6, 2, 16, 2);
    b.rect(4, 18, 6, 6, 3);
    b.rect(62, 18, 6, 6, 3);
    b.rect(30, 10, 12, 8, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintStake() {
    Bitmap b(10, 22);
    b.rect(4, 4, 2, 16, 1);
    b.rect(2, 2, 6, 4, 2);
    b.rect(3, 18, 4, 3, 3);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(12, 8);
    b.ellipse(6, 4, 5, 2.4f, 1);
    b.ellipse(6, 4, 2.2f, 1.1f, 2);
    return b;
}

Bitmap paintTree() {
    Bitmap b(28, 36);
    b.rect(12, 20, 4, 14, 1);
    b.ellipse(14, 14, 10, 9, 2);
    b.ellipse(10, 12, 5, 4, 3);
    b.outline(4, false);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 13, 4), gs::rgb4(8, 15, 10), gs::rgb4(15, 6, 4),
                          gs::rgb4(6, 10, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 3)});
    setPal(vdp, PAL_SHELL, {0, gs::rgb4(15, 15, 13), gs::rgb4(12, 13, 14), gs::rgb4(4, 4, 5), gs::rgb4(7, 6, 5),
                            gs::rgb4(4, 12, 6), gs::rgb4(13, 4, 3), gs::rgb4(14, 10, 7), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(3, 9, 3), gs::rgb4(4, 11, 3), gs::rgb4(6, 13, 4), gs::rgb4(2, 7, 2),
                            gs::rgb4(5, 8, 3)});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(6, 8, 3), gs::rgb4(8, 10, 4), gs::rgb4(4, 6, 2)});
    setPal(vdp, PAL_REED, {0, gs::rgb4(2, 6, 2), gs::rgb4(3, 10, 3), gs::rgb4(7, 13, 4)});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(10, 7, 4), gs::rgb4(7, 5, 2), gs::rgb4(5, 4, 3), gs::rgb4(12, 11, 7),
                           gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(13, 15, 15), gs::rgb4(8, 12, 14)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 5, 10), gs::rgb4(3, 7, 12), gs::rgb4(4, 9, 13), gs::rgb4(1, 3, 8)});
    setPal(vdp, PAL_STAKE, {0, gs::rgb4(12, 12, 11), gs::rgb4(14, 3, 3), gs::rgb4(6, 5, 4), gs::rgb4(2, 5, 2)});

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
    art.mat = gs::uploadMipped(vdp, paintMat());
    art.tuft = gs::uploadMipped(vdp, paintTuft());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.dock = gs::uploadMipped(vdp, paintDock());
    art.stake = gs::uploadMipped(vdp, paintStake());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.tree = gs::uploadMipped(vdp, paintTree());
    loadFont(vdp, art);
}

}  // namespace scullgrass
