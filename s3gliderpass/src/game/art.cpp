#include "game/art.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace gliderpass {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
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

Wing drawWing(gs::VDP& vdp, float att) {
    const float cgx = 130.f, cgy = 108.f;
    Bitmap b(260, 210);
    // Parasol ridge glider, side on. +x nose, +y up, origin at the CG.
    polyM(b, cgx, cgy, att,
          {{-3.55f, 0.08f}, {-3.15f, 0.32f}, {-1.1f, 0.40f}, {1.15f, 0.46f}, {2.7f, 0.34f}, {4.35f, 0.06f},
           {4.12f, -0.14f}, {2.15f, -0.40f}, {0.15f, -0.52f}, {-2.15f, -0.36f}, {-3.45f, -0.06f}},
          2);
    polyM(b, cgx, cgy, att, {{-3.2f, -0.48f}, {-0.2f, -0.62f}, {1.4f, -0.58f}, {1.1f, -0.46f}, {-2.4f, -0.42f}}, 3);
    polyM(b, cgx, cgy, att, {{2.85f, 0.22f}, {4.35f, 0.06f}, {4.12f, -0.14f}, {2.65f, -0.02f}}, 4);
    polyM(b, cgx, cgy, att, {{-0.35f, 1.22f}, {1.72f, 1.08f}, {1.88f, 0.78f}, {-0.48f, 0.90f}}, 1);
    polyM(b, cgx, cgy, att, {{-0.2f, 1.18f}, {1.55f, 1.06f}, {1.15f, 0.96f}, {-0.05f, 1.02f}}, 9);
    polyM(b, cgx, cgy, att, {{-3.72f, 0.12f}, {-3.22f, 0.22f}, {-3.05f, 2.28f}, {-3.48f, 2.48f}, {-3.78f, 2.18f}}, 3);
    polyM(b, cgx, cgy, att, {{-3.95f, 2.18f}, {-2.55f, 2.28f}, {-2.62f, 1.98f}, {-3.88f, 1.88f}}, 1);
    polyM(b, cgx, cgy, att, {{-3.7f, 2.12f}, {-2.85f, 2.18f}, {-2.9f, 2.05f}, {-3.65f, 1.98f}}, 4);
    Pt a = xform(cgx, cgy, att, 0.15f, 0.28f);
    Pt c = xform(cgx, cgy, att, 0.42f, 1.02f);
    b.line(a.first, a.second, c.first, c.second, 7, 2.2f);
    Pt d = xform(cgx, cgy, att, 1.05f, 0.22f);
    Pt e = xform(cgx, cgy, att, 0.72f, 0.98f);
    b.line(d.first, d.second, e.first, e.second, 7, 2.2f);
    Pt canopy = xform(cgx, cgy, att, 1.85f, 0.42f);
    b.ellipse(canopy.first, canopy.second, 15.f, 9.f, 5);
    Pt helm = xform(cgx, cgy, att, 1.72f, 0.48f);
    b.ellipse(helm.first, helm.second, 5.5f, 5.f, 10);
    Pt glint = xform(cgx, cgy, att, 2.05f, 0.55f);
    b.ellipse(glint.first, glint.second, 3.2f, 2.2f, 6);
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
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) cropped.set(x - x0, y - y0, b.get(x, y));
    Wing w;
    w.img = gs::uploadMipped(vdp, cropped);
    w.ax = cgx - float(x0);
    w.ay = cgy - float(y0);
    w.ppm = PPM;
    return w;
}

Bitmap rockArt() {
    Bitmap b(32, 24);
    b.rect(0, 0, 32, 24, 1);
    for (int row = 0; row < 3; row++) {
        int y = row * 8;
        int shift = (row & 1) ? 8 : 0;
        for (int x = -8; x < 32; x += 16) {
            int tone = ((x + row * 3) & 2) ? 3 : 1;
            b.rect(x + shift, y, 15, 7, tone);
            b.rect(x + shift + 2, y + 1, 6, 3, 2);
            if ((hash2(x, row) % 5) == 0) b.rect(x + shift + 8, y + 3, 4, 2, 4);
        }
        b.rect(0, y + 7, 32, 1, 3);
    }
    return b;
}

Bitmap snowArt() {
    Bitmap b(36, 16);
    b.rect(0, 6, 36, 10, 1);
    b.rect(0, 4, 36, 4, 2);
    for (int x = 0; x < 36; x += 4) {
        int h = 3 + int(hash2(x, 2) % 4);
        b.poly({{float(x), 6.f}, {float(x + 2), float(6 - h)}, {float(x + 4), 6.f}}, 1);
        if ((hash2(x, 8) % 3) == 0) b.set(x + 2, 8, 3);
    }
    return b;
}

