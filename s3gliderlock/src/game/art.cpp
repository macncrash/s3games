#include "art.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace gliderlock {
namespace {

using gs::Bitmap;
using gs::Pt;

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
    const float ppm = 20.f;
    const float ca = std::cos(-ang), sa = std::sin(-ang);
    std::vector<Pt> p;
    p.reserve(meters.size());
    for (const Pt& m : meters) {
        float dx = m.first * ppm, dy = -m.second * ppm;
        p.push_back({cgx + dx * ca - dy * sa, cgy + dx * sa + dy * ca});
    }
    b.poly(p, c);
}

Pt xform(float cgx, float cgy, float ang, float mx, float my) {
    const float ppm = 20.f;
    const float ca = std::cos(-ang), sa = std::sin(-ang);
    float dx = mx * ppm, dy = -my * ppm;
    return {cgx + dx * ca - dy * sa, cgy + dx * sa + dy * ca};
}

Wing drawWing(gs::VDP& vdp, float att) {
    const float cgx = 170.f, cgy = 130.f;
    Bitmap b(360, 280);
    // Tail first, then the wing, then the fuselage so the body covers the root.
    polyM(b, cgx, cgy, att, {{-3.85f, 0.35f}, {-2.55f, 3.15f}, {-3.15f, 3.25f}, {-4.05f, 0.55f}}, 3);
    polyM(b, cgx, cgy, att, {{-4.15f, 2.55f}, {-2.25f, 2.72f}, {-2.3f, 2.28f}, {-4.05f, 2.12f}}, 1);
    polyM(b, cgx, cgy, att, {{-0.55f, 0.55f}, {1.85f, 0.95f}, {1.45f, 2.35f}, {-0.15f, 2.55f}}, 1);
    polyM(b, cgx, cgy, att, {{-0.35f, 0.7f}, {1.55f, 1.05f}, {1.25f, 1.45f}, {-0.1f, 1.25f}}, 4);
    polyM(b, cgx, cgy, att,
           {{-4.05f, 0.12f}, {-3.1f, 0.62f}, {-0.2f, 0.78f}, {2.4f, 0.55f}, {4.75f, 0.16f}, {4.55f, -0.08f},
            {2.8f, -0.48f}, {-0.2f, -0.62f}, {-3.55f, -0.32f}},
           1);
    polyM(b, cgx, cgy, att, {{2.2f, 0.22f}, {4.7f, 0.08f}, {4.45f, -0.12f}, {2.15f, 0.02f}}, 4);
    polyM(b, cgx, cgy, att, {{-3.4f, -0.22f}, {-0.4f, -0.48f}, {-0.2f, -0.68f}, {-3.5f, -0.42f}}, 7);
    polyM(b, cgx, cgy, att, {{0.35f, 0.42f}, {2.15f, 0.62f}, {2.35f, 0.22f}, {0.55f, 0.08f}}, 2);
    Pt head = xform(cgx, cgy, att, 1.55f, 0.48f);
    b.ellipse(head.first, head.second, 9.f, 8.f, 6);
    Pt canopy = xform(cgx, cgy, att, 2.15f, 0.42f);
    b.ellipse(canopy.first, canopy.second, 16.f, 9.f, 5);
    Pt glint = xform(cgx, cgy, att, 2.45f, 0.58f);
    b.ellipse(glint.first, glint.second, 4.f, 3.f, 2);
    Pt a = xform(cgx, cgy, att, -0.15f, 1.15f);
    Pt c = xform(cgx, cgy, att, 0.35f, 0.35f);
    b.line(a.first, a.second, c.first, c.second, 3, 2.2f);
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
    Wing w;
    w.img = gs::uploadMipped(vdp, cropped);
    w.ax = cgx - float(x0);
    w.ay = cgy - float(y0);
    w.ppm = 20.f;
    return w;
}

