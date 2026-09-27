#include "game/art.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace fkilo {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float PPM = 10.f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 2, 3));
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 2246822519u + uint32_t(y) * 3266489917u;
    h = (h ^ (h >> 13)) * 16777619u;
    return h ^ (h >> 16);
}

void polyM(Bitmap& b, float cgx, float cgy, float ang, std::initializer_list<Pt> meters, int c) {
    const float ca = std::cos(-ang), sa = std::sin(-ang);
    std::vector<Pt> p;
    p.reserve(meters.size());
    for (const Pt& m : meters) {
        float dx = m.first * PPM, dy = -m.second * PPM;
        p.push_back({cgx + dx * ca - dy * sa, cgy + dx * sa + dy * ca});
    }
    b.poly(p, c);
}

Pt xform(float cgx, float cgy, float ang, float mx, float my) {
    const float ca = std::cos(-ang), sa = std::sin(-ang);
    float dx = mx * PPM, dy = -my * PPM;
    return {cgx + dx * ca - dy * sa, cgy + dx * sa + dy * ca};
}

Hull drawHull(gs::VDP& vdp, float att) {
    const float cgx = 140.f, cgy = 120.f;
    Bitmap b(280, 200);
    // White car ferry, bow to the right. Black boot topping, red funnel, ramp wheel.
    polyM(b, cgx, cgy, att, {{-4.7f, -0.15f}, {3.6f, -0.22f}, {4.55f, 0.05f}, {3.4f, 0.55f}, {-4.55f, 0.48f}}, 1);
    polyM(b, cgx, cgy, att, {{-4.6f, -0.55f}, {3.3f, -0.62f}, {4.15f, -0.18f}, {3.5f, -0.05f}, {-4.5f, 0.02f}}, 2);
    polyM(b, cgx, cgy, att, {{-4.4f, 0.42f}, {3.2f, 0.48f}, {3.05f, 0.72f}, {-4.3f, 0.66f}}, 3);
    polyM(b, cgx, cgy, att, {{-1.6f, 0.62f}, {1.5f, 0.66f}, {1.35f, 1.85f}, {-1.45f, 1.78f}}, 4);
    polyM(b, cgx, cgy, att, {{-1.2f, 1.15f}, {-0.2f, 1.18f}, {-0.25f, 1.55f}, {-1.15f, 1.5f}}, 8);
    polyM(b, cgx, cgy, att, {{0.15f, 1.15f}, {1.15f, 1.18f}, {1.1f, 1.55f}, {0.2f, 1.5f}}, 8);
    polyM(b, cgx, cgy, att, {{-3.85f, 0.55f}, {-3.15f, 0.58f}, {-3.2f, 2.45f}, {-3.75f, 2.4f}}, 5);
    polyM(b, cgx, cgy, att, {{-3.95f, 2.35f}, {-3.05f, 2.4f}, {-3.15f, 2.62f}, {-3.85f, 2.55f}}, 6);
    polyM(b, cgx, cgy, att, {{2.4f, 0.55f}, {3.5f, 0.62f}, {3.35f, 1.05f}, {2.3f, 0.95f}}, 7);
    Pt a = xform(cgx, cgy, att, 1.7f, -0.15f);
    Pt c = xform(cgx, cgy, att, 1.7f, -0.72f);
    b.line(a.first, a.second, c.first, c.second, 9, 2.4f);
    Pt wheel = xform(cgx, cgy, att, 1.7f, -0.95f);
    b.ellipse(wheel.first, wheel.second, 0.22f * PPM, 0.22f * PPM, 9);
    b.ellipse(wheel.first, wheel.second, 0.08f * PPM, 0.08f * PPM, 10);
    Pt car = xform(cgx, cgy, att, -0.4f, 0.85f);
    b.rect(car.first - 8, car.second - 5, 16, 7, 7);
    b.outline(11, false);

    int x0 = b.w, y0 = b.h, x1 = -1, y1 = -1;
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++)
            if (b.get(x, y)) {
                x0 = std::min(x0, x);
                y0 = std::min(y0, y);
                x1 = std::max(x1, x);
                y1 = std::max(y1, y);
            }
    Bitmap cropped(std::max(1, x1 - x0 + 1), std::max(1, y1 - y0 + 1));
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) cropped.set(x - x0, y - y0, b.get(x, y));
    Hull s;
    s.img = gs::uploadMipped(vdp, cropped);
    s.ax = cgx - float(x0);
    s.ay = cgy - float(y0);
    s.ppm = PPM;
    return s;
}

