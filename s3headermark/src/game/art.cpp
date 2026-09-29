#include "game/art.h"

#include <cmath>
#include <vector>

namespace headermark {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

gs::Bitmap hullArt(float ang) {
    gs::Bitmap b(56, 56);
    auto P = [&](float x, float y) -> gs::Pt {
        float c = std::cos(ang), s = std::sin(ang);
        return {28.f + x * c - y * s, 28.f - (x * s + y * c)};
    };
    // Dark green header: long sheer, cream sail, red pennant, white boot stripe.
    b.poly({P(-20, -6), P(10, -5.5f), P(20, -1.5f), P(22, 0), P(20, 1.5f), P(10, 5.5f), P(-20, 6)}, 1);
    b.poly({P(-18, -3.2f), P(8, -2.6f), P(14, 0), P(8, 2.6f), P(-18, 3.2f)}, 2);
    b.line(P(-16, -5.2f).first, P(-16, -5.2f).second, P(12, -4.4f).first, P(12, -4.4f).second, 6, 1.2f);
    b.line(P(-16, 5.2f).first, P(-16, 5.2f).second, P(12, 4.4f).first, P(12, 4.4f).second, 6, 1.2f);
    b.poly({P(-1, 0), P(7, 0.4f), P(3, -18), P(-6, -16)}, 4);
    b.line(P(0, 0).first, P(0, 0).second, P(1, -20).first, P(1, -20).second, 3, 1.5f);
    b.poly({P(1, -20), P(7, -17), P(1, -16)}, 5);
    b.ellipse(P(16, 0).first, P(16, 0).second, 1.8f, 1.8f, 7);
    b.rect(int(P(-8, -1).first), int(P(-8, -1).second), 2, 2, 3);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(64, 64);
    b.ellipse(32, 32, 30, 30, 1);
    b.ellipse(32, 32, 22, 22, 2);
    b.ellipse(32, 32, 14, 14, 1);
    b.ellipse(32, 32, 6, 6, 3);
    return b;
}

gs::Bitmap buoyArt() {
    gs::Bitmap b(14, 18);
    b.poly({{7, 1}, {12, 8}, {7, 16}, {2, 8}}, 1);
    b.poly({{7, 4}, {10, 8}, {7, 13}, {4, 8}}, 2);
    b.rect(6, 0, 2, 3, 3);
    return b;
}

gs::Bitmap wakeArt() {
    gs::Bitmap b(32, 14);
    b.ellipse(16, 7, 14, 5, 1);
    b.ellipse(18, 7, 7, 2, 2);
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
    uint8_t water[64] = {};
    for (int i = 0; i < 64; i++) water[i] = ((i * 3 + (i / 8)) & 3) == 0 ? 3 : (((i / 8) & 1) ? 1 : 2);
    a.waterTile = tiles.alloc(1);
    vdp.loadTile(a.waterTile, water);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 2, 3);
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t hud[16] = {0, ink, gs::rgb4(8, 14, 15), gs::rgb4(15, 12, 3), gs::rgb4(15, 5, 3),
                              gs::rgb4(5, 14, 8), gs::rgb4(7, 11, 14), 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t hull[16] = {0,
                               gs::rgb4(2, 8, 5),
                               gs::rgb4(6, 10, 7),
                               gs::rgb4(4, 3, 2),
                               gs::rgb4(15, 15, 12),
                               gs::rgb4(14, 3, 2),
                               gs::rgb4(14, 14, 12),
                               gs::rgb4(15, 8, 2),
                               0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t mark[16] = {0, gs::rgb4(15, 11, 2), gs::rgb4(15, 15, 13), gs::rgb4(12, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t buoy[16] = {0, gs::rgb4(14, 3, 2), gs::rgb4(15, 14, 4), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t wake[16] = {0, gs::rgb4(6, 12, 15), gs::rgb4(12, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t water[16] = {0, gs::rgb4(1, 6, 11), gs::rgb4(1, 4, 9), gs::rgb4(3, 9, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_HULL, hull);
    setPal(vdp, PAL_MARK, mark);
    setPal(vdp, PAL_BUOY, buoy);
    setPal(vdp, PAL_WAKE, wake);
    setPal(vdp, PAL_WATER, water);
    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.hull[i] = gs::uploadMipped(vdp, hullArt(i * (6.2831853f / 16.f)));
    art.mark = gs::uploadMipped(vdp, markArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.wake = gs::uploadMipped(vdp, wakeArt());
}

}  // namespace headermark
