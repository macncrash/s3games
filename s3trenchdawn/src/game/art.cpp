#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace trench {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 2246822519u + uint32_t(y) * 3266489917u;
    h = (h ^ (h >> 15)) * 668265263u;
    return h ^ (h >> 16);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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

gs::Bitmap parapetArt() {
    gs::Bitmap b(320, 56);
    b.rect(0, 18, 320, 38, 2);
    b.poly({{0, 22}, {48, 8}, {96, 16}, {150, 4}, {210, 14}, {270, 6}, {320, 18}, {320, 22}, {0, 22}}, 3);
    b.rect(0, 40, 320, 8, 4);
    for (int i = 0; i < 14; i++) {
        int x = 6 + i * 23;
        int h = 8 + int(hash2(i, 3) % 7);
        b.ellipse(float(x), 38.f, 9, float(h), (i & 1) ? 5 : 6);
        b.ellipse(float(x) + 6.f, 40.f, 7, 5, 7);
    }
    for (int y = 22; y < 52; y += 4)
        for (int x = 2; x < 318; x += 9) {
            uint32_t h = hash2(x, y);
            if ((h % 11) == 0) b.set(x, y, (h & 1) ? 8 : 9);
        }
    b.rect(0, 50, 320, 6, 1);
    return b;
}

gs::Bitmap duckArt() {
    gs::Bitmap b(48, 14);
    b.rect(2, 4, 44, 3, 1);
    b.rect(2, 9, 44, 3, 2);
    for (int i = 0; i < 6; i++) b.rect(4 + i * 7, 2, 2, 11, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap brazierArt() {
    gs::Bitmap b(28, 24);
    b.poly({{4, 10}, {8, 20}, {20, 20}, {24, 10}}, 1);
    b.rect(2, 8, 24, 4, 2);
    b.rect(6, 18, 4, 5, 3);
    b.rect(18, 18, 4, 5, 3);
    b.rect(8, 12, 12, 4, 4);
    b.ellipse(14, 13, 3, 2, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap flameArt(int step) {
    gs::Bitmap b(18, 30);
    float lean = step ? 3.f : -2.f;
    b.ellipse(9 + lean, 18, 5.5f, 9, 1);
    b.ellipse(9 + lean * 0.5f, 12, 3.5f, 7, 2);
    b.ellipse(9, 8, 2.f, 4.5f, 3);
    b.rect(8, 22, 2, 6, 4);
    return b;
}

gs::Bitmap sentryArt(int step) {
    gs::Bitmap b(32, 46);
    b.ellipse(16, 10, 7, 6, 3);
    b.poly({{9, 8}, {16, 2}, {23, 8}}, 4);
    b.rect(10, 16, 12, 14, 1);
    b.rect(10, 16, 4, 14, 2);
    int leg = step ? 3 : 0;
    b.rect(10, 30, 4, 12 + leg, 5);
    b.rect(18, 30, 4, 14 - leg, 5);
    b.rect(22, 18, 8, 3, 6);
    b.rect(8, 20, 4, 8, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(18, 12, 9, 9, 0);
    b.ellipse(10, 16, 2, 2, 2);
    b.ellipse(13, 20, 1.4f, 1.4f, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(30, 30);
    b.ellipse(15, 15, 8, 8, 1);
    b.ellipse(15, 15, 5, 5, 2);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785f;
        float c = std::cos(a), s = std::sin(a);
        b.line(15 + c * 9, 15 + s * 9, 15 + c * 13, 15 + s * 13, 3, 1.4f);
    }
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(9, 9);
    b.line(4, 0, 4, 8, 1, 1.f);
    b.line(0, 4, 8, 4, 1, 1.f);
    b.set(2, 2, 2);
    b.set(6, 2, 2);
    b.set(2, 6, 2);
    b.set(6, 6, 2);
    return b;
}

gs::Bitmap burstArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 3, 1);
    b.ellipse(8, 8, 3, 6, 2);
    b.ellipse(8, 8, 2, 2, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    setPal(vdp, PAL_HUD, {rgb4(0, 0, 0), rgb4(14, 13, 10), rgb4(6, 6, 7), rgb4(2, 2, 3)});
    setPal(vdp, PAL_EARTH, {rgb4(0, 0, 0), rgb4(2, 2, 1), rgb4(5, 4, 2), rgb4(7, 6, 3), rgb4(4, 3, 2),
                            rgb4(8, 7, 4), rgb4(6, 5, 3), rgb4(9, 8, 5), rgb4(3, 3, 2), rgb4(10, 8, 4)});
    setPal(vdp, PAL_WOOD, {rgb4(0, 0, 0), rgb4(8, 5, 2), rgb4(6, 4, 1), rgb4(4, 3, 1), rgb4(2, 1, 0)});
    setPal(vdp, PAL_YOU, {rgb4(0, 0, 0), rgb4(4, 6, 3), rgb4(3, 5, 2), rgb4(12, 9, 6), rgb4(5, 5, 4),
                          rgb4(3, 3, 2), rgb4(9, 8, 6), rgb4(2, 2, 2), rgb4(1, 1, 1)});
    setPal(vdp, PAL_IRON, {rgb4(0, 0, 0), rgb4(5, 5, 6), rgb4(8, 8, 9), rgb4(3, 3, 4), rgb4(10, 6, 2),
                           rgb4(14, 10, 3), rgb4(1, 1, 2)});
    setPal(vdp, PAL_FIRE, {rgb4(0, 0, 0), rgb4(14, 6, 1), rgb4(15, 12, 3), rgb4(15, 15, 10), rgb4(8, 3, 1),
                           rgb4(14, 10, 2)});
    setPal(vdp, PAL_MOON, {rgb4(0, 0, 0), rgb4(12, 12, 11), rgb4(8, 8, 9)});
    setPal(vdp, PAL_STAR, {rgb4(0, 0, 0), rgb4(14, 14, 12), rgb4(8, 8, 10)});
    setPal(vdp, PAL_BAG, {rgb4(0, 0, 0), rgb4(6, 6, 4), rgb4(8, 7, 4), rgb4(4, 5, 3), rgb4(3, 3, 2),
                          rgb4(9, 8, 5)});
    setPal(vdp, PAL_GOLD, {rgb4(0, 0, 0), rgb4(15, 13, 4), rgb4(8, 6, 1), rgb4(4, 3, 1)});
    setPal(vdp, PAL_ALERT, {rgb4(0, 0, 0), rgb4(15, 3, 2), rgb4(8, 1, 1)});
    setPal(vdp, PAL_OK, {rgb4(0, 0, 0), rgb4(6, 14, 5), rgb4(2, 6, 2)});
    setPal(vdp, PAL_SHELL, {rgb4(0, 0, 0), rgb4(12, 11, 8), rgb4(14, 8, 3), rgb4(15, 14, 8)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.parapet = gs::uploadMipped(vdp, parapetArt());
    art.duck = gs::uploadMipped(vdp, duckArt());
    art.brazier = gs::uploadMipped(vdp, brazierArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.sentry[0] = gs::uploadMipped(vdp, sentryArt(0));
    art.sentry[1] = gs::uploadMipped(vdp, sentryArt(1));
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.burst = gs::uploadMipped(vdp, burstArt());
}

}  // namespace trench