Bitmap grassArt() {
    Bitmap b(40, 16);
    b.rect(0, 7, 40, 9, 4);
    b.rect(0, 5, 40, 3, 1);
    for (int x = 1; x < 40; x += 3) {
        int h = 3 + int(hash2(x, 5) % 5);
        b.line(float(x), 7, float(x + ((x & 1) ? 1 : -1)), float(7 - h), 2, 1.2f);
        if ((hash2(x, 9) % 6) == 0) b.set(x, 6 - h, 3);
    }
    return b;
}

Bitmap pineArt() {
    Bitmap b(40, 64);
    b.rect(17, 40, 6, 22, 4);
    b.poly({{20, 4}, {36, 28}, {4, 28}}, 1);
    b.poly({{20, 14}, {34, 40}, {6, 40}}, 2);
    b.poly({{20, 26}, {32, 50}, {8, 50}}, 1);
    b.poly({{20, 8}, {28, 22}, {20, 18}}, 3);
    b.outline(5, false);
    return b;
}

Bitmap tarnArt() {
    Bitmap b(48, 18);
    b.rect(0, 4, 48, 14, 1);
    b.rect(0, 2, 48, 5, 2);
    for (int i = 0; i < 5; i++) {
        b.ellipse(6.f + i * 9.f, 6.f, 4.f, 1.8f, 3);
        b.ellipse(3.f + i * 9.f, 12.f, 3.f, 1.2f, 4);
    }
    return b;
}

Bitmap deepArt() {
    Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.set(2, 5, 2);
    b.set(6, 2, 2);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(80, 32);
    b.ellipse(24, 18, 18, 10, 2);
    b.ellipse(44, 15, 22, 12, 1);
    b.ellipse(60, 18, 14, 8, 2);
    b.ellipse(36, 14, 10, 6, 3);
    return b;
}

Bitmap nimbusArt() {
    Bitmap b(96, 48);
    b.ellipse(30, 26, 26, 16, 1);
    b.ellipse(58, 22, 30, 18, 2);
    b.ellipse(78, 28, 16, 12, 1);
    b.ellipse(48, 18, 14, 8, 3);
    b.rect(22, 34, 8, 10, 4);
    b.rect(48, 36, 6, 12, 4);
    b.rect(70, 34, 5, 8, 4);
    return b;
}

Bitmap wallArt() {
    Bitmap b(28, 96);
    for (int y = 0; y < 96; y++) {
        int tone = 1 + (y / 18) % 3;
        for (int x = 0; x < 28; x++)
            if ((hash2(x, y) % 5) != 0) b.set(x, y, tone);
    }
    for (int i = 0; i < 8; i++) {
        int x = 4 + int(hash2(i, 3) % 16);
        b.line(float(x), 8.f + i * 10.f, float(x - 3), 28.f + i * 10.f, 4, 1.4f);
    }
    return b;
}

Bitmap rainArt() {
    Bitmap b(48, 64);
    for (int i = 0; i < 14; i++) {
        float x = float(hash2(i, 1) % 40) + 4.f;
        float y = float(hash2(i, 2) % 40);
        b.line(x, y, x - 3.f, y + 12.f, 1, 1.1f);
    }
    return b;
}

Bitmap peakArt() {
    Bitmap b(96, 48);
    b.poly({{4, 46}, {28, 10}, {48, 46}}, 1);
    b.poly({{30, 46}, {58, 6}, {90, 46}}, 2);
    b.poly({{24, 18}, {28, 10}, {34, 20}}, 3);
    b.poly({{52, 16}, {58, 6}, {66, 18}}, 3);
    return b;
}

Bitmap sunArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 8, 8, 1);
    b.ellipse(14, 14, 3, 3, 2);
    for (int i = 0; i < 8; i++) {
        float a = i * 6.2831853f / 8.f;
        b.line(16 + std::cos(a) * 10.f, 16 + std::sin(a) * 10.f, 16 + std::cos(a) * 14.f, 16 + std::sin(a) * 14.f, 1, 1.6f);
    }
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 72);
    b.rect(5, 2, 6, 66, 1);
    b.rect(6, 4, 2, 62, 2);
    b.rect(3, 0, 10, 6, 3);
    for (int y = 10; y < 64; y += 8) b.rect(5, y, 6, 2, 3);
    b.outline(4, false);
    return b;
}

Bitmap tapeArt() {
    Bitmap b(48, 12);
    for (int x = 0; x < 48; x += 8) b.rect(x, 1, 8, 10, (x / 8) & 1 ? 1 : 2);
    b.rect(0, 0, 48, 2, 3);
    b.rect(0, 10, 48, 2, 3);
    return b;
}

Bitmap hutArt() {
    Bitmap b(56, 40);
    b.rect(8, 18, 40, 18, 1);
    b.poly({{4, 20}, {28, 4}, {52, 20}}, 2);
    b.rect(24, 24, 10, 12, 3);
    b.rect(12, 22, 8, 7, 4);
    b.rect(36, 22, 8, 7, 5);
    b.rect(38, 24, 3, 3, 6);
    b.outline(3, false);
    return b;
}

