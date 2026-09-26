#include "game/art.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace gkilo {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float PPM = 11.f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(2, 1, 1));
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
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

Ship drawShip(gs::VDP& vdp, float att) {
    const float cgx = 130.f, cgy = 108.f;
    Bitmap b(260, 220);
    // Tailplane, fin, fuselage, wing root, canopy. Nose to the right. +y is up.
    polyM(b, cgx, cgy, att, {{-3.72f, 1.58f}, {-2.62f, 1.64f}, {-2.68f, 1.88f}, {-3.68f, 1.82f}}, 1);
    polyM(b, cgx, cgy, att, {{-3.55f, 0.18f}, {-2.82f, 0.24f}, {-2.92f, 1.78f}, {-3.48f, 1.70f}}, 1);
    polyM(b, cgx, cgy, att, {{-3.42f, 1.15f}, {-3.02f, 1.22f}, {-3.08f, 1.62f}, {-3.46f, 1.55f}}, 3);
    polyM(b, cgx, cgy, att, {{-3.65f, -0.22f}, {2.55f, -0.16f}, {3.20f, 0.02f}, {2.35f, 0.32f}, {-3.45f, 0.26f}}, 1);
    polyM(b, cgx, cgy, att, {{-3.55f, -0.04f}, {2.70f, 0.02f}, {2.55f, 0.12f}, {-3.50f, 0.06f}}, 3);
    polyM(b, cgx, cgy, att, {{-0.70f, 0.18f}, {0.95f, 0.26f}, {0.72f, 0.58f}, {-0.42f, 0.50f}}, 2);
    polyM(b, cgx, cgy, att, {{0.55f, 0.26f}, {1.85f, 0.32f}, {2.15f, 0.58f}, {1.55f, 0.82f}, {0.70f, 0.60f}}, 7);
    polyM(b, cgx, cgy, att, {{1.15f, 0.48f}, {1.85f, 0.52f}, {1.70f, 0.74f}, {1.10f, 0.66f}}, 9);
    polyM(b, cgx, cgy, att, {{2.45f, -0.08f}, {3.22f, 0.02f}, {3.05f, 0.16f}, {2.30f, 0.10f}}, 3);
    Pt head = xform(cgx, cgy, att, 1.35f, 0.48f);
    b.ellipse(head.first, head.second, 3.4f, 3.4f, 6);
    Pt a = xform(cgx, cgy, att, 0.45f, -0.18f);
    Pt c = xform(cgx, cgy, att, 0.45f, -0.70f);
    b.line(a.first, a.second, c.first, c.second, 4, 2.2f);
    Pt wheel = xform(cgx, cgy, att, 0.45f, -0.92f);
    b.ellipse(wheel.first, wheel.second, 0.26f * PPM, 0.26f * PPM, 5);
    b.ellipse(wheel.first, wheel.second, 0.10f * PPM, 0.10f * PPM, 8);
    b.outline(8, false);

    int x0 = b.w, y0 = b.h, x1 = -1, y1 = -1;
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++)
            if (b.get(x, y)) {
                x0 = std::min(x0, x);
                y0 = std::min(y0, y);
                x1 = std::max(x1, x);
                y1 = std::max(y1, y);
            }
    Bitmap cropped(x1 - x0 + 1, y1 - y0 + 1);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) cropped.set(x - x0, y - y0, b.get(x, y));
    Ship s;
    s.img = gs::uploadMipped(vdp, cropped);
    s.ax = cgx - float(x0);
    s.ay = cgy - float(y0);
    s.ppm = PPM;
    return s;
}

Bitmap wheelArt(float ang) {
    const float cx = 48.f, cy = 48.f, R = 44.f;
    Bitmap b(96, 96);
    b.ellipse(cx, cy, R, R, 1);
    b.ellipse(cx, cy, R - 6.f, R - 6.f, 2);
    b.ellipse(cx, cy, R - 10.f, R - 10.f, 0);
    for (int i = 0; i < 8; i++) {
        float a = ang + float(i) * 0.785398f;
        float ca = std::cos(a), sa = std::sin(a);
        b.line(cx + ca * 8.f, cy + sa * 8.f, cx + ca * (R - 8.f), cy + sa * (R - 8.f), 3, 2.4f);
    }
    b.ellipse(cx, cy, 7.f, 7.f, 4);
    b.ellipse(cx, cy, 3.f, 3.f, 5);
    for (int i = 0; i < 4; i++) {
        float a = ang + 0.4f + float(i) * 1.5708f;
        b.ellipse(cx + std::cos(a) * 5.2f, cy + std::sin(a) * 5.2f, 1.3f, 1.3f, 6);
    }
    b.outline(7, false);
    return b;
}

