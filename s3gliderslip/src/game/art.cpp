#include "art.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace gslip {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 2, 3));
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

Ship seal(gs::VDP& vdp, Bitmap& b, float hintX) {
    int ax = int(std::lround(hintX));
    int bestY = -1, bestX = ax;
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++)
            if (b.get(x, y) && std::abs(x - ax) <= 10 && y >= bestY) {
                bestY = y;
                bestX = x;
            }
    if (bestY < 0) {
        for (int y = 0; y < b.h; y++)
            for (int x = 0; x < b.w; x++)
                if (b.get(x, y) && y >= bestY) {
                    bestY = y;
                    bestX = x;
                }
    }
    if (bestY < 0) bestY = 0;
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
    s.ax = float(bestX - x0);
    s.ay = float(bestY - y0);
    s.ppm = PPM;
    return s;
}

// Stepped-hull flying boat. Positive att is nose-up. No propeller.
Ship drawBoat(gs::VDP& vdp, float att) {
    const float cgx = 120.f, cgy = 96.f;
    Bitmap b(240, 180);
    polyM(b, cgx, cgy, att,
          {{-4.45f, 0.06f},
           {-4.35f, 0.32f},
           {-2.1f, 0.48f},
           {-0.15f, 0.56f},
           {1.05f, 0.68f},
           {2.45f, 0.50f},
           {3.25f, 0.16f},
           {2.85f, 0.00f},
           {1.55f, -0.02f},
           {0.82f, -0.05f},
           {0.62f, -0.30f},
           {0.02f, -0.32f},
           {-0.08f, -0.04f},
           {-4.15f, 0.00f}},
          1);
    polyM(b, cgx, cgy, att,
          {{-4.1f, 0.02f}, {-0.05f, -0.02f}, {0.02f, -0.30f}, {0.58f, -0.28f}, {0.78f, -0.04f}, {2.6f, 0.02f}, {1.4f, -0.01f}},
          2);
    polyM(b, cgx, cgy, att, {{-3.6f, 0.22f}, {2.2f, 0.40f}, {2.15f, 0.30f}, {-3.55f, 0.12f}}, 4);
    polyM(b, cgx, cgy, att, {{1.15f, 0.52f}, {2.35f, 0.58f}, {2.55f, 0.36f}, {1.25f, 0.32f}}, 8);
    polyM(b, cgx, cgy, att, {{-1.55f, 1.05f}, {1.15f, 1.28f}, {1.35f, 0.90f}, {-1.65f, 0.70f}}, 5);
    polyM(b, cgx, cgy, att, {{-1.25f, 1.12f}, {1.0f, 1.28f}, {1.08f, 1.10f}, {-1.15f, 0.96f}}, 4);
    polyM(b, cgx, cgy, att, {{-0.15f, 0.55f}, {0.05f, 0.55f}, {0.02f, 1.05f}, {-0.18f, 1.02f}}, 6);
    polyM(b, cgx, cgy, att, {{-4.35f, 0.28f}, {-3.75f, 0.40f}, {-3.9f, 1.62f}, {-4.5f, 1.48f}}, 1);
    polyM(b, cgx, cgy, att, {{-4.55f, 1.52f}, {-3.45f, 1.68f}, {-3.5f, 1.40f}, {-4.5f, 1.26f}}, 5);
    polyM(b, cgx, cgy, att, {{-4.2f, 1.15f}, {-3.85f, 1.22f}, {-3.9f, 1.42f}, {-4.25f, 1.36f}}, 4);
    polyM(b, cgx, cgy, att, {{-0.55f, 0.42f}, {0.35f, 0.48f}, {0.5f, 0.22f}, {-0.4f, 0.16f}}, 9);
    Pt strut = xform(cgx, cgy, att, -0.1f, 0.55f);
    Pt foot = xform(cgx, cgy, att, -0.1f, 0.18f);
    b.line(strut.first, strut.second, foot.first, foot.second, 6, 2.f);
    Pt step = xform(cgx, cgy, att, 0.28f, -0.32f);
    b.outline(7, false);
    return seal(vdp, b, step.first);
}

