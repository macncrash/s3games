#include "game/art.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace gboom {
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

Pt xform(float cgx, float cgy, float ang, float mx, float my, float ppm) {
    const float ca = std::cos(-ang), sa = std::sin(-ang);
    float dx = mx * ppm, dy = -my * ppm;
    return {cgx + dx * ca - dy * sa, cgy + dx * sa + dy * ca};
}

void polyM(Bitmap& b, float cgx, float cgy, float ang, float ppm, std::initializer_list<Pt> meters, int c) {
    const float ca = std::cos(-ang), sa = std::sin(-ang);
    std::vector<Pt> p;
    p.reserve(meters.size());
    for (const Pt& m : meters) {
        float dx = m.first * ppm, dy = -m.second * ppm;
        p.push_back({cgx + dx * ca - dy * sa, cgy + dx * sa + dy * ca});
    }
    b.poly(p, c);
}

void lineM(Bitmap& b, float cgx, float cgy, float ang, float ppm, float x0, float y0, float x1, float y1, int c,
           float thick) {
    Pt a = xform(cgx, cgy, ang, x0, y0, ppm);
    Pt d = xform(cgx, cgy, ang, x1, y1, ppm);
    b.line(a.first, a.second, d.first, d.second, c, thick);
}

Spr finishAt(gs::VDP& vdp, const Bitmap& b, float ax, float ay, float ppm) {
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
        Bitmap dot(2, 2);
        dot.set(0, 0, 1);
        Spr s;
        s.img = gs::uploadMipped(vdp, dot);
        s.ax = 0;
        s.ay = 0;
        s.ppm = ppm;
        return s;
    }
    Bitmap cropped(x1 - x0 + 1, y1 - y0 + 1);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) cropped.set(x - x0, y - y0, b.get(x, y));
    Spr s;
    s.img = gs::uploadMipped(vdp, cropped);
    s.ax = ax - float(x0);
    s.ay = ay - float(y0);
    s.ppm = ppm;
    return s;
}

Spr drawShip(gs::VDP& vdp, float att) {
    const float ppm = 16.f;
    const float cgx = 180.f, cgy = 160.f;
    Bitmap b(360, 280);
    polyM(b, cgx, cgy, att, ppm, {{-3.55f, -0.06f}, {-1.35f, -0.14f}, {-1.3f, 0.16f}, {-3.5f, 0.22f}}, 2);
    polyM(b, cgx, cgy, att, ppm, {{-3.48f, 0.14f}, {-3.12f, 1.78f}, {-2.52f, 0.18f}}, 9);
    polyM(b, cgx, cgy, att, ppm, {{-3.42f, 1.35f}, {-3.05f, 1.62f}, {-2.72f, 1.28f}, {-3.15f, 1.08f}}, 4);
    polyM(b, cgx, cgy, att, ppm, {{-3.4f, 0.02f}, {-2.15f, 0.16f}, {-2.2f, -0.1f}, {-3.42f, -0.16f}}, 1);
    polyM(b, cgx, cgy, att, ppm, {{-1.55f, -0.42f}, {2.2f, -0.34f}, {2.4f, 0.28f}, {-1.5f, 0.2f}}, 1);
    polyM(b, cgx, cgy, att, ppm, {{-1.2f, -0.08f}, {2.05f, 0.02f}, {2.05f, 0.12f}, {-1.2f, 0.04f}}, 4);
    polyM(b, cgx, cgy, att, ppm, {{2.05f, -0.22f}, {3.28f, 0.0f}, {3.12f, 0.16f}, {2.1f, 0.22f}}, 4);
    polyM(b, cgx, cgy, att, ppm, {{-0.95f, 0.4f}, {1.2f, 0.64f}, {1.28f, 0.32f}, {-0.85f, 0.16f}}, 3);
    polyM(b, cgx, cgy, att, ppm, {{-0.7f, 0.48f}, {0.85f, 0.58f}, {0.8f, 0.44f}, {-0.65f, 0.36f}}, 10);
    polyM(b, cgx, cgy, att, ppm, {{0.9f, 0.16f}, {1.2f, 0.48f}, {1.85f, 0.44f}, {2.05f, 0.16f}}, 5);
    polyM(b, cgx, cgy, att, ppm, {{1.25f, 0.28f}, {1.45f, 0.42f}, {1.75f, 0.36f}, {1.55f, 0.22f}}, 6);
    lineM(b, cgx, cgy, att, ppm, -0.35f, 0.28f, 0.15f, -0.32f, 9, 2.2f);
    lineM(b, cgx, cgy, att, ppm, 0.85f, 0.3f, 0.55f, -0.28f, 9, 2.2f);
    lineM(b, cgx, cgy, att, ppm, -0.15f, -0.4f, 1.35f, -0.58f, 7, 2.4f);
    lineM(b, cgx, cgy, att, ppm, -0.55f, -0.36f, -0.15f, -0.52f, 7, 2.f);
    b.outline(8, false);
    return finishAt(vdp, b, cgx, cgy, ppm);
}