Bitmap towerArt() {
    Bitmap b(100, 120);
    b.poly({{46, 8}, {16, 114}, {28, 114}, {52, 18}}, 1);
    b.poly({{54, 8}, {84, 114}, {72, 114}, {48, 18}}, 1);
    b.poly({{48, 10}, {18, 112}, {24, 112}, {50, 16}}, 2);
    b.poly({{52, 10}, {82, 112}, {76, 112}, {50, 16}}, 2);
    b.poly({{30, 62}, {70, 70}, {68, 76}, {28, 68}}, 3);
    b.poly({{32, 88}, {68, 94}, {66, 100}, {30, 94}}, 3);
    b.rect(44, 4, 12, 8, 4);
    return b;
}

Bitmap bedArt() {
    Bitmap b(54, 28);
    b.poly({{4, 16}, {48, 12}, {50, 18}, {6, 22}}, 1);
    b.rect(8, 8, 34, 8, 2);
    b.poly({{8, 8}, {18, 2}, {40, 4}, {42, 8}}, 3);
    b.rect(44, 14, 6, 8, 4);
    b.outline(5, false);
    return b;
}

Bitmap beamArt() {
    Bitmap b(48, 14);
    b.rect(1, 3, 46, 8, 1);
    b.rect(1, 3, 46, 3, 2);
    b.outline(3, false);
    return b;
}

Bitmap cableArt() {
    Bitmap b(6, 16);
    b.rect(2, 0, 2, 16, 1);
    return b;
}

Bitmap grassArt() {
    Bitmap b(32, 24);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x, y);
            int c = 1;
            if ((h & 7) == 0) c = 2;
            if ((h % 19) == 0) c = 3;
            if (y < 2) c = 4;
            if (y == 2 && (h & 1)) c = 5;
            b.set(x, y, c);
        }
    return b;
}

Bitmap hillArt() {
    Bitmap b(120, 48);
    b.poly({{0, 46}, {18, 28}, {34, 34}, {58, 12}, {82, 26}, {104, 16}, {120, 46}}, 1);
    b.poly({{40, 30}, {58, 16}, {74, 28}, {52, 36}}, 2);
    b.rect(0, 42, 120, 6, 3);
    for (int i = 0; i < 5; i++) {
        float x = 16.f + float(i) * 22.f;
        b.poly({{x, 40}, {x + 4, 30}, {x + 8, 40}}, 4);
    }
    return b;
}

Bitmap cloudArt() {
    Bitmap b(78, 30);
    b.ellipse(18, 16, 14, 8, 1);
    b.ellipse(38, 13, 20, 11, 1);
    b.ellipse(58, 16, 14, 8, 1);
    b.ellipse(36, 17, 16, 7, 2);
    return b;
}

Bitmap deckArt() {
    Bitmap b(64, 28);
    b.ellipse(16, 16, 16, 9, 1);
    b.ellipse(36, 12, 22, 11, 1);
    b.ellipse(52, 16, 14, 8, 2);
    b.rect(0, 18, 64, 10, 1);
    return b;
}

Bitmap sunArt() {
    Bitmap b(34, 34);
    b.ellipse(17, 17, 8, 8, 1);
    b.ellipse(17, 17, 5, 5, 2);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785f;
        b.line(17 + std::cos(a) * 10, 17 + std::sin(a) * 10, 17 + std::cos(a) * 15, 17 + std::sin(a) * 15, 1, 2.f);
    }
    return b;
}

Bitmap treeArt() {
    Bitmap b(34, 46);
    b.rect(15, 26, 4, 18, 4);
    b.poly({{17, 30}, {4, 28}, {17, 10}}, 1);
    b.poly({{17, 30}, {30, 28}, {17, 10}}, 2);
    b.poly({{17, 22}, {8, 20}, {17, 4}, {26, 20}}, 3);
    return b;
}

Bitmap birdArt(int frame) {
    Bitmap b(22, 12);
    float lift = frame ? 2.f : 6.f;
    b.poly({{1, 8}, {10, 6}, {11, lift}, {12, 6}, {21, 8}, {12, 7}, {11, 8}, {10, 7}}, 1);
    return b;
}

