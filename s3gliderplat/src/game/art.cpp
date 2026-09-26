#include "game/art.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace gliderplat {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float PPM = 12.f;

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

// ang is nose-up radians. +y in meters is up.
void polyM(Bitmap& b, float ox, float oy, float ang, std::initializer_list<Pt> meters, int c) {
    const float ca = std::cos(-ang), sa = std::sin(-ang);
    std::vector<Pt> p;
    p.reserve(meters.size());
    for (const Pt& m : meters) {
        float dx = m.first * PPM, dy = -m.second * PPM;
        p.push_back({ox + dx * ca - dy * sa, oy + dx * sa + dy * ca});
    }
    b.poly(p, c);
}

Pt xform(float ox, float oy, float ang, float mx, float my) {
    const float ca = std::cos(-ang), sa = std::sin(-ang);
    float dx = mx * PPM, dy = -my * PPM;
    return {ox + dx * ca - dy * sa, oy + dx * sa + dy * ca};
}

Wing drawWing(gs::VDP& vdp, float att) {
    const float ox = 156.f, oy = 168.f;
    Bitmap b(320, 250);
    // Tail first, then the white boom, the green wing, the red nose.
    polyM(b, ox, oy, att, {{-5.05f, 1.18f}, {-3.85f, 1.34f}, {-3.9f, 1.08f}, {-5.0f, 0.98f}}, 2);
    polyM(b, ox, oy, att, {{-4.85f, 0.42f}, {-4.22f, 0.5f}, {-4.08f, 1.58f}, {-4.72f, 1.42f}}, 1);
    polyM(b, ox, oy, att, {{-4.62f, 1.12f}, {-4.28f, 1.18f}, {-4.2f, 1.5f}, {-4.55f, 1.4f}}, 3);
    polyM(b, ox, oy, att, {{-4.8f, 0.3f}, {-4.7f, 0.58f}, {-1.15f, 0.7f}, {-1.05f, 0.28f}}, 1);
    polyM(b, ox, oy, att, {{-1.2f, 0.28f}, {-1.05f, 0.78f}, {1.85f, 0.9f}, {2.55f, 0.52f}, {2.35f, 0.26f}, {0.15f, 0.24f}}, 1);
    polyM(b, ox, oy, att, {{2.15f, 0.28f}, {2.35f, 0.7f}, {3.72f, 0.44f}, {3.45f, 0.26f}}, 3);
    polyM(b, ox, oy, att, {{-1.2f, 0.92f}, {-0.15f, 1.32f}, {0.95f, 1.2f}, {0.72f, 0.88f}, {-0.95f, 0.74f}}, 4);
    polyM(b, ox, oy, att, {{-0.35f, 1.16f}, {0.55f, 1.12f}, {0.48f, 0.98f}, {-0.28f, 1.0f}}, 3);
    polyM(b, ox, oy, att, {{0.45f, 0.74f}, {1.55f, 1.02f}, {2.2f, 0.66f}, {1.05f, 0.52f}}, 5);
    Pt a = xform(ox, oy, att, -0.85f, 0.95f);
    Pt c = xform(ox, oy, att, 0.35f, 0.32f);
    b.line(a.first, a.second, c.first, c.second, 8, 2.2f);
    a = xform(ox, oy, att, 0.15f, 0.32f);
    c = xform(ox, oy, att, 0.05f, 0.16f);
    b.line(a.first, a.second, c.first, c.second, 8, 2.4f);
    Pt head = xform(ox, oy, att, 1.35f, 0.62f);
    b.ellipse(head.first, head.second, 4.2f, 4.2f, 6);
    Pt wheel = xform(ox, oy, att, 0.02f, 0.16f);
    b.ellipse(wheel.first, wheel.second, 5.2f, 5.2f, 7);
    b.ellipse(wheel.first, wheel.second, 2.1f, 2.1f, 8);
    Pt skid = xform(ox, oy, att, -3.6f, 0.22f);
    Pt skid2 = xform(ox, oy, att, -4.5f, 0.18f);
    b.line(skid.first, skid.second, skid2.first, skid2.second, 8, 2.f);
    b.outline(7, false);

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
    if (x1 >= x0) {
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) cropped.set(x - x0, y - y0, b.get(x, y));
    }
    Wing w;
    w.img = gs::uploadMipped(vdp, cropped);
    w.ax = ox - float(x0);
    w.ay = oy - float(y0);
    w.ppm = PPM;
    return w;
}

