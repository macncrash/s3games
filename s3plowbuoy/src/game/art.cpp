#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace plow {
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
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(2, 3, 4));
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    return {cx + ly * c + lx * s, cy - (ly * s - lx * c)};
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
    auto blob = [&](float lx, float ly, float rx, float ry, int col) {
        Pt p = spin(cx, cy, lx, ly, c, s);
        b.ellipse(p.first, p.second, rx, ry, col);
    };
    auto stroke = [&](float ax, float ay, float bx, float by, int col, float th) {
        Pt A = spin(cx, cy, ax, ay, c, s);
        Pt B = spin(cx, cy, bx, by, c, s);
        b.line(A.first, A.second, B.first, B.second, col, th);
    };
    blob(-14.f, -8.f, 5.f, 7.f, 8);
    blob(14.f, -8.f, 5.f, 7.f, 8);
    blob(-14.f, 16.f, 5.f, 7.f, 8);
    blob(14.f, 16.f, 5.f, 7.f, 8);
    poly({{-16.f, -18.f}, {16.f, -18.f}, {18.f, 22.f}, {-18.f, 22.f}}, 2);
    poly({{-12.f, -10.f}, {12.f, -10.f}, {12.f, 8.f}, {-12.f, 8.f}}, 3);
    poly({{-10.f, -6.f}, {10.f, -6.f}, {8.f, 4.f}, {-8.f, 4.f}}, 4);
    poly({{-22.f, 24.f}, {22.f, 24.f}, {26.f, 34.f}, {-26.f, 34.f}}, 1);
    stroke(-20.f, 28.f, 20.f, 28.f, 5, 2.2f);
    blob(0.f, -14.f, 3.2f, 3.2f, 6);
    stroke(0.f, -18.f, 0.f, -24.f, 7, 1.6f);
    blob(-6.f, 2.f, 1.6f, 1.6f, 9);
    blob(6.f, 2.f, 1.6f, 1.6f, 9);
    b.outline(10, false);
    return b;
}

Bitmap nunArt(const char* num) {
    Bitmap b(40, 56);
    b.ellipse(20, 48, 12, 4, 3);
    b.poly({{20, 6}, {34, 42}, {6, 42}}, 1);
    b.rect(12, 24, 16, 12, 2);
    b.ellipse(20, 10, 3.f, 3.f, 4);
    b.outline(5, false);
    gs::TextStyle st{2, 5, 0, 0, 0};
    Bitmap t = gs::textBitmap(num, st);
    b.blit(t, 20 - t.w / 2, 26);
    return b;
}

Bitmap canArt(const char* num) {
    Bitmap b(40, 56);
    b.ellipse(20, 48, 12, 4, 3);
    b.rect(9, 16, 22, 28, 1);
    b.ellipse(20, 16, 11, 4, 1);
    b.rect(11, 24, 18, 12, 2);
    b.rect(18, 6, 4, 12, 4);
    b.ellipse(20, 6, 2.4f, 2.4f, 5);
    b.outline(3, false);
    gs::TextStyle st{2, 5, 0, 0, 0};
    Bitmap t = gs::textBitmap(num, st);
    b.blit(t, 20 - t.w / 2, 26);
    return b;
}

Bitmap quayArt() {
    Bitmap b(88, 40);
    b.rect(0, 8, 88, 24, 1);
    b.rect(0, 8, 88, 5, 2);
    for (int i = 0; i < 5; i++) b.rect(6 + i * 16, 16, 4, 14, 3);
    b.rect(0, 30, 88, 4, 4);
    b.outline(5, false);
    return b;
}

Bitmap shedArt() {
    Bitmap b(44, 36);
    b.poly({{4, 18}, {22, 4}, {40, 18}}, 2);
    b.rect(8, 18, 28, 14, 1);
    b.rect(18, 22, 8, 10, 3);
    b.rect(11, 22, 5, 5, 4);
    b.outline(5, false);
    return b;
}

Bitmap pileArt() {
    Bitmap b(14, 32);
    b.rect(5, 4, 4, 24, 1);
    b.ellipse(7, 5, 3, 3, 2);
    b.outline(3, false);
    return b;
}

