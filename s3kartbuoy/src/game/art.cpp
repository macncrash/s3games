#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace buoy {
namespace {

constexpr float PI = 3.14159265f;

void setPal(gs::VDP& v, int pal, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) v.setColor(pal * 16 + i++, c);
}

void fillTile(uint8_t* px, int c) {
    for (int i = 0; i < 64; i++) px[i] = uint8_t(c);
}

gs::Bitmap rotate(const gs::Bitmap& src, float ang) {
    gs::Bitmap d(src.w, src.h);
    const float c = std::cos(ang), s = std::sin(ang);
    const float cx = (src.w - 1) * 0.5f, cy = (src.h - 1) * 0.5f;
    for (int y = 0; y < d.h; y++) {
        for (int x = 0; x < d.w; x++) {
            float dx = float(x) - cx, dy = float(y) - cy;
            int sx = int(std::lround(c * dx + s * dy + cx));
            int sy = int(std::lround(-s * dx + c * dy + cy));
            int p = src.get(sx, sy);
            if (p) d.set(x, y, p);
        }
    }
    return d;
}

// Nose points east.
gs::Bitmap kartArt() {
    gs::Bitmap b(48, 48);
    b.ellipse(12, 12, 7, 5, 4);
    b.ellipse(36, 12, 7, 5, 4);
    b.ellipse(12, 36, 7, 5, 4);
    b.ellipse(36, 36, 7, 5, 4);
    b.ellipse(11, 12, 3, 2, 5);
    b.ellipse(35, 12, 3, 2, 5);
    b.ellipse(11, 36, 3, 2, 5);
    b.ellipse(35, 36, 3, 2, 5);
    b.poly({{8, 18}, {40, 16}, {44, 24}, {40, 32}, {8, 30}}, 1);
    b.ellipse(22, 24, 7, 6, 2);
    b.rect(30, 21, 8, 6, 3);
    b.rect(40, 22, 4, 4, 6);
    b.rect(14, 20, 3, 8, 5);
    return b;
}

gs::Bitmap buoyArt() {
    gs::Bitmap b(32, 48);
    b.rect(14, 22, 4, 20, 2);
    b.rect(12, 38, 8, 3, 4);
    b.ellipse(16, 16, 11, 11, 1);
    b.ellipse(16, 16, 8, 8, 3);
    b.ellipse(12, 12, 3, 2, 5);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(16, 32);
    b.rect(6, 8, 4, 20, 1);
    b.ellipse(8, 8, 5, 3, 2);
    b.rect(5, 26, 6, 3, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 16);
    b.ellipse(16, 8, 14, 6, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 1);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
    uint8_t px[64];
    fillTile(px, 1);
    px[0] = 3;
    px[9] = 2;
    px[18] = 3;
    px[27] = 2;
    px[36] = 3;
    px[45] = 2;
    px[54] = 3;
    px[63] = 2;
    a.water = tiles.alloc(1);
    vdp.loadTile(a.water, px);

    fillTile(px, 1);
    px[8] = 2;
    px[17] = 2;
    px[26] = 2;
    px[35] = 2;
    px[4] = 3;
    px[41] = 3;
    px[55] = 3;
    a.quay = tiles.alloc(1);
    vdp.loadTile(a.quay, px);

    fillTile(px, 1);
    for (int x = 0; x < 8; x++) px[3 * 8 + x] = 4;
    a.line = tiles.alloc(1);
    vdp.loadTile(a.line, px);

    fillTile(px, 5);
    for (int y = 0; y < 8; y++) px[y * 8 + 0] = 6;
    px[20] = 7;
    px[44] = 7;
    a.dock = tiles.alloc(1);
    vdp.loadTile(a.dock, px);

    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) px[y * 8 + x] = ((x < 4) ^ (y < 4)) ? 8 : 9;
    a.check = tiles.alloc(1);
    vdp.loadTile(a.check, px);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 15);
    const uint16_t shade = gs::rgb4(1, 2, 3);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(10, 12, 14), gs::rgb4(15, 13, 6), gs::rgb4(15, 6, 4),
                          gs::rgb4(6, 14, 8), gs::rgb4(8, 10, 12), gs::rgb4(15, 12, 3), gs::rgb4(4, 6, 8),
                          0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 7, 12), gs::rgb4(4, 10, 14), gs::rgb4(8, 13, 15), gs::rgb4(1, 4, 8),
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_QUAY, {0, gs::rgb4(9, 9, 8), gs::rgb4(6, 6, 6), gs::rgb4(12, 12, 11), gs::rgb4(14, 12, 3),
                           gs::rgb4(8, 6, 3), gs::rgb4(5, 4, 2), gs::rgb4(11, 8, 4), gs::rgb4(15, 15, 15),
                           gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_KART, {0, gs::rgb4(15, 8, 1), gs::rgb4(12, 5, 1), gs::rgb4(2, 6, 12), gs::rgb4(1, 1, 1),
                           gs::rgb4(4, 4, 4), gs::rgb4(15, 14, 8), gs::rgb4(15, 12, 4), 0, 0, 0, 0, 0, 0, 0, shade});
    auto buoyPal = [&](int pal, uint16_t hi, uint16_t lo) {
        setPal(vdp, pal, {0, hi, gs::rgb4(6, 5, 3), lo, gs::rgb4(3, 3, 3), gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    };
    buoyPal(PAL_RED, gs::rgb4(15, 3, 2), gs::rgb4(10, 1, 1));
    buoyPal(PAL_GOLD, gs::rgb4(15, 13, 2), gs::rgb4(12, 8, 1));
    buoyPal(PAL_GREEN, gs::rgb4(4, 14, 5), gs::rgb4(2, 8, 3));
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(7, 6, 4), gs::rgb4(10, 9, 6), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});

    loadFont(vdp, art);
    gs::Bitmap kart = kartArt();
    for (int i = 0; i < 16; i++) art.kart[i] = gs::uploadMipped(vdp, rotate(kart, i * (PI / 8.0f)));
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    (void)PI;
}

}  // namespace buoy
