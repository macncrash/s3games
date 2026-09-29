#include "game/art.h"

#include <cmath>
#include <string>

namespace headergrass {
namespace {

constexpr float TAU = 6.2831853f;

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void hullPt(float lx, float ly, float h, float& ox, float& oy) {
    float x = lx * std::sin(h) + ly * std::cos(h);
    float y = -lx * std::cos(h) + ly * std::sin(h);
    ox = x;
    oy = -y;
}

Bitmap boatArt(float heading) {
    Bitmap b(72, 72);
    const float cx = 36, cy = 36;
    auto P = [&](float lx, float ly) {
        float x, y;
        hullPt(lx, ly, heading, x, y);
        return gs::Pt{cx + x, cy + y};
    };
    b.poly({P(0, 26), P(8, 6), P(7, -20), P(-7, -20), P(-8, 6)}, 2);
    b.poly({P(0, 22), P(5, 6), P(4, -16), P(-4, -16), P(-5, 6)}, 1);
    b.poly({P(1, 14), P(16, -4), P(2, -8)}, 4);
    b.poly({P(-1, 12), P(-15, -6), P(-1, -10)}, 5);
    b.poly({P(-1, 16), P(1, 16), P(1, -14), P(-1, -14)}, 3);
    b.ellipse(P(0, -12).first, P(0, -12).second, 3.2f, 2.4f, 6);
    b.outline(7, false);
    return b;
}

Bitmap meadowArt() {
    Bitmap b(96, 64);
    b.rect(0, 0, 96, 64, 2);
    for (int y = 0; y < 64; y += 8)
        for (int x = (y / 8) & 1 ? 4 : 0; x < 96; x += 8) b.rect(float(x), float(y), 4, 4, 1);
    for (int i = 0; i < 7; i++) {
        int x = 8 + i * 12;
        b.poly({{float(x), 40}, {float(x - 4), 58}, {float(x + 5), 58}}, 3);
        b.line(float(x), 40, float(x), 18, 4, 1.2f);
    }
    b.rect(0, 0, 96, 6, 5);
    b.rect(0, 58, 96, 6, 5);
    return b;
}

Bitmap tuftArt() {
    Bitmap b(24, 28);
    b.poly({{12, 2}, {4, 26}, {20, 26}}, 1);
    b.poly({{8, 8}, {2, 26}, {12, 24}}, 2);
    b.poly({{16, 6}, {12, 24}, {22, 26}}, 3);
    b.line(12, 26, 12, 4, 4, 1.2f);
    return b;
}

Bitmap flagArt() {
    Bitmap b(28, 48);
    b.rect(4, 8, 3, 36, 2);
    b.poly({{7, 8}, {24, 14}, {7, 22}}, 1);
    b.poly({{7, 14}, {20, 18}, {7, 22}}, 3);
    b.ellipse(5, 44, 6, 3, 4);
    b.outline(5, false);
    return b;
}

Bitmap wakeArt() {
    Bitmap b(28, 14);
    b.ellipse(14, 7, 12, 5, 1);
    b.ellipse(14, 7, 6, 2, 2);
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
    const uint16_t ink = gs::rgb4(15, 15, 15);
    const uint16_t shadow = gs::rgb4(1, 2, 3);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 12, 15), gs::rgb4(15, 15, 15), gs::rgb4(15, 5, 3), gs::rgb4(4, 14, 8),
                          gs::rgb4(15, 13, 4), gs::rgb4(6, 10, 14), gs::rgb4(4, 6, 8), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 8, 2), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(10, 1, 1), gs::rgb4(15, 12, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(6, 15, 8), gs::rgb4(2, 9, 5), gs::rgb4(14, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOAT, {0, gs::rgb4(15, 15, 13), gs::rgb4(3, 8, 12), gs::rgb4(8, 6, 4), gs::rgb4(14, 14, 11),
                           gs::rgb4(12, 11, 7), gs::rgb4(2, 3, 4), gs::rgb4(15, 8, 2), gs::rgb4(1, 2, 3), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MEADOW, {0, gs::rgb4(4, 12, 4), gs::rgb4(2, 9, 3), gs::rgb4(6, 13, 4), gs::rgb4(3, 8, 2),
                             gs::rgb4(8, 11, 5), gs::rgb4(5, 10, 6), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(15, 14, 6), gs::rgb4(10, 8, 3), gs::rgb4(15, 10, 2), gs::rgb4(3, 7, 3),
                           gs::rgb4(2, 2, 2), gs::rgb4(8, 6, 2), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WAKE, {0, gs::rgb4(11, 15, 15), gs::rgb4(7, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SHORE, {0, gs::rgb4(10, 9, 4), gs::rgb4(7, 8, 3), gs::rgb4(12, 11, 6), gs::rgb4(4, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.boat[i] = gs::uploadMipped(vdp, boatArt(i * TAU / 16.0f));
    art.meadow = gs::uploadMipped(vdp, meadowArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.wake = gs::uploadMipped(vdp, wakeArt());
}

}  // namespace headergrass