Bitmap leafArt() {
    Bitmap b(40, 96);
    b.rect(6, 4, 28, 88, 1);
    b.rect(8, 6, 10, 84, 2);
    b.rect(22, 6, 8, 84, 3);
    for (int i = 0; i < 6; i++) {
        int y = 8 + i * 14;
        b.rect(6, y, 28, 4, (i & 1) ? 4 : 2);
        b.rect(10, y + 1, 4, 2, 5);
        b.rect(26, y + 1, 4, 2, 5);
    }
    b.line(10, 10, 30, 86, 3, 3.f);
    b.line(30, 10, 10, 86, 5, 2.f);
    b.poly({{14, 88}, {26, 88}, {20, 94}}, 3);
    b.outline(3, false);
    return b;
}

Bitmap pierArt() {
    Bitmap b(36, 120);
    for (int row = 0; row < 14; row++) {
        int y = 8 + row * 8;
        int inset = (row & 1) ? 4 : 2;
        b.rect(inset, y, 32 - inset, 7, (row % 3) == 0 ? 3 : 1);
        b.rect(inset + 2, y + 1, 8, 4, 2);
        if ((row % 4) == 1) b.rect(inset + 14, y + 2, 6, 3, 6);
    }
    b.rect(0, 4, 36, 8, 2);
    b.rect(4, 0, 28, 6, 3);
    b.rect(14, 112, 8, 8, 5);
    b.outline(3, false);
    return b;
}

Bitmap lintelArt() {
    Bitmap b(72, 18);
    b.rect(0, 4, 72, 12, 1);
    b.rect(0, 4, 72, 3, 2);
    b.rect(0, 13, 72, 3, 3);
    for (int x = 6; x < 70; x += 12) b.rect(x, 7, 3, 6, 4);
    b.outline(3, false);
    return b;
}

Bitmap sillArt() {
    Bitmap b(64, 28);
    for (int row = 0; row < 3; row++) {
        int y = 2 + row * 8;
        b.rect(2, y, 60, 7, row == 2 ? 5 : 1);
        b.rect(6, y + 1, 16, 4, 2);
        b.rect(36, y + 2, 12, 3, 3);
    }
    b.rect(0, 24, 64, 4, 6);
    b.outline(3, false);
    return b;
}

Bitmap waterArt() {
    Bitmap b(48, 20);
    b.rect(0, 6, 48, 14, 1);
    b.rect(0, 4, 48, 6, 2);
    b.rect(0, 2, 48, 3, 3);
    for (int i = 0; i < 5; i++) {
        b.ellipse(6.f + i * 9.f, 6.f, 4.f, 2.f, 4);
        b.ellipse(4.f + i * 9.f, 12.f, 3.f, 1.4f, 5);
    }
    return b;
}

Bitmap deepArt() {
    Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.set(2, 3, 2);
    b.set(6, 6, 2);
    return b;
}

Bitmap grassArt() {
    Bitmap b(40, 18);
    b.rect(0, 8, 40, 10, 5);
    b.rect(0, 6, 40, 4, 1);
    for (int x = 1; x < 40; x += 3) {
        int h = 4 + int(hash2(x, 3) % 5);
        b.line(float(x), 8, float(x + ((x & 1) ? 1 : -1)), float(8 - h), 2, 1.2f);
        if ((hash2(x, 9) % 7) == 0) b.set(x, 8 - h, 3);
    }
    return b;
}

Bitmap stoneArt() {
    Bitmap b(32, 24);
    b.rect(0, 0, 32, 24, 1);
    for (int row = 0; row < 3; row++) {
        int y = row * 8;
        int shift = (row & 1) ? 8 : 0;
        for (int x = -8; x < 32; x += 16) {
            b.rect(x + shift, y, 15, 7, ((x + row) & 2) ? 3 : 1);
            b.rect(x + shift + 2, y + 1, 6, 3, 2);
        }
        b.rect(0, y + 7, 32, 1, 4);
    }
    if ((hash2(4, 4) % 2) == 0) b.rect(18, 10, 5, 3, 6);
    return b;
}

Bitmap reedArt() {
    Bitmap b(28, 40);
    for (int i = 0; i < 7; i++) {
        float x = 4.f + i * 3.2f;
        float lean = ((i & 1) ? 4.f : -3.f);
        b.line(x, 38, x + lean, 8 + float(i % 3), 1, 1.4f);
        b.ellipse(x + lean, 8 + float(i % 3), 2.2f, 3.4f, 3);
    }
    b.rect(2, 36, 24, 3, 4);
    return b;
}

