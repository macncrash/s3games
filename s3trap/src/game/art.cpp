#include "game/art.h"

#include <cmath>
#include <string>

namespace trap {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Pt rot(float cx, float cy, float x, float y, float bank) {
    float dx = x - cx, dy = y - cy, c = std::cos(bank), s = std::sin(bank);
    return {cx + dx * c - dy * s, cy + dx * s + dy * c};
}

void quad(Bitmap& b, float x, float y, float w, float h, int c, float bank, float cx, float cy) {
    b.poly({rot(cx, cy, x, y, bank), rot(cx, cy, x + w, y, bank), rot(cx, cy, x + w, y + h, bank),
            rot(cx, cy, x, y + h, bank)},
           c);
}

void blob(Bitmap& b, float x, float y, float rx, float ry, int c, float bank, float cx, float cy) {
    Pt p = rot(cx, cy, x, y, bank);
    b.ellipse(p.first, p.second, rx, ry, c);
}

Bitmap jetRear(float bank) {
    Bitmap b(110, 140);
    const float cx = 55, cy = 70;
    quad(b, 46, 18, 18, 96, 3, bank, cx, cy);
    quad(b, 48, 16, 14, 92, 2, bank, cx, cy);
    quad(b, 8, 48, 94, 16, 3, bank, cx, cy);
    b.poly({rot(cx, cy, 10, 46, bank), rot(cx, cy, 48, 40, bank), rot(cx, cy, 50, 62, bank), rot(cx, cy, 14, 66, bank)}, 1);
    b.poly({rot(cx, cy, 100, 46, bank), rot(cx, cy, 62, 40, bank), rot(cx, cy, 60, 62, bank), rot(cx, cy, 96, 66, bank)}, 1);
    quad(b, 18, 102, 16, 22, 3, bank, cx, cy);
    quad(b, 76, 102, 16, 22, 2, bank, cx, cy);
    quad(b, 50, 96, 10, 28, 5, bank, cx, cy);
    blob(b, 55, 28, 8, 10, 7, bank, cx, cy);
    blob(b, 55, 26, 5, 6, 6, bank, cx, cy);
    blob(b, 42, 108, 5, 4, 9, bank, cx, cy);
    blob(b, 68, 108, 5, 4, 9, bank, cx, cy);
    quad(b, 40, 118, 4, 10, 5, bank, cx, cy);
    quad(b, 66, 118, 4, 10, 5, bank, cx, cy);
    quad(b, 52, 124, 6, 12, 5, bank, cx, cy);
    b.line(rot(cx, cy, 55, 118, bank).first, rot(cx, cy, 55, 118, bank).second, rot(cx, cy, 62, 136, bank).first,
           rot(cx, cy, 62, 136, bank).second, 5, 2.0f);
    b.outline(5, false);
    return b;
}

Bitmap islandArt() {
    Bitmap b(48, 110);
    b.rect(8, 36, 32, 72, 2);
    b.rect(10, 38, 28, 68, 3);
    b.rect(4, 28, 40, 12, 2);
    b.ellipse(24, 18, 10, 6, 4);
    b.rect(22, 8, 4, 14, 5);
    for (int y = 48; y < 96; y += 10)
        for (int x = 14; x < 34; x += 8) b.rect(x, y, 4, 5, (x + y) % 16 ? 6 : 1);
    b.outline(5, false);
    return b;
}

Bitmap wireArt() {
    Bitmap b(80, 8);
    b.rect(0, 2, 80, 2, 3);
    b.rect(0, 5, 80, 2, 1);
    return b;
}

Bitmap mastArt() {
    Bitmap b(16, 48);
    b.rect(6, 10, 4, 36, 2);
    b.rect(2, 8, 12, 6, 1);
    b.ellipse(8, 8, 3, 3, 4);
    return b;
}

Bitmap buildingArt() {
    Bitmap b(40, 96);
    b.rect(4, 16, 32, 78, 2);
    b.rect(6, 18, 28, 74, 3);
    for (int y = 24; y < 84; y += 10)
        for (int x = 10; x < 32; x += 8) b.rect(x, y, 4, 5, (y / 10 + x) % 3 ? 1 : 6);
    b.outline(5, false);
    return b;
}

Bitmap wallArt() {
    Bitmap b(36, 120);
    b.poly({{4, 118}, {10, 8}, {28, 4}, {32, 118}}, 2);
    b.poly({{8, 110}, {14, 20}, {24, 16}, {26, 110}}, 3);
    b.outline(5, false);
    return b;
}

Bitmap boxcarArt() {
    Bitmap b(120, 40);
    b.rect(4, 6, 112, 22, 2);
    b.rect(8, 8, 104, 16, 3);
    b.rect(16, 12, 18, 8, 1);
    b.rect(52, 12, 18, 8, 1);
    b.rect(86, 12, 18, 8, 1);
    b.ellipse(24, 32, 6, 6, 5);
    b.ellipse(96, 32, 6, 6, 5);
    b.outline(5, false);
    return b;
}

Bitmap derrickArt() {
    Bitmap b(48, 110);
    b.line(8, 108, 24, 8, 2, 3);
    b.line(40, 108, 24, 8, 2, 3);
    b.line(8, 108, 40, 108, 2, 3);
    for (int y = 20; y < 100; y += 16) b.line(10 + (y / 16), float(y), 38 - (y / 16), float(y), 3, 2);
    b.rect(20, 4, 8, 8, 4);
    return b;
}

Bitmap bridgeArt() {
    Bitmap b(140, 48);
    b.rect(0, 8, 140, 8, 2);
    b.rect(8, 16, 8, 30, 3);
    b.rect(124, 16, 8, 30, 3);
    b.rect(4, 4, 132, 4, 1);
    return b;
}

Bitmap iceArt() {
    Bitmap b(48, 36);
    b.poly({{4, 30}, {16, 6}, {32, 10}, {44, 28}}, 1);
    b.poly({{10, 28}, {18, 12}, {28, 14}, {34, 26}}, 3);
    return b;
}

Bitmap spireArt() {
    Bitmap b(28, 80);
    b.poly({{14, 4}, {4, 76}, {24, 76}}, 2);
    b.poly({{14, 12}, {10, 70}, {18, 70}}, 4);
    return b;
}

Bitmap poleArt() {
    Bitmap b(10, 48);
    b.rect(4, 8, 2, 40, 2);
    b.ellipse(5, 6, 3, 3, 1);
    return b;
}

Bitmap chevArt(bool up) {
    Bitmap b(18, 14);
    if (up) {
        b.line(2, 12, 9, 2, 1, 2);
        b.line(16, 12, 9, 2, 1, 2);
    } else {
        b.line(2, 2, 9, 12, 1, 2);
        b.line(16, 2, 9, 12, 1, 2);
    }
    return b;
}

Bitmap donutArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 1);
    b.ellipse(8, 8, 3, 3, 0);
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
    const uint16_t ink = gs::rgb4(15, 15, 15);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(9, 10, 12), ink, gs::rgb4(15, 4, 3), gs::rgb4(4, 14, 6), gs::rgb4(15, 12, 2),
                          gs::rgb4(3, 8, 15), gs::rgb4(4, 4, 6), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 2), gs::rgb4(12, 8, 1), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 3, 2), gs::rgb4(8, 1, 1), gs::rgb4(15, 12, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(5, 15, 6), gs::rgb4(2, 9, 3), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_JET, {0, gs::rgb4(11, 12, 13), gs::rgb4(7, 8, 9), gs::rgb4(4, 5, 6), gs::rgb4(14, 14, 15), gs::rgb4(1, 1, 1),
                          gs::rgb4(15, 15, 15), gs::rgb4(6, 10, 14), gs::rgb4(2, 3, 4), gs::rgb4(15, 8, 2), gs::rgb4(12, 10, 4), 0,
                          0, 0, 0, shadow});
    setPal(vdp, PAL_SEA, {0, gs::rgb4(14, 14, 12), gs::rgb4(8, 8, 9), gs::rgb4(5, 6, 7), gs::rgb4(15, 12, 2), gs::rgb4(2, 2, 2),
                          gs::rgb4(12, 12, 13), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CITY, {0, gs::rgb4(15, 13, 6), gs::rgb4(6, 6, 8), gs::rgb4(3, 3, 5), gs::rgb4(12, 4, 2), gs::rgb4(9, 9, 10),
                           gs::rgb4(1, 1, 1), gs::rgb4(4, 6, 10), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 13, 3), gs::rgb4(15, 7, 1), gs::rgb4(15, 15, 15), gs::rgb4(14, 3, 2), gs::rgb4(1, 1, 1),
                         gs::rgb4(10, 12, 14), gs::rgb4(6, 7, 8), 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    for (int i = 0; i < 3; i++) art.jet[i] = gs::uploadMipped(vdp, jetRear(i * 0.38f));
    Bitmap flame(16, 28);
    flame.poly({{8, 2}, {2, 26}, {14, 26}}, 2);
    flame.poly({{8, 8}, {5, 24}, {11, 24}}, 1);
    art.flame = gs::uploadMipped(vdp, flame);
    art.island = gs::uploadMipped(vdp, islandArt());
    art.wire = gs::uploadMipped(vdp, wireArt());
    art.mast = gs::uploadMipped(vdp, mastArt());
    art.building = gs::uploadMipped(vdp, buildingArt());
    art.wall = gs::uploadMipped(vdp, wallArt());
    art.boxcar = gs::uploadMipped(vdp, boxcarArt());
    art.derrick = gs::uploadMipped(vdp, derrickArt());
    art.bridge = gs::uploadMipped(vdp, bridgeArt());
    art.ice = gs::uploadMipped(vdp, iceArt());
    art.spire = gs::uploadMipped(vdp, spireArt());
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.chev = gs::uploadMipped(vdp, chevArt(true));
    art.tri = gs::uploadMipped(vdp, chevArt(false));
    art.donut = gs::uploadMipped(vdp, donutArt());
    Bitmap ball(12, 12);
    ball.ellipse(6, 6, 5, 5, 1);
    art.ball = gs::uploadMipped(vdp, ball);
    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace trap
