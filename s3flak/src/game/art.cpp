#include "game/art.h"

#include <cmath>
#include <string>

namespace flak {
namespace {

constexpr float kPi = 3.14159265f;

using gs::Bitmap;
using gs::Pt;

void pal(gs::VDP& v, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) v.setColor(p * 16 + i, c);
        ++i;
    }
    for (; i < 16; ++i) v.setColor(p * 16 + i, 0);
}

void textPal(gs::VDP& v, int p, uint16_t ink) {
    pal(v, p, {0, ink});
    v.setColor(p * 16 + 15, gs::rgb4(1, 1, 2));
}

// Nose points right. prop is 0 or 1, a spinning disc.
Bitmap bomber(int prop) {
    Bitmap b(112, 56);
    const int hi = 1, mid = 2, lo = 3, glass = 4, ink = 5, mark = 6, lit = 7, belly = 8, eng = 9;
    b.poly({{14.f, 30.f}, {30.f, 28.f}, {22.f, 6.f}, {10.f, 10.f}}, mid);
    b.poly({{16.f, 28.f}, {26.f, 26.f}, {20.f, 10.f}}, hi);
    b.rect(14, 8, 8, 4, mark);
    b.poly({{6.f, 26.f}, {32.f, 24.f}, {32.f, 31.f}, {6.f, 33.f}}, lo);
    b.ellipse(62, 30, 42, 11, mid);
    b.ellipse(66, 28, 36, 8, hi);
    b.ellipse(60, 35, 26, 5, belly);
    b.ellipse(98, 28, 12, 7, mid);
    b.ellipse(102, 27, 8, 5, glass);
    b.ellipse(104, 26, 3, 2, lit);
    b.ellipse(78, 21, 9, 5, glass);
    b.ellipse(79, 20, 5, 3, lit);
    b.poly({{38.f, 26.f}, {92.f, 22.f}, {96.f, 30.f}, {42.f, 34.f}}, lo);
    b.poly({{44.f, 26.f}, {90.f, 24.f}, {92.f, 28.f}, {48.f, 30.f}}, mid);
    for (float ex : {52.f, 76.f}) {
        b.ellipse(ex, 37, 11, 7, eng);
        b.ellipse(ex, 36, 7, 4, lo);
        float ry = prop ? 7.f : 3.2f;
        b.ellipse(ex + 10, 36, 2.4f, ry, lit);
        b.rect(ex + 8, 35, 3, 2, mid);
    }
    b.rect(56, 38, 18, 3, ink);
    b.poly({{46.f, 29.f}, {54.f, 23.f}, {62.f, 29.f}, {54.f, 35.f}}, mark);
    b.poly({{50.f, 29.f}, {54.f, 26.f}, {58.f, 29.f}, {54.f, 32.f}}, ink);
    b.rect(18, 22, 3, 10, mark);
    b.outline(ink, false);
    return b;
}

Bitmap barrelArt(float deg) {
    Bitmap b(168, 112);
    const float px = 84.f, py = 104.f, len = 90.f;
    const float rad = deg * kPi / 180.f;
    const float s = std::sin(rad), c = std::cos(rad);
    const float ox = c * 4.2f, oy = s * 4.2f;
    auto tube = [&](float sign, int col, float thick) {
        float x0 = px + ox * sign, y0 = py + oy * sign;
        b.line(x0, y0, x0 + s * len, y0 - c * len, col, thick);
    };
    tube(-1.f, 2, 5.5f);
    tube(1.f, 2, 5.5f);
    tube(-1.f, 1, 2.4f);
    tube(1.f, 1, 2.4f);
    float x0 = px + s * (len - 10.f), y0 = py - c * (len - 10.f);
    float x1 = px + s * len, y1 = py - c * len;
    b.line(x0 - ox, y0 - oy, x1 - ox, y1 - oy, 3, 3.2f);
    b.line(x0 + ox, y0 + oy, x1 + ox, y1 + oy, 3, 3.2f);
    return b;
}

