#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace ferrypass {
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

Bitmap paintHull(float heading) {
    Bitmap b(kHullPx, kHullPx);
    const float cx = kHullPx * 0.5f, cy = kHullPx * 0.5f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, c, s));
        b.poly(w, col);
    };
    // Nose is -local Y so heading 0 points up the bitmap.
    poly({{0.f, -28.f}, {11.f, -10.f}, {12.f, 18.f}, {8.f, 26.f}, {-8.f, 26.f}, {-12.f, 18.f}, {-11.f, -10.f}}, 1);
    poly({{0.f, -20.f}, {7.f, -6.f}, {7.f, 16.f}, {-7.f, 16.f}, {-7.f, -6.f}}, 2);
    poly({{-5.f, 4.f}, {5.f, 4.f}, {5.f, 14.f}, {-5.f, 14.f}}, 3);
    b.rect(cx - 3.f, cy - 2.f, 6.f, 4.f, 4);
    b.ellipse(cx, cy - 14.f, 2.2f, 2.2f, 5);
    return b;
}

Bitmap cliffTile() {
    Bitmap b(28, 40);
    b.rect(2, 2, 24, 36, 1);
    b.rect(4, 4, 8, 14, 2);
    b.rect(14, 16, 10, 12, 3);
    b.poly({{2.f, 2.f}, {14.f, 0.f}, {26.f, 4.f}, {22.f, 10.f}, {6.f, 8.f}}, 4);
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
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 12, 14), gs::rgb4(15, 12, 4), gs::rgb4(15, 6, 3), shadow});
    setPal(vdp, PAL_FERRY, {0, gs::rgb4(14, 13, 10), gs::rgb4(4, 9, 8), gs::rgb4(8, 5, 3), gs::rgb4(2, 3, 4),
                            gs::rgb4(15, 14, 6), shadow});
    setPal(vdp, PAL_CLIFF, {0, gs::rgb4(6, 7, 6), gs::rgb4(9, 8, 6), gs::rgb4(4, 5, 4), gs::rgb4(8, 10, 5)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(9, 13, 14)});
    setPal(vdp, PAL_TAPE, {0, gs::rgb4(15, 14, 3), gs::rgb4(12, 3, 3), ink});
    setPal(vdp, PAL_STORM, {0, gs::rgb4(4, 4, 7), gs::rgb4(8, 8, 11), gs::rgb4(12, 12, 14)});
    setPal(vdp, PAL_GULL, {0, ink, gs::rgb4(9, 9, 10)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(7, 15, 6), ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 12, 4), ink});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(7, 8, 9)});
    setPal(vdp, PAL_HUT, {0, gs::rgb4(10, 6, 4), gs::rgb4(5, 3, 2), gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_LIGHT, {0, gs::rgb4(15, 14, 5), gs::rgb4(8, 6, 2)});

    for (int i = 0; i < 8; i++) art.hull[i] = gs::uploadMipped(vdp, paintHull(i * 3.14159265f / 4.f));
    art.cliff = gs::uploadMipped(vdp, cliffTile());
    {
        Bitmap s(22, 12);
        s.ellipse(11, 8, 9, 4, 1);
        s.ellipse(7, 6, 4, 3, 2);
        art.scrub = gs::uploadMipped(vdp, s);
    }
    {
        Bitmap r(16, 14);
        r.ellipse(8, 8, 6, 5, 1);
        r.ellipse(6, 6, 2, 2, 2);
        art.rock = gs::uploadMipped(vdp, r);
    }
    {
        Bitmap f(18, 8);
        f.ellipse(9, 4, 8, 3, 1);
        f.ellipse(5, 4, 3, 2, 2);
        art.foam = gs::uploadMipped(vdp, f);
    }
    {
        Bitmap p(8, 22);
        p.rect(3, 2, 2, 18, 1);
        p.rect(1, 2, 6, 3, 2);
        art.post = gs::uploadMipped(vdp, p);
    }
    {
        Bitmap t(40, 8);
        for (int x = 0; x < 40; x += 8) t.rect(float(x), 2, 4, 4, (x / 8) & 1 ? 1 : 2);
        art.tape = gs::uploadMipped(vdp, t);
    }
    {
        Bitmap h(26, 20);
        h.rect(3, 8, 20, 10, 1);
        h.poly({{2.f, 8.f}, {13.f, 1.f}, {24.f, 8.f}}, 2);
        h.rect(11, 12, 5, 6, 3);
        art.hut = gs::uploadMipped(vdp, h);
    }
    {
        Bitmap l(8, 18);
        l.rect(3, 6, 2, 12, 2);
        l.ellipse(4, 4, 3, 3, 1);
        art.lamp = gs::uploadMipped(vdp, l);
    }
    {
        Bitmap g(18, 8);
        g.poly({{1.f, 5.f}, {8.f, 3.f}, {16.f, 2.f}, {9.f, 5.f}}, 1);
        g.ellipse(4, 5, 2, 1.4f, 1);
        art.gull[0] = gs::uploadMipped(vdp, g);
        Bitmap g2(18, 8);
        g2.poly({{1.f, 3.f}, {8.f, 5.f}, {16.f, 4.f}, {9.f, 6.f}}, 1);
        g2.ellipse(4, 5, 2, 1.4f, 1);
        art.gull[1] = gs::uploadMipped(vdp, g2);
    }
    art.title = gs::uploadMipped(vdp, gs::textBitmap("FERRY PASS", gs::TextStyle{3, 1, 0, 0, 1}));
    art.clearWord = gs::uploadMipped(vdp, gs::textBitmap("CLEAR", gs::TextStyle{4, 1, 0, 0, 1}));
    art.missed = gs::uploadMipped(vdp, gs::textBitmap("MISSED THE END", gs::TextStyle{2, 1, 0, 0, 1}));
    art.paused = gs::uploadMipped(vdp, gs::textBitmap("PAUSED", gs::TextStyle{3, 1, 0, 0, 1}));
    loadFont(vdp, art);
}

}  // namespace ferrypass
