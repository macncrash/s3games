#include "game/art.h"

#include <cmath>
#include <string>
#include <vector>

namespace glider {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

Pt rot(float cx, float cy, float x, float y, float a) {
    float dx = x - cx, dy = y - cy;
    float c = std::cos(a), s = std::sin(a);
    return {cx + dx * c - dy * s, cy + dx * s + dy * c};
}

void poly(Bitmap& b, float cx, float cy, float ang, std::initializer_list<Pt> pts, int c) {
    std::vector<Pt> r;
    r.reserve(pts.size());
    for (const Pt& p : pts) r.push_back(rot(cx, cy, p.first, p.second, ang));
    b.poly(r, c);
}

// Nose points right. Positive ang is clockwise (nose down on screen).
Bitmap sailplane(float ang) {
    Bitmap b(200, 140);
    const float cx = 100, cy = 70;
    poly(b, cx, cy, ang, {{28, 66}, {150, 58}, {168, 68}, {150, 80}, {28, 76}}, 1);
    poly(b, cx, cy, ang, {{40, 74}, {148, 76}, {160, 80}, {36, 80}}, 2);
    poly(b, cx, cy, ang, {{62, 52}, {128, 46}, {134, 60}, {64, 66}}, 1);
    poly(b, cx, cy, ang, {{64, 52}, {126, 46}, {120, 52}, {70, 56}}, 3);
    poly(b, cx, cy, ang, {{30, 68}, {48, 64}, {42, 28}, {24, 36}}, 1);
    poly(b, cx, cy, ang, {{16, 30}, {56, 26}, {56, 36}, {16, 40}}, 1);
    poly(b, cx, cy, ang, {{118, 56}, {156, 58}, {162, 68}, {140, 74}, {112, 68}}, 5);
    poly(b, cx, cy, ang, {{128, 58}, {150, 60}, {146, 66}, {124, 64}}, 6);
    poly(b, cx, cy, ang, {{146, 60}, {172, 66}, {168, 74}, {142, 72}}, 3);
    poly(b, cx, cy, ang, {{78, 78}, {92, 78}, {92, 90}, {78, 90}}, 7);
    poly(b, cx, cy, ang, {{48, 70}, {120, 66}, {118, 70}, {48, 74}}, 8);
    b.outline(7, false);
    return b;
}

Bitmap pineArt() {
    Bitmap b(48, 76);
    b.rect(21, 46, 6, 26, 4);
    b.rect(23, 48, 2, 22, 5);
    b.poly({{24, 2}, {46, 30}, {2, 30}}, 2);
    b.poly({{24, 16}, {48, 46}, {0, 46}}, 1);
    b.poly({{24, 30}, {44, 58}, {4, 58}}, 3);
    b.outline(6, false);
    return b;
}

Bitmap broadArt() {
    Bitmap b(56, 64);
    b.rect(25, 36, 6, 24, 4);
    b.ellipse(28, 28, 22, 16, 1);
    b.ellipse(20, 26, 12, 10, 2);
    b.ellipse(36, 24, 10, 9, 3);
    b.outline(6, false);
    return b;
}

Bitmap column(int kind) {
    Bitmap b(16, 64);
    for (int y = 0; y < 64; y++) {
        int c = 4;
        if (y < 3) c = 6;
        else if (y < 8) c = 1;
        else if (y < 14) c = (y & 1) ? 2 : 3;
        else if (y < 28) c = (y / 3 & 1) ? 4 : 5;
        else c = (y / 5 & 1) ? 5 : 4;
        if (kind == 1 && y > 6 && ((y * 3) % 11 == 0)) c = 5;
        if (kind == 2 && y < 12 && (y % 4) < 2) c = 2;
        for (int x = 0; x < 16; x++) {
            int k = c;
            if (kind == 0 && y > 16 && ((x * 5 + y * 3) % 17 == 0)) k = 6;
            if (kind == 1 && y > 10 && ((x + y) % 9 == 0)) k = 3;
            b.set(x, y, k);
        }
    }
    return b;
}

Bitmap cloudArt() {
    Bitmap b(88, 36);
    b.ellipse(28, 20, 22, 12, 1);
    b.ellipse(48, 16, 26, 14, 1);
    b.ellipse(66, 20, 16, 10, 2);
    b.ellipse(42, 14, 14, 8, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(40, 40);
    b.ellipse(20, 20, 10, 10, 3);
    b.ellipse(18, 18, 4, 4, 5);
    for (int i = 0; i < 8; i++) {
        float a = i * 6.2831853f / 8.f;
        b.line(20 + std::cos(a) * 13, 20 + std::sin(a) * 13, 20 + std::cos(a) * 18, 20 + std::sin(a) * 18, 4, 2);
    }
    return b;
}

Bitmap hillArt() {
    Bitmap b(120, 56);
    b.poly({{0, 56}, {0, 34}, {22, 22}, {48, 30}, {70, 10}, {96, 26}, {120, 18}, {120, 56}}, 1);
    b.poly({{8, 56}, {18, 36}, {40, 40}, {60, 24}, {88, 36}, {112, 28}, {112, 56}}, 2);
    return b;
}

Bitmap chevronArt() {
    Bitmap b(32, 28);
    b.line(3, 22, 16, 6, 1, 3);
    b.line(16, 6, 29, 22, 1, 3);
    b.line(7, 22, 16, 12, 2, 2);
    b.line(16, 12, 25, 22, 2, 2);
    return b;
}

Bitmap shadeArt() {
    Bitmap b(48, 16);
    b.ellipse(24, 8, 22, 6, 1);
    return b;
}

Bitmap puffArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 18, 12, 8, 2);
    b.ellipse(12, 16, 6, 5, 1);
    b.ellipse(20, 14, 5, 4, 3);
    return b;
}

