#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace metrobuoy {
namespace {

constexpr float kTau = 6.2831853f;
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
    return {cx + ly * c + lx * s, cy - (ly * s - lx * c)};
}

Bitmap paintHull(float heading) {
    Bitmap b(112, 112);
    const float cx = 56.f, cy = 56.f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, c, s));
        b.poly(w, col);
    };
    auto blob = [&](float lx, float ly, float rx, float ry, int col) {
        Pt p = spin(cx, cy, lx, ly, c, s);
        b.ellipse(p.first, p.second, rx, ry, col);
    };
    // Water-metro catamaran: orange stripe, white cabin, bow lamp.
    poly({{-22.f, 22.f}, {22.f, 22.f}, {18.f, -30.f}, {8.f, -40.f}, {-8.f, -40.f}, {-18.f, -30.f}}, 2);
    poly({{-16.f, 16.f}, {16.f, 16.f}, {14.f, -22.f}, {6.f, -30.f}, {-6.f, -30.f}, {-14.f, -22.f}}, 3);
    poly({{-12.f, 10.f}, {12.f, 10.f}, {12.f, -8.f}, {-12.f, -8.f}}, 4);
    poly({{-10.f, 6.f}, {-2.f, 6.f}, {-2.f, -4.f}, {-10.f, -4.f}}, 5);
    poly({{2.f, 6.f}, {10.f, 6.f}, {10.f, -4.f}, {2.f, -4.f}}, 5);
    poly({{-14.f, 18.f}, {14.f, 18.f}, {12.f, 12.f}, {-12.f, 12.f}}, 8);
    blob(0.f, -34.f, 3.f, 2.4f, 9);
    blob(-8.f, 0.f, 1.6f, 1.6f, 7);
    blob(8.f, 0.f, 1.6f, 1.6f, 7);
    b.outline(1, false);
    return b;
}

Bitmap sphere(int body, int band, const char* num) {
    Bitmap b(40, 52);
    b.ellipse(20, 28, 14, 14, body);
    b.rect(8, 24, 24, 6, band);
    b.ellipse(20, 46, 10, 3, 3);
    b.ellipse(15, 22, 3, 2, 6);
    gs::TextStyle st{1, 7, 0, 0, 0};
    Bitmap t = gs::textBitmap(num, st);
    b.blit(t, 20 - t.w / 2, 22);
    b.outline(1, false);
    return b;
}

Bitmap pierArt(int roof) {
    Bitmap b(72, 36);
    b.rect(4, 16, 64, 14, 2);
    b.rect(4, 16, 64, 3, roof);
    b.rect(8, 6, 56, 10, 4);
    for (int i = 0; i < 5; i++) b.rect(10 + i * 12, 20, 4, 8, 5);
    b.outline(1, false);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 1);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
    uint8_t water[64];
    for (int i = 0; i < 64; i++) water[i] = uint8_t(1 + ((i * 3 + (i / 8)) & 3));
    int tw = tiles.alloc(1);
    vdp.loadTile(tw, water);
    vdp.A.enabled = false;
    vdp.B.resize(64, 32);
    for (int cy = 0; cy < 32; cy++)
        for (int cx = 0; cx < 64; cx++) vdp.B.set(cx, cy, gs::entry(tw, PAL_WATER));
}