Bitmap skiffArt() {
    Bitmap b(96, 52);
    b.poly({{6, 30}, {78, 28}, {90, 36}, {74, 46}, {14, 46}, {2, 36}}, 1);
    b.poly({{10, 36}, {80, 34}, {84, 40}, {18, 42}}, 2);
    b.poly({{36, 30}, {44, 12}, {68, 12}, {74, 30}}, 3);
    b.rect(48, 16, 12, 8, 8);
    b.rect(58, 4, 7, 10, 4);
    b.ellipse(16, 40, 4.5f, 3.2f, 7);
    b.ellipse(78, 38, 4.2f, 3.f, 7);
    b.rect(40, 44, 16, 3, 5);
    b.line(30, 18, 30, 6, 6, 1.6f);
    b.poly({{30, 7}, {46, 12}, {30, 16}}, 6);
    return b;
}

Bitmap waterArt(int frame, bool berth) {
    Bitmap b(48, 32);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x + frame * 5, y + (berth ? 40 : 0));
            int wave = (x + frame * 4 + y / 2) % 8;
            int c = berth ? 9 : 1;
            if (wave == 0) c = berth ? 10 : 3;
            else if ((h % 7) == 0) c = berth ? 10 : 2;
            if ((h % 19) == 0) c = 4;
            b.set(x, y, c);
        }
    return b;
}

Bitmap mudArt() {
    Bitmap b(40, 28);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x + 2, y + 11);
            int c = 5;
            if ((h & 5) == 0) c = 6;
            if ((h % 17) == 0) c = 14;
            b.set(x, y, c);
        }
    return b;
}

Bitmap rockArt() {
    Bitmap b(36, 28);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x + 8, y + 3);
            b.set(x, y, (h & 3) == 0 ? 8 : 7);
        }
    return b;
}

Bitmap bluffArt() {
    Bitmap b(48, 36);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x, y + 6);
            int c = y < 10 ? ((h & 3) == 0 ? 13 : 12) : ((h & 2) ? 6 : 14);
            b.set(x, y, c);
        }
    return b;
}

Bitmap plankArt() {
    Bitmap b(32, 16);
    b.rect(0, 0, 32, 16, 1);
    b.rect(0, 0, 32, 3, 3);
    b.rect(0, 12, 32, 4, 2);
    for (int x = 4; x < 32; x += 8) b.rect(float(x), 3, 1, 9, 4);
    return b;
}

Bitmap pierArt() {
    Bitmap b(40, 96);
    b.rect(4, 18, 6, 76, 1);
    b.rect(17, 18, 6, 76, 2);
    b.rect(30, 18, 6, 76, 1);
    b.rect(4, 40, 32, 4, 3);
    b.rect(4, 64, 32, 4, 3);
    b.rect(0, 8, 40, 12, 1);
    b.rect(0, 8, 40, 3, 3);
    b.rect(8, 4, 4, 6, 5);
    return b;
}

Bitmap dolphinArt() {
    Bitmap b(28, 72);
    b.rect(3, 16, 4, 54, 1);
    b.rect(12, 16, 4, 54, 2);
    b.rect(21, 16, 4, 54, 1);
    b.rect(1, 10, 26, 8, 3);
    b.rect(1, 10, 26, 2, 5);
    b.ellipse(8, 28, 3.2f, 2.4f, 6);
    b.ellipse(20, 36, 3.2f, 2.4f, 6);
    return b;
}

Bitmap quayArt() {
    Bitmap b(88, 72);
    b.rect(0, 28, 88, 44, 1);
    b.rect(6, 8, 58, 24, 3);
    b.poly({{4, 28}, {35, 2}, {66, 28}}, 4);
    b.rect(28, 48, 16, 24, 10);
    b.rect(14, 36, 12, 10, 9);
    b.rect(44, 36, 12, 10, 9);
    b.rect(62, 40, 18, 32, 2);
    b.rect(66, 18, 8, 16, 5);
    b.rect(0, 26, 88, 4, 13);
    return b;
}

