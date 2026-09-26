#include "game/art.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace gmark {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float PPM = 13.f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 2, 2));
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 2246822519u + uint32_t(y) * 3266489917u;
    h = (h ^ (h >> 13)) * 16777619u;
    return h ^ (h >> 16);
}

// ang is nose-up radians. +x is the nose, +y is up.
void stamp(Bitmap& b, float cgx, float cgy, float ang, std::initializer_list<Pt> meters, int c) {
    const float ca = std::cos(ang), sa = std::sin(ang);
    std::vector<Pt> p;
    p.reserve(meters.size());
    for (const Pt& m : meters) {
        float xw = m.first * ca - m.second * sa;
        float yw = m.first * sa + m.second * ca;
        p.push_back({cgx + xw * PPM, cgy - yw * PPM});
    }
    b.poly(p, c);
}

Pt xform(float cgx, float cgy, float ang, float mx, float my) {
    const float ca = std::cos(ang), sa = std::sin(ang);
    float xw = mx * ca - my * sa;
    float yw = mx * sa + my * ca;
    return {cgx + xw * PPM, cgy - yw * PPM};
}

// T-tail sailplane, nose to the right. Wheel centre sits 0.72 m under the CG.
Ship drawShip(gs::VDP& vdp, float att) {
    const float cgx = 110.f, cgy = 78.f;
    Bitmap b(220, 160);
    stamp(b, cgx, cgy, att, {{-3.55f, 1.18f}, {-2.35f, 1.32f}, {-2.28f, 1.02f}, {-3.48f, 0.92f}}, 1);
    stamp(b, cgx, cgy, att, {{-3.42f, 1.16f}, {-2.55f, 1.22f}, {-2.5f, 1.08f}, {-3.38f, 1.02f}}, 2);
    stamp(b, cgx, cgy, att, {{-3.42f, 0.02f}, {-2.72f, 0.1f}, {-2.62f, 1.22f}, {-3.28f, 1.16f}}, 1);
    stamp(b, cgx, cgy, att, {{-3.22f, 0.08f}, {-2.92f, 0.12f}, {-2.84f, 1.05f}, {-3.16f, 1.0f}}, 2);
    stamp(b, cgx, cgy, att, {{-3.62f, 0.06f}, {2.15f, 0.16f}, {2.05f, -0.16f}, {-3.55f, -0.2f}}, 1);
    stamp(b, cgx, cgy, att, {{-3.3f, -0.02f}, {1.7f, 0.04f}, {1.62f, -0.1f}, {-3.28f, -0.14f}}, 2);
    stamp(b, cgx, cgy, att, {{1.85f, 0.16f}, {3.55f, 0.05f}, {3.4f, -0.2f}, {1.7f, -0.12f}}, 3);
    stamp(b, cgx, cgy, att, {{0.15f, 0.12f}, {1.35f, 0.46f}, {1.85f, 0.18f}, {0.35f, 0.0f}}, 4);
    stamp(b, cgx, cgy, att, {{0.45f, 0.16f}, {1.15f, 0.36f}, {1.45f, 0.18f}, {0.6f, 0.06f}}, 9);
    stamp(b, cgx, cgy, att, {{-0.35f, 0.42f}, {1.35f, 0.58f}, {1.42f, 0.24f}, {-0.22f, 0.16f}}, 1);
    stamp(b, cgx, cgy, att, {{-0.05f, 0.4f}, {1.05f, 0.5f}, {1.1f, 0.32f}, {0.02f, 0.26f}}, 5);
    Pt a = xform(cgx, cgy, att, 0.15f, 0.28f);
    Pt c = xform(cgx, cgy, att, 0.42f, -0.05f);
    b.line(a.first, a.second, c.first, c.second, 8, 2.2f);
    Pt head = xform(cgx, cgy, att, 0.95f, 0.18f);
    b.ellipse(head.first, head.second, 4.2f, 4.2f, 6);
    Pt wheel = xform(cgx, cgy, att, 0.55f, -0.72f);
    b.ellipse(wheel.first, wheel.second, 5.5f, 5.5f, 7);
    b.ellipse(wheel.first - 1.f, wheel.second - 1.f, 2.1f, 2.1f, 5);
    b.outline(7, false);

    int x0 = b.w, y0 = b.h;
    for (int y = 0; y < b.h; ++y)
        for (int x = 0; x < b.w; ++x)
            if (b.get(x, y)) {
                x0 = std::min(x0, x);
                y0 = std::min(y0, y);
            }
    Bitmap cropped = b.cropToContent(0);
    Ship s;
    s.img = gs::uploadMipped(vdp, cropped);
    s.ax = cgx - float(x0);
    s.ay = cgy - float(y0);
    s.ppm = PPM;
    return s;
}

