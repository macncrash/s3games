#include "game/art.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace glane {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float PPM = 16.f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(2, 2, 3));
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

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
    const float ox = 180.f, oy = 150.f;
    Bitmap b(360, 260);
    polyM(b, ox, oy, att, {{-4.05f, -0.08f}, {-3.55f, 0.2f}, {2.05f, 0.24f}, {2.55f, 0.08f}, {2.35f, -0.1f}, {0.2f, -0.16f}}, 1);
    polyM(b, ox, oy, att, {{1.85f, 0.2f}, {3.05f, 0.12f}, {3.15f, -0.02f}, {2.15f, -0.06f}}, 3);
    polyM(b, ox, oy, att, {{-0.7f, 0.18f}, {-0.35f, 0.58f}, {1.85f, 0.5f}, {2.05f, 0.16f}}, 4);
    polyM(b, ox, oy, att, {{-0.15f, 0.22f}, {1.45f, 0.22f}, {1.55f, 0.34f}, {-0.05f, 0.36f}}, 5);
    polyM(b, ox, oy, att, {{0.15f, 0.46f}, {0.95f, 0.44f}, {0.88f, 0.34f}, {0.2f, 0.36f}}, 9);
    polyM(b, ox, oy, att, {{-3.95f, 0.12f}, {-3.4f, 0.16f}, {-3.5f, 1.05f}, {-4.15f, 0.78f}}, 1);
    polyM(b, ox, oy, att, {{-3.9f, 0.5f}, {-3.55f, 0.54f}, {-3.6f, 0.88f}, {-3.98f, 0.8f}}, 3);
    polyM(b, ox, oy, att, {{-4.45f, 0.86f}, {-3.15f, 0.96f}, {-3.1f, 0.74f}, {-4.5f, 0.66f}}, 4);
    Pt head = xform(ox, oy, att, 1.15f, 0.3f);
    b.ellipse(head.first, head.second, 7.5f, 5.5f, 6);
    Pt pilot = xform(ox, oy, att, 1.05f, 0.32f);
    b.ellipse(pilot.first, pilot.second, 3.2f, 3.2f, 7);
    Pt wheel = xform(ox, oy, att, 0.15f, -0.16f);
    b.ellipse(wheel.first, wheel.second, 4.2f, 4.2f, 8);
    b.ellipse(wheel.first, wheel.second, 1.8f, 1.8f, 2);
    Pt a = xform(ox, oy, att, -3.3f, -0.12f);
    Pt c = xform(ox, oy, att, -4.15f, -0.22f);
    b.line(a.first, a.second, c.first, c.second, 8, 2.f);
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

Bitmap railArt() {
    Bitmap b(36, 10);
    for (int x = 0; x < 36; x++) {
        int c = ((x / 6) & 1) ? 1 : 2;
        for (int y = 0; y < 10; y++) b.set(x, y, c);
    }
    return b;
}

Bitmap postArt() {
    Bitmap b(10, 32);
    for (int y = 0; y < 32; y++) {
        int c = ((y / 6) & 1) ? 1 : 2;
        for (int x = 2; x < 8; x++) b.set(x, y, c);
    }
    b.rect(1, 0, 8, 4, 5);
    return b;
}

Bitmap flagArt() {
    Bitmap b(16, 12);
    b.poly({{1, 6}, {14, 1}, {14, 11}}, 4);
    b.rect(0, 1, 2, 10, 2);
    return b;
}

Bitmap chevArt() {
    Bitmap b(26, 16);
    b.poly({{2, 2}, {20, 8}, {2, 14}, {7, 8}}, 1);
    return b;
}

Bitmap signArt(const char* word) {
    gs::TextStyle st{4, 1, 0, 0, 1};
    Bitmap label = gs::textBitmap(word, st);
    Bitmap b(label.w + 16, label.h + 14);
    b.rect(0, 0, float(b.w), float(b.h), 3);
    b.rect(3, 3, float(b.w - 6), float(b.h - 6), 2);
    b.blit(label, 8, 5);
    return b;
}

Bitmap shedArt() {
    Bitmap b(48, 36);
    b.rect(6, 14, 36, 20, 1);
    b.poly({{4, 16}, {24, 3}, {44, 16}}, 2);
    b.rect(20, 22, 8, 12, 3);
    b.rect(10, 20, 7, 6, 4);
    b.rect(31, 20, 7, 6, 4);
    return b;
}

Bitmap sockArt(int frame) {
    Bitmap b(40, 32);
    b.rect(5, 6, 3, 24, 2);
    float droop = 3.f + float(frame) * 3.5f;
    b.poly({{8, 8}, {36, 5 + droop}, {34, 13 + droop}, {8, 15}}, 1);
    b.poly({{10, 10}, {30, 8 + droop}, {28, 12 + droop}, {10, 13}}, 5);
    return b;
}

Bitmap reedArt() {
    Bitmap b(28, 36);
    b.poly({{14, 2}, {26, 30}, {2, 30}}, 1);
    b.poly({{14, 10}, {22, 32}, {6, 32}}, 2);
    b.rect(12, 28, 4, 8, 4);
    return b;
}

