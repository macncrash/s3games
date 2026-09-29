#include "game/art.h"

#include <cmath>
#include <vector>

namespace headerbox {
namespace {

constexpr float kPi = 3.14159265f;

void setPal(gs::VDP& vdp, int pal, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

gs::Bitmap boatArt(float ang) {
    gs::Bitmap b(48, 48);
    auto P = [&](float x, float y) -> gs::Pt {
        float c = std::cos(ang), s = std::sin(ang);
        return {24.f + x * c - y * s, 24.f - (x * s + y * c)};
    };
    b.poly({P(-16, -5), P(12, -4), P(18, 0), P(12, 4), P(-16, 5)}, 1);
    b.poly({P(-14, -3), P(8, -2), P(10, 0), P(8, 2), P(-14, 3)}, 2);
    b.poly({P(-2, 0), P(6, 0), P(2, -14), P(-4, -13)}, 4);
    b.line(P(-2, 0).first, P(-2, 0).second, P(2, -16).first, P(2, -16).second, 3, 1.4f);
    b.ellipse(P(14, 0).first, P(14, 0).second, 1.6f, 1.6f, 5);
    return b;
}

gs::Bitmap boxArt() {
    gs::Bitmap b(72, 56);
    b.rect(0, 0, 72, 3, 1);
    b.rect(0, 53, 72, 3, 1);
    b.rect(0, 0, 3, 56, 1);
    b.rect(69, 0, 3, 56, 1);
    b.rect(3, 3, 66, 50, 2);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 14);
    b.rect(3, 2, 4, 12, 1);
    b.rect(2, 0, 6, 3, 2);
    b.rect(1, 12, 8, 2, 3);
    return b;
}

gs::Bitmap wakeArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(14, 6, 12, 4, 1);
    b.ellipse(16, 6, 6, 2, 2);
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
    for (int i = 0; i < 64; i++) water[i] = ((i / 8) & 1) ? 1 : 2;
    water[8 * 3 + 2] = 3;
    water[8 * 6 + 5] = 3;
    a.waterTile = tiles.alloc(1);
    vdp.loadTile(a.waterTile, water);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 2, 4);
    const uint16_t ink = gs::rgb4(15, 15, 15);
    const uint16_t hud[16] = {0, ink, gs::rgb4(8, 13, 15), gs::rgb4(15, 13, 4), gs::rgb4(15, 5, 3),
                              gs::rgb4(4, 14, 8), gs::rgb4(6, 10, 14), 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t boat[16] = {0, gs::rgb4(14, 12, 8), gs::rgb4(8, 5, 3), gs::rgb4(6, 4, 3), gs::rgb4(15, 15, 13),
                               gs::rgb4(15, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t sail[16] = {0, gs::rgb4(15, 15, 14), gs::rgb4(10, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t box[16] = {0, gs::rgb4(15, 12, 3), gs::rgb4(3, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t post[16] = {0, gs::rgb4(12, 9, 5), gs::rgb4(15, 14, 8), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    const uint16_t water[16] = {0, gs::rgb4(2, 7, 12), gs::rgb4(1, 5, 10), gs::rgb4(4, 10, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_BOAT, boat);
    setPal(vdp, PAL_SAIL, sail);
    setPal(vdp, PAL_BOX, box);
    setPal(vdp, PAL_POST, post);
    setPal(vdp, PAL_WATER, water);
    (void)kPi;
    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.boat[i] = gs::uploadMipped(vdp, boatArt(i * (6.2831853f / 16.f)));
    art.box = gs::uploadMipped(vdp, boxArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.wake = gs::uploadMipped(vdp, wakeArt());
}

}  // namespace headerbox