Bitmap plankArt() {
    Bitmap b(48, 16);
    b.rect(0, 0, 48, 16, 1);
    for (int x = 0; x < 48; x++) {
        b.set(x, 0, 2);
        b.set(x, 15, 4);
        if (x % 12 == 0) {
            for (int y = 0; y < 16; y++) b.set(x, y, 3);
        }
    }
    for (int i = 0; i < 8; i++) b.set(int(hash2(i, 3) % 48), 6 + int(hash2(i, 1) % 6), 2);
    return b;
}

Bitmap stripeArt() {
    Bitmap b(32, 16);
    for (int x = 0; x < 32; x++) {
        int c = ((x / 4) & 1) ? 1 : 2;
        for (int y = 0; y < 16; y++) b.set(x, y, c);
    }
    return b;
}

Bitmap trestleArt() {
    Bitmap b(28, 72);
    b.rect(4, 4, 4, 66, 1);
    b.rect(20, 4, 4, 66, 1);
    b.rect(5, 6, 2, 62, 2);
    b.rect(21, 6, 2, 62, 2);
    b.line(6, 8, 22, 64, 3, 2.f);
    b.line(22, 8, 6, 64, 3, 2.f);
    b.rect(2, 2, 24, 5, 4);
    return b;
}

Bitmap rockArt() {
    Bitmap b(40, 48);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x, y);
            int c = 1;
            if ((h & 7) == 0) c = 2;
            if ((h % 11) == 0) c = 3;
            if (y > 36 && (h & 3) == 0) c = 4;
            b.set(x, y, c);
        }
    b.rect(0, 0, 40, 3, 3);
    return b;
}

Bitmap grassArt() {
    Bitmap b(40, 32);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x + 9, y + 4);
            int c = 1 + int(h % 3);
            if ((h % 29) == 0) c = 4;
            if (y % 8 == 0 && (h & 3) == 0) c = 5;
            b.set(x, y, c);
        }
    return b;
}

Bitmap pineArt() {
    Bitmap b(36, 52);
    b.poly({{18, 2}, {33, 28}, {3, 28}}, 1);
    b.poly({{18, 12}, {31, 36}, {5, 36}}, 2);
    b.poly({{18, 22}, {29, 44}, {7, 44}}, 3);
    b.rect(16, 40, 4, 12, 4);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(72, 28);
    b.ellipse(18, 16, 14, 8, 1);
    b.ellipse(36, 12, 18, 10, 1);
    b.ellipse(54, 16, 14, 8, 1);
    b.ellipse(34, 16, 16, 7, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(36, 36);
    b.ellipse(18, 18, 8, 8, 1);
    b.ellipse(18, 18, 5, 5, 2);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785f;
        b.line(18 + std::cos(a) * 11, 18 + std::sin(a) * 11, 18 + std::cos(a) * 16, 18 + std::sin(a) * 16, 1, 2.f);
    }
    return b;
}

Bitmap ridgeArt() {
    Bitmap b(110, 46);
    b.poly({{0, 44}, {16, 26}, {34, 32}, {52, 8}, {74, 22}, {92, 12}, {110, 44}}, 1);
    b.poly({{48, 14}, {58, 8}, {70, 18}, {54, 22}}, 3);
    b.rect(0, 40, 110, 6, 2);
    return b;
}

Bitmap sockArt(int frame) {
    Bitmap b(44, 36);
    b.rect(6, 4, 3, 30, 3);
    b.ellipse(7, 4, 3, 3, 4);
    float droop = 4.f + float(frame) * 4.f;
    b.poly({{8, 8}, {40, 6 + droop}, {38, 14 + droop}, {8, 16}}, 1);
    b.poly({{10, 10}, {34, 9 + droop}, {32, 13 + droop}, {10, 14}}, 2);
    return b;
}

Bitmap cabinArt() {
    Bitmap b(48, 40);
    b.rect(6, 16, 36, 22, 1);
    b.poly({{4, 18}, {24, 4}, {44, 18}}, 2);
    b.rect(20, 24, 8, 14, 3);
    b.rect(10, 22, 7, 6, 4);
    b.rect(31, 22, 7, 6, 4);
    b.rect(8, 36, 32, 3, 5);
    return b;
}

Bitmap signArt() {
    gs::TextStyle st{4, 1, 0, 0, 1};
    Bitmap word = gs::textBitmap("END", st);
    Bitmap b(word.w + 16, word.h + 18);
    b.rect(0, 0, float(b.w), float(b.h), 3);
    b.rect(3, 3, float(b.w - 6), float(b.h - 6), 2);
    b.blit(word, 8, 6);
    b.rect(float(b.w) * 0.5f - 2, float(b.h) - 3, 4, 3, 3);
    return b;
}