Bitmap word(const char* s, int scale, int color) {
    gs::TextStyle st{scale, color, 0, 15, 1};
    return gs::textBitmap(s, st);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12), gs::rgb4(8, 8, 6)});
    setPal(vdp, PAL_HULL, {0, gs::rgb4(2, 2, 3), gs::rgb4(12, 5, 2), gs::rgb4(15, 8, 3), gs::rgb4(14, 14, 13),
                           gs::rgb4(6, 10, 13), gs::rgb4(2, 2, 2), gs::rgb4(15, 12, 4), gs::rgb4(15, 6, 1),
                           gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(3, 1, 0), gs::rgb4(14, 8, 1), gs::rgb4(6, 4, 1), gs::rgb4(15, 12, 3),
                            gs::rgb4(8, 4, 1), gs::rgb4(15, 14, 8), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_TEAL, {0, gs::rgb4(0, 3, 3), gs::rgb4(2, 12, 10), gs::rgb4(1, 5, 4), gs::rgb4(8, 15, 12),
                           gs::rgb4(1, 7, 6), gs::rgb4(14, 15, 14), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_PIER, {0, gs::rgb4(2, 2, 3), gs::rgb4(9, 9, 8), gs::rgb4(5, 5, 5), gs::rgb4(12, 6, 2),
                           gs::rgb4(4, 4, 5), gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_WAKE, {0, gs::rgb4(10, 13, 14), gs::rgb4(14, 15, 15), gs::rgb4(6, 9, 11)});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(2, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(12, 8, 4)});
    setPal(vdp, PAL_CITY, {0, gs::rgb4(3, 3, 5), gs::rgb4(7, 7, 9), gs::rgb4(11, 10, 8), gs::rgb4(14, 12, 6),
                           gs::rgb4(5, 8, 11)});
    setPal(vdp, PAL_CRATE, {0, gs::rgb4(4, 2, 1), gs::rgb4(10, 6, 2), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_OTHER, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4), gs::rgb4(8, 4, 6),
                            gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 12, 4), gs::rgb4(8, 6, 2), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 8), gs::rgb4(2, 6, 3), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(6, 1, 1), gs::rgb4(15, 12, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 6, 8), gs::rgb4(3, 8, 10), gs::rgb4(4, 9, 11), gs::rgb4(2, 5, 7)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(4, 4, 3), gs::rgb4(15, 14, 6), gs::rgb4(8, 8, 6)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(3, 3, 3), gs::rgb4(8, 7, 5), gs::rgb4(12, 10, 6)});

    for (int i = 0; i < 8; i++) art.hull[i] = gs::uploadMipped(vdp, paintHull(i * kTau / 8.f));
    art.amber = gs::uploadMipped(vdp, sphere(2, 4, "1"));
    art.teal = gs::uploadMipped(vdp, sphere(2, 4, "2"));
    art.violet = gs::uploadMipped(vdp, sphere(2, 5, "3"));
    art.pier = gs::uploadMipped(vdp, pierArt(4));
    art.canopy = gs::uploadMipped(vdp, pierArt(5));
    {
        Bitmap b(16, 28);
        b.rect(7, 8, 2, 18, 1);
        b.ellipse(8, 6, 4, 4, 2);
        b.outline(3, false);
        art.lamp = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap b(18, 24);
        b.rect(8, 8, 2, 14, 1);
        b.poly({{10, 8}, {16, 11}, {10, 14}}, 2);
        art.flag = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap b(28, 40);
        b.rect(4, 8, 20, 28, 2);
        b.rect(6, 12, 6, 6, 5);
        b.rect(14, 12, 6, 6, 4);
        b.rect(6, 22, 6, 6, 3);
        art.block = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap b(20, 16);
        b.rect(2, 2, 16, 12, 2);
        b.line(2, 2, 18, 14, 1, 1);
        art.crate = gs::uploadMipped(vdp, b);
    }
    for (int f = 0; f < 2; f++) {
        Bitmap b(28, 14);
        b.ellipse(14, 8, 4, 3, 2);
        if (f == 0) {
            b.poly({{10, 8}, {2, 4}, {8, 9}}, 2);
            b.poly({{18, 8}, {26, 4}, {20, 9}}, 2);
        } else {
            b.poly({{10, 8}, {3, 10}, {8, 9}}, 2);
            b.poly({{18, 8}, {25, 10}, {20, 9}}, 2);
        }
        b.ellipse(16, 7, 1, 1, 1);
        art.gull[f] = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap b(16, 10);
        b.ellipse(8, 5, 6, 3, 2);
        art.wake = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap b(24, 10);
        b.ellipse(12, 5, 10, 3, 2);
        b.ellipse(12, 5, 6, 2, 0);
        art.ring = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap b(12, 16);
        b.poly({{6, 1}, {11, 10}, {1, 10}}, 1);
        b.rect(5, 10, 2, 5, 2);
        art.pin = gs::uploadMipped(vdp, b);
    }
    art.title = gs::uploadMipped(vdp, word("METRO BUOY", 3, 1));
    art.round = gs::uploadMipped(vdp, word("ROUND TO PORT", 2, 3));
    art.home = gs::uploadMipped(vdp, word("SAME DOCK", 3, 1));
    art.made = gs::uploadMipped(vdp, word("LEG MADE", 2, 3));
    art.missed = gs::uploadMipped(vdp, word("MISSED THE END", 2, 1));
    art.wrong = gs::uploadMipped(vdp, word("WRONG DOCK", 2, 1));
    art.paused = gs::uploadMipped(vdp, word("HELD", 3, 1));
    loadFont(vdp, art);
}

}  // namespace metrobuoy
