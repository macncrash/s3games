#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace scullslip {
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

Bitmap paintShell(float heading, int phase, bool rival) {
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
    const int hull = rival ? 8 : 1;
    const int stripe = rival ? 9 : 2;
    const int oar = rival ? 10 : 4;
    const int blade = rival ? 11 : 5;
    const float reach = phase ? 8.f : -16.f;
    const float dip = phase ? 14.f : -22.f;
    stroke(-3.2f, 2.f, -40.f, reach, oar, 1.7f);
    stroke(3.2f, 2.f, 40.f, reach, oar, 1.7f);
    poly({{-42.f, dip - 1.f}, {-33.f, dip - 2.f}, {-33.f, dip + 6.f}, {-42.f, dip + 5.f}}, blade);
    poly({{42.f, dip - 1.f}, {33.f, dip - 2.f}, {33.f, dip + 6.f}, {42.f, dip + 5.f}}, blade);
    poly({{0.f, 40.f}, {2.6f, 28.f}, {3.1f, -18.f}, {1.6f, -36.f}, {-1.6f, -36.f}, {-3.1f, -18.f}, {-2.6f, 28.f}}, hull);
    poly({{0.f, 32.f}, {1.2f, 16.f}, {1.3f, -12.f}, {0.7f, -28.f}, {-0.7f, -28.f}, {-1.3f, -12.f}, {-1.2f, 16.f}}, stripe);
    stroke(-5.5f, -2.f, 5.5f, -2.f, 3, 1.6f);
    poly({{-1.6f, -6.f}, {1.6f, -6.f}, {1.3f, 2.f}, {-1.3f, 2.f}}, 6);
    b.ellipse(cx + 1.4f * s, cy - 1.4f * c, 1.4f, 1.4f, 7);
    b.outline(12, false);
    return b;
}

Bitmap paintPlank() {
    Bitmap b(40, 16);
    b.rect(1, 2, 38, 12, 1);
    for (int x = 4; x < 36; x += 6) b.rect(x, 2, 1, 12, 2);
    b.rect(2, 3, 36, 2, 3);
    b.rect(8, 11, 4, 3, 4);
    b.rect(28, 11, 4, 3, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintPile() {
    Bitmap b(12, 28);
    b.rect(4, 2, 4, 22, 1);
    b.rect(2, 1, 8, 4, 2);
    b.rect(3, 22, 6, 4, 3);
    b.line(5, 6, 7, 20, 4, 1.2f);
    return b;
}

Bitmap paintReed() {
    Bitmap b(20, 28);
    b.ellipse(10, 24, 6, 2.2f, 1);
    b.line(10, 23, 4, 4, 2, 1.6f);
    b.line(10, 23, 10, 1, 3, 1.7f);
    b.line(10, 23, 16, 5, 2, 1.6f);
    b.ellipse(4, 4, 2.2f, 1.4f, 4);
    b.ellipse(16, 5, 2.2f, 1.4f, 4);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(14, 8);
    b.ellipse(7, 4, 6, 2.6f, 1);
    b.ellipse(5, 4, 2.f, 1.f, 2);
    b.ellipse(10, 3, 1.6f, 0.8f, 2);
    return b;
}

Bitmap paintMark() {
    Bitmap b(22, 18);
    b.rect(2, 4, 18, 10, 1);
    b.rect(8, 2, 6, 14, 2);
    b.rect(4, 7, 14, 3, 3);
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
    a.waterTile = 1;
    uint8_t water[64];
    for (int i = 0; i < 64; i++) {
        int x = i % 8, y = i / 8;
        int n = (x * 3 + y * 5) & 7;
        water[i] = n == 0 ? 3 : (n < 3 ? 2 : 1);
    }
    vdp.loadTile(1, water);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 13), gs::rgb4(15, 12, 3), gs::rgb4(6, 15, 9), gs::rgb4(15, 5, 3),
                          gs::rgb4(8, 12, 15), gs::rgb4(4, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_SHELL, {0, gs::rgb4(14, 13, 10), gs::rgb4(9, 4, 2), gs::rgb4(6, 5, 6), gs::rgb4(11, 8, 4),
                            gs::rgb4(12, 3, 2), gs::rgb4(3, 3, 4), gs::rgb4(15, 12, 8), gs::rgb4(8, 9, 11),
                            gs::rgb4(4, 5, 8), gs::rgb4(10, 9, 6), gs::rgb4(3, 6, 10), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_PIER, {0, gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(13, 10, 5), gs::rgb4(4, 3, 2),
                           gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_PILE, {0, gs::rgb4(8, 8, 7), gs::rgb4(12, 11, 8), gs::rgb4(4, 4, 3), gs::rgb4(6, 7, 8)});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(8, 9, 11), gs::rgb4(4, 5, 8), gs::rgb4(6, 5, 6), gs::rgb4(10, 9, 6),
                           gs::rgb4(3, 6, 10), gs::rgb4(3, 3, 4), gs::rgb4(15, 12, 8), gs::rgb4(14, 13, 10),
                           gs::rgb4(9, 4, 2), gs::rgb4(11, 8, 4), gs::rgb4(12, 3, 2), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 14, 6), gs::rgb4(12, 4, 3), gs::rgb4(15, 15, 12), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(13, 14, 15), gs::rgb4(8, 11, 13)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(1, 4, 8), gs::rgb4(2, 6, 11), gs::rgb4(4, 8, 12), gs::rgb4(8, 12, 14)});
    setPal(vdp, PAL_REED, {0, gs::rgb4(2, 5, 2), gs::rgb4(3, 8, 3), gs::rgb4(5, 11, 4), gs::rgb4(8, 10, 3)});

    for (int face = 0; face < 8; face++) {
        float h = face * (kPi * 0.25f);
        art.shell[face * 2] = gs::uploadMipped(vdp, paintShell(h, 0, false));
        art.shell[face * 2 + 1] = gs::uploadMipped(vdp, paintShell(h, 1, false));
    }
    art.crew = gs::uploadMipped(vdp, paintShell(0.f, 0, true));
    art.plank = gs::uploadMipped(vdp, paintPlank());
    art.pile = gs::uploadMipped(vdp, paintPile());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.mark = gs::uploadMipped(vdp, paintMark());
    loadFont(vdp, art);
}

}  // namespace scullslip
