#include "art.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace ggrass {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float PPM = 14.f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(2, 2, 1));
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// ang is nose-up radians. Bitmap y grows downward.
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

Ship finish(gs::VDP& vdp, Bitmap& b, float ax, float ay) {
    int x0 = b.w, y0 = b.h, x1 = -1, y1 = -1;
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++)
            if (b.get(x, y)) {
                x0 = std::min(x0, x);
                y0 = std::min(y0, y);
                x1 = std::max(x1, x);
                y1 = std::max(y1, y);
            }
    if (x1 < x0) {
        x0 = y0 = 0;
        x1 = y1 = 1;
    }
    Bitmap cropped(x1 - x0 + 1, y1 - y0 + 1);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) cropped.set(x - x0, y - y0, b.get(x, y));
    Ship s;
    s.img = gs::uploadMipped(vdp, cropped);
    s.ax = ax - float(x0);
    s.ay = ay - float(y0);
    s.ppm = PPM;
    return s;
}

// White sailplane, green wing stripe, yellow nose. Wheel contact is the anchor.
Ship drawShip(gs::VDP& vdp, float att) {
    const float cgx = 150.f, cgy = 120.f;
    Bitmap b(300, 240);
    polyM(b, cgx, cgy, att, {{-5.35f, 0.42f}, {2.15f, 0.55f}, {3.15f, 0.18f}, {2.05f, -0.18f}, {-5.25f, 0.02f}}, 1);
    polyM(b, cgx, cgy, att, {{-4.7f, 0.28f}, {1.85f, 0.36f}, {1.8f, 0.14f}, {-4.65f, 0.08f}}, 3);
    polyM(b, cgx, cgy, att, {{2.05f, 0.32f}, {4.15f, 0.1f}, {3.55f, -0.16f}, {1.95f, -0.02f}}, 5);
    polyM(b, cgx, cgy, att, {{1.05f, 0.32f}, {2.45f, 0.72f}, {2.85f, 0.28f}, {1.25f, 0.08f}}, 4);
    polyM(b, cgx, cgy, att, {{-1.35f, 0.95f}, {1.35f, 1.28f}, {1.55f, 0.78f}, {-1.45f, 0.5f}}, 2);
    polyM(b, cgx, cgy, att, {{-1.05f, 1.12f}, {1.2f, 1.32f}, {1.28f, 1.05f}, {-0.95f, 0.86f}}, 3);
    polyM(b, cgx, cgy, att, {{-5.25f, 0.28f}, {-4.55f, 0.38f}, {-4.42f, 2.45f}, {-5.15f, 2.25f}}, 1);
    polyM(b, cgx, cgy, att, {{-5.15f, 1.15f}, {-4.6f, 1.22f}, {-4.55f, 1.55f}, {-5.1f, 1.48f}}, 3);
    polyM(b, cgx, cgy, att, {{-5.7f, 2.22f}, {-4.05f, 2.4f}, {-4.12f, 2.02f}, {-5.62f, 1.86f}}, 2);
    polyM(b, cgx, cgy, att, {{-5.45f, 2.18f}, {-4.25f, 2.28f}, {-4.28f, 2.1f}, {-5.42f, 2.0f}}, 3);
    Pt a = xform(cgx, cgy, att, 0.15f, 0.15f);
    Pt c = xform(cgx, cgy, att, 0.15f, -0.55f);
    b.line(a.first, a.second, c.first, c.second, 8, 2.2f);
    Pt wheel = xform(cgx, cgy, att, 0.15f, -0.62f);
    b.ellipse(wheel.first, wheel.second, 6.2f, 6.2f, 6);
    b.ellipse(wheel.first, wheel.second, 2.4f, 2.4f, 7);
    b.outline(6, false);
    Pt contact = xform(cgx, cgy, att, 0.15f, -0.95f);
    return finish(vdp, b, contact.first, contact.second);
}

Bitmap grassArt(int stripe) {
    Bitmap b(40, 32);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x + stripe * 19, y);
            int blade = (x + (y / 3)) % 5;
            int c = stripe ? (blade < 2 ? 6 : 5) : (blade < 3 ? 5 : 6);
            if ((h % 9) == 0) c = 7;
            if (y > 22 && (h % 23) == 0) c = 8;
            if (y > 24 && (h % 29) == 0) c = 13;
            b.set(x, y, c);
        }
    return b;
}

