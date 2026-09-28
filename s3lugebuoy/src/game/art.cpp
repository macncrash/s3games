#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace luge {
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

gs::Bitmap paintPod(float ang) {
    gs::Bitmap b(48, 48);
    for (float u = -18.f; u <= 18.f; u += 1.f) {
        for (float v = -7.f; v <= 7.f; v += 1.f) {
            float e = (u * u) / (18.f * 18.f) + (v * v) / (7.f * 7.f);
            if (e > 1.f) continue;
            int c = (u > 8.f) ? 4 : (std::fabs(v) > 5.2f ? 1 : 3);
            if (u > 11.f && std::fabs(v) < 3.2f) c = 6;
            stamp(b, ang, u, v, c);
        }
    }
    for (float u = -16.f; u <= 15.f; u += 1.f) {
        stamp(b, ang, u, 8.2f, 5);
        stamp(b, ang, u, -8.2f, 5);
        if (u > 12.f) {
            stamp(b, ang, u, 9.4f, 2);
            stamp(b, ang, u, -9.4f, 2);
        }
    }
    stamp(b, ang, 14.f, 0.f, 12);
    return b;
}

gs::Bitmap nunArt(const char* digit) {
    gs::Bitmap b(18, 42);
    b.rect(6, 2, 6, 8, 4);
    b.rect(4, 10, 10, 26, 1);
    b.rect(7, 14, 4, 16, 2);
    gs::TextStyle st{1, 2, 0, 0, 0};
    b.blit(gs::textBitmap(digit, st), 6, 18);
    b.ellipse(9, 38, 3.2f, 2.2f, 3);
    return b;
}

gs::Bitmap canArt(const char* digit) {
    gs::Bitmap b(20, 36);
    b.ellipse(10, 6, 7, 3, 4);
    b.rect(3, 6, 14, 24, 1);
    b.rect(6, 12, 8, 12, 2);
    gs::TextStyle st{1, 2, 0, 0, 0};
    b.blit(gs::textBitmap(digit, st), 7, 15);
    b.ellipse(10, 30, 7, 3, 3);
    return b;
}

gs::Bitmap quayArt() {
    gs::Bitmap b(40, 16);
    b.rect(0, 4, 40, 10, 1);
    for (int x = 2; x < 40; x += 6) b.rect(x, 6, 2, 6, 3);
    b.rect(0, 2, 40, 3, 4);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(36, 28);
    b.poly({{2, 16}, {18, 4}, {34, 16}}, 4);
    b.rect(4, 16, 28, 10, 1);
    b.rect(15, 18, 6, 8, 3);
    b.rect(8, 18, 4, 4, 2);
    b.rect(24, 18, 4, 4, 2);
    return b;
}

gs::Bitmap pileArt() {
    gs::Bitmap b(8, 22);
    b.rect(2, 0, 4, 20, 1);
    b.rect(1, 18, 6, 3, 3);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(16, 24);
    b.line(3, 2, 3, 22, 3, 1.4f);
    b.poly({{4, 3}, {14, 7}, {4, 11}}, 1);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(22, 30);
    b.poly({{11, 1}, {2, 16}, {20, 16}}, 1);
    b.poly({{11, 8}, {4, 22}, {18, 22}}, 2);
    b.rect(9, 22, 4, 7, 4);
    b.set(11, 4, 3);
    return b;
}

gs::Bitmap driftArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(14, 7, 12, 4, 1);
    b.ellipse(8, 6, 5, 2.4f, 2);
    return b;
}

gs::Bitmap birdArt(bool up) {
    gs::Bitmap b(16, 10);
    if (up) {
        b.line(1, 7, 8, 3, 1, 1.2f);
        b.line(8, 3, 15, 7, 1, 1.2f);
    } else {
        b.line(1, 3, 8, 7, 1, 1.2f);
        b.line(8, 7, 15, 3, 1, 1.2f);
    }
    b.set(8, 5, 3);
    return b;
}

gs::Bitmap sprayArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 2.2f, 1);
    b.set(2, 3, 2);
    b.set(6, 4, 2);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(24, 24);
    b.ellipse(12, 12, 10, 10, 1);
    b.ellipse(12, 12, 6, 6, 0);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 12);
    b.ellipse(4, 4, 3, 3, 1);
    b.line(4, 7, 4, 11, 2, 1.2f);
    return b;
}

gs::Bitmap pinArt() {
    gs::Bitmap b(10, 12);
    b.poly({{5, 1}, {9, 8}, {1, 8}}, 1);
    b.rect(4, 8, 2, 3, 2);
    return b;
}

