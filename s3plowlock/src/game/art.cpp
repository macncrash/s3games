#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace plowlock {
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

Bitmap paintPlow(float heading) {
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
    poly({{-18.f, 8.f}, {18.f, 8.f}, {14.f, 16.f}, {-14.f, 16.f}}, 2);
    poly({{-16.f, 10.f}, {16.f, 10.f}, {12.f, 14.f}, {-12.f, 14.f}}, 3);
    stroke(-14.f, 12.f, -8.f, 2.f, 4, 2.f);
    stroke(14.f, 12.f, 8.f, 2.f, 4, 2.f);
    poly({{0.f, 6.f}, {8.f, 0.f}, {7.f, -14.f}, {4.f, -20.f}, {-4.f, -20.f}, {-7.f, -14.f}, {-8.f, 0.f}}, 1);
    poly({{0.f, 2.f}, {4.f, -2.f}, {3.f, -10.f}, {-3.f, -10.f}, {-4.f, -2.f}}, 5);
    poly({{-5.f, -4.f}, {-2.6f, -4.f}, {-2.6f, -12.f}, {-5.f, -12.f}}, 6);
    poly({{5.f, -4.f}, {2.6f, -4.f}, {2.6f, -12.f}, {5.f, -12.f}}, 6);
    b.outline(8, false);
    return b;
}

Bitmap paintGate() {
    Bitmap b(12, 48);
    b.rect(1, 0, 10, 48, 1);
    for (int i = 0; i < 6; i++) b.rect(2, 4 + i * 7, 8, 3, 2);
    b.rect(0, 0, 3, 48, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintBank() {
    Bitmap b(28, 20);
    b.rect(0, 4, 28, 12, 1);
    b.rect(0, 4, 28, 4, 2);
    b.rect(4, 14, 6, 4, 3);
    b.rect(16, 14, 8, 4, 3);
    return b;
}

Bitmap paintPost() {
    Bitmap b(10, 28);
    b.rect(3, 6, 4, 22, 1);
    b.rect(1, 2, 8, 6, 2);
    b.rect(2, 3, 6, 3, 3);
    return b;
}

Bitmap paintBanner() {
    Bitmap b(36, 16);
    b.poly({{2, 8}, {18, 2}, {34, 8}, {18, 14}}, 1);
    b.poly({{8, 8}, {18, 5}, {28, 8}, {18, 11}}, 2);
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
                          gs::rgb4(6, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 3, 4)});
    setPal(vdp, PAL_PLOW, {0, gs::rgb4(15, 8, 1), gs::rgb4(13, 13, 14), gs::rgb4(9, 10, 11), gs::rgb4(4, 4, 5),
                           gs::rgb4(2, 5, 9), gs::rgb4(14, 14, 12), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(10, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(14, 10, 4), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(8, 9, 10), gs::rgb4(12, 13, 14), gs::rgb4(5, 5, 6)});
    setPal(vdp, PAL_END, {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 15, 14), gs::rgb4(12, 3, 2)});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(10, 12, 14), gs::rgb4(13, 14, 15), gs::rgb4(8, 10, 12), gs::rgb4(6, 8, 11)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(5, 4, 3), gs::rgb4(12, 3, 2), gs::rgb4(15, 14, 6)});

    uint8_t snow[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int c = 1;
            if (((x * 3 + y * 5) % 11) == 0) c = 2;
            if (((x + y * 7) % 13) == 0) c = 3;
            snow[y * 8 + x] = uint8_t(c);
        }
    vdp.loadTile(1, snow);
    art.snowTile = 1;

    for (int h = 0; h < 8; h++) art.plow[h] = gs::uploadMipped(vdp, paintPlow(h * (kPi * 2.f / 8.f)));
    art.gate = gs::uploadMipped(vdp, paintGate());
    art.bank = gs::uploadMipped(vdp, paintBank());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.banner = gs::uploadMipped(vdp, paintBanner());
    loadFont(vdp, art);
}

}  // namespace plowlock