Bitmap woodsArt() {
    Bitmap b(36, 28);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x, y + 3);
            b.set(x, y, (h % 5) == 0 ? 2 : 1);
        }
    return b;
}

Bitmap gravelArt() {
    Bitmap b(36, 28);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x + 4, y + 9);
            int c = 3;
            if ((h & 7) == 0) c = 4;
            if ((h % 19) == 0) c = 12;
            b.set(x, y, c);
        }
    return b;
}

Bitmap creekArt() {
    Bitmap b(36, 28);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            int c = (y % 5 == 1) ? 10 : 9;
            if ((hash2(x, y) % 17) == 0) c = 10;
            b.set(x, y, c);
        }
    return b;
}

Bitmap bankArt() {
    Bitmap b(32, 24);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) b.set(x, y, (hash2(x, y) & 3) == 0 ? 4 : 12);
    return b;
}

Bitmap solidArt(int c) {
    Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, c);
    return b;
}

Bitmap pineArt() {
    Bitmap b(34, 58);
    b.poly({{17, 2}, {3, 26}, {31, 26}}, 1);
    b.poly({{17, 14}, {6, 36}, {28, 36}}, 2);
    b.poly({{17, 24}, {8, 46}, {26, 46}}, 3);
    b.rect(15, 44, 4, 12, 4);
    return b;
}

Bitmap oakArt() {
    Bitmap b(40, 46);
    b.ellipse(20, 18, 16, 13, 5);
    b.ellipse(12, 20, 8, 8, 6);
    b.ellipse(28, 16, 7, 7, 2);
    b.rect(18, 28, 4, 16, 4);
    return b;
}

Bitmap tuftArt() {
    Bitmap b(22, 18);
    b.line(4, 16, 6, 2, 2, 1.6f);
    b.line(8, 16, 7, 3, 3, 1.6f);
    b.line(12, 16, 14, 4, 2, 1.5f);
    b.line(16, 16, 15, 6, 3, 1.4f);
    b.line(10, 16, 11, 1, 6, 1.3f);
    return b;
}

Bitmap reedArt() {
    Bitmap b(18, 42);
    b.line(8, 40, 5, 4, 2, 1.7f);
    b.line(8, 40, 13, 8, 3, 1.5f);
    b.line(9, 38, 3, 14, 1, 1.4f);
    b.ellipse(5, 5, 2.2f, 2.2f, 6);
    return b;
}

Bitmap barnArt() {
    Bitmap b(72, 58);
    b.poly({{4, 34}, {36, 8}, {68, 34}}, 1);
    b.poly({{14, 18}, {36, 10}, {58, 22}, {20, 30}}, 2);
    b.rect(12, 32, 48, 22, 3);
    b.rect(30, 40, 12, 14, 9);
    b.rect(18, 38, 8, 7, 4);
    b.rect(46, 38, 8, 7, 4);
    b.rect(34, 14, 4, 10, 9);
    return b;
}

Bitmap fenceArt() {
    Bitmap b(48, 36);
    b.rect(6, 10, 3, 24, 6);
    b.rect(39, 10, 3, 24, 6);
    b.rect(6, 14, 36, 3, 7);
    b.rect(6, 22, 36, 3, 7);
    b.rect(22, 12, 3, 22, 6);
    return b;
}

Bitmap signArt() {
    gs::TextStyle st{4, 1, 0, 0, 1};
    Bitmap word = gs::textBitmap("GRASS", st);
    Bitmap b(word.w + 16, word.h + 14);
    b.rect(0, 0, float(b.w), float(b.h), 6);
    b.rect(3, 3, float(b.w - 6), float(b.h - 6), 3);
    b.blit(word, 8, 7);
    return b;
}

Bitmap sockArt(int frame) {
    Bitmap b(44, 52);
    b.rect(8, 14, 3, 34, 6);
    b.ellipse(9, 12, 3.2f, 3.2f, 5);
    float droop = 4.f + float(frame) * 5.f;
    b.poly({{10, 16}, {40, 8 + droop}, {38, 16 + droop}, {10, 24}}, 4);
    b.poly({{12, 18}, {34, 12 + droop}, {32, 16 + droop}, {12, 22}}, 5);
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
    b.ellipse(18, 18, 8, 8, 3);
    b.ellipse(18, 18, 5, 5, 4);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785f;
        b.line(18 + std::cos(a) * 11, 18 + std::sin(a) * 11, 18 + std::cos(a) * 16, 18 + std::sin(a) * 16, 4, 2.f);
    }
    return b;
}