Bitmap cairnArt() {
    Bitmap b(28, 36);
    b.ellipse(14, 28, 12, 5, 1);
    b.rect(6, 18, 16, 10, 2);
    b.rect(9, 10, 11, 10, 1);
    b.rect(11, 4, 7, 8, 3);
    b.rect(13, 1, 3, 5, 2);
    b.outline(4, false);
    return b;
}

Bitmap sockArt() {
    Bitmap b(20, 40);
    b.rect(8, 14, 4, 24, 1);
    b.poly({{4, 16}, {16, 8}, {16, 18}, {4, 22}}, 2);
    b.poly({{6, 16}, {14, 11}, {14, 16}}, 3);
    b.outline(4, false);
    return b;
}

Bitmap pennantArt() {
    Bitmap b(28, 20);
    b.rect(2, 2, 3, 16, 1);
    b.poly({{5, 3}, {24, 8}, {5, 13}}, 2);
    b.poly({{5, 5}, {16, 8}, {5, 11}}, 3);
    return b;
}

Bitmap puffArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 12, 10, 1);
    b.ellipse(14, 14, 7, 6, 2);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(40, 14);
    b.ellipse(20, 7, 16, 5, 1);
    return b;
}

Bitmap gullArt(bool up) {
    Bitmap b(36, 16);
    if (up) {
        b.poly({{2, 10}, {16, 6}, {18, 8}, {4, 13}}, 1);
        b.poly({{34, 10}, {18, 6}, {16, 8}, {32, 13}}, 1);
    } else {
        b.poly({{2, 4}, {16, 8}, {18, 6}, {6, 12}}, 1);
        b.poly({{34, 4}, {18, 8}, {16, 6}, {30, 12}}, 1);
    }
    b.ellipse(17, 8, 2.2f, 1.6f, 2);
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
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 15));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 7));

    setPal(vdp, PAL_SHIP, {0, gs::rgb4(15, 15, 13), gs::rgb4(12, 13, 14), gs::rgb4(6, 8, 10), gs::rgb4(13, 2, 2),
                           gs::rgb4(2, 6, 12), gs::rgb4(10, 14, 15), gs::rgb4(3, 2, 2), shadow, gs::rgb4(15, 8, 2),
                           gs::rgb4(12, 8, 5), 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(7, 7, 8), gs::rgb4(10, 10, 11), gs::rgb4(4, 4, 5), gs::rgb4(5, 7, 4),
                           gs::rgb4(3, 3, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(14, 15, 15), gs::rgb4(11, 13, 15), gs::rgb4(8, 10, 13), 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, shadow});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(2, 6, 3), gs::rgb4(3, 8, 4), gs::rgb4(6, 10, 5), gs::rgb4(6, 4, 2),
                           gs::rgb4(1, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STORM, {0, gs::rgb4(3, 3, 6), gs::rgb4(5, 5, 8), gs::rgb4(8, 8, 11), gs::rgb4(12, 12, 14),
                            gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TAPE, {0, gs::rgb4(8, 5, 3), gs::rgb4(12, 9, 6), gs::rgb4(4, 3, 2), gs::rgb4(2, 1, 1),
                           gs::rgb4(14, 3, 3), gs::rgb4(15, 15, 15), gs::rgb4(6, 1, 1), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_HUT, {0, gs::rgb4(9, 6, 4), gs::rgb4(8, 2, 2), gs::rgb4(3, 2, 2), gs::rgb4(4, 7, 10),
                          gs::rgb4(15, 12, 5), gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 12), gs::rgb4(14, 15, 15), gs::rgb4(6, 7, 9),
                          gs::rgb4(4, 5, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(14, 14, 15), gs::rgb4(4, 4, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(13, 13, 14), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 5, 9), gs::rgb4(4, 8, 12), gs::rgb4(8, 12, 14), gs::rgb4(3, 6, 10), 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(5, 3, 2), gs::rgb4(14, 3, 2), gs::rgb4(15, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, shadow});

    const float poses[] = {0.30f, 0.12f, 0.f, -0.12f, -0.30f};
    for (int i = 0; i < 5; i++) art.wing[i] = drawWing(vdp, poses[i]);
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.snow = gs::uploadMipped(vdp, snowArt());
    art.grass = gs::uploadMipped(vdp, grassArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.tarn = gs::uploadMipped(vdp, tarnArt());
    art.deep = gs::uploadMipped(vdp, deepArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.nimbus = gs::uploadMipped(vdp, nimbusArt());
    art.wall = gs::uploadMipped(vdp, wallArt());
    art.rain = gs::uploadMipped(vdp, rainArt());
    art.peak = gs::uploadMipped(vdp, peakArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.tape = gs::uploadMipped(vdp, tapeArt());
    art.hut = gs::uploadMipped(vdp, hutArt());
    art.cairn = gs::uploadMipped(vdp, cairnArt());
    art.sock = gs::uploadMipped(vdp, sockArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(false));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(true));
    loadFont(vdp, art);
}

}  // namespace gliderpass