Bitmap gateArt() {
    Bitmap b(48, 16);
    for (int x = 0; x < 48; x++) {
        int c = ((x / 6) & 1) ? 1 : 3;
        for (int y = 4; y < 12; y++) b.set(x, y, c);
    }
    return b;
}

Bitmap sockArt(int frame) {
    Bitmap b(48, 64);
    b.rect(8, 20, 3, 42, 2);
    b.ellipse(9, 18, 4, 4, 1);
    float tip = 18.f + frame * 10.f;
    float droop = 8.f + frame * 6.f;
    b.poly({{11, 22}, {tip, 18 + droop}, {tip - 4, 28 + droop}, {11, 30}}, 1);
    b.poly({{11, 24}, {tip - 6, 20 + droop}, {tip - 8, 26 + droop}, {11, 28}}, 3);
    b.outline(7, false);
    return b;
}

Bitmap barnArt() {
    Bitmap b(64, 52);
    b.rect(8, 22, 48, 26, 4);
    b.poly({{4, 24}, {32, 6}, {60, 24}}, 1);
    b.rect(28, 30, 10, 18, 5);
    b.rect(14, 28, 8, 7, 2);
    b.rect(42, 28, 8, 7, 2);
    b.outline(7, false);
    return b;
}

Bitmap birdArt(bool up) {
    Bitmap b(40, 20);
    if (up) {
        b.line(2, 16, 20, 6, 1, 2.4f);
        b.line(20, 6, 38, 16, 1, 2.4f);
    } else {
        b.line(2, 4, 20, 12, 1, 2.4f);
        b.line(20, 12, 38, 4, 1, 2.4f);
    }
    b.ellipse(20, 10, 3, 2, 2);
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
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 15));
    vdp.setColor(PAL_HUD * 16 + 2, gs::rgb4(10, 10, 12));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 6));

    setPal(vdp, PAL_SHIP, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 10, 12), gs::rgb4(15, 8, 2), gs::rgb4(12, 4, 1),
                           gs::rgb4(2, 5, 12), gs::rgb4(7, 13, 15), gs::rgb4(1, 1, 2), gs::rgb4(15, 13, 3)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 8, 2), gs::rgb4(3, 11, 3), gs::rgb4(6, 14, 5), gs::rgb4(8, 5, 2),
                           gs::rgb4(5, 3, 1), gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(4, 13, 3), gs::rgb4(3, 9, 2), gs::rgb4(2, 6, 2), gs::rgb4(8, 6, 3),
                            gs::rgb4(5, 4, 2), gs::rgb4(11, 10, 6)});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(9, 9, 10), gs::rgb4(5, 5, 6), gs::rgb4(11, 10, 9), gs::rgb4(4, 7, 3),
                           gs::rgb4(2, 2, 3), gs::rgb4(13, 12, 11)});
    setPal(vdp, PAL_FIELD, {0, gs::rgb4(12, 14, 4), gs::rgb4(14, 12, 3), gs::rgb4(8, 10, 3), gs::rgb4(10, 8, 3),
                            gs::rgb4(6, 5, 2), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), gs::rgb4(15, 13, 4), gs::rgb4(15, 9, 2),
                          gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_FAR, {0, gs::rgb4(4, 6, 9), gs::rgb4(3, 4, 7), gs::rgb4(6, 8, 10)});
    setPal(vdp, PAL_LIFT, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 13, 15), gs::rgb4(4, 8, 12)});
    setPal(vdp, PAL_PROP, {0, gs::rgb4(15, 7, 2), gs::rgb4(14, 14, 14), gs::rgb4(15, 12, 4), gs::rgb4(10, 6, 3),
                           gs::rgb4(4, 2, 1), gs::rgb4(6, 8, 10), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 4, 2)});

    loadFont(vdp, art);
    for (int i = 0; i < 5; i++) art.ship[i] = gs::uploadMipped(vdp, sailplane(-0.46f + i * 0.23f));
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.broad = gs::uploadMipped(vdp, broadArt());
    art.grass = gs::uploadMipped(vdp, column(0));
    art.rock = gs::uploadMipped(vdp, column(1));
    art.field = gs::uploadMipped(vdp, column(2));
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.chevron = gs::uploadMipped(vdp, chevronArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    for (int i = 0; i < 3; i++) art.sock[i] = gs::uploadMipped(vdp, sockArt(i));
    art.barn = gs::uploadMipped(vdp, barnArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(false));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(true));
}

}  // namespace glider