Bitmap hillArt() {
    Bitmap b(96, 40);
    b.poly({{0, 38}, {16, 22}, {34, 28}, {52, 8}, {74, 20}, {96, 38}}, 1);
    b.poly({{28, 30}, {52, 12}, {66, 24}, {40, 34}}, 2);
    b.rect(0, 34, 96, 6, 3);
    return b;
}

Bitmap dustArt() {
    Bitmap b(24, 14);
    b.ellipse(12, 8, 10, 5, 1);
    b.ellipse(8, 7, 4, 3, 2);
    return b;
}

Bitmap shadeArt() {
    Bitmap b(40, 12);
    b.ellipse(20, 6, 16, 4, 1);
    return b;
}

Bitmap birdArt(int flap) {
    Bitmap b(28, 14);
    float y = flap ? 4.f : 8.f;
    b.poly({{2, y}, {12, 8}, {2, y + 3}}, 2);
    b.poly({{26, y}, {16, 8}, {26, y + 3}}, 2);
    b.ellipse(14, 8, 3.2f, 2.4f, 1);
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
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 6));

    setPal(vdp, PAL_SHIP, {0, gs::rgb4(15, 15, 14), gs::rgb4(11, 14, 12), gs::rgb4(3, 12, 5), gs::rgb4(5, 12, 14),
                           gs::rgb4(15, 13, 3), gs::rgb4(2, 2, 3), gs::rgb4(15, 15, 15), gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_RIVAL, {0, gs::rgb4(13, 3, 3), gs::rgb4(14, 12, 11), gs::rgb4(8, 2, 2), gs::rgb4(6, 10, 13),
                            gs::rgb4(15, 11, 4), gs::rgb4(2, 1, 1), gs::rgb4(15, 8, 7), gs::rgb4(5, 4, 4)});
    setPal(vdp, PAL_FIELD,
           {0, gs::rgb4(2, 4, 2), gs::rgb4(3, 6, 3), gs::rgb4(11, 10, 7), gs::rgb4(13, 12, 8), gs::rgb4(4, 12, 3),
            gs::rgb4(7, 15, 5), gs::rgb4(2, 8, 2), gs::rgb4(15, 14, 3), gs::rgb4(3, 7, 12), gs::rgb4(7, 12, 15),
            gs::rgb4(15, 15, 15), gs::rgb4(8, 6, 3), gs::rgb4(15, 8, 10)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 6, 2), gs::rgb4(3, 9, 3), gs::rgb4(6, 12, 4), gs::rgb4(7, 5, 2),
                           gs::rgb4(3, 8, 3), gs::rgb4(5, 11, 4)});
    setPal(vdp, PAL_PROP, {0, gs::rgb4(13, 3, 2), gs::rgb4(9, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(15, 8, 2),
                           gs::rgb4(13, 5, 1), gs::rgb4(6, 4, 2), gs::rgb4(9, 7, 4), gs::rgb4(14, 12, 6),
                           gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 15, 15), gs::rgb4(15, 14, 6), gs::rgb4(15, 11, 3)});
    setPal(vdp, PAL_FAR, {0, gs::rgb4(5, 8, 4), gs::rgb4(7, 11, 6), gs::rgb4(3, 6, 3)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(3, 5, 2), gs::rgb4(6, 8, 4)});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(3, 3, 4), gs::rgb4(7, 7, 8)});

    loadFont(vdp, art);
    const float atts[5] = {0.36f, 0.18f, 0.f, -0.18f, -0.36f};
    for (int i = 0; i < 5; i++) art.ship[i] = drawShip(vdp, atts[i]);
    art.grass[0] = gs::uploadMipped(vdp, grassArt(0));
    art.grass[1] = gs::uploadMipped(vdp, grassArt(1));
    art.woods = gs::uploadMipped(vdp, woodsArt());
    art.gravel = gs::uploadMipped(vdp, gravelArt());
    art.creek = gs::uploadMipped(vdp, creekArt());
    art.bank = gs::uploadMipped(vdp, bankArt());
    art.white = gs::uploadMipped(vdp, solidArt(11));
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.oak = gs::uploadMipped(vdp, oakArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.barn = gs::uploadMipped(vdp, barnArt());
    art.fence = gs::uploadMipped(vdp, fenceArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    for (int i = 0; i < 3; i++) art.sock[i] = gs::uploadMipped(vdp, sockArt(i));
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(0));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(1));
}

}  // namespace ggrass
