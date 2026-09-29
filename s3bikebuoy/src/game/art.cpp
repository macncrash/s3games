#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace bike {
namespace {

constexpr float kTau = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) vdp.setColor(pal * 16 + i++, c);
}

void stamp(gs::Bitmap& b, float ang, float lx, float ly, int c) {
    float cs = std::cos(ang), sn = std::sin(ang);
    int x = int(std::lround(b.w * 0.5f + lx * cs - ly * sn));
    int y = int(std::lround(b.h * 0.5f + lx * sn + ly * cs));
    b.set(x, y, c);
}

gs::Bitmap paintBike(int dir) {
    // dir 0 faces east; each step is 45 degrees, screen y grows south.
    float ang = -dir * (kTau / 8.f);
    gs::Bitmap b(40, 40);
    for (float u = -15.f; u <= 15.f; u += 1.f) {
        for (float v = -5.5f; v <= 5.5f; v += 1.f) {
            float e = (u * u) / (15.f * 15.f) + (v * v) / (5.5f * 5.5f);
            if (e > 1.f) continue;
            int c = 1;
            if (u > 6.f) c = 2;
            if (std::fabs(v) > 4.2f) c = 3;
            if (u > 10.f && std::fabs(v) < 2.4f) c = 6;
            stamp(b, ang, u, v, c);
        }
    }
    for (float v = -3.f; v <= 3.f; v += 1.f) stamp(b, ang, -2.f, v, 4);
    stamp(b, ang, -4.f, 0.f, 5);
    stamp(b, ang, -1.f, 0.f, 7);
    for (float u = -12.f; u <= 8.f; u += 2.f) {
        stamp(b, ang, u, 6.4f, 8);
        stamp(b, ang, u, -6.4f, 8);
    }
    return b;
}

gs::Bitmap buoyArt() {
    gs::Bitmap b(18, 36);
    b.rect(7, 2, 4, 6, 4);
    b.rect(4, 8, 10, 20, 1);
    b.rect(6, 12, 6, 10, 2);
    b.ellipse(9, 30, 5.5f, 3.2f, 3);
    return b;
}

gs::Bitmap quayArt() {
    gs::Bitmap b(48, 18);
    b.rect(0, 4, 48, 12, 1);
    for (int x = 2; x < 48; x += 8) b.rect(x, 6, 3, 8, 2);
    b.rect(0, 2, 48, 3, 3);
    return b;
}

gs::Bitmap pileArt() {
    gs::Bitmap b(8, 28);
    b.rect(2, 0, 4, 24, 1);
    b.rect(1, 22, 6, 4, 3);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(40, 30);
    b.poly({{2, 16}, {20, 3}, {38, 16}}, 4);
    b.rect(5, 16, 30, 12, 1);
    b.rect(17, 18, 7, 10, 3);
    b.rect(8, 19, 5, 5, 2);
    b.rect(27, 19, 5, 5, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 16);
    b.ellipse(5, 5, 3.2f, 3.2f, 1);
    b.line(5, 8, 5, 15, 2, 1.3f);
    return b;
}

gs::Bitmap wakeArt() {
    gs::Bitmap b(12, 8);
    b.ellipse(6, 4, 5, 2.4f, 1);
    b.set(3, 3, 2);
    b.set(8, 4, 2);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(16, 22);
    b.line(3, 1, 3, 21, 3, 1.3f);
    b.poly({{4, 3}, {14, 7}, {4, 11}}, 1);
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
    for (int i = 0; i < 64; i++) {
        int x = i % 8, y = i / 8;
        if (((x / 2) + (y / 2)) & 1) a[i] = 1;
        else w[i] = (x == 1 || y == 6) ? 2 : 1;
    }
    int t0 = tiles.alloc(1);
    int t1 = tiles.alloc(1);
    vdp.loadTile(t0, a);
    vdp.loadTile(t1, w);
    for (int cy = 0; cy < vdp.B.h; cy++)
        for (int cx = 0; cx < vdp.B.w; cx++) vdp.B.set(cx, cy, gs::entry(((cx + cy) & 1) ? t1 : t0, PAL_WATER));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 2, 3);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 12, 14), gs::rgb4(15, 12, 5), gs::rgb4(4, 14, 8),
                          gs::rgb4(15, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BIKE,
           {0, gs::rgb4(1, 8, 10), gs::rgb4(3, 13, 14), gs::rgb4(0, 4, 6), gs::rgb4(2, 2, 3), gs::rgb4(12, 7, 4),
            gs::rgb4(15, 15, 12), gs::rgb4(14, 3, 3), gs::rgb4(6, 7, 8), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 14, 12), gs::rgb4(4, 8, 10), gs::rgb4(15, 10, 2), 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(2, 11, 4), gs::rgb4(15, 15, 14), gs::rgb4(3, 8, 10), gs::rgb4(12, 14, 6), 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(13, 10, 2), gs::rgb4(15, 15, 12), gs::rgb4(4, 8, 9), gs::rgb4(15, 13, 4), 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 6, 3), gs::rgb4(5, 4, 2), gs::rgb4(12, 10, 6), gs::rgb4(3, 6, 4), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WAKE, {0, gs::rgb4(12, 15, 15), gs::rgb4(8, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SHED, {0, gs::rgb4(9, 8, 7), gs::rgb4(6, 12, 14), gs::rgb4(3, 3, 4), gs::rgb4(12, 4, 3), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 7, 11), gs::rgb4(4, 10, 13), gs::rgb4(1, 5, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, shadow});
    vdp.setFogColor(gs::rgb4(3, 8, 12));

    for (int i = 0; i < 8; i++) art.bike[i] = gs::uploadMipped(vdp, paintBike(i));
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.quay = gs::uploadMipped(vdp, quayArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.wake = gs::uploadMipped(vdp, wakeArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    loadFont(vdp, art);
}

}  // namespace bike