Spr drawDrive(gs::VDP& vdp) {
    const float ppm = 14.f;
    int w = std::max(12, int(std::lround(kDriveHalf * 2.0 * ppm)));
    int h = std::max(12, int(std::lround(kDriveThick * ppm)));
    Bitmap b(w, h);
    auto log = [&](float cy, float ry, int body, int shade) {
        float inset = 3.f;
        b.ellipse(inset + ry, cy, ry, ry * 0.92f, shade);
        b.ellipse(w - inset - ry, cy, ry, ry * 0.92f, shade);
        b.rect(inset + ry, cy - ry * 0.92f, w - 2.f * (inset + ry), ry * 1.84f, body);
        b.ellipse(inset + ry, cy, ry * 0.45f, ry * 0.45f, 4);
        b.ellipse(w - inset - ry, cy, ry * 0.45f, ry * 0.45f, 4);
        b.ellipse(inset + ry, cy, ry * 0.18f, ry * 0.18f, 5);
        b.ellipse(w - inset - ry, cy, ry * 0.18f, ry * 0.18f, 5);
    };
    log(h * 0.72f, h * 0.22f, 2, 3);
    log(h * 0.40f, h * 0.20f, 1, 2);
    log(h * 0.58f, h * 0.16f, 1, 3);
    for (int i = 0; i < 4; i++) {
        float x = w * (0.18f + 0.2f * float(i));
        b.rect(x, h * 0.16f, 3.f, h * 0.7f, 6);
        b.rect(x + 1.f, h * 0.16f, 1.f, h * 0.7f, 7);
    }
    b.rect(w * 0.42f, h * 0.30f, w * 0.16f, h * 0.18f, 8);
    b.outline(9, false);
    return finishAt(vdp, b, w * 0.5f, h * 0.5f, float(h) / float(kDriveThick));
}

Spr drawStick(gs::VDP& vdp) {
    Bitmap b(28, 96);
    b.rect(10, 8, 8, 86, 6);
    b.rect(12, 10, 3, 80, 7);
    b.rect(8, 6, 12, 6, 8);
    b.rect(11, 28, 6, 4, 7);
    b.rect(11, 52, 6, 4, 7);
    b.poly({{18, 18}, {26, 10}, {26, 16}, {18, 24}}, 8);
    b.rect(17, 70, 8, 3, 9);
    b.rect(16, 78, 10, 3, 10);
    b.outline(12, false);
    Spr s = finishAt(vdp, b, 14.f, float(b.h), float(b.h) / float(kPostTop));
    return s;
}

Spr drawLog(gs::VDP& vdp) {
    Bitmap b(52, 22);
    b.ellipse(8, 11, 8, 8, 3);
    b.ellipse(44, 11, 8, 8, 3);
    b.rect(8, 3, 36, 16, 2);
    b.rect(8, 5, 36, 4, 1);
    b.ellipse(8, 11, 3, 3, 4);
    b.ellipse(44, 11, 3, 3, 4);
    b.rect(14, 9, 3, 5, 9);
    b.rect(34, 9, 3, 5, 10);
    b.outline(12, false);
    return finishAt(vdp, b, 26.f, 2.f, 22.f / 1.15f);
}

Spr drawSign(gs::VDP& vdp) {
    gs::TextStyle st{4, 3, 0, 0, 1};
    Bitmap word = gs::textBitmap("BOOM", st);
    Bitmap b(word.w + 16, word.h + 14);
    b.rect(0, 0, float(b.w), float(b.h), 2);
    b.rect(3, 3, float(b.w - 6), float(b.h - 6), 1);
    b.blit(word, 8, 7);
    b.rect(b.w * 0.5f - 1.f, float(b.h - 2), 2.f, 8.f, 2);
    b.outline(4, false);
    return finishAt(vdp, b, b.w * 0.5f, b.h * 0.5f, float(b.h) / 2.4f);
}