Bitmap mountArt() {
    Bitmap b(96, 56);
    b.ellipse(48, 44, 40, 12, 2);
    b.ellipse(48, 42, 32, 8, 1);
    b.ellipse(48, 40, 18, 5, 2);
    b.poly({{16.f, 40.f}, {18.f, 16.f}, {78.f, 16.f}, {80.f, 40.f}}, 1);
    b.poly({{22.f, 36.f}, {26.f, 18.f}, {70.f, 18.f}, {74.f, 36.f}}, 2);
    b.rect(40, 22, 16, 4, 2);
    b.rect(42, 23, 12, 2, 9);
    b.ellipse(48, 14, 9, 8, 7);
    b.ellipse(48, 13, 6, 5, 8);
    b.rect(43, 16, 10, 3, 8);
    b.rect(28, 30, 7, 4, 8);
    b.rect(61, 30, 7, 4, 8);
    b.rect(46, 34, 4, 8, 2);
    b.outline(2, false);
    return b;
}

Bitmap deckArt() {
    Bitmap b(160, 44);
    for (int y = 0; y < 44; y++) {
        int seam = (y % 9) == 0;
        int plank = (y / 9) & 1;
        int c = seam ? 3 : (plank ? 2 : 1);
        for (int x = 0; x < 160; x++) b.set(x, y, (x + y) % 37 == 0 ? 3 : c);
    }
    for (int y = 4; y < 44; y += 9)
        for (int x = 6; x < 160; x += 18) b.set(x, y, 8);
    b.rect(0, 0, 160, 2, 5);
    return b;
}

Bitmap railArt() {
    Bitmap b(80, 26);
    b.rect(0, 4, 80, 3, 4);
    b.rect(0, 5, 80, 1, 6);
    b.rect(0, 14, 80, 2, 5);
    for (int x = 4; x < 80; x += 14) {
        b.rect(float(x), 3, 3, 20, 5);
        b.rect(float(x), 3, 1, 20, 4);
    }
    return b;
}

Bitmap bowArt() {
    Bitmap b(78, 40);
    b.poly({{39.f, 2.f}, {74.f, 36.f}, {4.f, 36.f}}, 4);
    b.poly({{39.f, 10.f}, {64.f, 36.f}, {14.f, 36.f}}, 5);
    b.line(39, 4, 39, 36, 6, 1.3f);
    b.rect(38, 8, 1, 12, 8);
    b.poly({{39.f, 8.f}, {54.f, 12.f}, {39.f, 16.f}}, 9);
    b.ellipse(39, 34, 18, 4, 6);
    return b;
}

Bitmap funnelArt() {
    Bitmap b(40, 52);
    b.poly({{8.f, 50.f}, {12.f, 16.f}, {28.f, 16.f}, {32.f, 50.f}}, 5);
    b.poly({{14.f, 48.f}, {16.f, 20.f}, {26.f, 20.f}, {28.f, 48.f}}, 4);
    b.rect(10, 12, 20, 6, 8);
    b.rect(12, 6, 16, 7, 7);
    b.ellipse(20, 8, 6, 3, 8);
    b.rect(18, 2, 4, 4, 5);
    return b;
}

Bitmap crateArt() {
    Bitmap b(30, 24);
    b.rect(2, 6, 26, 16, 2);
    b.rect(2, 6, 26, 5, 1);
    b.line(2, 6, 15, 14, 3, 1.2f);
    b.line(28, 6, 15, 14, 3, 1.2f);
    b.rect(6, 16, 18, 3, 9);
    b.outline(8, false);
    return b;
}

Bitmap ringArt() {
    Bitmap b(20, 20);
    b.ellipse(10, 10, 9, 9, 9);
    b.ellipse(10, 10, 4, 4, 0);
    b.rect(8, 1, 4, 4, 6);
    b.rect(8, 15, 4, 4, 6);
    b.rect(1, 8, 4, 4, 6);
    b.rect(15, 8, 4, 4, 6);
    return b;
}

Bitmap roundArt() {
    Bitmap b(8, 16);
    b.rect(2, 4, 4, 9, 4);
    b.rect(2, 3, 4, 2, 5);
    b.rect(3, 1, 2, 2, 6);
    b.rect(3, 12, 2, 2, 5);
    return b;
}

