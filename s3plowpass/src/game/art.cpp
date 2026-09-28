#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace plowpass {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    return {cx + lx * c - ly * s, cy + lx * s + ly * c};
}

Bitmap paintPlow(float heading) {
    Bitmap b(kPlowPx, kPlowPx);
    const float cx = kPlowPx * 0.5f, cy = kPlowPx * 0.5f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, c, s));
        b.poly(w, col);
    };
    // Nose is -local Y so heading 0 points up the bitmap. Blade is the wide bar.
    poly({{-16.f, -22.f}, {16.f, -22.f}, {14.f, -16.f}, {-14.f, -16.f}}, 1);
    poly({{-3.f, -20.f}, {3.f, -20.f}, {2.f, -10.f}, {-2.f, -10.f}}, 2);
    poly({{-9.f, -12.f}, {9.f, -12.f}, {10.f, 12.f}, {-10.f, 12.f}}, 3);
    poly({{-7.f, -8.f}, {7.f, -8.f}, {6.f, 2.f}, {-6.f, 2.f}}, 4);
    poly({{-8.f, 8.f}, {8.f, 8.f}, {7.f, 16.f}, {-7.f, 16.f}}, 5);
    b.ellipse(cx, cy - 4.f, 2.2f, 2.2f, 6);
    b.rect(cx - 8.f, cy + 10.f, 4.f, 3.f, 7);
    b.rect(cx + 4.f, cy + 10.f, 4.f, 3.f, 7);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(10, 13, 15), gs::rgb4(15, 12, 4), gs::rgb4(15, 6, 3), shadow});
    setPal(vdp, PAL_PLOW, {0, gs::rgb4(15, 11, 2), gs::rgb4(6, 6, 7), gs::rgb4(14, 8, 2), gs::rgb4(3, 5, 8),
                           gs::rgb4(2, 2, 3), gs::rgb4(15, 14, 4), gs::rgb4(1, 1, 1), shadow});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(13, 14, 15), gs::rgb4(9, 11, 13), gs::rgb4(6, 8, 11), gs::rgb4(4, 6, 8)});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(1, 5, 3), gs::rgb4(3, 8, 4), gs::rgb4(5, 4, 2)});
    setPal(vdp, PAL_TAPE, {0, gs::rgb4(15, 14, 3), gs::rgb4(12, 3, 3), ink});
    setPal(vdp, PAL_STORM, {0, gs::rgb4(5, 5, 8), gs::rgb4(9, 9, 12), gs::rgb4(13, 13, 15)});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 13, 15)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(7, 15, 6), ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 12, 4), ink});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(7, 8, 10)});
    setPal(vdp, PAL_SHED, {0, gs::rgb4(8, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(12, 10, 7)});
    setPal(vdp, PAL_LIGHT, {0, gs::rgb4(15, 13, 4), gs::rgb4(6, 5, 3)});

    for (int i = 0; i < 8; i++) art.plow[i] = gs::uploadMipped(vdp, paintPlow(i * 3.14159265f / 4.f));
    {
        Bitmap b(30, 18);
        b.poly({{0.f, 16.f}, {6.f, 4.f}, {16.f, 1.f}, {26.f, 6.f}, {30.f, 16.f}}, 1);
        b.poly({{4.f, 16.f}, {10.f, 8.f}, {18.f, 6.f}, {24.f, 10.f}}, 2);
        b.rect(8, 12, 6, 4, 3);
        art.bank = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap p(16, 28);
        p.poly({{8.f, 1.f}, {15.f, 12.f}, {1.f, 12.f}}, 1);
        p.poly({{8.f, 8.f}, {14.f, 20.f}, {2.f, 20.f}}, 2);
        p.rect(7, 20, 2, 7, 3);
        art.pine = gs::uploadMipped(vdp, p);
    }
    {
        Bitmap d(20, 10);
        d.ellipse(10, 6, 9, 3.5f, 1);
        d.ellipse(6, 5, 3, 2, 2);
        art.drift = gs::uploadMipped(vdp, d);
    }
    {
        Bitmap s(16, 10);
        s.ellipse(5, 6, 4, 2, 1);
        s.ellipse(11, 4, 3, 2, 2);
        art.spray = gs::uploadMipped(vdp, s);
    }
    {
        Bitmap p(8, 24);
        p.rect(3, 2, 2, 20, 3);
        p.rect(1, 2, 6, 3, 2);
        art.post = gs::uploadMipped(vdp, p);
    }
    {
        Bitmap t(48, 8);
        for (int x = 0; x < 48; x += 8) t.rect(float(x), 2, 4, 4, (x / 8) & 1 ? 1 : 2);
        art.tape = gs::uploadMipped(vdp, t);
    }
    {
        Bitmap h(28, 18);
        h.rect(3, 7, 22, 10, 1);
        h.poly({{2.f, 7.f}, {14.f, 1.f}, {26.f, 7.f}}, 2);
        h.rect(12, 11, 5, 6, 3);
        art.shed = gs::uploadMipped(vdp, h);
    }
    {
        Bitmap l(8, 18);
        l.rect(3, 6, 2, 12, 2);
        l.ellipse(4, 4, 3, 3, 1);
        art.lamp = gs::uploadMipped(vdp, l);
    }
    {
        Bitmap f(6, 6);
        f.ellipse(3, 3, 2, 2, 1);
        art.flake[0] = gs::uploadMipped(vdp, f);
        Bitmap f2(5, 5);
        f2.rect(2, 0, 1, 5, 1);
        f2.rect(0, 2, 5, 1, 1);
        art.flake[1] = gs::uploadMipped(vdp, f2);
    }
    art.title = gs::uploadMipped(vdp, gs::textBitmap("PLOW PASS", gs::TextStyle{3, 1, 0, 0, 1}));
    art.clearWord = gs::uploadMipped(vdp, gs::textBitmap("CLEAR", gs::TextStyle{4, 1, 0, 0, 1}));
    art.missed = gs::uploadMipped(vdp, gs::textBitmap("MISSED THE END", gs::TextStyle{2, 1, 0, 0, 1}));
    art.paused = gs::uploadMipped(vdp, gs::textBitmap("PAUSED", gs::TextStyle{3, 1, 0, 0, 1}));
    loadFont(vdp, art);
}

}  // namespace plowpass