Bitmap wheelArt(float ang) {
    const float cx = 40.f, cy = 40.f, R = 36.f;
    Bitmap b(80, 80);
    b.ellipse(cx, cy, R, R, 1);
    b.ellipse(cx, cy, R - 5.f, R - 5.f, 2);
    b.ellipse(cx, cy, R - 9.f, R - 9.f, 0);
    for (int i = 0; i < 6; i++) {
        float a = ang + float(i) * 1.0472f;
        b.line(cx + std::cos(a) * 7.f, cy + std::sin(a) * 7.f, cx + std::cos(a) * (R - 7.f),
               cy + std::sin(a) * (R - 7.f), 3, 2.2f);
    }
    b.ellipse(cx, cy, 6.f, 6.f, 4);
    b.ellipse(cx, cy, 2.4f, 2.4f, 5);
    b.outline(6, false);
    return b;
}

Bitmap millArt() {
    Bitmap b(70, 90);
    b.poly({{30, 8}, {12, 86}, {22, 86}, {34, 16}}, 1);
    b.poly({{40, 8}, {58, 86}, {48, 86}, {36, 16}}, 1);
    b.rect(18, 40, 34, 6, 2);
    b.rect(20, 62, 30, 5, 2);
    b.rect(28, 2, 14, 8, 3);
    return b;
}

Bitmap bedArt() {
    Bitmap b(48, 26);
    b.poly({{2, 16}, {44, 12}, {46, 18}, {4, 22}}, 1);
    b.rect(6, 6, 28, 8, 2);
    b.poly({{8, 6}, {16, 1}, {32, 3}, {34, 8}}, 3);
    b.rect(36, 10, 8, 8, 4);
    b.outline(5, false);
    return b;
}

Bitmap beamArt() {
    Bitmap b(44, 12);
    b.rect(0, 2, 44, 8, 1);
    b.rect(0, 2, 44, 3, 2);
    return b;
}

Bitmap cableArt() {
    Bitmap b(4, 16);
    b.rect(1, 0, 2, 16, 1);
    return b;
}

Bitmap waterArt() {
    Bitmap b(28, 20);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x, y);
            int c = 1;
            if ((h & 5) == 0) c = 2;
            if ((h % 17) == 0) c = 3;
            if (y < 2 && (h & 1)) c = 4;
            b.set(x, y, c);
        }
    return b;
}

Bitmap bankArt() {
    Bitmap b(100, 36);
    b.poly({{0, 34}, {14, 18}, {30, 24}, {52, 8}, {74, 20}, {100, 34}}, 1);
    b.rect(0, 30, 100, 6, 2);
    for (int i = 0; i < 4; i++) b.rect(12 + i * 22, 22, 3, 8, 3);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(64, 24);
    b.ellipse(16, 14, 12, 7, 1);
    b.ellipse(32, 12, 16, 8, 1);
    b.ellipse(48, 15, 11, 6, 2);
    return b;
}

Bitmap railArt() {
    Bitmap b(36, 10);
    b.rect(0, 3, 36, 4, 1);
    b.rect(0, 3, 36, 2, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 8, 8, 1);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785f;
        b.line(14 + std::cos(a) * 10, 14 + std::sin(a) * 10, 14 + std::cos(a) * 13, 14 + std::sin(a) * 13, 1, 1.6f);
    }
    return b;
}

Bitmap reedArt() {
    Bitmap b(22, 36);
    b.rect(10, 16, 3, 18, 3);
    b.poly({{11, 18}, {2, 14}, {11, 2}}, 1);
    b.poly({{12, 18}, {20, 12}, {12, 4}}, 2);
    return b;
}