Bitmap sightArt() {
    Bitmap b(48, 48);
    for (int i = 0; i < 72; i++) {
        if ((i % 18) < 3) continue;
        float a = i * (kPi * 2.f / 72.f);
        int x = int(std::lround(24 + std::cos(a) * 18));
        int y = int(std::lround(24 + std::sin(a) * 18));
        b.set(x, y, 1);
        b.set(x, y + 1, 4);
        int x2 = int(std::lround(24 + std::cos(a) * 16));
        int y2 = int(std::lround(24 + std::sin(a) * 16));
        b.set(x2, y2, 2);
    }
    b.rect(23, 2, 2, 7, 3);
    b.rect(23, 39, 2, 7, 3);
    b.rect(2, 23, 7, 2, 3);
    b.rect(39, 23, 7, 2, 3);
    b.rect(23, 23, 2, 2, 3);
    return b;
}

Bitmap tracerArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3.2f, 3.2f, 2);
    b.ellipse(4, 4, 1.4f, 1.4f, 1);
    return b;
}

Bitmap puffArt() {
    Bitmap b(44, 40);
    b.ellipse(22, 21, 16, 13, 6);
    b.ellipse(15, 16, 10, 8, 5);
    b.ellipse(28, 17, 9, 8, 4);
    b.ellipse(20, 22, 6, 5, 5);
    b.ellipse(23, 18, 3, 3, 7);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            if (!b.get(x, y)) continue;
            float dx = x - 22.f, dy = y - 20.f;
            if (dx * dx + dy * dy > 150.f && ((x * 3 + y * 5) & 3) == 0) b.set(x, y, 0);
        }
    }
    return b;
}

Bitmap flashArt() {
    Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 6, 2);
    b.ellipse(10, 10, 4, 3, 1);
    b.ellipse(10, 10, 2, 1.4f, 3);
    return b;
}

Bitmap smokeArt() {
    Bitmap b(28, 20);
    b.ellipse(14, 12, 10, 6, 4);
    b.ellipse(9, 11, 6, 4, 5);
    b.ellipse(18, 8, 5, 4, 4);
    return b;
}

Bitmap bombArt() {
    Bitmap b(10, 18);
    b.ellipse(5, 10, 4, 6, 3);
    b.rect(4, 3, 2, 5, 9);
    b.rect(4, 2, 2, 2, 6);
    b.line(5, 16, 5, 17, 5, 1.f);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(80, 30);
    b.ellipse(22, 18, 16, 9, 2);
    b.ellipse(40, 15, 20, 11, 1);
    b.ellipse(58, 18, 14, 8, 2);
    b.ellipse(36, 13, 10, 6, 1);
    return b;
}

Bitmap sunArt() {
    Bitmap b(36, 36);
    b.ellipse(18, 18, 16, 16, 4);
    b.ellipse(18, 18, 10, 10, 3);
    b.ellipse(14, 14, 4, 3, 1);
    return b;
}

Bitmap birdArt(int flap) {
    Bitmap b(14, 8);
    if (flap == 0) {
        b.line(0, 5, 6, 1, 5, 1.2f);
        b.line(6, 1, 13, 5, 5, 1.2f);
    } else {
        b.line(0, 1, 6, 5, 5, 1.2f);
        b.line(6, 5, 13, 1, 5, 1.2f);
    }
    return b;
}

Bitmap waveArt() {
    Bitmap b(72, 16);
    b.ellipse(16, 10, 14, 4, 2);
    b.ellipse(40, 9, 16, 5, 1);
    b.ellipse(58, 11, 12, 3, 2);
    b.ellipse(36, 8, 8, 2, 3);
    return b;
}

Bitmap gateArt() {
    Bitmap b(6, 40);
    for (int y = 0; y < 40; y += 7) b.rect(2, y, 2, 3, 1);
    return b;
}

Bitmap freighterArt() {
    Bitmap b(56, 18);
    b.poly({{2.f, 14.f}, {52.f, 14.f}, {46.f, 9.f}, {8.f, 9.f}}, 5);
    b.rect(30, 3, 8, 7, 5);
    b.rect(33, 1, 3, 3, 8);
    b.rect(12, 6, 12, 4, 4);
    b.rect(16, 11, 6, 3, 9);
    return b;
}