Bitmap lightArt() {
    Bitmap b(40, 88);
    b.poly({{8, 78}, {32, 78}, {26, 28}, {14, 28}}, 1);
    b.rect(12, 16, 16, 14, 6);
    b.rect(14, 18, 12, 8, 5);
    b.poly({{10, 16}, {20, 4}, {30, 16}}, 4);
    b.rect(18, 78, 4, 8, 2);
    b.ellipse(20, 70, 3.f, 2.f, 7);
    return b;
}

Bitmap buoyArt() {
    Bitmap b(22, 36);
    b.poly({{11, 2}, {18, 16}, {16, 28}, {6, 28}, {4, 16}}, 7);
    b.rect(6, 14, 10, 5, 8);
    b.ellipse(11, 30, 5.f, 2.4f, 2);
    b.line(11, 2, 11, 0, 6, 1.2f);
    return b;
}

Bitmap pennantArt(int frame) {
    Bitmap b(40, 56);
    b.rect(6, 8, 3, 46, 13);
    float droop = 2.f + float(frame) * 4.f;
    b.poly({{9, 10}, {36, 6 + droop}, {34, 14 + droop}, {9, 18}}, 11);
    b.poly({{12, 12}, {30, 10 + droop}, {28, 14 + droop}, {12, 16}}, 12);
    return b;
}

Bitmap signArt() {
    gs::TextStyle st{3, 10, 0, 0, 1};
    Bitmap word = gs::textBitmap("SLIP", st);
    Bitmap b(word.w + 14, word.h + 12);
    b.rect(0, 0, float(b.w), float(b.h), 7);
    b.rect(3, 3, float(b.w - 6), float(b.h - 6), 6);
    b.blit(word, 7, 6);
    return b;
}

Bitmap staffArt() {
    Bitmap b(16, 80);
    b.rect(6, 4, 4, 74, 13);
    for (int i = 0; i < 5; i++) b.rect(2, float(10 + i * 12), 12, 2, i == 2 ? 11 : 6);
    b.ellipse(8, 6, 3.f, 3.f, 5);
    return b;
}

Bitmap gullArt(int flap) {
    Bitmap b(30, 16);
    float y = flap ? 3.f : 8.f;
    b.poly({{1, y}, {13, 9}, {1, y + 3}}, 2);
    b.poly({{29, y}, {17, 9}, {29, y + 3}}, 2);
    b.ellipse(15, 9, 3.4f, 2.2f, 1);
    b.line(18, 9, 22, 8, 3, 1.3f);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(78, 30);
    b.ellipse(20, 18, 16, 8, 1);
    b.ellipse(40, 12, 20, 11, 1);
    b.ellipse(58, 17, 15, 8, 1);
    b.ellipse(38, 18, 16, 6, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(40, 40);
    b.ellipse(20, 20, 9, 9, 3);
    b.ellipse(20, 20, 5, 5, 4);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785f;
        b.line(20 + std::cos(a) * 12, 20 + std::sin(a) * 12, 20 + std::cos(a) * 18, 20 + std::sin(a) * 18, 3, 2.f);
    }
    return b;
}

Bitmap headArt() {
    Bitmap b(120, 48);
    b.poly({{0, 46}, {18, 28}, {40, 34}, {62, 10}, {88, 26}, {120, 46}}, 1);
    b.poly({{48, 32}, {62, 14}, {78, 28}, {58, 36}}, 2);
    b.rect(0, 40, 120, 8, 3);
    return b;
}

Bitmap sprayArt() {
    Bitmap b(28, 16);
    b.ellipse(14, 9, 12, 5, 1);
    b.ellipse(8, 8, 5, 3, 2);
    b.ellipse(18, 7, 4, 2, 2);
    return b;
}

