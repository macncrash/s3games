#include "game/art.h"

#include <cmath>
#include <string>

namespace kartturn {
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

// Rear view of an open-wheel kart. Positive bank rolls the chassis to screen right.
Bitmap kartArt(float bank) {
    Bitmap b(88, 64);
    const float cx = 44, cy = 36;
    blob(b, 16, 46, 11, 14, 2, bank, cx, cy);
    blob(b, 72, 46, 11, 14, 2, bank, cx, cy);
    blob(b, 16, 46, 5, 7, 3, bank, cx, cy);
    blob(b, 72, 46, 5, 7, 3, bank, cx, cy);
    quad(b, 22, 38, 44, 14, 1, bank, cx, cy);
    quad(b, 26, 34, 36, 8, 4, bank, cx, cy);
    quad(b, 18, 28, 52, 6, 5, bank, cx, cy);
    quad(b, 30, 22, 28, 14, 6, bank, cx, cy);
    blob(b, 44, 20, 8, 8, 7, bank, cx, cy);
    blob(b, 46, 18, 4, 3, 8, bank, cx, cy);
    quad(b, 38, 30, 12, 4, 9, bank, cx, cy);
    b.outline(10, false);
    return b;
}

Bitmap tippedArt() {
    Bitmap b(88, 64);
    b.ellipse(22, 48, 12, 8, 2);
    b.ellipse(62, 30, 12, 8, 2);
    b.poly({{18, 40}, {70, 22}, {74, 34}, {22, 52}}, 1);
    b.poly({{28, 28}, {64, 16}, {66, 24}, {30, 36}}, 5);
    b.ellipse(50, 24, 8, 6, 7);
    b.ellipse(52, 22, 3, 2, 8);
    b.outline(10, false);
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

Bitmap gateArt() {
    Bitmap b(36, 48);
    b.rect(2, 6, 6, 40, 1);
    b.rect(28, 6, 6, 40, 1);
    b.rect(2, 2, 32, 8, 2);
    b.rect(8, 4, 20, 4, 3);
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
    setPal(vdp, PAL_KART,
           {0, gs::rgb4(15, 12, 1), gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 9), gs::rgb4(13, 8, 1), gs::rgb4(12, 2, 2),
            gs::rgb4(3, 5, 9), gs::rgb4(14, 12, 9), gs::rgb4(4, 10, 14), gs::rgb4(2, 2, 3), gs::rgb4(6, 4, 2), 0, 0, 0,
            0, shadow});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 6, 1), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TITLE, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 1), gs::rgb4(3, 5, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(14, 14, 15), gs::rgb4(2, 10, 4), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t field[16] = {
        0,
        gs::rgb4(3, 6, 2), gs::rgb4(2, 5, 2), gs::rgb4(3, 3, 3),
        gs::rgb4(5, 5, 5), gs::rgb4(3, 3, 4),
        gs::rgb4(13, 13, 12), gs::rgb4(8, 8, 8),
        gs::rgb4(2, 2, 2), gs::rgb4(6, 6, 5), gs::rgb4(9, 8, 4),
        gs::rgb4(2, 5, 9), gs::rgb4(1, 3, 6), gs::rgb4(8, 10, 12),
        gs::rgb4(14, 13, 8), gs::rgb4(10, 9, 6),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FIELD * 16 + i, field[i]);
    vdp.setFogColor(gs::rgb4(5, 7, 10));

    loadFont(vdp, art);
    const float banks[5] = {-0.55f, -0.26f, 0.f, 0.26f, 0.55f};
    for (int i = 0; i < 5; i++) art.kart[i] = gs::uploadMipped(vdp, kartArt(banks[i]));
    art.tipped = gs::uploadMipped(vdp, tippedArt());
    art.chevL = gs::uploadMipped(vdp, chevron(true));
    art.chevR = gs::uploadMipped(vdp, chevron(false));
    art.gate = gs::uploadMipped(vdp, gateArt());
    gs::TextStyle big{3, 1, 2, 0, 1};
    gs::TextStyle sub{2, 2, 0, 0, 1};
    art.title = gs::uploadImage(vdp, gs::textBitmap("KARTTURN", big));
    art.sub = gs::uploadImage(vdp, gs::textBitmap("THREE TURNS", sub));
}

}  // namespace kartturn
