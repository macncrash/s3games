#include "game/art.h"

#include <cmath>
#include <string>

namespace keel {
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
    b.line(a.first, a.second, d.first, d.second, c, 2.0f);
}

// Chase view of a keelboat. The fin under the hull is the keel.
Bitmap boatArt(float bank) {
    Bitmap b(96, 112);
    const float cx = 48, cy = 70;
    blob(b, 48, 78, 22, 6, 8, bank, cx, cy);
    quad(b, 28, 70, 40, 14, 3, bank, cx, cy);
    quad(b, 30, 72, 36, 8, 2, bank, cx, cy);
    b.poly({rot(cx, cy, 30, 72, bank), rot(cx, cy, 24, 84, bank), rot(cx, cy, 34, 84, bank), rot(cx, cy, 40, 74, bank)}, 3);
    b.poly({rot(cx, cy, 66, 72, bank), rot(cx, cy, 74, 82, bank), rot(cx, cy, 62, 84, bank), rot(cx, cy, 56, 74, bank)}, 2);
    quad(b, 44, 78, 8, 22, 4, bank, cx, cy);
    quad(b, 46, 82, 4, 16, 5, bank, cx, cy);
    strut(b, 48, 74, 48, 18, 5, bank, cx, cy);
    b.poly({rot(cx, cy, 48, 20, bank), rot(cx, cy, 78, 58, bank), rot(cx, cy, 48, 66, bank), rot(cx, cy, 46, 40, bank)}, 1);
    b.poly({rot(cx, cy, 48, 24, bank), rot(cx, cy, 70, 56, bank), rot(cx, cy, 48, 62, bank)}, 6);
    b.poly({rot(cx, cy, 48, 28, bank), rot(cx, cy, 22, 64, bank), rot(cx, cy, 48, 68, bank)}, 7);
    blob(b, 40, 66, 4, 3, 9, bank, cx, cy);
    blob(b, 52, 64, 3, 2.4f, 9, bank, cx, cy);
    strut(b, 48, 18, 62, 14, 6, bank, cx, cy);
    quad(b, 60, 10, 10, 6, 6, bank, cx, cy);
    b.outline(5, false);
    return b;
}

Bitmap buoyArt() {
    Bitmap b(40, 64);
    b.ellipse(20, 46, 12, 5, 4);
    b.poly({{20, 8}, {30, 40}, {10, 40}}, 1);
    b.poly({{20, 16}, {27, 40}, {13, 40}}, 2);
    b.rect(16, 22, 8, 6, 3);
    b.rect(17, 32, 6, 4, 3);
    b.line(20, 8, 20, 2, 5, 1.5f);
    b.rect(16, 0, 8, 4, 6);
    b.outline(5, false);
    return b;
}

Bitmap wakeArt() {
    Bitmap b(72, 28);
    b.ellipse(36, 16, 30, 8, 1);
    b.ellipse(28, 16, 12, 4, 2);
    b.ellipse(48, 15, 10, 3, 2);
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
    const uint16_t shadow = gs::rgb4(1, 2, 3);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(10, 12, 14), gs::rgb4(15, 14, 8), gs::rgb4(15, 5, 3), gs::rgb4(4, 14, 8),
                          gs::rgb4(15, 12, 4), gs::rgb4(6, 10, 15), gs::rgb4(6, 8, 9), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOAT,
           {0, gs::rgb4(15, 15, 13), gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2),
            gs::rgb4(14, 3, 3), gs::rgb4(4, 8, 14), gs::rgb4(6, 10, 12), gs::rgb4(15, 12, 6), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GHOST,
           {0, gs::rgb4(11, 13, 14), gs::rgb4(7, 8, 9), gs::rgb4(5, 6, 7), gs::rgb4(3, 4, 5), gs::rgb4(2, 2, 3),
            gs::rgb4(9, 4, 4), gs::rgb4(5, 7, 10), gs::rgb4(5, 8, 9), gs::rgb4(10, 10, 8), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MARK,
           {0, gs::rgb4(15, 3, 2), gs::rgb4(15, 14, 12), gs::rgb4(14, 14, 14), gs::rgb4(3, 6, 8), gs::rgb4(1, 1, 1),
            gs::rgb4(15, 12, 2), gs::rgb4(2, 6, 4), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CREW,
           {0, gs::rgb4(14, 14, 8), gs::rgb4(8, 6, 3), gs::rgb4(5, 4, 2), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 1),
            gs::rgb4(12, 2, 2), gs::rgb4(3, 5, 10), gs::rgb4(4, 7, 8), gs::rgb4(12, 10, 5), 0, 0, 0, 0, 0, shadow});

    const uint16_t field[16] = {
        0,
        gs::rgb4(6, 9, 4), gs::rgb4(4, 7, 3), gs::rgb4(8, 8, 4),
        gs::rgb4(12, 11, 6), gs::rgb4(9, 8, 4),
        gs::rgb4(4, 8, 10), gs::rgb4(3, 6, 8),
        gs::rgb4(10, 10, 8), gs::rgb4(7, 7, 5), gs::rgb4(5, 5, 4),
        gs::rgb4(3, 8, 12), gs::rgb4(2, 6, 10), gs::rgb4(8, 13, 15),
        gs::rgb4(14, 14, 12), gs::rgb4(11, 12, 12),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FIELD * 16 + i, field[i]);
    vdp.setFogColor(gs::rgb4(10, 12, 13));

    loadFont(vdp, art);
    for (int i = 0; i < 5; i++) {
        float bank = (i - 2) * 0.28f;
        art.boat[i] = gs::uploadMipped(vdp, boatArt(bank));
    }
    art.ghost = gs::uploadMipped(vdp, boatArt(0));
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.wake = gs::uploadMipped(vdp, wakeArt());
}

}  // namespace keel