Bitmap shadeArt() {
    Bitmap b(44, 12);
    b.ellipse(22, 6, 18, 4, 1);
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
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 8));

    setPal(vdp, PAL_SHIP,
           {0, gs::rgb4(15, 14, 12), gs::rgb4(2, 8, 10), gs::rgb4(4, 12, 13), gs::rgb4(14, 8, 2), gs::rgb4(14, 15, 15),
            gs::rgb4(8, 12, 13), gs::rgb4(2, 3, 6), gs::rgb4(6, 12, 14), gs::rgb4(12, 3, 3), gs::rgb4(1, 2, 3),
            gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(3, 10, 6), gs::rgb4(2, 6, 4), gs::rgb4(14, 13, 10), gs::rgb4(8, 4, 2), gs::rgb4(2, 2, 2),
            gs::rgb4(15, 15, 14), gs::rgb4(13, 3, 2), gs::rgb4(5, 10, 13)});
    setPal(vdp, PAL_HARBOR,
           {0, gs::rgb4(2, 5, 10), gs::rgb4(3, 7, 12), gs::rgb4(8, 13, 15), gs::rgb4(12, 15, 15), gs::rgb4(8, 6, 3),
            gs::rgb4(11, 8, 4), gs::rgb4(6, 6, 6), gs::rgb4(4, 4, 5), gs::rgb4(4, 9, 12), gs::rgb4(7, 13, 14),
            gs::rgb4(14, 12, 4), gs::rgb4(4, 9, 4), gs::rgb4(2, 6, 3), gs::rgb4(12, 10, 6)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(9, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(12, 9, 5), gs::rgb4(5, 3, 2), gs::rgb4(3, 10, 8),
            gs::rgb4(12, 3, 2)});
    setPal(vdp, PAL_TOWN,
           {0, gs::rgb4(9, 9, 8), gs::rgb4(5, 5, 6), gs::rgb4(12, 5, 3), gs::rgb4(6, 2, 2), gs::rgb4(15, 13, 5),
            gs::rgb4(15, 15, 14), gs::rgb4(13, 3, 2), gs::rgb4(15, 15, 15), gs::rgb4(6, 10, 13), gs::rgb4(2, 2, 3),
            gs::rgb4(14, 7, 2), gs::rgb4(15, 11, 5), gs::rgb4(8, 6, 3), gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 15, 15), gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_FAR, {0, gs::rgb4(4, 7, 8), gs::rgb4(6, 9, 9), gs::rgb4(3, 5, 6)});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 12, 14)});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 14), gs::rgb4(6, 7, 8), gs::rgb4(14, 8, 2)});

    loadFont(vdp, art);
    const float atts[5] = {0.34f, 0.17f, 0.f, -0.17f, -0.34f};
    for (int i = 0; i < 5; i++) art.boat[i] = drawBoat(vdp, atts[i]);
    art.skiff = gs::uploadMipped(vdp, skiffArt());
    art.bay[0] = gs::uploadMipped(vdp, waterArt(0, false));
    art.bay[1] = gs::uploadMipped(vdp, waterArt(1, false));
    art.slipW[0] = gs::uploadMipped(vdp, waterArt(0, true));
    art.slipW[1] = gs::uploadMipped(vdp, waterArt(1, true));
    art.mud = gs::uploadMipped(vdp, mudArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.bluff = gs::uploadMipped(vdp, bluffArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.dolphin = gs::uploadMipped(vdp, dolphinArt());
    art.quay = gs::uploadMipped(vdp, quayArt());
    art.light = gs::uploadMipped(vdp, lightArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    for (int i = 0; i < 3; i++) art.pennant[i] = gs::uploadMipped(vdp, pennantArt(i));
    art.sign = gs::uploadMipped(vdp, signArt());
    art.staff = gs::uploadMipped(vdp, staffArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(0));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(1));
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.head = gs::uploadMipped(vdp, headArt());
    art.spray = gs::uploadMipped(vdp, sprayArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
}

}  // namespace gslip
