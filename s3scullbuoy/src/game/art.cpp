#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace scull {
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
    // Bow is +local Y. Oars reach forward on the catch and aft on the finish.
    const float oar = phase ? -12.f : 14.f;
    const float blade = phase ? -18.f : 20.f;
    stroke(-4.f, 2.f, -30.f, oar, 3, 2.2f);
    stroke(4.f, 2.f, 30.f, oar, 3, 2.2f);
    poly({{-30.f, blade - 3.f}, {-24.f, blade + 4.f}, {-33.f, blade + 1.f}}, 4);
    poly({{30.f, blade - 3.f}, {24.f, blade + 4.f}, {33.f, blade + 1.f}}, 4);
    poly({{0.f, 30.f}, {5.5f, 22.f}, {6.2f, -16.f}, {4.f, -24.f}, {-4.f, -24.f}, {-6.2f, -16.f}, {-5.5f, 22.f}}, 1);
    poly({{0.f, 26.f}, {3.2f, 18.f}, {3.4f, -14.f}, {2.f, -20.f}, {-2.f, -20.f}, {-3.4f, -14.f}, {-3.2f, 18.f}}, 2);
    stroke(-7.f, 4.f, 7.f, 4.f, 5, 2.4f);
    stroke(0.f, 6.f, 0.f, -2.f, 5, 1.6f);
    poly({{-2.2f, 1.f}, {2.2f, 1.f}, {2.2f, 7.f}, {-2.2f, 7.f}}, 7);
    b.ellipse(36 + (0 * c - 4 * s), 36 - (0 * s + 4 * c), 2.1f, 2.1f, 6);
    b.outline(8, false);
    return b;
}

Bitmap paintBuoy(bool gold) {
    Bitmap b(28, 40);
    b.ellipse(14, 30, 9, 5, 1);
    b.ellipse(14, 29, 5, 2.4f, 2);
    b.rect(12, 14, 4, 14, 3);
    b.poly({{14.f, 3.f}, {22.f, 16.f}, {6.f, 16.f}}, gold ? 5 : 4);
    b.rect(12, 18, 4, 3, gold ? 4 : 5);
    b.outline(6, false);
    return b;
}

Bitmap paintDock() {
    Bitmap b(96, 64);
    b.rect(8, 8, 80, 48, 1);
    for (int y = 12; y < 52; y += 6) b.rect(10, y, 76, 2, 2);
    for (int x = 16; x < 84; x += 14) b.rect(x, 8, 3, 48, 3);
    b.rect(40, 18, 16, 22, 4);
    b.rect(44, 28, 8, 12, 5);
    b.rect(4, 20, 8, 6, 6);
    b.rect(84, 20, 8, 6, 6);
    b.outline(7, false);
    return b;
}

Bitmap paintReed() {
    Bitmap b(28, 36);
    b.ellipse(14, 30, 10, 4, 1);
    b.line(14, 28, 8, 6, 2, 2.2f);
    b.line(14, 28, 14, 4, 3, 2.2f);
    b.line(14, 28, 21, 8, 2, 2.2f);
    b.line(10, 18, 4, 12, 4, 1.6f);
    b.line(18, 16, 24, 10, 4, 1.6f);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 7, 3.2f, 1);
    b.ellipse(8, 6, 3.5f, 1.6f, 2);
    return b;
}

Bitmap paintChevron() {
    Bitmap b(16, 16);
    b.poly({{8.f, 2.f}, {14.f, 12.f}, {8.f, 9.f}, {2.f, 12.f}}, 1);
    b.outline(2, false);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 4), gs::rgb4(8, 14, 12), gs::rgb4(15, 6, 4),
                          gs::rgb4(6, 8, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 4)});
    setPal(vdp, PAL_SHELL, {0, gs::rgb4(15, 15, 13), gs::rgb4(12, 8, 3), gs::rgb4(7, 5, 2), gs::rgb4(13, 2, 2),
                            gs::rgb4(5, 5, 6), gs::rgb4(14, 9, 6), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3),
                            gs::rgb4(9, 13, 15)});
    setPal(vdp, PAL_RIVAL, {0, gs::rgb4(6, 8, 13), gs::rgb4(3, 4, 8), gs::rgb4(4, 4, 5), gs::rgb4(15, 13, 3),
                            gs::rgb4(8, 8, 9), gs::rgb4(12, 8, 6), gs::rgb4(2, 2, 4), gs::rgb4(1, 1, 3),
                            gs::rgb4(8, 10, 14)});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(2, 6, 8), gs::rgb4(6, 12, 14), gs::rgb4(12, 12, 12), gs::rgb4(14, 2, 2),
                           gs::rgb4(15, 12, 2), gs::rgb4(1, 1, 2), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(10, 7, 3), gs::rgb4(7, 5, 2), gs::rgb4(13, 9, 4), gs::rgb4(5, 6, 7),
                           gs::rgb4(3, 3, 4), gs::rgb4(14, 12, 6), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_SHORE, {0, gs::rgb4(2, 7, 3), gs::rgb4(3, 10, 4), gs::rgb4(5, 12, 5), gs::rgb4(8, 12, 4),
                            gs::rgb4(1, 4, 2)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(13, 15, 15), gs::rgb4(8, 13, 15)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 14, 6), gs::rgb4(8, 6, 1)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 6, 11), gs::rgb4(3, 8, 13), gs::rgb4(4, 10, 14), gs::rgb4(2, 5, 9)});

    uint8_t water[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int c = 1;
            if (((x + y * 3) % 7) == 0) c = 2;
            if (((x * 2 + y) % 11) == 0) c = 3;
            water[y * 8 + x] = uint8_t(c);
        }
    vdp.loadTile(1, water);
    art.waterTile = 1;

    for (int h = 0; h < 8; h++) {
        float a = h * (kPi * 2.f / 8.f);
        art.shell[h * 2] = gs::uploadMipped(vdp, paintShell(a, 0));
        art.shell[h * 2 + 1] = gs::uploadMipped(vdp, paintShell(a, 1));
    }
    art.buoy[0] = gs::uploadMipped(vdp, paintBuoy(false));
    art.buoy[1] = gs::uploadMipped(vdp, paintBuoy(true));
    art.dock = gs::uploadMipped(vdp, paintDock());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.chevron = gs::uploadMipped(vdp, paintChevron());
    loadFont(vdp, art);
}

}  // namespace scull
