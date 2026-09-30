#include "game/art.h"

#include <cmath>
#include <string>

namespace heliturn {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Pt rot(float cx, float cy, float x, float y, float bank) {
    float dx = x - cx, dy = y - cy;
    float c = std::cos(bank), s = std::sin(bank);
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

void strut(Bitmap& b, float x0, float y0, float x1, float y1, int c, float bank, float cx, float cy) {
    Pt a = rot(cx, cy, x0, y0, bank);
    Pt d = rot(cx, cy, x1, y1, bank);
    b.line(a.first, a.second, d.first, d.second, c, 2.2f);
}

// Side view. Positive bank drops the nose toward the right of the frame.
Bitmap heliArt(float bank, int rotor) {
    Bitmap b(120, 84);
    const float cx = 58, cy = 46;
    strut(b, 22, 58, 78, 58, 5, bank, cx, cy);
    strut(b, 30, 62, 70, 62, 5, bank, cx, cy);
    strut(b, 28, 58, 28, 64, 3, bank, cx, cy);
    strut(b, 68, 58, 68, 64, 3, bank, cx, cy);
    quad(b, 34, 40, 40, 16, 1, bank, cx, cy);
    quad(b, 38, 36, 28, 8, 2, bank, cx, cy);
    blob(b, 58, 40, 10, 8, 6, bank, cx, cy);
    blob(b, 60, 39, 6, 5, 9, bank, cx, cy);
    strut(b, 34, 46, 12, 40, 2, bank, cx, cy);
    quad(b, 8, 34, 10, 14, 1, bank, cx, cy);
    strut(b, 10, 34, 16, 28, 4, bank, cx, cy);
    strut(b, 48, 40, 48, 22, 3, bank, cx, cy);
    if (rotor == 0) {
        blob(b, 48, 18, 40, 4, 7, bank, cx, cy);
        blob(b, 48, 18, 28, 2, 8, bank, cx, cy);
    } else {
        strut(b, 10, 18, 86, 16, 7, bank, cx, cy);
        strut(b, 18, 14, 78, 22, 8, bank, cx, cy);
        blob(b, 48, 18, 5, 5, 4, bank, cx, cy);
    }
    blob(b, 48, 20, 3, 3, 4, bank, cx, cy);
    b.outline(3, false);
    return b;
}

Bitmap wreckArt() {
    Bitmap b(120, 84);
    b.ellipse(70, 58, 22, 12, 1);
    b.ellipse(78, 52, 10, 8, 6);
    b.line(40, 62, 96, 50, 2, 4);
    b.line(30, 40, 100, 28, 7, 3);
    b.line(36, 70, 88, 66, 5, 3);
    b.ellipse(24, 36, 8, 8, 4);
    b.outline(3, false);
    return b;
}

Bitmap chevron(bool left) {
    Bitmap b(28, 22);
    if (left) {
        b.poly({{22, 2}, {8, 11}, {22, 20}, {18, 11}}, 1);
        b.poly({{16, 4}, {4, 11}, {16, 18}, {12, 11}}, 2);
    } else {
        b.poly({{6, 2}, {20, 11}, {6, 20}, {10, 11}}, 1);
        b.poly({{12, 4}, {24, 11}, {12, 18}, {16, 11}}, 2);
    }
    return b;
}

Bitmap ridgeArt() {
    Bitmap b(48, 64);
    b.poly({{4, 63}, {24, 4}, {44, 63}}, 1);
    b.poly({{14, 63}, {24, 18}, {34, 63}}, 2);
    return b;
}

Bitmap padArt() {
    Bitmap b(36, 18);
    b.rect(1, 1, 34, 16, 1);
    b.rect(4, 4, 28, 10, 2);
    b.rect(15, 5, 6, 8, 3);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(10, 12, 14), gs::rgb4(15, 12, 4), gs::rgb4(15, 4, 3), gs::rgb4(4, 14, 6),
                          gs::rgb4(8, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_HELI,
           {0, gs::rgb4(12, 4, 2), gs::rgb4(7, 2, 1), gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4),
            gs::rgb4(11, 14, 15), gs::rgb4(14, 14, 12), gs::rgb4(6, 7, 8), gs::rgb4(8, 12, 14), 0, 0, 0, 0, 0,
            shadow});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 6, 1), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TITLE, {0, gs::rgb4(15, 14, 10), gs::rgb4(15, 8, 2), gs::rgb4(4, 6, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RIDGE, {0, gs::rgb4(8, 5, 3), gs::rgb4(12, 8, 5), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PAD, {0, gs::rgb4(4, 8, 4), gs::rgb4(12, 12, 8), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    vdp.setFogColor(gs::rgb4(8, 10, 13));
    loadFont(vdp, art);
    const float banks[5] = {-0.70f, -0.32f, 0.f, 0.32f, 0.70f};
    for (int i = 0; i < 5; i++)
        for (int r = 0; r < 2; r++) art.heli[i][r] = gs::uploadMipped(vdp, heliArt(banks[i], r));
    art.wreck = gs::uploadMipped(vdp, wreckArt());
    art.chevL = gs::uploadMipped(vdp, chevron(true));
    art.chevR = gs::uploadMipped(vdp, chevron(false));
    art.ridge = gs::uploadMipped(vdp, ridgeArt());
    art.pad = gs::uploadMipped(vdp, padArt());
    gs::TextStyle big{3, 1, 2, 0, 1};
    gs::TextStyle sub{2, 2, 0, 0, 1};
    art.title = gs::uploadImage(vdp, gs::textBitmap("HELITURN", big));
    art.sub = gs::uploadImage(vdp, gs::textBitmap("THREE TURNS", sub));
}

}  // namespace heliturn
