#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace tram {
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

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int fill, int edge) {
    gs::TextStyle st{scale, fill, edge, 0, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

// Bow is local +Y. Heading 0 faces east on screen.
Pt spin(float cx, float cy, float lx, float ly, float heading) {
    float c = std::cos(heading), s = std::sin(heading);
    float sx = ly * c + lx * s;
    float sy = lx * c - ly * s;
    return {cx + sx, cy + sy};
}

Bitmap paintTram(float heading) {
    Bitmap b(80, 80);
    const float cx = 40.f, cy = 40.f;
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        w.reserve(local.size());
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, heading));
        b.poly(w, col);
    };
    auto box = [&](float x0, float y0, float x1, float y1, int col) {
        poly({{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}}, col);
    };
    // Cream water-tram: squared cabin, yellow waist, pantograph.
    poly({{0.f, 30.f}, {12.f, 22.f}, {14.f, -18.f}, {10.f, -26.f}, {-10.f, -26.f}, {-14.f, -18.f}, {-12.f, 22.f}}, 1);
    box(-12.f, -16.f, 12.f, 16.f, 7);
    box(-13.f, -4.f, 13.f, 2.f, 2);
    for (int i = 0; i < 4; i++) {
        float y = 12.f - i * 6.f;
        box(-9.f, y - 2.2f, -2.f, y + 1.4f, 3);
        box(2.f, y - 2.2f, 9.f, y + 1.4f, 3);
    }
    box(-8.f, 18.f, 8.f, 22.f, 4);
    box(-3.f, 22.f, 3.f, 26.f, 6);
    poly({{-2.f, 4.f}, {2.f, 4.f}, {5.f, 14.f}, {0.f, 18.f}, {-5.f, 14.f}}, 5);
    b.line(spin(cx, cy, 0, 8, heading).first, spin(cx, cy, 0, 8, heading).second,
           spin(cx, cy, 0, 16, heading).first, spin(cx, cy, 0, 16, heading).second, 5, 1.4f);
    b.outline(5, false);
    return b;
}

Bitmap dockArt() {
    Bitmap b(120, 56);
    b.rect(8, 18, 104, 22, 1);
    for (int x = 12; x < 108; x += 10) b.rect(x, 20, 6, 18, 2);
    b.rect(18, 8, 36, 16, 3);
    b.poly({{16.f, 8.f}, {36.f, 0.f}, {56.f, 8.f}}, 4);
    b.rect(30, 12, 10, 12, 5);
    b.rect(70, 22, 28, 10, 6);
    b.rect(4, 36, 8, 16, 2);
    b.rect(108, 36, 8, 16, 2);
    b.outline(7, false);
    return b;
}

Bitmap buoyArt() {
    Bitmap b(28, 40);
    b.ellipse(14, 30, 10, 5, 2);
    b.ellipse(14, 28, 7, 3, 1);
    b.rect(12, 14, 4, 14, 4);
    b.poly({{14.f, 2.f}, {22.f, 16.f}, {6.f, 16.f}}, 3);
    b.poly({{14.f, 6.f}, {18.f, 14.f}, {10.f, 14.f}}, 1);
    b.outline(4, false);
    return b;
}

Bitmap wakeArt() {
    Bitmap b(16, 10);
    b.ellipse(8, 5, 6, 3, 1);
    b.ellipse(8, 5, 3, 1.4f, 2);
    return b;
}

Bitmap ringArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(14, 14, 8, 8, 0);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_TRAM,
           {0, gs::rgb4(14, 13, 10), gs::rgb4(15, 12, 2), gs::rgb4(3, 8, 13), gs::rgb4(13, 2, 2), gs::rgb4(1, 1, 2),
            gs::rgb4(15, 15, 14), gs::rgb4(11, 10, 8), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_DOCK,
           {0, gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(12, 11, 9), gs::rgb4(11, 3, 2), gs::rgb4(4, 6, 8),
            gs::rgb4(14, 12, 6), ink, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BUOY0, {0, gs::rgb4(15, 15, 15), gs::rgb4(2, 8, 10), gs::rgb4(14, 2, 2), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BUOY1, {0, gs::rgb4(15, 15, 15), gs::rgb4(2, 8, 10), gs::rgb4(2, 12, 4), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BUOY2, {0, gs::rgb4(15, 15, 15), gs::rgb4(2, 8, 10), gs::rgb4(15, 11, 1), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_WAKE, {0, gs::rgb4(12, 15, 15), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 14, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 8), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 8), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 4), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    for (int i = 0; i < 8; i++) art.hull[i] = gs::uploadMipped(vdp, paintTram(i * (kPi * 2.f / 8.f)));
    art.dock = gs::uploadMipped(vdp, dockArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.wake = gs::uploadMipped(vdp, wakeArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    loadFont(vdp, art);
    art.title = words(vdp, "TRAMBUOY", 3, 1, 2);
    art.home = words(vdp, "SAME DOCK", 2, 1, 2);
    (void)kPi;
}

}  // namespace tram