Spr drawMill(gs::VDP& vdp) {
    Bitmap b(78, 70);
    b.rect(8, 28, 52, 40, 1);
    b.rect(10, 30, 48, 8, 2);
    b.poly({{4, 30}, {34, 8}, {64, 30}}, 3);
    b.poly({{14, 30}, {34, 14}, {54, 30}}, 4);
    b.rect(18, 40, 12, 16, 5);
    b.rect(38, 42, 10, 10, 6);
    b.rect(62, 36, 8, 32, 7);
    b.ellipse(66, 34, 10, 10, 7);
    b.ellipse(66, 34, 3, 3, 8);
    b.line(66, 34, 74, 28, 8, 1.5f);
    b.line(66, 34, 58, 40, 8, 1.5f);
    b.outline(9, false);
    return finishAt(vdp, b, 40.f, float(b.h - 1), float(b.h) / 8.2f);
}

Spr drawPine(gs::VDP& vdp) {
    Bitmap b(40, 64);
    b.poly({{20, 2}, {36, 28}, {4, 28}}, 1);
    b.poly({{20, 16}, {38, 44}, {2, 44}}, 2);
    b.poly({{20, 30}, {36, 54}, {4, 54}}, 3);
    b.rect(17, 52, 6, 12, 4);
    b.outline(5, false);
    return finishAt(vdp, b, 20.f, float(b.h - 1), float(b.h) / 7.2f);
}

Spr drawReed(gs::VDP& vdp) {
    Bitmap b(24, 32);
    b.line(6, 30, 8, 6, 1, 1.4f);
    b.line(12, 30, 11, 4, 2, 1.4f);
    b.line(18, 30, 16, 10, 1, 1.4f);
    b.ellipse(8, 6, 3, 2, 3);
    b.ellipse(11, 4, 3, 2, 3);
    return finishAt(vdp, b, 12.f, float(b.h - 1), float(b.h) / 2.4f);
}

Spr drawCloud(gs::VDP& vdp) {
    Bitmap b(72, 28);
    b.ellipse(18, 16, 14, 8, 1);
    b.ellipse(36, 12, 18, 10, 1);
    b.ellipse(54, 16, 14, 8, 1);
    b.ellipse(34, 16, 14, 6, 2);
    return finishAt(vdp, b, 36.f, 14.f, 28.f / 3.2f);
}

Spr drawSun(gs::VDP& vdp) {
    Bitmap b(36, 36);
    b.ellipse(18, 18, 8, 8, 1);
    b.ellipse(18, 18, 5, 5, 2);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785398f;
        b.line(18 + std::cos(a) * 11, 18 + std::sin(a) * 11, 18 + std::cos(a) * 16, 18 + std::sin(a) * 16, 1, 1.6f);
    }
    return finishAt(vdp, b, 18.f, 18.f, 1.f);
}

Spr drawHill(gs::VDP& vdp) {
    Bitmap b(120, 48);
    b.poly({{0, 46}, {18, 26}, {40, 34}, {62, 10}, {88, 28}, {120, 18}, {120, 48}}, 1);
    b.poly({{40, 36}, {62, 16}, {84, 32}, {50, 42}}, 2);
    b.rect(0, 42, 120, 6, 3);
    return finishAt(vdp, b, 60.f, 40.f, 1.f);
}

Spr drawChev(gs::VDP& vdp) {
    Bitmap b(28, 16);
    b.poly({{2, 3}, {14, 13}, {26, 3}, {20, 3}, {14, 8}, {8, 3}}, 1);
    return finishAt(vdp, b, 14.f, 8.f, 16.f / 1.1f);
}

Spr drawSplash(gs::VDP& vdp) {
    Bitmap b(40, 28);
    b.ellipse(20, 18, 14, 6, 1);
    b.ellipse(10, 12, 4, 6, 2);
    b.ellipse(30, 10, 3, 7, 2);
    b.ellipse(20, 8, 3, 5, 3);
    b.ellipse(14, 16, 2, 2, 3);
    return finishAt(vdp, b, 20.f, 20.f, 28.f / 2.2f);
}

Spr drawShade(gs::VDP& vdp) {
    Bitmap b(36, 12);
    b.ellipse(18, 6, 16, 4, 5);
    return finishAt(vdp, b, 18.f, 6.f, 12.f / 1.2f);
}

Spr drawBird(gs::VDP& vdp, int frame) {
    Bitmap b(28, 16);
    float lift = frame ? 4.f : 1.f;
    b.poly({{2, 8 + lift}, {14, 9}, {14, 11}, {4, 12}}, 1);
    b.poly({{26, 8 + lift}, {14, 9}, {14, 11}, {24, 12}}, 1);
    b.ellipse(14, 10, 2, 2, 2);
    return finishAt(vdp, b, 14.f, 10.f, 16.f / 1.4f);
}