Bitmap crackArt() {
    Bitmap b(48, 28);
    b.line(4, 2, 18, 14, 8, 1.6f);
    b.line(18, 14, 12, 26, 8, 1.4f);
    b.line(18, 14, 40, 6, 8, 1.4f);
    b.line(18, 14, 36, 24, 8, 1.2f);
    b.line(18, 14, 8, 16, 8, 1.f);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8 && x + 2 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

const float kBarrelDeg[9] = {-64.f, -48.f, -32.f, -16.f, 0.f, 16.f, 32.f, 48.f, 64.f};

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_WHITE, gs::rgb4(15, 15, 15));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 7));
    // Sight shares the amber ink at index 1, plus bright, white, and a dim tick.
    vdp.setColor(PAL_AMBER * 16 + 2, gs::rgb4(15, 14, 8));
    vdp.setColor(PAL_AMBER * 16 + 3, gs::rgb4(15, 15, 13));
    vdp.setColor(PAL_AMBER * 16 + 4, gs::rgb4(6, 4, 1));

    pal(vdp, PAL_SHIP,
        {0, gs::rgb4(12, 9, 5), gs::rgb4(9, 6, 3), gs::rgb4(5, 3, 2), gs::rgb4(10, 11, 12), gs::rgb4(4, 5, 6),
         gs::rgb4(14, 14, 13), gs::rgb4(11, 5, 2), gs::rgb4(1, 1, 2), gs::rgb4(13, 2, 2), gs::rgb4(8, 7, 5), 0, 0, 0,
         0, gs::rgb4(1, 1, 2)});
    pal(vdp, PAL_PLANE,
        {0, gs::rgb4(12, 13, 8), gs::rgb4(7, 8, 4), gs::rgb4(3, 4, 2), gs::rgb4(10, 14, 15), gs::rgb4(1, 1, 1),
         gs::rgb4(15, 13, 2), gs::rgb4(15, 15, 13), gs::rgb4(5, 5, 4), gs::rgb4(4, 4, 5), gs::rgb4(8, 4, 2), 0, 0, 0,
         0, gs::rgb4(1, 1, 2)});
    pal(vdp, PAL_BURST,
        {0, gs::rgb4(15, 15, 15), gs::rgb4(15, 13, 3), gs::rgb4(15, 7, 2), gs::rgb4(12, 12, 11), gs::rgb4(8, 8, 7),
         gs::rgb4(4, 4, 4), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0});
    pal(vdp, PAL_GUN,
        {0, gs::rgb4(8, 9, 10), gs::rgb4(3, 3, 4), gs::rgb4(13, 14, 15), gs::rgb4(14, 11, 4), gs::rgb4(8, 6, 2),
         gs::rgb4(15, 14, 8), gs::rgb4(3, 4, 3), gs::rgb4(12, 8, 5), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0});
    pal(vdp, PAL_SKY,
        {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 11, 13), gs::rgb4(15, 14, 6), gs::rgb4(15, 10, 4), gs::rgb4(2, 2, 3),
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    pal(vdp, PAL_SEA,
        {0, gs::rgb4(13, 15, 15), gs::rgb4(6, 12, 14), gs::rgb4(14, 15, 15), gs::rgb4(3, 8, 11), 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0});

    vdp.setFogColor(gs::rgb4(12, 11, 10));
    loadFont(vdp, art);

    art.bomber[0] = gs::uploadMipped(vdp, bomber(0));
    art.bomber[1] = gs::uploadMipped(vdp, bomber(1));
    for (int i = 0; i < 9; i++) art.barrel[i] = gs::uploadMipped(vdp, barrelArt(kBarrelDeg[i]));
    art.mount = gs::uploadMipped(vdp, mountArt());
    art.deck = gs::uploadMipped(vdp, deckArt());
    art.rail = gs::uploadMipped(vdp, railArt());
    art.bow = gs::uploadMipped(vdp, bowArt());
    art.funnel = gs::uploadMipped(vdp, funnelArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.round = gs::uploadMipped(vdp, roundArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
    art.tracer = gs::uploadMipped(vdp, tracerArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.smoke = gs::uploadMipped(vdp, smokeArt());
    art.bomb = gs::uploadMipped(vdp, bombArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(0));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(1));
    art.wave = gs::uploadMipped(vdp, waveArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.freighter = gs::uploadMipped(vdp, freighterArt());
    art.crack = gs::uploadMipped(vdp, crackArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace flak
