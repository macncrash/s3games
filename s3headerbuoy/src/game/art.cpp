#include "game/art.h"

#include <cmath>
#include <string>

namespace headerbuoy {
namespace {

constexpr float TAU = 6.2831853f;

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

// Local boat point: lx starboard, ly forward. Screen y grows downward.
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
    b.poly({P(0, 26), P(9, 8), P(8, -18), P(-8, -18), P(-9, 8)}, 2);
    b.poly({P(0, 22), P(6, 8), P(5, -14), P(-5, -14), P(-6, 8)}, 1);
    b.poly({P(0, 16), P(14, -2), P(2, -6), P(-2, 4)}, 4);
    b.poly({P(0, 14), P(-13, -4), P(-2, -8), P(1, 2)}, 5);
    b.poly({P(-1, 18), P(1, 18), P(1, -16), P(-1, -16)}, 3);
    b.ellipse(P(0, 20).first, P(0, 20).second, 2.2f, 2.2f, 6);
    b.outline(7, false);
    return b;
}

Bitmap buoyArt() {
    Bitmap b(40, 56);
    b.ellipse(20, 16, 14, 8, 3);
    b.ellipse(20, 14, 12, 6, 1);
    b.rect(17, 18, 6, 16, 2);
    b.rect(18, 22, 4, 8, 4);
    b.poly({{20, 34}, {8, 48}, {32, 48}}, 5);
    b.ellipse(20, 46, 12, 4, 6);
    b.line(20, 8, 20, 0, 2, 1.5f);
    b.poly({{20, 0}, {32, 4}, {20, 8}}, 1);
    b.outline(7, false);
    return b;
}

Bitmap dockArt() {
    Bitmap b(96, 48);
    b.rect(8, 10, 80, 22, 2);
    b.rect(10, 12, 76, 18, 1);
    for (int i = 0; i < 5; i++) b.rect(16 + i * 14, 12, 3, 18, 3);
    b.rect(4, 28, 8, 16, 4);
    b.rect(84, 28, 8, 16, 4);
    b.rect(44, 28, 8, 14, 4);
    b.rect(30, 16, 36, 8, 5);
    b.outline(6, false);
    return b;
}

Bitmap wakeArt() {
    Bitmap b(32, 16);
    b.ellipse(16, 8, 14, 5, 1);
    b.ellipse(16, 8, 8, 3, 2);
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
    const uint16_t shadow = gs::rgb4(1, 2, 4);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 12, 15), gs::rgb4(15, 15, 15), gs::rgb4(15, 5, 3), gs::rgb4(4, 14, 8),
                          gs::rgb4(15, 13, 4), gs::rgb4(6, 10, 14), gs::rgb4(4, 6, 8), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 8, 2), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(10, 1, 1), gs::rgb4(15, 12, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(6, 15, 8), gs::rgb4(2, 9, 5), gs::rgb4(14, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOAT, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 4, 3), gs::rgb4(7, 5, 3), gs::rgb4(15, 15, 12),
                           gs::rgb4(13, 12, 8), gs::rgb4(15, 6, 2), gs::rgb4(2, 2, 3), gs::rgb4(4, 8, 12), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(15, 8, 1), gs::rgb4(15, 15, 15), gs::rgb4(12, 3, 1), gs::rgb4(2, 6, 10),
                           gs::rgb4(3, 8, 6), gs::rgb4(8, 12, 14), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(12, 9, 5), gs::rgb4(8, 6, 3), gs::rgb4(5, 4, 2), gs::rgb4(3, 3, 3),
                           gs::rgb4(15, 14, 8), gs::rgb4(2, 2, 2), gs::rgb4(14, 8, 3), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SAIL, {0, gs::rgb4(14, 14, 12), gs::rgb4(9, 10, 12), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WAKE, {0, gs::rgb4(12, 15, 15), gs::rgb4(8, 13, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.boat[i] = gs::uploadMipped(vdp, boatArt(i * TAU / 16.0f));
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.dock = gs::uploadMipped(vdp, dockArt());
    art.wake = gs::uploadMipped(vdp, wakeArt());
}

}  // namespace headerbuoy