Bitmap willowArt() {
    Bitmap b(70, 88);
    b.rect(32, 40, 6, 46, 4);
    b.rect(30, 78, 10, 6, 5);
    b.ellipse(34, 36, 22, 16, 1);
    b.ellipse(22, 40, 12, 10, 2);
    b.ellipse(48, 42, 14, 11, 2);
    b.ellipse(34, 28, 10, 8, 3);
    for (int i = 0; i < 8; i++) {
        float x = 16.f + i * 6.f;
        b.line(x, 44, x + ((i & 1) ? 2.f : -2.f), 70 + float(i % 3) * 3.f, 2, 1.3f);
    }
    return b;
}

Bitmap cottageArt() {
    Bitmap b(78, 58);
    b.rect(10, 28, 58, 26, 1);
    b.poly({{4, 30}, {39, 8}, {74, 30}}, 2);
    b.poly({{16, 26}, {39, 12}, {62, 26}}, 4);
    b.rect(34, 36, 10, 18, 5);
    b.rect(18, 36, 10, 8, 3);
    b.rect(50, 36, 10, 8, 3);
    b.rect(20, 38, 3, 3, 6);
    b.rect(8, 48, 62, 6, 4);
    b.rect(36, 22, 6, 10, 3);
    b.outline(4, false);
    return b;
}

Bitmap heronArt() {
    Bitmap b(36, 48);
    b.line(16, 30, 14, 46, 4, 1.6f);
    b.line(20, 30, 22, 46, 4, 1.6f);
    b.ellipse(18, 24, 7, 10, 1);
    b.ellipse(17, 22, 3, 6, 2);
    b.line(18, 16, 28, 10, 1, 2.f);
    b.line(28, 10, 33, 12, 3, 1.6f);
    b.ellipse(16, 14, 3.5f, 3.f, 2);
    b.line(12, 22, 6, 28, 1, 1.4f);
    return b;
}

Bitmap sockArt(int frame) {
    Bitmap b(48, 64);
    b.rect(8, 18, 3, 42, 4);
    b.ellipse(9, 16, 3, 3, 3);
    float droop = 4.f + float(frame) * 6.f;
    b.poly({{10, 20}, {40, 12 + droop}, {38, 22 + droop}, {10, 28}}, 1);
    b.poly({{12, 21}, {34, 16 + droop}, {32, 20 + droop}, {12, 26}}, 2);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(80, 32);
    b.ellipse(24, 18, 16, 9, 1);
    b.ellipse(42, 16, 18, 10, 1);
    b.ellipse(58, 18, 12, 7, 2);
    b.ellipse(30, 14, 8, 5, 2);
    return b;
}

Bitmap hillArt() {
    Bitmap b(120, 48);
    b.poly({{0, 46}, {18, 28}, {34, 34}, {58, 12}, {78, 26}, {96, 16}, {120, 40}, {120, 48}, {0, 48}}, 1);
    b.poly({{40, 30}, {58, 16}, {74, 28}, {52, 34}}, 2);
    b.rect(0, 42, 120, 6, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(36, 36);
    b.ellipse(18, 18, 8, 8, 4);
    b.ellipse(18, 18, 5, 5, 3);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785f;
        b.line(18 + std::cos(a) * 11.f, 18 + std::sin(a) * 11.f, 18 + std::cos(a) * 16.f, 18 + std::sin(a) * 16.f, 3,
               1.6f);
    }
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 80);
    b.rect(6, 4, 4, 72, 3);
    b.rect(5, 6, 1, 68, 1);
    b.rect(2, 72, 12, 4, 4);
    b.rect(4, 2, 8, 5, 2);
    return b;
}

Bitmap buntingArt() {
    Bitmap b(48, 14);
    for (int x = 0; x < 48; x++) b.set(x, 1, 4);
    for (int i = 0; i < 8; i++) {
        int c = (i & 1) ? 2 : 1;
        b.poly({{float(i * 6), 2}, {float(i * 6 + 6), 2}, {float(i * 6 + 3), 12}}, c);
    }
    return b;
}