Bitmap flagArt() {
    Bitmap b(28, 36);
    b.rect(4, 4, 2, 28, 1);
    b.poly({{6, 4}, {24, 10}, {6, 16}}, 2);
    b.outline(3, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(16, 28);
    b.rect(7, 10, 2, 16, 2);
    b.ellipse(8, 8, 5, 5, 1);
    b.outline(3, false);
    return b;
}

Bitmap crateArt() {
    Bitmap b(28, 24);
    b.rect(2, 4, 24, 16, 1);
    b.line(2, 4, 26, 20, 2, 1.4f);
    b.line(26, 4, 2, 20, 2, 1.4f);
    b.outline(3, false);
    return b;
}

Bitmap sprayArt() {
    Bitmap b(16, 10);
    b.ellipse(8, 5, 7, 3, 1);
    b.ellipse(5, 4, 2, 1.4f, 2);
    return b;
}

Bitmap ringArt() {
    Bitmap b(24, 24);
    b.ellipse(12, 12, 10, 10, 1);
    b.ellipse(12, 12, 6, 6, 0);
    return b;
}

Bitmap pinArt() {
    Bitmap b(10, 14);
    b.poly({{5, 1}, {9, 8}, {5, 13}, {1, 8}}, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
    uint8_t a[64] = {};
    uint8_t w[64] = {};
    a[2 * 8 + 2] = 1;
    a[4 * 8 + 5] = 2;
    a[6 * 8 + 1] = 3;
    w[1 * 8 + 6] = 2;
    w[3 * 8 + 3] = 1;
    w[5 * 8 + 4] = 3;
    int t0 = tiles.alloc(1);
    int t1 = tiles.alloc(1);
    vdp.loadTile(t0, a);
    vdp.loadTile(t1, w);
    for (int cy = 0; cy < vdp.B.h; cy++)
        for (int cx = 0; cx < vdp.B.w; cx++) vdp.B.set(cx, cy, gs::entry(((cx + cy) & 1) ? t1 : t0, PAL_FIELD));
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 2, 3);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(7, 9, 11), gs::rgb4(14, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_PLOW,
           {0, gs::rgb4(15, 9, 2), gs::rgb4(12, 6, 2), gs::rgb4(6, 4, 3), gs::rgb4(9, 12, 14), gs::rgb4(15, 13, 4),
            gs::rgb4(15, 4, 2), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 2), gs::rgb4(14, 14, 12), gs::rgb4(1, 1, 2), 0, 0, 0, 0,
            ink});
    setPal(vdp, PAL_NUN, {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(4, 1, 1), gs::rgb4(15, 12, 3),
                          gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CAN, {0, gs::rgb4(2, 11, 4), gs::rgb4(15, 15, 15), gs::rgb4(1, 4, 2), gs::rgb4(14, 12, 3),
                          gs::rgb4(15, 14, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(9, 6, 3), gs::rgb4(13, 10, 6), gs::rgb4(5, 3, 2), gs::rgb4(12, 11, 8),
                           gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(14, 15, 15), gs::rgb4(10, 13, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ICE, {0, gs::rgb4(12, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(11, 9, 6), gs::rgb4(15, 13, 6), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(4, 8, 14), gs::rgb4(2, 4, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_OTHER, {0, gs::rgb4(7, 5, 4), gs::rgb4(11, 8, 6), gs::rgb4(4, 3, 2), gs::rgb4(13, 12, 10),
                            gs::rgb4(8, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 9), gs::rgb4(3, 4, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 7), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FIELD, {0, gs::rgb4(13, 14, 15), gs::rgb4(10, 12, 14), gs::rgb4(8, 11, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BLADE, {0, gs::rgb4(14, 14, 15), gs::rgb4(8, 9, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    loadFont(vdp, art);
    for (int i = 0; i < 8; i++) art.plow[i] = gs::uploadMipped(vdp, paintPlow(i * kTau / 8.f));
    art.nun = gs::uploadMipped(vdp, nunArt("1"));
    art.nun3 = gs::uploadMipped(vdp, nunArt("3"));
    art.can = gs::uploadMipped(vdp, canArt("2"));
    art.quay = gs::uploadMipped(vdp, quayArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.spray = gs::uploadMipped(vdp, sprayArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.pin = gs::uploadMipped(vdp, pinArt());
    art.title = words(vdp, "PLOW BUOY", 3);
    art.round = words(vdp, "ROUND TO PORT", 2);
    art.same = words(vdp, "SAME DOCK", 3);
    art.made = words(vdp, "BEAT THE CREW", 2);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.wrong = words(vdp, "WRONG DOCK", 2);
    art.crew = words(vdp, "OTHER CREW", 2);
    art.paused = words(vdp, "PAUSED", 3);
}

}  // namespace plow
