#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace plowbox {
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

Bitmap paintPlow(float heading, int phase) {
    Bitmap b(88, 88);
    const float cx = 44.f, cy = 44.f;
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
    // Local +Y is the nose. The blade sits ahead of the cab; phase drops it into the snow.
    const float drop = phase ? 2.4f : 0.f;
    poly({{-22.f, 10.f + drop}, {22.f, 10.f + drop}, {18.f, 18.f + drop}, {-18.f, 18.f + drop}}, 2);
    poly({{-20.f, 12.f + drop}, {20.f, 12.f + drop}, {16.f, 16.f + drop}, {-16.f, 16.f + drop}}, 3);
    stroke(-16.f, 14.f, -10.f, 4.f, 4, 2.2f);
    stroke(16.f, 14.f, 10.f, 4.f, 4, 2.2f);
    poly({{0.f, 8.f}, {9.f, 2.f}, {8.f, -16.f}, {5.f, -22.f}, {-5.f, -22.f}, {-8.f, -16.f}, {-9.f, 2.f}}, 1);
    poly({{0.f, 4.f}, {5.f, -2.f}, {4.f, -12.f}, {-4.f, -12.f}, {-5.f, -2.f}}, 5);
    poly({{-6.f, -6.f}, {-3.2f, -6.f}, {-3.2f, -14.f}, {-6.f, -14.f}}, 6);
    poly({{6.f, -6.f}, {3.2f, -6.f}, {3.2f, -14.f}, {6.f, -14.f}}, 6);
    b.ellipse(cx + (0 * c - (-8.f) * s), cy - (0 * s + (-8.f) * c), 2.2f, 2.2f, 7);
    stroke(-7.f, -18.f, -7.f, -26.f, 4, 2.f);
    stroke(7.f, -18.f, 7.f, -26.f, 4, 2.f);
    b.ellipse(cx + (-7.f * c - (-26.f) * s), cy - (-7.f * s + (-26.f) * c), 3.2f, 3.2f, 8);
    b.ellipse(cx + (7.f * c - (-26.f) * s), cy - (7.f * s + (-26.f) * c), 3.2f, 3.2f, 8);
    b.outline(8, false);
    return b;
}

Bitmap paintStake() {
    Bitmap b(16, 30);
    b.rect(6, 6, 4, 20, 1);
    b.rect(3, 2, 10, 6, 2);
    b.rect(4, 24, 8, 4, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintTape() {
    Bitmap b(20, 8);
    b.rect(0, 2, 20, 4, 1);
    b.rect(0, 2, 6, 4, 2);
    b.rect(10, 2, 6, 4, 2);
    return b;
}

Bitmap paintBarn() {
    Bitmap b(96, 56);
    b.poly({{8, 36}, {48, 8}, {88, 36}, {80, 50}, {16, 50}}, 1);
    b.poly({{20, 34}, {48, 14}, {76, 34}, {70, 46}, {26, 46}}, 2);
    b.rect(40, 30, 16, 20, 3);
    b.rect(18, 28, 10, 8, 4);
    b.rect(68, 28, 10, 8, 4);
    b.rect(44, 34, 8, 10, 5);
    b.outline(6, false);
    return b;
}

Bitmap paintBank() {
    Bitmap b(28, 36);
    b.ellipse(14, 28, 12, 5, 1);
    b.poly({{6, 26}, {14, 4}, {22, 26}}, 2);
    b.poly({{10, 24}, {14, 10}, {18, 24}}, 3);
    return b;
}

Bitmap paintSpray() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 7, 4, 1);
    b.ellipse(5, 5, 2.2f, 1.4f, 2);
    b.ellipse(11, 7, 1.8f, 1.2f, 2);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 3), gs::rgb4(8, 13, 15), gs::rgb4(15, 5, 3),
                          gs::rgb4(10, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 3, 5)});
    setPal(vdp, PAL_PLOW, {0, gs::rgb4(15, 8, 2), gs::rgb4(12, 12, 13), gs::rgb4(8, 9, 10), gs::rgb4(4, 4, 5),
                           gs::rgb4(3, 6, 10), gs::rgb4(14, 14, 12), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_BOX, {0, gs::rgb4(14, 14, 13), gs::rgb4(15, 4, 2), gs::rgb4(5, 4, 3), gs::rgb4(2, 2, 2),
                          gs::rgb4(15, 11, 2)});
    setPal(vdp, PAL_BARN, {0, gs::rgb4(12, 4, 3), gs::rgb4(8, 3, 2), gs::rgb4(6, 4, 3), gs::rgb4(10, 12, 14),
                           gs::rgb4(3, 2, 2), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(12, 13, 14), gs::rgb4(15, 15, 15), gs::rgb4(8, 10, 12)});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(13, 14, 15), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(11, 12, 13), gs::rgb4(13, 14, 15), gs::rgb4(9, 10, 12), gs::rgb4(7, 9, 11)});

    uint8_t snow[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int c = 1;
            if (((x * 5 + y * 3) % 11) == 0) c = 2;
            if (((x + y * 4) % 17) == 0) c = 3;
            snow[y * 8 + x] = uint8_t(c);
        }
    vdp.loadTile(1, snow);
    art.snowTile = 1;

    for (int h = 0; h < 8; h++) {
        float a = h * (kPi * 2.f / 8.f);
        art.plow[h * 2] = gs::uploadMipped(vdp, paintPlow(a, 0));
        art.plow[h * 2 + 1] = gs::uploadMipped(vdp, paintPlow(a, 1));
    }
    art.stake = gs::uploadMipped(vdp, paintStake());
    art.tape = gs::uploadMipped(vdp, paintTape());
    art.barn = gs::uploadMipped(vdp, paintBarn());
    art.bank = gs::uploadMipped(vdp, paintBank());
    art.spray = gs::uploadMipped(vdp, paintSpray());
    loadFont(vdp, art);
}

}  // namespace plowbox
