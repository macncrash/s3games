#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace rick {
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
}

void rot(float ang, float lx, float ly, float& x, float& y) {
    float c = std::cos(ang), s = std::sin(ang);
    x = c * lx - s * ly;
    y = s * lx + c * ly;
}

gs::Pt rp(float ang, float ox, float oy, float lx, float ly) {
    float x, y;
    rot(ang, lx, ly, x, y);
    return {ox + x, oy + y};
}

Bitmap shawArt(float ang) {
    Bitmap b(48, 48);
    const float ox = 24, oy = 24;
    auto wheel = [&](float lx, float ly) {
        float x, y;
        rot(ang, lx, ly, x, y);
        b.ellipse(ox + x, oy + y, 5.2f, 5.2f, 3);
        b.ellipse(ox + x, oy + y, 2.1f, 2.1f, 7);
    };
    wheel(-7.f, -8.f);
    wheel(-7.f, 8.f);
    b.poly({rp(ang, ox, oy, 6, -7), rp(ang, ox, oy, 6, 7), rp(ang, ox, oy, -10, 8), rp(ang, ox, oy, -10, -8)}, 1);
    b.poly({rp(ang, ox, oy, 4, -6), rp(ang, ox, oy, 4, 6), rp(ang, ox, oy, -6, 6), rp(ang, ox, oy, -6, -6)}, 2);
    b.ellipse(rp(ang, ox, oy, -1, 0).first, rp(ang, ox, oy, -1, 0).second, 2.4f, 2.4f, 8);
    b.poly({rp(ang, ox, oy, 8, -1.2f), rp(ang, ox, oy, 18, -1.2f), rp(ang, ox, oy, 18, 1.2f), rp(ang, ox, oy, 8, 1.2f)}, 4);
    float hx, hy;
    rot(ang, 16.f, 0.f, hx, hy);
    b.ellipse(ox + hx, oy + hy, 3.2f, 3.2f, 5);
    b.ellipse(ox + hx + std::cos(ang) * 1.2f, oy + hy + std::sin(ang) * 1.2f, 1.3f, 1.3f, 6);
    return b;
}

Bitmap nunArt(const char* num) {
    Bitmap b(28, 40);
    b.ellipse(14, 30, 8, 4, 3);
    b.rect(10, 12, 8, 18, 1);
    b.rect(10, 20, 8, 4, 2);
    b.ellipse(14, 10, 6, 5, 1);
    b.ellipse(14, 9, 2, 2, 4);
    gs::TextStyle st{1, 2, 0, 0, 0};
    Bitmap t = gs::textBitmap(num, st);
    b.blit(t, 14 - t.w / 2, 21);
    return b;
}

Bitmap canArt(const char* num) {
    Bitmap b(28, 40);
    b.ellipse(14, 30, 8, 4, 3);
    b.rect(9, 12, 10, 18, 1);
    b.rect(9, 18, 10, 4, 2);
    b.poly({{8, 14}, {14, 4}, {20, 14}}, 1);
    gs::TextStyle st{1, 2, 0, 0, 0};
    Bitmap t = gs::textBitmap(num, st);
    b.blit(t, 14 - t.w / 2, 20);
    return b;
}

Bitmap quayArt(int variant) {
    Bitmap b(40, 28);
    for (int i = 0; i < 4; i++) b.rect(1, 8 + i * 4, 38, 3, (i & 1) ? 1 : 2);
    b.rect(float((variant & 1) ? 6 : 26), 10, 6, 5, 3);
    b.rect(16, 20, 8, 4, 6);
    return b;
}

Bitmap stallArt() {
    Bitmap b(56, 40);
    b.rect(6, 18, 44, 16, 1);
    b.poly({{2, 20}, {28, 4}, {54, 20}}, 2);
    b.rect(24, 22, 8, 12, 3);
    b.rect(10, 22, 8, 6, 4);
    b.rect(36, 22, 8, 6, 5);
    return b;
}

Bitmap pileArt() {
    Bitmap b(12, 20);
    b.rect(4, 4, 4, 14, 1);
    b.ellipse(6, 4, 4, 2.4f, 2);
    return b;
}

Bitmap signArt() {
    Bitmap b(64, 16);
    b.rect(1, 2, 62, 12, 1);
    gs::TextStyle st{1, 2, 0, 0, 0};
    Bitmap t = gs::textBitmap("SAME DOCK", st);
    b.blit(t, 32 - t.w / 2, 4);
    return b;
}

Bitmap lampPostArt() {
    Bitmap b(16, 36);
    b.rect(7, 12, 2, 22, 1);
    b.ellipse(8, 8, 5, 5, 2);
    return b;
}

Bitmap wrongArt() {
    Bitmap b(48, 22);
    b.rect(2, 4, 44, 14, 1);
    gs::TextStyle st{1, 2, 0, 0, 0};
    Bitmap t = gs::textBitmap("OTHER", st);
    b.blit(t, 24 - t.w / 2, 6);
    return b;
}

Bitmap crateArt() {
    Bitmap b(24, 20);
    b.rect(2, 4, 20, 14, 1);
    b.line(2, 4, 22, 18, 2, 1.2f);
    b.line(22, 4, 2, 18, 3, 1.2f);
    return b;
}