Bitmap grassArt(int which) {
    Bitmap b(48, 32);
    for (int y = 0; y < b.h; ++y)
        for (int x = 0; x < b.w; ++x) {
            uint32_t h = hash2(x + which * 17, y + 3);
            int c = 1;
            if ((h & 7) == 0) c = 2;
            if ((h % 29) == 0) c = 3;
            if ((h & 31) == 1) c = 4;
            if (which && (x + y) % 13 == 0) c = 5;
            b.set(x, y, c);
        }
    return b;
}

Bitmap lakeArt(int which) {
    Bitmap b(48, 32);
    for (int y = 0; y < b.h; ++y)
        for (int x = 0; x < b.w; ++x) {
            uint32_t h = hash2(x + 9, y + which * 5);
            int c = (y / 6 + which) & 1 ? 1 : 2;
            if ((h & 15) == 0) c = 3;
            if (y % 7 == 2 && (x + which * 3) % 5 == 0) c = 4;
            b.set(x, y, c);
        }
    return b;
}

Bitmap foamArt() {
    Bitmap b(36, 18);
    b.rect(0, 8, 36, 8, 2);
    for (int x = 0; x < 36; x += 4) b.ellipse(float(x), 8, 3.f, 4.f, 1);
    b.rect(0, 12, 36, 6, 3);
    return b;
}

Bitmap markArt() {
    Bitmap b(128, 40);
    b.ellipse(64, 20, 60, 16, 1);
    b.ellipse(64, 20, 46, 11, 0);
    b.rect(60, 4, 8, 32, 2);
    b.rect(14, 16, 100, 7, 2);
    b.ellipse(64, 20, 7, 5, 3);
    b.ellipse(64, 20, 3, 2, 4);
    return b;
}

Bitmap chevArt() {
    Bitmap b(26, 14);
    b.poly({{2, 3}, {16, 7}, {2, 11}, {6, 7}}, 3);
    b.poly({{8, 3}, {22, 7}, {8, 11}, {12, 7}}, 1);
    return b;
}

Bitmap flagArt(int frame) {
    Bitmap b(36, 64);
    b.rect(6, 8, 3, 54, 3);
    b.rect(7, 10, 1, 50, 5);
    float flick = frame ? 5.f : 0.f;
    b.poly({{9, 8}, {30, 12 + flick}, {28, 22 + flick}, {9, 20}}, 1);
    b.poly({{11, 11}, {26, 14 + flick}, {24, 19 + flick}, {11, 17}}, 2);
    b.ellipse(7, 7, 3, 3, 1);
    return b;
}

Bitmap sockArt(int frame) {
    Bitmap b(46, 52);
    b.rect(30, 14, 3, 36, 3);
    b.ellipse(31, 12, 3, 3, 1);
    float droop = 4.f + float(frame) * 3.5f;
    b.poly({{30, 14}, {8, 10 + droop}, {6, 18 + droop}, {30, 22}}, 1);
    b.poly({{28, 16}, {12, 14 + droop}, {11, 18 + droop}, {28, 20}}, 2);
    return b;
}

Bitmap towerArt() {
    Bitmap b(70, 88);
    b.rect(14, 70, 4, 16, 3);
    b.rect(50, 70, 4, 16, 3);
    b.rect(16, 78, 36, 3, 4);
    b.rect(10, 36, 50, 36, 5);
    b.rect(14, 40, 42, 28, 4);
    b.poly({{8, 38}, {35, 16}, {62, 38}}, 1);
    b.rect(32, 22, 4, 16, 3);
    gs::TextStyle st{2, 6, 0, 0, 1};
    Bitmap word = gs::textBitmap("MARK", st);
    b.blit(word, 16, 46);
    b.outline(7, false);
    return b;
}

Bitmap pineArt() {
    Bitmap b(40, 56);
    b.poly({{20, 2}, {36, 26}, {4, 26}}, 1);
    b.poly({{20, 14}, {38, 40}, {2, 40}}, 2);
    b.poly({{20, 26}, {34, 48}, {6, 48}}, 3);
    b.rect(18, 46, 4, 8, 4);
    return b;
}

Bitmap reedArt() {
    Bitmap b(28, 30);
    b.line(8, 28, 6, 8, 1, 1.6f);
    b.line(14, 28, 16, 4, 2, 1.6f);
    b.line(20, 28, 22, 10, 1, 1.6f);
    b.ellipse(6, 8, 3, 2, 3);
    b.ellipse(16, 4, 3, 2, 3);
    return b;
}

