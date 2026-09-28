#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace plowgrass {
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
    auto disc = [&](float lx, float ly, float r, int col) {
        Pt p = spin(cx, cy, lx, ly, c, s);
        b.ellipse(p.first, p.second, r, r, col);
    };
    // Local +Y is the nose. A farm tractor towing a moldboard, not a snow blade.
    poly({{-11.f, -6.f}, {11.f, -6.f}, {12.f, -28.f}, {-12.f, -28.f}}, 1);
    poly({{-8.f, -8.f}, {8.f, -8.f}, {7.f, -20.f}, {-7.f, -20.f}}, 5);
    poly({{-14.f, 2.f}, {14.f, 2.f}, {13.f, -8.f}, {-13.f, -8.f}}, 6);
    disc(-13.f, -4.f, 7.2f, 4);
    disc(13.f, -4.f, 7.2f, 4);
    disc(-13.f, -4.f, 3.1f, 3);
    disc(13.f, -4.f, 3.1f, 3);
    disc(-10.f, 16.f, 5.4f, 4);
    disc(10.f, 16.f, 5.4f, 4);
    stroke(0.f, -24.f, 0.f, -34.f, 7, 2.4f);
    disc(0.f, -35.f, 2.4f, 3);
    poly({{-16.f, 8.f}, {-6.f, 8.f}, {-4.f, 22.f}, {-18.f, 24.f}}, 2);
    poly({{16.f, 8.f}, {6.f, 8.f}, {4.f, 22.f}, {18.f, 24.f}}, 2);
    stroke(-10.f, 4.f, -14.f, 18.f, 3, 2.f);
    stroke(10.f, 4.f, 14.f, 18.f, 3, 2.f);
    poly({{-20.f, 20.f}, {4.f, 18.f}, {8.f, 28.f}, {-22.f, 30.f}}, 2);
    poly({{-18.f, 22.f}, {2.f, 20.f}, {4.f, 26.f}, {-18.f, 28.f}}, 8);
    b.outline(3, false);
    return b;
}

Bitmap paintSod() {
    Bitmap b(28, 22);
    b.rect(0, 0, 28, 22, 1);
    for (int i = 0; i < 18; i++) {
        int x = (i * 7 + 3) % 26;
        int y = (i * 5 + 2) % 18;
        b.line(float(x), float(y + 4), float(x + 1), float(y), 2, 1.2f);
        if (i % 3 == 0) b.line(float(x + 2), float(y + 5), float(x + 3), float(y + 1), 3, 1.f);
    }
    return b;
}

Bitmap paintTuft() {
    Bitmap b(14, 16);
    b.line(3, 14, 2, 3, 1, 1.6f);
    b.line(7, 15, 7, 1, 2, 1.8f);
    b.line(11, 14, 12, 4, 1, 1.6f);
    b.line(5, 13, 4, 6, 3, 1.2f);
    return b;
}

Bitmap paintPost() {
    Bitmap b(10, 22);
    b.rect(3, 2, 4, 18, 1);
    b.rect(2, 1, 6, 3, 2);
    b.rect(3, 18, 4, 3, 3);
    return b;
}

Bitmap paintCrew() {
    Bitmap b(48, 28);
    b.rect(4, 10, 34, 12, 1);
    b.rect(22, 4, 14, 10, 2);
    b.rect(24, 6, 8, 5, 3);
    b.ellipse(12, 22, 5, 5, 4);
    b.ellipse(32, 22, 5, 5, 4);
    b.rect(36, 12, 8, 4, 5);
    return b;
}

Bitmap paintDust() {
    Bitmap b(12, 10);
    b.ellipse(6, 5, 5, 3, 1);
    b.ellipse(4, 4, 2, 1.2f, 2);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 13), gs::rgb4(15, 13, 4), gs::rgb4(6, 14, 5), gs::rgb4(15, 5, 3),
                          gs::rgb4(9, 11, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 3, 2)});
    setPal(vdp, PAL_PLOW, {0, gs::rgb4(3, 11, 4), gs::rgb4(10, 5, 2), gs::rgb4(1, 1, 1), gs::rgb4(14, 12, 2),
                           gs::rgb4(8, 13, 15), gs::rgb4(5, 13, 6), gs::rgb4(4, 4, 4), gs::rgb4(13, 8, 3)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(2, 10, 3), gs::rgb4(5, 13, 4), gs::rgb4(1, 7, 2), gs::rgb4(8, 14, 5)});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(12, 3, 2), gs::rgb4(14, 6, 3), gs::rgb4(10, 12, 14), gs::rgb4(2, 2, 2),
                           gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(9, 7, 3), gs::rgb4(14, 14, 12), gs::rgb4(5, 4, 2)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(10, 7, 3), gs::rgb4(13, 10, 5)});
    setPal(vdp, PAL_DIRT, {0, gs::rgb4(8, 5, 2), gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(5, 8, 3)});

    uint8_t dirt[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int c = 1;
            if (((x * 3 + y * 5) % 7) == 0) c = 2;
            if (((x * 2 + y) % 11) == 0) c = 3;
            if (((x + y * 3) % 13) == 0) c = 4;
            dirt[y * 8 + x] = uint8_t(c);
        }
    vdp.loadTile(1, dirt);
    art.dirtTile = 1;

    for (int h = 0; h < 8; h++) {
        float a = h * (kPi * 2.f / 8.f);
        art.plow[h] = gs::uploadMipped(vdp, paintPlow(a));
    }
    art.sod = gs::uploadMipped(vdp, paintSod());
    art.tuft = gs::uploadMipped(vdp, paintTuft());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.crew = gs::uploadMipped(vdp, paintCrew());
    art.dust = gs::uploadMipped(vdp, paintDust());
    loadFont(vdp, art);
}

}  // namespace plowgrass
