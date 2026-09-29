#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace subturn {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float kTau = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    uint16_t c[16] = {};
    int i = 0;
    for (uint16_t v : cs)
        if (i < 16) c[i++] = v;
    for (int k = 0; k < 16; k++) vdp.setColor(pal * 16 + k, c[k]);
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    float wx = ly * c + lx * s;
    float wy = ly * s - lx * c;
    return {cx + wx, cy - wy};
}

Bitmap paintSub(float heading) {
    Bitmap b(64, 64);
    const float cx = 32.f, cy = 32.f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        w.reserve(local.size());
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, c, s));
        b.poly(w, col);
    };
    auto blob = [&](float lx, float ly, float rx, float ry, int col) {
        Pt p = spin(cx, cy, lx, ly, c, s);
        b.ellipse(p.first, p.second, rx, ry, col);
    };
    poly({{0.f, 26.f}, {7.f, 16.f}, {8.f, 0.f}, {7.f, -16.f}, {3.f, -22.f}, {-3.f, -22.f}, {-7.f, -16.f}, {-8.f, 0.f}, {-7.f, 16.f}}, 8);
    poly({{0.f, 22.f}, {5.5f, 14.f}, {6.f, 0.f}, {5.f, -14.f}, {2.f, -19.f}, {-2.f, -19.f}, {-5.f, -14.f}, {-6.f, 0.f}, {-5.5f, 14.f}}, 1);
    poly({{-5.5f, 4.f}, {5.5f, 4.f}, {4.5f, -8.f}, {-4.5f, -8.f}}, 6);
    poly({{0.f, 10.f}, {2.2f, 6.f}, {2.2f, -2.f}, {-2.2f, -2.f}, {-2.2f, 6.f}}, 4);
    blob(0.f, 2.f, 1.4f, 1.2f, 5);
    blob(0.f, -18.f, 1.6f, 1.2f, 3);
    poly({{-9.f, -12.f}, {-6.f, -10.f}, {-6.f, -16.f}, {-9.f, -15.f}}, 2);
    poly({{9.f, -12.f}, {6.f, -10.f}, {6.f, -16.f}, {9.f, -15.f}}, 2);
    b.outline(15, false);
    return b;
}

Bitmap paintSand() {
    Bitmap b(24, 24);
    b.rect(0, 0, 24, 24, 1);
    b.rect(2, 2, 20, 20, 2);
    b.rect(10, 3, 3, 7, 4);
    b.rect(10, 14, 3, 7, 4);
    return b;
}

Bitmap paintKelp() {
    Bitmap b(16, 28);
    b.line(8, 26, 6, 4, 1, 2.2f);
    b.line(8, 20, 12, 10, 2, 1.6f);
    b.ellipse(6, 4, 3, 2, 3);
    return b;
}

Bitmap paintWreck() {
    Bitmap b(36, 22);
    b.poly({{2.f, 16.f}, {30.f, 18.f}, {34.f, 12.f}, {18.f, 6.f}, {6.f, 8.f}}, 1);
    b.rect(10, 8, 8, 5, 2);
    b.rect(22, 10, 6, 4, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(14, 20);
    b.ellipse(7, 7, 5, 5, 1);
    b.ellipse(7, 7, 2, 2, 2);
    b.rect(6, 12, 2, 6, 3);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(16, 22);
    b.rect(3, 2, 2, 18, 1);
    b.poly({{5.f, 3.f}, {14.f, 7.f}, {5.f, 12.f}}, 2);
    return b;
}

Bitmap paintBubble() {
    Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(4, 4, 1, 1, 2);
    return b;
}

gs::Mipped word(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st{scale, 1, 14, 0, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    const uint16_t ink = gs::rgb4(12, 15, 14);
    const uint16_t line = gs::rgb4(1, 2, 3);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(5, 8, 9), gs::rgb4(15, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 4), line});
    setPal(vdp, PAL_SUB,
           {0, gs::rgb4(10, 13, 8), gs::rgb4(4, 6, 5), gs::rgb4(2, 2, 2), gs::rgb4(14, 10, 2), gs::rgb4(8, 14, 15),
            gs::rgb4(3, 5, 6), gs::rgb4(15, 8, 1), gs::rgb4(2, 4, 3), gs::rgb4(14, 15, 15), gs::rgb4(6, 8, 5),
            gs::rgb4(12, 4, 2), gs::rgb4(15, 15, 14), gs::rgb4(15, 14, 6), gs::rgb4(3, 4, 4), line});
    setPal(vdp, PAL_TRENCH,
           {0, gs::rgb4(2, 4, 5), gs::rgb4(4, 6, 5), gs::rgb4(6, 7, 5), gs::rgb4(12, 13, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line});
    setPal(vdp, PAL_KELP,
           {0, gs::rgb4(2, 7, 3), gs::rgb4(4, 10, 4), gs::rgb4(8, 13, 5), gs::rgb4(3, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(10, 15, 12), gs::rgb4(4, 10, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 2), line});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 8, 5), gs::rgb4(10, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 1), line});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(13, 15, 15), gs::rgb4(4, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 3), line});
    setPal(vdp, PAL_CREW,
           {0, gs::rgb4(6, 8, 7), gs::rgb4(3, 4, 4), gs::rgb4(1, 1, 1), gs::rgb4(9, 7, 3), gs::rgb4(6, 10, 11),
            gs::rgb4(2, 3, 4), 0, gs::rgb4(2, 3, 3), 0, 0, 0, 0, 0, 0, line});
    setPal(vdp, PAL_MARK,
           {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 6, 2), gs::rgb4(8, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line});

    for (int i = 0; i < 8; i++) art.sub[i] = gs::uploadMipped(vdp, paintSub(kTau * float(i) / 8.f));
    art.sand = gs::uploadMipped(vdp, paintSand());
    art.kelp = gs::uploadMipped(vdp, paintKelp());
    art.wreck = gs::uploadMipped(vdp, paintWreck());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.bubble = gs::uploadMipped(vdp, paintBubble());
    art.title = word(vdp, "SUB TURN", 3);
    art.cleared = word(vdp, "UPRIGHT", 3);
    art.tipped = word(vdp, "TIPPED", 3);
    art.crew = word(vdp, "CREW WON", 3);
    art.trench = word(vdp, "OFF TRENCH", 2);
    art.paused = word(vdp, "HOLD", 3);
    loadFont(vdp, art);
}

}  // namespace subturn