Bitmap lampArt() {
    Bitmap b(12, 12);
    b.ellipse(6, 6, 4, 4, 1);
    b.ellipse(6, 6, 2, 2, 2);
    return b;
}

Bitmap puffArt() {
    Bitmap b(20, 12);
    b.ellipse(10, 7, 8, 4, 1);
    b.ellipse(6, 6, 3, 2.4f, 2);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(36, 12);
    b.ellipse(18, 6, 16, 4, 1);
    return b;
}

Bitmap gullArt(int frame) {
    Bitmap b(32, 16);
    float dip = frame ? 4.f : 1.f;
    b.line(2, 8 + dip, 16, 8, 1, 1.6f);
    b.line(16, 8, 30, 8 + dip, 1, 1.6f);
    b.line(14, 8, 18, 10, 2, 1.4f);
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
                if (g && g[y * 5 + x]) {
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
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 7));

    setPal(vdp, PAL_SHIP, {0, gs::rgb4(14, 13, 10), gs::rgb4(15, 15, 13), gs::rgb4(6, 7, 9), gs::rgb4(13, 3, 2),
                           gs::rgb4(5, 12, 14), gs::rgb4(3, 2, 2), gs::rgb4(5, 4, 3), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(9, 6, 3), gs::rgb4(12, 9, 5), gs::rgb4(5, 3, 2), gs::rgb4(12, 12, 13),
                           gs::rgb4(4, 4, 5), gs::rgb4(6, 8, 4)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(12, 10, 7), gs::rgb4(14, 13, 9), gs::rgb4(7, 6, 5), gs::rgb4(9, 8, 7),
                            gs::rgb4(5, 6, 6), gs::rgb4(5, 7, 4)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 4, 8), gs::rgb4(3, 6, 11), gs::rgb4(5, 9, 13), gs::rgb4(13, 14, 14),
                            gs::rgb4(8, 11, 13)});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(4, 9, 3), gs::rgb4(3, 6, 2), gs::rgb4(8, 10, 3), gs::rgb4(6, 4, 2),
                           gs::rgb4(7, 5, 3)});
    setPal(vdp, PAL_HOUSE, {0, gs::rgb4(13, 11, 8), gs::rgb4(10, 4, 3), gs::rgb4(15, 13, 6), gs::rgb4(5, 3, 2),
                            gs::rgb4(6, 4, 3), gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 14, 13), gs::rgb4(11, 10, 12), gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 12),
                          gs::rgb4(7, 6, 9), gs::rgb4(5, 4, 7)});
    setPal(vdp, PAL_TAPE, {0, gs::rgb4(15, 15, 14), gs::rgb4(13, 3, 3), gs::rgb4(11, 11, 12), gs::rgb4(14, 11, 4)});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(9, 9, 10), gs::rgb4(14, 14, 13), gs::rgb4(12, 8, 3), gs::rgb4(8, 6, 4)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(3, 4, 6), gs::rgb4(12, 12, 12), gs::rgb4(8, 9, 10)});

    loadFont(vdp, art);
    const float atts[5] = {0.30f, 0.15f, 0.f, -0.15f, -0.30f};
    for (int i = 0; i < 5; i++) art.wing[i] = drawWing(vdp, atts[i]);
    art.leaf = gs::uploadMipped(vdp, leafArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.lintel = gs::uploadMipped(vdp, lintelArt());
    art.sill = gs::uploadMipped(vdp, sillArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.deep = gs::uploadMipped(vdp, deepArt());
    art.grass = gs::uploadMipped(vdp, grassArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.willow = gs::uploadMipped(vdp, willowArt());
    art.cottage = gs::uploadMipped(vdp, cottageArt());
    art.heron = gs::uploadMipped(vdp, heronArt());
    for (int i = 0; i < 3; i++) art.sock[i] = gs::uploadMipped(vdp, sockArt(i));
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.bunting = gs::uploadMipped(vdp, buntingArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(0));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(1));
}

}  // namespace gliderlock