gs::Bitmap panelArt() {
    gs::Bitmap b(36, 48);
    b.rect(1, 1, 34, 46, 1);
    b.rect(3, 3, 30, 42, 2);
    return b;
}

gs::Bitmap crackArt() {
    gs::Bitmap b(10, 36);
    b.line(5, 1, 2, 16, 2, 1.4f);
    b.line(2, 16, 7, 34, 2, 1.4f);
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
    a[6 * 8 + 1] = 1;
    w[1 * 8 + 6] = 2;
    w[3 * 8 + 3] = 1;
    w[5 * 8 + 4] = 2;
    int t0 = tiles.alloc(1);
    int t1 = tiles.alloc(1);
    vdp.loadTile(t0, a);
    vdp.loadTile(t1, w);
    for (int cy = 0; cy < vdp.B.h; cy++)
        for (int cx = 0; cx < vdp.B.w; cx++) vdp.B.set(cx, cy, gs::entry(((cx + cy) & 1) ? t1 : t0, PAL_SNOW));
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(2, 2, 3);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(9, 11, 13), gs::rgb4(15, 13, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_POD,
           {0, gs::rgb4(3, 4, 6), gs::rgb4(12, 3, 2), gs::rgb4(14, 12, 8), gs::rgb4(8, 9, 12), gs::rgb4(11, 12, 14),
            gs::rgb4(6, 14, 15), gs::rgb4(15, 8, 3), gs::rgb4(2, 2, 3), gs::rgb4(9, 6, 3), gs::rgb4(1, 1, 2),
            gs::rgb4(13, 6, 4), gs::rgb4(15, 15, 15), gs::rgb4(4, 2, 2), gs::rgb4(10, 13, 15), shadow});
    setPal(vdp, PAL_NUN, {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(4, 1, 1), gs::rgb4(15, 12, 3),
                          gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CAN, {0, gs::rgb4(2, 11, 4), gs::rgb4(15, 15, 15), gs::rgb4(1, 4, 2), gs::rgb4(14, 12, 3),
                          gs::rgb4(1, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(11, 8, 5), gs::rgb4(14, 15, 15), gs::rgb4(5, 3, 2), gs::rgb4(12, 9, 4),
                           gs::rgb4(7, 5, 3), gs::rgb4(9, 3, 3), gs::rgb4(3, 11, 5), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 7), gs::rgb4(12, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 6, 3), gs::rgb4(4, 9, 5), gs::rgb4(14, 15, 15), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DRIFT, {0, gs::rgb4(14, 15, 15), gs::rgb4(9, 11, 13), gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_OTHER, {0, gs::rgb4(8, 5, 3), gs::rgb4(13, 14, 15), gs::rgb4(4, 2, 2), gs::rgb4(12, 9, 4),
                            gs::rgb4(6, 4, 3), gs::rgb4(10, 4, 2), gs::rgb4(13, 8, 2), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 10), gs::rgb4(3, 4, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 7), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(13, 15, 15), gs::rgb4(8, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(1, 3, 6), gs::rgb4(9, 13, 15), gs::rgb4(3, 5, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.pod[i] = gs::uploadMipped(vdp, paintPod(i * kTau / 16.f));
    art.nun = gs::uploadMipped(vdp, nunArt("1"));
    art.nun3 = gs::uploadMipped(vdp, nunArt("3"));
    art.can = gs::uploadMipped(vdp, canArt("2"));
    art.quay = gs::uploadMipped(vdp, quayArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.drift = gs::uploadMipped(vdp, driftArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(true));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(false));
    art.spray = gs::uploadMipped(vdp, sprayArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.pin = gs::uploadMipped(vdp, pinArt());
    gs::Bitmap dot(6, 6);
    dot.ellipse(3, 3, 2.2f, 2.2f, 1);
    art.dot = gs::uploadMipped(vdp, dot);
    art.panel = gs::uploadMipped(vdp, panelArt());
    art.crack = gs::uploadMipped(vdp, crackArt());
    art.title = words(vdp, "LUGE BUOY", 3);
    art.round = words(vdp, "ROUND THE BUOYS", 2);
    art.same = words(vdp, "SAME DOCK", 3);
    art.made = words(vdp, "THE LEG IS MADE", 2);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.wrong = words(vdp, "WRONG DOCK", 2);
    art.paused = words(vdp, "PAUSED", 3);
}

}  // namespace luge
