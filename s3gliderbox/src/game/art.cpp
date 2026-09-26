#include "game/art.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace gbox {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(2, 1, 1));
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// ang is nose-up radians. Screen rotation is the opposite, y down.
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

// Nose to the right. CG is the origin in meters. +y is up.
Ship drawShip(gs::VDP& vdp, float att) {
    const float cgx = 120.f, cgy = 100.f;
    Bitmap b(250, 210);
    polyM(b, cgx, cgy, att, {{-4.45f, 0.05f}, {-2.55f, 0.35f}, {-2.7f, 3.35f}, {-4.25f, 3.15f}}, 1);
    polyM(b, cgx, cgy, att, {{-4.2f, 2.05f}, {-2.85f, 2.2f}, {-2.95f, 3.2f}, {-4.15f, 3.05f}}, 3);
    polyM(b, cgx, cgy, att, {{-4.3f, -0.22f}, {-0.2f, 0.05f}, {-0.15f, 0.38f}, {-4.35f, 0.12f}}, 4);
    polyM(b, cgx, cgy, att, {{-1.85f, 1.15f}, {2.25f, 1.55f}, {2.5f, 0.55f}, {-1.6f, 0.28f}}, 1);
    polyM(b, cgx, cgy, att, {{-1.55f, 0.55f}, {2.15f, 0.75f}, {2.25f, 0.35f}, {-1.45f, 0.18f}}, 2);
    polyM(b, cgx, cgy, att, {{1.7f, 1.35f}, {2.5f, 1.5f}, {2.45f, 0.55f}, {1.55f, 0.62f}}, 3);
    polyM(b, cgx, cgy, att, {{-1.25f, 0.35f}, {1.55f, 0.78f}, {2.2f, 0.28f}, {1.95f, -0.55f}, {-0.05f, -0.68f}, {-1.35f, -0.12f}}, 1);
    polyM(b, cgx, cgy, att, {{1.75f, 0.28f}, {4.15f, 0.05f}, {3.9f, -0.42f}, {1.55f, -0.22f}}, 3);
    polyM(b, cgx, cgy, att, {{0.2f, 0.62f}, {1.45f, 0.85f}, {1.6f, 0.32f}, {0.3f, 0.18f}}, 7);
    polyM(b, cgx, cgy, att, {{-0.1f, -0.62f}, {1.7f, -0.5f}, {1.6f, -1.15f}, {0.0f, -1.22f}}, 9);
    Pt head = xform(cgx, cgy, att, 0.85f, 0.28f);
    b.ellipse(head.first, head.second, 6.5f, 6.5f, 6);
    Pt a = xform(cgx, cgy, att, -1.2f, 0.95f);
    Pt c = xform(cgx, cgy, att, -0.55f, 0.1f);
    b.line(a.first, a.second, c.first, c.second, 4, 2.4f);
    a = xform(cgx, cgy, att, 1.55f, 1.15f);
    c = xform(cgx, cgy, att, 1.15f, 0.08f);
    b.line(a.first, a.second, c.first, c.second, 4, 2.4f);
    Pt wheel = xform(cgx, cgy, att, 0.9f, -1.15f);
    b.ellipse(wheel.first, wheel.second, 5.5f, 5.5f, 8);
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

Bitmap saltArt() {
    Bitmap b(48, 40);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x, y);
            int c = 1;
            if ((h & 15) == 0) c = 2;
            if ((h % 37) == 0) c = 3;
            if (y % 9 == 0 && (h & 3) == 0) c = 4;
            b.set(x, y, c);
        }
    return b;
}

Bitmap solidArt(int c) {
    Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, c);
    return b;
}

Bitmap postArt(int wave) {
    Bitmap b(36, 78);
    b.rect(15, 16, 5, 62, 4);
    b.rect(16, 18, 2, 58, 5);
    b.rect(13, 14, 9, 4, 3);
    float tip = 8.f + float(wave) * 3.f;
    b.poly({{18, 16}, {32, tip}, {30, tip + 8}, {18, 22}}, 1);
    b.poly({{18, 18}, {28, tip + 3}, {27, tip + 7}, {18, 21}}, 2);
    b.outline(6, false);
    return b;
}

Bitmap sockArt(int frame) {
    Bitmap b(40, 56);
    b.rect(18, 16, 3, 40, 4);
    b.ellipse(19, 14, 3, 3, 3);
    float droop = 6.f + float(frame) * 5.f;
    b.poly({{18, 16}, {36, 10 + droop}, {34, 18 + droop}, {18, 22}}, 1);
    b.poly({{20, 17}, {32, 13 + droop}, {30, 17 + droop}, {20, 21}}, 2);
    return b;
}

