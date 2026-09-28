#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace plowboom {
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
    // Local +Y is the nose. The blade is empty: the drive rides ahead of it.
    poly({{-22.f, 12.f}, {22.f, 12.f}, {16.f, 20.f}, {-16.f, 20.f}}, 2);
    poly({{-18.f, 14.f}, {18.f, 14.f}, {13.f, 18.f}, {-13.f, 18.f}}, 3);
    stroke(-14.f, 16.f, -8.f, 6.f, 4, 2.2f);
    stroke(14.f, 16.f, 8.f, 6.f, 4, 2.2f);
    poly({{0.f, 8.f}, {10.f, 0.f}, {8.f, -18.f}, {4.f, -22.f}, {-4.f, -22.f}, {-8.f, -18.f}, {-10.f, 0.f}}, 1);
    poly({{0.f, 3.f}, {5.f, -3.f}, {4.f, -12.f}, {-4.f, -12.f}, {-5.f, -3.f}}, 5);
    poly({{-6.2f, -4.f}, {-3.4f, -4.f}, {-3.4f, -13.f}, {-6.2f, -13.f}}, 6);
    poly({{6.2f, -4.f}, {3.4f, -4.f}, {3.4f, -13.f}, {6.2f, -13.f}}, 6);
    b.ellipse(cx, cy - (-8.f) * c, 2.1f, 2.1f, 7);
    stroke(0.f, -20.f, 0.f, -28.f, 4, 2.f);
    b.ellipse(cx + (0.f * c - (-28.f) * s), cy - (0.f * s + (-28.f) * c), 3.4f, 3.4f, 8);
    b.outline(8, false);
    return b;
}

Bitmap paintDrive(float heading) {
    Bitmap b(40, 72);
    const float cx = 20.f, cy = 36.f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        w.reserve(local.size());
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, c, s));
        b.poly(w, col);
    };
    poly({{-5.f, -22.f}, {5.f, -22.f}, {5.f, 22.f}, {-5.f, 22.f}}, 1);
    poly({{-3.2f, -20.f}, {3.2f, -20.f}, {3.2f, 20.f}, {-3.2f, 20.f}}, 2);
    poly({{-7.f, -18.f}, {7.f, -18.f}, {7.f, -12.f}, {-7.f, -12.f}}, 3);
    poly({{-7.f, 12.f}, {7.f, 12.f}, {7.f, 18.f}, {-7.f, 18.f}}, 3);
    poly({{-6.f, -2.f}, {6.f, -2.f}, {6.f, 3.f}, {-6.f, 3.f}}, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintPost() {
    Bitmap b(18, 40);
    b.rect(6, 4, 6, 32, 1);
    b.rect(4, 2, 10, 6, 2);
    b.rect(3, 32, 12, 5, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintBeam() {
    Bitmap b(64, 14);
    b.rect(0, 3, 64, 8, 1);
    b.rect(0, 3, 10, 8, 2);
    b.rect(54, 3, 10, 8, 2);
    b.rect(28, 4, 8, 6, 3);
    return b;
}

Bitmap paintPlank() {
    Bitmap b(28, 10);
    b.rect(0, 2, 28, 6, 1);
    b.rect(4, 2, 3, 6, 2);
    b.rect(14, 2, 3, 6, 2);
    b.rect(22, 2, 3, 6, 2);
    return b;
}

Bitmap paintBank() {
    Bitmap b(28, 36);
    b.ellipse(14, 28, 12, 5, 1);
    b.poly({{6, 26}, {14, 4}, {22, 26}}, 2);
    b.poly({{10, 24}, {14, 10}, {18, 24}}, 3);
    return b;
}

Bitmap paintShed() {
    Bitmap b(72, 40);
    b.poly({{4, 22}, {36, 6}, {68, 22}, {62, 34}, {10, 34}}, 1);
    b.rect(28, 18, 16, 16, 2);
    b.rect(12, 18, 8, 6, 3);
    b.rect(52, 18, 8, 6, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintSpray() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 7, 4, 1);
    b.ellipse(5, 5, 2.2f, 1.4f, 2);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 3), gs::rgb4(8, 14, 12), gs::rgb4(15, 5, 3),
                          gs::rgb4(10, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 3, 5)});
    setPal(vdp, PAL_PLOW, {0, gs::rgb4(14, 7, 2), gs::rgb4(12, 13, 14), gs::rgb4(9, 10, 11), gs::rgb4(4, 4, 5),
                           gs::rgb4(3, 7, 11), gs::rgb4(14, 14, 11), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_DRIVE, {0, gs::rgb4(12, 9, 3), gs::rgb4(15, 13, 4), gs::rgb4(8, 6, 2), gs::rgb4(15, 15, 10),
                            gs::rgb4(3, 2, 1), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BOOM, {0, gs::rgb4(6, 7, 8), gs::rgb4(13, 8, 2), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 2),
                           gs::rgb4(10, 11, 12)});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(12, 13, 14), gs::rgb4(15, 15, 15), gs::rgb4(8, 10, 12)});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(13, 14, 15), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(10, 12, 13), gs::rgb4(13, 14, 15), gs::rgb4(8, 10, 12), gs::rgb4(6, 8, 10)});

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
        art.plow[h] = gs::uploadMipped(vdp, paintPlow(a));
        art.drive[h] = gs::uploadMipped(vdp, paintDrive(a));
    }
    art.post = gs::uploadMipped(vdp, paintPost());
    art.beam = gs::uploadMipped(vdp, paintBeam());
    art.plank = gs::uploadMipped(vdp, paintPlank());
    art.bank = gs::uploadMipped(vdp, paintBank());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.spray = gs::uploadMipped(vdp, paintSpray());
    loadFont(vdp, art);
}

}  // namespace plowboom
