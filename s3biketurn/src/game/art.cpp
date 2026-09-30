#include "game/art.h"

#include <cmath>
#include <string>

namespace biketurn {
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

// Side view of a road bike. Positive bank leans the machine toward the right of the screen.
Bitmap bikeArt(float bank) {
    Bitmap b(96, 72);
    const float cx = 48, cy = 40;
    blob(b, 24, 50, 13, 13, 3, bank, cx, cy);
    blob(b, 72, 50, 13, 13, 3, bank, cx, cy);
    blob(b, 24, 50, 7, 7, 4, bank, cx, cy);
    blob(b, 72, 50, 7, 7, 4, bank, cx, cy);
    strut(b, 24, 50, 46, 36, 5, bank, cx, cy);
    strut(b, 72, 50, 50, 38, 5, bank, cx, cy);
    strut(b, 28, 50, 58, 28, 5, bank, cx, cy);
    quad(b, 34, 34, 28, 8, 1, bank, cx, cy);
    quad(b, 36, 32, 22, 6, 2, bank, cx, cy);
    quad(b, 58, 26, 16, 10, 1, bank, cx, cy);
    blob(b, 62, 22, 8, 7, 6, bank, cx, cy);
    blob(b, 60, 20, 5, 4, 9, bank, cx, cy);
    quad(b, 40, 18, 10, 16, 7, bank, cx, cy);
    blob(b, 44, 16, 6, 6, 8, bank, cx, cy);
    blob(b, 46, 15, 3, 2, 9, bank, cx, cy);
    quad(b, 46, 30, 8, 4, 6, bank, cx, cy);
    b.outline(5, false);
    return b;
}

Bitmap tumbleArt() {
    Bitmap b(96, 72);
    b.ellipse(30, 40, 14, 14, 3);
    b.ellipse(58, 28, 14, 14, 3);
    b.ellipse(30, 40, 6, 6, 4);
    b.ellipse(58, 28, 6, 6, 4);
    b.line(24, 34, 64, 48, 1, 4);
    b.line(40, 18, 22, 52, 2, 3);
    b.ellipse(48, 36, 8, 6, 7);
    b.ellipse(50, 34, 4, 3, 8);
    b.outline(5, false);
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
    setPal(vdp, PAL_BIKE,
           {0, gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(1, 1, 1), gs::rgb4(9, 9, 10), gs::rgb4(4, 4, 5),
            gs::rgb4(15, 13, 2), gs::rgb4(12, 8, 5), gs::rgb4(14, 11, 8), gs::rgb4(3, 8, 14), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 6, 1), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TITLE, {0, gs::rgb4(15, 14, 10), gs::rgb4(15, 8, 2), gs::rgb4(4, 6, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(6, 6, 6), gs::rgb4(10, 9, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t field[16] = {
        0,
        gs::rgb4(4, 7, 3), gs::rgb4(3, 6, 2), gs::rgb4(3, 3, 3),
        gs::rgb4(5, 5, 5), gs::rgb4(3, 3, 4),
        gs::rgb4(12, 12, 11), gs::rgb4(8, 8, 8),
        gs::rgb4(2, 2, 2), gs::rgb4(6, 6, 5), gs::rgb4(9, 8, 4),
        gs::rgb4(2, 5, 9), gs::rgb4(1, 3, 6), gs::rgb4(8, 10, 12),
        gs::rgb4(14, 13, 8), gs::rgb4(10, 9, 6),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FIELD * 16 + i, field[i]);
    vdp.setFogColor(gs::rgb4(6, 8, 11));

    loadFont(vdp, art);
    const float banks[5] = {-0.62f, -0.30f, 0.f, 0.30f, 0.62f};
    for (int i = 0; i < 5; i++) art.bike[i] = gs::uploadMipped(vdp, bikeArt(banks[i]));
    art.tumble = gs::uploadMipped(vdp, tumbleArt());
    art.chevL = gs::uploadMipped(vdp, chevron(true));
    art.chevR = gs::uploadMipped(vdp, chevron(false));
    gs::TextStyle big{3, 1, 2, 0, 1};
    gs::TextStyle sub{2, 2, 0, 0, 1};
    art.title = gs::uploadImage(vdp, gs::textBitmap("BIKETURN", big));
    art.sub = gs::uploadImage(vdp, gs::textBitmap("THREE TURNS", sub));
}

}  // namespace biketurn