Bitmap mesaArt() {
    Bitmap b(96, 42);
    b.poly({{2, 40}, {18, 16}, {28, 22}, {46, 6}, {70, 18}, {84, 10}, {94, 40}}, 1);
    b.poly({{20, 28}, {46, 10}, {62, 22}, {30, 34}}, 2);
    b.rect(0, 36, 96, 6, 3);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(70, 28);
    b.ellipse(18, 16, 14, 8, 1);
    b.ellipse(36, 13, 18, 10, 1);
    b.ellipse(52, 16, 14, 8, 1);
    b.ellipse(34, 16, 16, 7, 2);
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

Bitmap bushArt() {
    Bitmap b(30, 22);
    b.ellipse(15, 13, 12, 8, 1);
    b.ellipse(10, 14, 6, 5, 2);
    b.ellipse(20, 12, 5, 4, 3);
    b.rect(14, 16, 2, 6, 4);
    return b;
}

Bitmap chevArt() {
    Bitmap b(28, 16);
    b.poly({{2, 13}, {14, 3}, {26, 13}, {20, 13}, {14, 8}, {8, 13}}, 1);
    return b;
}

Bitmap dustArt() {
    Bitmap b(24, 16);
    b.ellipse(12, 9, 10, 5, 1);
    b.ellipse(8, 8, 4, 3, 2);
    return b;
}

Bitmap shadeArt() {
    Bitmap b(36, 12);
    b.ellipse(18, 6, 16, 4, 1);
    return b;
}

Bitmap signArt() {
    gs::TextStyle st{5, 3, 0, 0, 1};
    Bitmap word = gs::textBitmap("BOX", st);
    Bitmap b(word.w + 16, word.h + 14);
    b.rect(0, 0, float(b.w), float(b.h), 2);
    b.rect(3, 3, float(b.w - 6), float(b.h - 6), 1);
    b.blit(word, 8, 7);
    b.outline(4, false);
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
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 7));

    setPal(vdp, PAL_SHIP, {0, gs::rgb4(15, 14, 11), gs::rgb4(8, 9, 11), gs::rgb4(13, 3, 2), gs::rgb4(9, 6, 2),
                           gs::rgb4(12, 8, 4), gs::rgb4(14, 12, 3), gs::rgb4(6, 13, 15), gs::rgb4(2, 2, 3),
                           gs::rgb4(5, 5, 6)});
    setPal(vdp, PAL_SALT, {0, gs::rgb4(13, 12, 8), gs::rgb4(11, 10, 7), gs::rgb4(8, 8, 6), gs::rgb4(14, 13, 10),
                           gs::rgb4(6, 7, 5)});
    setPal(vdp, PAL_PAD, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 13, 14), gs::rgb4(15, 8, 2), gs::rgb4(10, 11, 12)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(15, 8, 2), gs::rgb4(15, 13, 4), gs::rgb4(4, 3, 2), gs::rgb4(8, 5, 2),
                           gs::rgb4(12, 8, 4), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_FAR, {0, gs::rgb4(8, 6, 8), gs::rgb4(6, 5, 7), gs::rgb4(5, 4, 5), gs::rgb4(10, 8, 7)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(15, 14, 12), gs::rgb4(15, 12, 5), gs::rgb4(15, 9, 3)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(14, 13, 10), gs::rgb4(12, 11, 8)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(15, 14, 10), gs::rgb4(9, 5, 2), gs::rgb4(14, 4, 2), gs::rgb4(3, 2, 1)});

    loadFont(vdp, art);
    const float atts[5] = {0.40f, 0.20f, 0.f, -0.20f, -0.40f};
    for (int i = 0; i < 5; i++) art.ship[i] = drawShip(vdp, atts[i]);
    art.salt = gs::uploadMipped(vdp, saltArt());
    art.pad = gs::uploadMipped(vdp, solidArt(1));
    art.edge = gs::uploadMipped(vdp, solidArt(1));
    art.post[0] = gs::uploadMipped(vdp, postArt(0));
    art.post[1] = gs::uploadMipped(vdp, postArt(1));
    for (int i = 0; i < 3; i++) art.sock[i] = gs::uploadMipped(vdp, sockArt(i));
    art.mesa = gs::uploadMipped(vdp, mesaArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.bush = gs::uploadMipped(vdp, bushArt());
    art.chev = gs::uploadMipped(vdp, chevArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.sign = gs::uploadMipped(vdp, signArt());
}

}  // namespace gbox