Bitmap lampArt() {
    Bitmap b(16, 28);
    b.rect(7, 8, 2, 20, 3);
    b.ellipse(8, 7, 5, 5, 1);
    b.ellipse(8, 7, 2, 2, 2);
    return b;
}

Bitmap chevArt() {
    Bitmap b(22, 14);
    b.poly({{2, 11}, {11, 2}, {20, 11}, {15, 11}, {11, 6}, {7, 11}}, 1);
    return b;
}

Bitmap gullArt(int frame) {
    Bitmap b(32, 16);
    float dip = frame ? 4.f : 0.f;
    b.poly({{2, 8}, {14, 6 + dip}, {16, 8}, {14, 9}}, 1);
    b.poly({{30, 8}, {18, 6 + dip}, {16, 8}, {18, 9}}, 1);
    b.ellipse(16, 8, 2, 2, 2);
    return b;
}

Bitmap dustArt() {
    Bitmap b(24, 14);
    b.ellipse(12, 8, 10, 4, 1);
    b.ellipse(8, 7, 4, 3, 2);
    return b;
}

Bitmap shadeArt() {
    Bitmap b(40, 12);
    b.ellipse(20, 6, 16, 4, 1);
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
    vdp.setColor(PAL_HUD * 16 + 2, gs::rgb4(8, 8, 9));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 8));

    setPal(vdp, PAL_SHIP, {0, gs::rgb4(15, 14, 12), gs::rgb4(15, 15, 15), gs::rgb4(13, 2, 2), gs::rgb4(2, 8, 4),
                           gs::rgb4(6, 12, 14), gs::rgb4(12, 8, 5), gs::rgb4(2, 2, 2), gs::rgb4(8, 5, 2),
                           gs::rgb4(14, 11, 3)});
    setPal(vdp, PAL_TIMBER, {0, gs::rgb4(10, 6, 3), gs::rgb4(13, 9, 5), gs::rgb4(6, 3, 2), gs::rgb4(4, 2, 1),
                             gs::rgb4(12, 8, 4)});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(8, 7, 6), gs::rgb4(6, 5, 5), gs::rgb4(11, 10, 9), gs::rgb4(5, 7, 4)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(3, 9, 3), gs::rgb4(2, 7, 2), gs::rgb4(5, 11, 4), gs::rgb4(8, 12, 5),
                            gs::rgb4(14, 13, 6)});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(1, 6, 2), gs::rgb4(2, 8, 3), gs::rgb4(4, 10, 4), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 11),
                          gs::rgb4(8, 8, 9)});
    setPal(vdp, PAL_END, {0, gs::rgb4(15, 12, 2), gs::rgb4(2, 2, 1), gs::rgb4(13, 3, 2), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_FAR, {0, gs::rgb4(7, 8, 11), gs::rgb4(5, 6, 9), gs::rgb4(13, 14, 15)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(14, 6, 2), gs::rgb4(15, 12, 4), gs::rgb4(5, 4, 3), gs::rgb4(9, 7, 4)});
    setPal(vdp, PAL_HOUSE, {0, gs::rgb4(12, 10, 7), gs::rgb4(8, 3, 2), gs::rgb4(4, 3, 2), gs::rgb4(8, 13, 14),
                            gs::rgb4(7, 5, 3)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(12, 11, 8), gs::rgb4(9, 8, 6)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(3, 2, 1), gs::rgb4(14, 12, 8), gs::rgb4(8, 4, 2), gs::rgb4(15, 12, 3)});

    loadFont(vdp, art);
    const float atts[5] = {0.36f, 0.16f, 0.f, -0.16f, -0.36f};
    for (int i = 0; i < 5; i++) art.wing[i] = drawWing(vdp, atts[i]);
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.trestle = gs::uploadMipped(vdp, trestleArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.grass = gs::uploadMipped(vdp, grassArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.ridge = gs::uploadMipped(vdp, ridgeArt());
    for (int i = 0; i < 3; i++) art.sock[i] = gs::uploadMipped(vdp, sockArt(i));
    art.cabin = gs::uploadMipped(vdp, cabinArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.chev = gs::uploadMipped(vdp, chevArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(0));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(1));
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
}

}  // namespace gliderplat