Bitmap boatArt() {
    Bitmap b(48, 22);
    b.poly({{4, 12}, {40, 12}, {34, 18}, {10, 18}}, 1);
    b.poly({{8, 12}, {36, 12}, {32, 16}, {12, 16}}, 2);
    b.rect(22, 4, 2, 10, 3);
    b.poly({{24, 5}, {36, 8}, {24, 12}}, 4);
    return b;
}

Bitmap hillArt() {
    Bitmap b(120, 48);
    b.poly({{0, 46}, {18, 28}, {34, 34}, {58, 10}, {82, 26}, {100, 16}, {119, 46}}, 1);
    b.poly({{40, 30}, {58, 14}, {74, 28}, {52, 36}}, 2);
    b.rect(0, 42, 120, 6, 3);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(72, 28);
    b.ellipse(18, 16, 14, 8, 1);
    b.ellipse(36, 12, 18, 10, 1);
    b.ellipse(54, 16, 14, 7, 1);
    b.ellipse(36, 16, 16, 6, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(36, 36);
    b.ellipse(18, 18, 8, 8, 1);
    b.ellipse(18, 18, 5, 5, 2);
    for (int i = 0; i < 8; ++i) {
        float a = float(i) * 0.785398f;
        b.line(18 + std::cos(a) * 11, 18 + std::sin(a) * 11, 18 + std::cos(a) * 16, 18 + std::sin(a) * 16, 1, 1.8f);
    }
    return b;
}

Bitmap dustArt() {
    Bitmap b(22, 14);
    b.ellipse(11, 8, 9, 4, 1);
    b.ellipse(7, 7, 3, 2, 2);
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
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
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
    vdp.setColor(PAL_HUD * 16 + 2, gs::rgb4(8, 10, 11));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 15, 8));

    setPal(vdp, PAL_SHIP, {0, gs::rgb4(15, 15, 14), gs::rgb4(2, 12, 11), gs::rgb4(15, 8, 2), gs::rgb4(2, 4, 7),
                           gs::rgb4(8, 10, 12), gs::rgb4(14, 11, 8), gs::rgb4(2, 2, 3), gs::rgb4(6, 5, 3),
                           gs::rgb4(10, 14, 15)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(4, 9, 3), gs::rgb4(3, 7, 2), gs::rgb4(8, 11, 4), gs::rgb4(6, 10, 3),
                            gs::rgb4(9, 12, 5)});
    setPal(vdp, PAL_LAKE, {0, gs::rgb4(2, 6, 11), gs::rgb4(3, 8, 13), gs::rgb4(5, 10, 14), gs::rgb4(12, 14, 15)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 15, 14), gs::rgb4(13, 2, 2), gs::rgb4(15, 8, 1), gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(15, 8, 2), gs::rgb4(15, 13, 5), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1),
                           gs::rgb4(12, 8, 4), gs::rgb4(15, 15, 14), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 6, 3), gs::rgb4(3, 8, 3), gs::rgb4(4, 10, 4), gs::rgb4(6, 4, 2),
                           gs::rgb4(8, 12, 5)});
    setPal(vdp, PAL_FAR, {0, gs::rgb4(5, 7, 10), gs::rgb4(7, 9, 12), gs::rgb4(4, 6, 8)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 14, 10), gs::rgb4(15, 15, 15), gs::rgb4(15, 11, 4), gs::rgb4(15, 8, 2)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(10, 12, 8), gs::rgb4(7, 9, 6)});
    setPal(vdp, PAL_BOAT, {0, gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 4), gs::rgb4(4, 3, 2), gs::rgb4(15, 14, 12)});

    loadFont(vdp, art);
    const float atts[5] = {0.22f, 0.08f, -0.06f, -0.20f, -0.34f};
    for (int i = 0; i < 5; ++i) art.ship[i] = drawShip(vdp, atts[i]);
    art.grass[0] = gs::uploadMipped(vdp, grassArt(0));
    art.grass[1] = gs::uploadMipped(vdp, grassArt(1));
    art.lake[0] = gs::uploadMipped(vdp, lakeArt(0));
    art.lake[1] = gs::uploadMipped(vdp, lakeArt(1));
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.mark = gs::uploadMipped(vdp, markArt());
    art.chev = gs::uploadMipped(vdp, chevArt());
    art.flag[0] = gs::uploadMipped(vdp, flagArt(0));
    art.flag[1] = gs::uploadMipped(vdp, flagArt(1));
    for (int i = 0; i < 3; ++i) art.sock[i] = gs::uploadMipped(vdp, sockArt(i));
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.boat = gs::uploadMipped(vdp, boatArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
}

}  // namespace gmark