void paintTile(Bitmap& b, int kind, int frame) {
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x + frame * 17, y + kind * 13);
            int c = 1;
            if (kind == 0) {
                int wave = (x + frame * 5 + y / 3) % 11;
                c = (y < 3) ? 4 : (wave == 0 ? 3 : ((h & 7) == 0 ? 2 : 1));
                if (y > b.h - 3 && (h & 3) == 0) c = 2;
            } else {
                c = 1;
                if ((h & 15) == 0) c = 2;
                if ((h % 29) == 0) c = 3;
                if (y < 2 && (h & 3) == 0) c = 4;
                if ((x + y + frame) % 9 == 0) c = 5;
            }
            b.set(x, y, c);
        }
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
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 7));

    setPal(vdp, PAL_SHIP, {0, gs::rgb4(15, 12, 2), gs::rgb4(12, 8, 1), gs::rgb4(15, 15, 12), gs::rgb4(13, 3, 2),
                           gs::rgb4(6, 13, 15), gs::rgb4(2, 7, 11), gs::rgb4(5, 3, 2), gs::rgb4(2, 1, 1),
                           gs::rgb4(5, 6, 8), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_DRIVE, {0, gs::rgb4(13, 9, 4), gs::rgb4(10, 6, 2), gs::rgb4(7, 4, 2), gs::rgb4(14, 12, 8),
                            gs::rgb4(8, 5, 3), gs::rgb4(11, 8, 4), gs::rgb4(14, 12, 7), gs::rgb4(15, 13, 2),
                            gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_BOOM, {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(15, 14, 10),
                           gs::rgb4(12, 11, 7), gs::rgb4(14, 7, 2), gs::rgb4(15, 15, 13), gs::rgb4(15, 12, 3),
                           gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 9), gs::rgb4(4, 6, 3), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 6, 10), gs::rgb4(3, 8, 12), gs::rgb4(6, 12, 14), gs::rgb4(10, 14, 15),
                            gs::rgb4(1, 4, 8)});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(6, 10, 3), gs::rgb4(8, 12, 4), gs::rgb4(4, 7, 2), gs::rgb4(10, 13, 5),
                           gs::rgb4(3, 5, 2), gs::rgb4(9, 8, 4)});
    setPal(vdp, PAL_MILL, {0, gs::rgb4(12, 7, 4), gs::rgb4(14, 10, 6), gs::rgb4(8, 3, 2), gs::rgb4(11, 5, 3),
                           gs::rgb4(4, 6, 8), gs::rgb4(10, 12, 13), gs::rgb4(6, 4, 3), gs::rgb4(3, 2, 2),
                           gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(3, 8, 3), gs::rgb4(4, 10, 4), gs::rgb4(2, 6, 2), gs::rgb4(6, 4, 2),
                           gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(14, 14, 15), gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 12),
                          gs::rgb4(7, 8, 11), gs::rgb4(5, 6, 9)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(14, 12, 8), gs::rgb4(6, 4, 2), gs::rgb4(14, 3, 2), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_SPLASH, {0, gs::rgb4(12, 15, 15), gs::rgb4(15, 15, 15), gs::rgb4(8, 13, 15)});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(3, 3, 4), gs::rgb4(14, 10, 3)});

    loadFont(vdp, art);
    const float atts[5] = {-0.34f, -0.17f, 0.f, 0.17f, 0.34f};
    for (int i = 0; i < 5; i++) art.ship[i] = drawShip(vdp, atts[i]);
    art.drive = drawDrive(vdp);
    art.stick = drawStick(vdp);
    art.log = drawLog(vdp);
    art.sign = drawSign(vdp);
    art.mill = drawMill(vdp);
    art.pine = drawPine(vdp);
    art.reed = drawReed(vdp);
    art.cloud = drawCloud(vdp);
    art.sun = drawSun(vdp);
    art.hill = drawHill(vdp);
    art.chev = drawChev(vdp);
    art.splash = drawSplash(vdp);
    art.shade = drawShade(vdp);
    art.bird[0] = drawBird(vdp, 0);
    art.bird[1] = drawBird(vdp, 1);

    Bitmap plank(32, 8);
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 32; x++) plank.set(x, y, (y == 0 || y == 7) ? 5 : 4);
    art.plank = gs::uploadMipped(vdp, plank);

    for (int f = 0; f < 2; f++) {
        Bitmap water(40, 28);
        paintTile(water, 0, f);
        art.water[f] = gs::uploadMipped(vdp, water);
        Bitmap grass(40, 28);
        paintTile(grass, 1, f);
        art.grass[f] = gs::uploadMipped(vdp, grass);
    }
}

}  // namespace gboom