Bitmap bankArt() {
    Bitmap b(40, 24);
    for (int y = 0; y < 24; y++)
        for (int x = 0; x < 40; x++) {
            uint32_t h = hash2(x + 3, y + 9);
            int c = 1 + int(h % 3);
            if (y > 16) c = 4;
            if ((h % 23) == 0) c = 5;
            b.set(x, y, c);
        }
    return b;
}

Bitmap waterArt() {
    Bitmap b(40, 28);
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 40; x++) {
            uint32_t h = hash2(x + 1, y + 2);
            int c = 1 + int((x / 5 + y / 4) & 1);
            if ((h % 17) == 0) c = 3;
            if (y < 3 && (h & 3) == 0) c = 4;
            b.set(x, y, c);
        }
    return b;
}

Bitmap hillArt() {
    Bitmap b(120, 48);
    b.poly({{0, 46}, {18, 28}, {40, 34}, {62, 10}, {86, 26}, {104, 16}, {120, 46}}, 1);
    b.poly({{54, 18}, {66, 10}, {78, 24}, {58, 26}}, 3);
    b.rect(0, 40, 120, 8, 2);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(64, 26);
    b.ellipse(16, 15, 12, 7, 1);
    b.ellipse(32, 11, 16, 9, 1);
    b.ellipse(48, 15, 12, 7, 1);
    b.ellipse(30, 15, 14, 6, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 7, 7, 3);
    b.ellipse(16, 16, 4, 4, 4);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785f;
        b.line(16 + std::cos(a) * 10, 16 + std::sin(a) * 10, 16 + std::cos(a) * 15, 16 + std::sin(a) * 15, 3, 2.f);
    }
    return b;
}

Bitmap gullArt(int frame) {
    Bitmap b(30, 14);
    float dip = frame ? 3.f : 0.f;
    b.poly({{1, 7}, {13, 5 + dip}, {15, 7}, {13, 8}}, 5);
    b.poly({{29, 7}, {17, 5 + dip}, {15, 7}, {17, 8}}, 5);
    b.ellipse(15, 7, 2, 2, 6);
    return b;
}

Bitmap dustArt() {
    Bitmap b(22, 12);
    b.ellipse(11, 6, 9, 4, 1);
    b.ellipse(7, 6, 4, 2, 2);
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

    setPal(vdp, PAL_SHIP, {0, gs::rgb4(14, 13, 11), gs::rgb4(15, 15, 14), gs::rgb4(12, 5, 2), gs::rgb4(2, 10, 9),
                           gs::rgb4(1, 6, 6), gs::rgb4(5, 9, 13), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 2),
                           gs::rgb4(14, 11, 3)});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(13, 9, 6), gs::rgb4(15, 12, 9), gs::rgb4(11, 3, 2), gs::rgb4(13, 7, 2),
                           gs::rgb4(8, 4, 1), gs::rgb4(6, 6, 9), gs::rgb4(3, 2, 2), gs::rgb4(2, 1, 1),
                           gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_RAIL, {0, gs::rgb4(15, 11, 2), gs::rgb4(2, 2, 1), gs::rgb4(14, 12, 9), gs::rgb4(12, 3, 2),
                           gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_TIGHT, {0, gs::rgb4(15, 15, 14), gs::rgb4(13, 2, 2), gs::rgb4(3, 2, 2), gs::rgb4(15, 10, 2),
                            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(3, 8, 3), gs::rgb4(2, 6, 2), gs::rgb4(5, 10, 4), gs::rgb4(7, 5, 3),
                           gs::rgb4(13, 12, 5)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 5, 9), gs::rgb4(3, 7, 11), gs::rgb4(5, 10, 13), gs::rgb4(12, 14, 14)});
    setPal(vdp, PAL_REED, {0, gs::rgb4(1, 5, 2), gs::rgb4(2, 7, 3), gs::rgb4(4, 9, 4), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 11),
                          gs::rgb4(8, 8, 9), gs::rgb4(14, 10, 4)});
    setPal(vdp, PAL_HILL, {0, gs::rgb4(6, 7, 10), gs::rgb4(4, 5, 8), gs::rgb4(12, 12, 14)});
    setPal(vdp, PAL_SHED, {0, gs::rgb4(11, 9, 7), gs::rgb4(8, 3, 2), gs::rgb4(4, 3, 2), gs::rgb4(8, 12, 13)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(12, 11, 9), gs::rgb4(8, 8, 7)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(2, 2, 1), gs::rgb4(14, 12, 8), gs::rgb4(8, 4, 2)});

    loadFont(vdp, art);
    const float atts[5] = {0.30f, 0.15f, 0.f, -0.15f, -0.30f};
    for (int i = 0; i < 5; i++) art.wing[i] = drawWing(vdp, atts[i]);
    art.rail = gs::uploadMipped(vdp, railArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.chev = gs::uploadMipped(vdp, chevArt());
    art.signLane = gs::uploadMipped(vdp, signArt("LANE"));
    art.signEnd = gs::uploadMipped(vdp, signArt("END"));
    art.shed = gs::uploadMipped(vdp, shedArt());
    for (int i = 0; i < 3; i++) art.sock[i] = gs::uploadMipped(vdp, sockArt(i));
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.bank = gs::uploadMipped(vdp, bankArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(0));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(1));
    art.dust = gs::uploadMipped(vdp, dustArt());
}

}  // namespace glane