Bitmap gullArt(int frame) {
    Bitmap b(20, 10);
    float lift = frame ? 1.f : 5.f;
    b.poly({{1, 7}, {9, 5}, {10, lift}, {11, 5}, {19, 7}, {11, 6}, {10, 7}, {9, 6}}, 1);
    return b;
}

Bitmap flagArt(int frame) {
    Bitmap b(36, 44);
    b.rect(6, 4, 2, 38, 3);
    float d = frame ? 8.f : 2.f;
    b.poly({{8, 6}, {30, 8 + d}, {28, 16 + d}, {8, 14}}, 1);
    b.poly({{10, 8}, {26, 10 + d}, {24, 14 + d}, {10, 12}}, 2);
    return b;
}

Bitmap poleArt() {
    Bitmap b(6, 36);
    b.rect(2, 0, 2, 36, 1);
    return b;
}

Bitmap bannerArt() {
    gs::TextStyle st{3, 1, 0, 0, 1};
    Bitmap word = gs::textBitmap("1 KM", st);
    Bitmap b(word.w + 16, word.h + 14);
    b.rect(0, 0, float(b.w), float(b.h), 2);
    b.rect(3, 3, float(b.w - 6), float(b.h - 6), 3);
    b.blit(word, 8, 7);
    b.outline(4, false);
    return b;
}

Bitmap shadeArt() {
    Bitmap b(28, 8);
    b.ellipse(14, 4, 12, 3, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 15, 8));

    setPal(vdp, PAL_SHIP,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(2, 3, 6), gs::rgb4(4, 10, 6), gs::rgb4(13, 14, 15), gs::rgb4(13, 3, 2),
            gs::rgb4(4, 1, 1), gs::rgb4(8, 8, 9), gs::rgb4(6, 12, 15), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2),
            gs::rgb4(1, 2, 3)});
    setPal(vdp, PAL_PADDLE, {0, gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(13, 10, 5), gs::rgb4(4, 3, 2),
                             gs::rgb4(2, 2, 2), gs::rgb4(14, 12, 8), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_LORRY, {0, gs::rgb4(8, 8, 7), gs::rgb4(5, 6, 8), gs::rgb4(12, 8, 3), gs::rgb4(3, 3, 3),
                            gs::rgb4(14, 12, 8), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_GANTRY, {0, gs::rgb4(8, 9, 11), gs::rgb4(5, 6, 8), gs::rgb4(12, 13, 14), gs::rgb4(3, 3, 4),
                             gs::rgb4(14, 11, 4), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 6, 12), gs::rgb4(3, 8, 13), gs::rgb4(1, 4, 9), gs::rgb4(10, 14, 15)});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(5, 7, 4), gs::rgb4(4, 5, 3), gs::rgb4(3, 6, 3)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 14, 15), gs::rgb4(15, 13, 5)});
    setPal(vdp, PAL_PIER, {0, gs::rgb4(3, 8, 3), gs::rgb4(4, 10, 4), gs::rgb4(6, 5, 2)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 8), gs::rgb4(11, 2, 2), gs::rgb4(15, 8, 3), gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(14, 14, 12), gs::rgb4(12, 3, 2), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(1, 2, 4)});

    loadFont(vdp, art);
    const float atts[5] = {0.28f, 0.14f, 0.f, -0.14f, -0.28f};
    for (int i = 0; i < 5; i++) art.hull[i] = drawHull(vdp, atts[i]);
    for (int i = 0; i < 4; i++) art.wheel[i] = gs::uploadMipped(vdp, wheelArt(float(i) * 0.26f));
    art.wheelRim = 74.f / 80.f;
    art.mill = gs::uploadMipped(vdp, millArt());
    art.millAx = 35.f;
    art.millAy = 6.f;
    art.millSpan = 80.f;
    art.bed = gs::uploadMipped(vdp, bedArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.cable = gs::uploadMipped(vdp, cableArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.bank = gs::uploadMipped(vdp, bankArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.rail = gs::uploadMipped(vdp, railArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(0));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(1));
    art.flag[0] = gs::uploadMipped(vdp, flagArt(0));
    art.flag[1] = gs::uploadMipped(vdp, flagArt(1));
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
}

}  // namespace fkilo