Bitmap sockArt(int frame) {
    Bitmap b(40, 52);
    b.rect(18, 18, 3, 32, 4);
    b.ellipse(19, 16, 3, 3, 3);
    float droop = 4.f + float(frame) * 6.f;
    b.poly({{18, 18}, {36, 10 + droop}, {34, 18 + droop}, {18, 24}}, 1);
    b.poly({{20, 19}, {32, 13 + droop}, {30, 17 + droop}, {20, 23}}, 2);
    return b;
}

Bitmap poleArt() {
    Bitmap b(8, 40);
    b.rect(3, 0, 2, 40, 1);
    b.rect(1, 6, 6, 5, 2);
    return b;
}

Bitmap bannerArt() {
    gs::TextStyle st{4, 1, 0, 0, 1};
    Bitmap word = gs::textBitmap("1 KM", st);
    Bitmap b(word.w + 18, word.h + 16);
    b.rect(0, 0, float(b.w), float(b.h), 2);
    b.rect(3, 3, float(b.w - 6), float(b.h - 6), 3);
    b.blit(word, 9, 8);
    b.outline(4, false);
    return b;
}

Bitmap shadeArt() {
    Bitmap b(36, 12);
    b.ellipse(18, 6, 16, 4, 1);
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
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 7));

    setPal(vdp, PAL_SHIP, {0, gs::rgb4(15, 14, 12), gs::rgb4(6, 8, 11), gs::rgb4(13, 3, 2), gs::rgb4(8, 5, 2),
                           gs::rgb4(2, 2, 3), gs::rgb4(14, 10, 7), gs::rgb4(4, 9, 13), gs::rgb4(1, 1, 2),
                           gs::rgb4(10, 14, 15)});
    setPal(vdp, PAL_MILL, {0, gs::rgb4(12, 8, 3), gs::rgb4(8, 5, 2), gs::rgb4(14, 11, 6), gs::rgb4(5, 4, 3),
                           gs::rgb4(3, 3, 3), gs::rgb4(15, 13, 8), gs::rgb4(2, 1, 1), gs::rgb4(6, 3, 1)});
    setPal(vdp, PAL_CART, {0, gs::rgb4(9, 7, 6), gs::rgb4(6, 4, 3), gs::rgb4(12, 8, 4), gs::rgb4(4, 3, 3),
                           gs::rgb4(2, 2, 2), gs::rgb4(14, 10, 6), gs::rgb4(3, 2, 1), gs::rgb4(5, 3, 2)});
    setPal(vdp, PAL_HANG, {0, gs::rgb4(7, 8, 10), gs::rgb4(4, 5, 7), gs::rgb4(11, 12, 14), gs::rgb4(3, 3, 4),
                           gs::rgb4(14, 12, 6), gs::rgb4(8, 7, 4), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(4, 10, 3), gs::rgb4(5, 12, 4), gs::rgb4(3, 8, 2), gs::rgb4(8, 13, 5),
                            gs::rgb4(6, 11, 4), gs::rgb4(2, 6, 2)});
    setPal(vdp, PAL_HILL, {0, gs::rgb4(6, 8, 10), gs::rgb4(5, 7, 8), gs::rgb4(4, 6, 5), gs::rgb4(3, 6, 4)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), gs::rgb4(15, 13, 6), gs::rgb4(15, 10, 4)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 7, 3), gs::rgb4(3, 9, 4), gs::rgb4(4, 11, 5), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 10), gs::rgb4(12, 2, 2), gs::rgb4(15, 6, 3), gs::rgb4(3, 1, 1),
                             gs::rgb4(8, 5, 2)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(14, 14, 13), gs::rgb4(13, 3, 2), gs::rgb4(4, 3, 2), gs::rgb4(9, 6, 3)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(2, 3, 2)});

    loadFont(vdp, art);
    const float atts[5] = {0.40f, 0.20f, 0.f, -0.20f, -0.40f};
    for (int i = 0; i < 5; i++) art.ship[i] = drawShip(vdp, atts[i]);
    for (int i = 0; i < 4; i++) art.wheel[i] = gs::uploadMipped(vdp, wheelArt(float(i) * 0.19635f));
    art.wheelRim = 90.f / 96.f;
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.towerAx = 50.f;
    art.towerAy = 8.f;
    art.towerSpan = 106.f;
    art.bed = gs::uploadMipped(vdp, bedArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.cable = gs::uploadMipped(vdp, cableArt());
    art.grass = gs::uploadMipped(vdp, grassArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.deck = gs::uploadMipped(vdp, deckArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(0));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(1));
    art.sock[0] = gs::uploadMipped(vdp, sockArt(0));
    art.sock[1] = gs::uploadMipped(vdp, sockArt(1));
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
}

}  // namespace gkilo