Bitmap birdArt(bool up) {
    Bitmap b(24, 14);
    float tip = up ? 2.f : 11.f;
    b.line(12, 7, 2, tip, 1, 1.4f);
    b.line(12, 7, 22, tip, 1, 1.4f);
    b.ellipse(12, 8, 2, 1.4f, 2);
    return b;
}

Bitmap dustArt() {
    Bitmap b(16, 10);
    b.ellipse(5, 6, 4, 2, 1);
    b.ellipse(11, 4, 3, 1.6f, 2);
    return b;
}

Bitmap smokeArt() {
    Bitmap b(14, 14);
    b.ellipse(7, 8, 5, 4, 1);
    b.ellipse(6, 5, 3, 2.4f, 2);
    return b;
}

Bitmap ringArt() {
    Bitmap b(64, 64);
    const float c = 31.5f, r = 28.f;
    for (int d = 0; d < 360; d += 14) {
        float a0 = d * kTau / 360.f;
        float a1 = (d + 7.f) * kTau / 360.f;
        b.line(c + std::cos(a0) * r, c + std::sin(a0) * r, c + std::cos(a1) * r, c + std::sin(a1) * r, 1, 2.f);
    }
    return b;
}

Bitmap lampArt() {
    Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 1.6f, 1.6f, 2);
    return b;
}

Bitmap pinArt() {
    Bitmap b(12, 12);
    b.poly({{6, 1}, {11, 6}, {6, 11}, {1, 6}}, 1);
    return b;
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
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
    uint8_t a[64] = {};
    uint8_t w[64] = {};
    a[2 * 8 + 3] = 1;
    a[5 * 8 + 6] = 2;
    w[1 * 8 + 1] = 2;
    w[4 * 8 + 4] = 1;
    int t0 = tiles.alloc(1);
    int t1 = tiles.alloc(1);
    vdp.loadTile(t0, a);
    vdp.loadTile(t1, w);
    for (int cy = 0; cy < vdp.B.h; cy++) {
        for (int cx = 0; cx < vdp.B.w; cx++) {
            vdp.B.set(cx, cy, gs::entry(((cx + cy) & 1) ? t1 : t0, PAL_ROAD));
        }
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 8, 7), gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SHAW,
           {0, gs::rgb4(14, 8, 2), gs::rgb4(12, 3, 2), gs::rgb4(2, 2, 2), gs::rgb4(6, 4, 2), gs::rgb4(13, 10, 6),
            gs::rgb4(15, 13, 8), gs::rgb4(5, 5, 5), gs::rgb4(15, 12, 9), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_NUN, {0, gs::rgb4(14, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(3, 1, 1), gs::rgb4(15, 10, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CAN, {0, gs::rgb4(2, 12, 4), gs::rgb4(15, 15, 15), gs::rgb4(1, 4, 1), gs::rgb4(15, 14, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(10, 8, 5), gs::rgb4(6, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(14, 12, 8), gs::rgb4(8, 6, 3), gs::rgb4(5, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(12, 10, 7), gs::rgb4(8, 7, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(4, 4, 5), gs::rgb4(12, 11, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CRATE, {0, gs::rgb4(11, 7, 3), gs::rgb4(7, 4, 2), gs::rgb4(14, 11, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(7, 7, 6), gs::rgb4(10, 9, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STALL, {0, gs::rgb4(13, 9, 4), gs::rgb4(12, 3, 2), gs::rgb4(3, 2, 1), gs::rgb4(15, 14, 8), gs::rgb4(4, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 12, 3), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 2), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(8, 8, 7), gs::rgb4(13, 12, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(2, 3, 2), gs::rgb4(10, 9, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    for (int i = 0; i < 8; i++) art.shaw[i] = gs::uploadMipped(vdp, shawArt(i * kTau / 8.f));
    art.buoy[0] = gs::uploadMipped(vdp, nunArt("1"));
    art.buoy[1] = gs::uploadMipped(vdp, canArt("2"));
    art.buoy[2] = gs::uploadMipped(vdp, nunArt("3"));
    art.quay = gs::uploadMipped(vdp, quayArt(0));
    art.quayB = gs::uploadMipped(vdp, quayArt(1));
    art.stall = gs::uploadMipped(vdp, stallArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    art.lampPost = gs::uploadMipped(vdp, lampPostArt());
    art.wrong = gs::uploadMipped(vdp, wrongArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(true));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(false));
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.smoke = gs::uploadMipped(vdp, smokeArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.pin = gs::uploadMipped(vdp, pinArt());
    Bitmap dot(6, 6);
    dot.ellipse(3, 3, 2.f, 2.f, 1);
    art.dot = gs::uploadMipped(vdp, dot);
    Bitmap panel(70, 56);
    panel.rect(0, 0, 70, 56, 1);
    art.panel = gs::uploadMipped(vdp, panel);
    art.title = words(vdp, "RICKSHAW", 3);
    art.sub = words(vdp, "ROUND THE BUOYS", 2);
    art.same = words(vdp, "SAME DOCK", 3);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.paused = words(vdp, "PAUSED", 3);
    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
}

}  // namespace rick
