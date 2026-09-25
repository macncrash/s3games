#include "game/art.h"

#include <cmath>

namespace torpedo {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

// The bow keel stays shallow. At bilge depth the water ahead of the red plate
// is empty, and the plate itself is vital color.
void sealHull(Bitmap& b) {
    for (int y = 0; y < b.h; ++y) {
        for (int x = 0; x < b.w; ++x) {
            const bool belly = x >= BELLY_X0 && x < BELLY_X1 && y >= BELLY_Y0 && y < BELLY_Y1;
            const bool lane = x < BELLY_X0 && y >= BELLY_Y0;
            if (belly) {
                if (b.get(x, y) != 12) b.set(x, y, 5);
            } else if (lane) {
                const int c = b.get(x, y);
                if (c == 4 || c == 5 || c == 6 || c == 11 || c == 12 || c == 13 || c == 15) b.set(x, y, 0);
            }
        }
    }
}

Bitmap paintShip() {
    Bitmap b(SHIP_W, SHIP_H);
    // Shallow red-lead bottom. Its keel ends above the bilge, so a deep shot
    // passes under the bow.
    b.rect(18, 52, 172, 16, 4);  // y 52..68
    b.poly({{10, 50}, {28, 50}, {22, 66}, {12, 62}}, 4);
    b.poly({{176, 50}, {196, 56}, {190, 68}, {168, 66}}, 4);
    // Side shell only over the boiler room, so the silhouette has no crack
    // where the bilge hangs down.
    b.rect(BELLY_X0, 60, BELLY_X1 - BELLY_X0, 22, 4);
    b.rect(BELLY_X0, BELLY_Y0, BELLY_X1 - BELLY_X0, BELLY_Y1 - BELLY_Y0, 5);
    // Plate seams inside the bilge. Color 12 still counts as a hit.
    b.line(132, 84, 172, 84, 12, 1.f);
    b.line(132, 90, 172, 90, 12, 1.f);
    b.line(148, 80, 148, 96, 12, 1.f);
    b.rect(156, 86, 8, 5, 12);

    // Boot topping and the white waterline.
    b.rect(16, 49, 176, 3, 6);
    b.rect(18, 48, 170, 2, 11);

    // Topsides: black merchant hull, cream houses.
    b.rect(30, 40, 150, 9, 3);
    b.poly({{12, 50}, {34, 38}, {34, 50}}, 3);
    b.poly({{168, 40}, {196, 48}, {186, 50}, {164, 50}}, 3);
    b.line(34, 40, 180, 40, 1, 1.f);
    // Hatches.
    b.rect(46, 42, 22, 5, 2);
    b.rect(74, 42, 22, 5, 2);
    b.rect(102, 42, 16, 5, 2);
    // Bridge and funnel sit over the bilge.
    b.rect(136, 24, 36, 16, 2);
    b.rect(140, 28, 10, 6, 9);
    b.rect(154, 28, 10, 6, 9);
    b.rect(148, 22, 8, 4, 1);
    b.poly({{FUNNEL_X - 8, 26}, {FUNNEL_X - 4, 6}, {FUNNEL_X + 10, 6}, {FUNNEL_X + 14, 26}}, 7);
    b.rect(FUNNEL_X - 4, 12, 14, 4, 8);
    b.rect(FUNNEL_X - 2, 8, 8, 3, 14);
    // Masts and stays.
    b.line(64, 40, 64, 8, 10, 1.6f);
    b.line(64, 12, 86, 20, 10, 1.1f);
    b.line(64, 12, 44, 22, 10, 1.1f);
    b.line(178, 40, 178, 14, 10, 1.6f);
    b.line(178, 18, 164, 26, 10, 1.1f);
    // Anchor and a rust streak on the bow.
    b.line(24, 44, 24, 56, 10, 1.3f);
    b.line(40, 56, 40, 66, 13, 1.f);
    b.line(96, 54, 96, 66, 13, 1.f);
    // Bow wave bits live on the sprite so the hull reads while it is still.
    b.ellipse(8, 50, 6, 2, 11);
    b.outline(15, false);
    sealHull(b);
    return b;
}

Bitmap tilt(const Bitmap& src, float ang) {
    Bitmap b(src.w, src.h);
    const float c = std::cos(ang), s = std::sin(ang);
    for (int y = 0; y < src.h; ++y) {
        for (int x = 0; x < src.w; ++x) {
            const float rx = x + 0.5f - WL_X;
            const float ry = y + 0.5f - WL_Y;
            const float sx = WL_X + (rx * c + ry * s) - 0.5f;
            const float sy = WL_Y + (-rx * s + ry * c) - 0.5f;
            b.set(x, y, src.get(int(std::lround(sx)), int(std::lround(sy))));
        }
    }
    return b;
}

Bitmap paintSub() {
    Bitmap b(120, 48);
    b.ellipse(62, 28, 52, 13, 2);
    b.ellipse(64, 26, 46, 9, 1);
    b.ellipse(60, 32, 36, 6, 3);
    // Nose cap and tube door. The door is the rightmost solid pixel.
    b.poly({{100, 22}, {114, 28}, {100, 34}}, 2);
    b.rect(108, 26, 6, 4, 5);
    b.ellipse(112, 28, 2, 2, 7);
    // Sail.
    b.rect(52, 12, 18, 14, 2);
    b.rect(56, 8, 10, 6, 3);
    b.line(61, 8, 61, 2, 6, 1.4f);
    b.rect(70, 16, 5, 3, 1);
    // Planes and screw.
    b.poly({{24, 22}, {40, 16}, {40, 24}}, 3);
    b.poly({{24, 34}, {40, 40}, {40, 32}}, 3);
    b.line(12, 22, 12, 34, 4, 1.6f);
    b.rect(8, 26, 6, 4, 4);
    b.outline(4, false);
    return b;
}

Bitmap paintTorp() {
    Bitmap b(40, 12);
    b.poly({{4, 4}, {30, 3}, {36, 6}, {30, 9}, {4, 8}}, 1);
    b.poly({{28, 4}, {36, 6}, {28, 8}}, 5);
    b.rect(2, 3, 4, 2, 4);
    b.rect(2, 7, 4, 2, 4);
    b.rect(6, 5, 3, 2, 3);
    return b;
}

Bitmap paintBubble() {
    Bitmap b(14, 14);
    b.ellipse(7, 7, 5, 5, 3);
    b.ellipse(7, 7, 3, 3, 0);
    b.set(5, 5, 1);
    return b;
}

Bitmap paintSplash() {
    Bitmap b(28, 22);
    b.poly({{4, 18}, {10, 4}, {14, 16}}, 2);
    b.poly({{12, 18}, {18, 2}, {22, 16}}, 1);
    b.poly({{8, 18}, {14, 10}, {20, 18}}, 4);
    b.ellipse(14, 18, 10, 3, 2);
    return b;
}

Bitmap paintPuff() {
    Bitmap b(28, 22);
    b.ellipse(14, 12, 10, 7, 6);
    b.ellipse(10, 10, 6, 5, 1);
    b.ellipse(18, 9, 5, 4, 1);
    return b;
}

Bitmap paintWave() {
    Bitmap b(36, 12);
    b.poly({{2, 8}, {10, 3}, {18, 8}, {26, 3}, {34, 8}, {34, 10}, {2, 10}}, 2);
    b.line(4, 8, 14, 4, 1, 1.f);
    return b;
}

Bitmap paintCloud() {
    Bitmap b(72, 26);
    b.ellipse(22, 16, 16, 8, 1);
    b.ellipse(40, 13, 18, 9, 1);
    b.ellipse(56, 16, 12, 7, 1);
    b.ellipse(34, 12, 10, 6, 2);
    return b;
}

Bitmap paintSun() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 8, 8, 3);
    b.ellipse(14, 14, 4, 4, 4);
    for (int i = 0; i < 8; ++i) {
        const float a = i * 6.2831853f / 8.f;
        b.line(14 + std::cos(a) * 9, 14 + std::sin(a) * 9, 14 + std::cos(a) * 13, 14 + std::sin(a) * 13, 3, 1.2f);
    }
    return b;
}

Bitmap paintOil() {
    Bitmap b(48, 12);
    b.ellipse(24, 6, 20, 4, 7);
    b.ellipse(18, 6, 8, 2, 9);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y) {
            for (int x = 0; x < 5; ++x) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        const int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 12, 14), gs::rgb4(15, 12, 6), gs::rgb4(15, 5, 3),
                          gs::rgb4(6, 15, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(12, 8, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(10, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 10, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    setPal(vdp, PAL_SHIP,
           {0, gs::rgb4(14, 13, 10), gs::rgb4(12, 11, 9), gs::rgb4(4, 4, 5), gs::rgb4(7, 1, 1), gs::rgb4(15, 3, 2),
            gs::rgb4(2, 1, 1), gs::rgb4(14, 6, 3), gs::rgb4(2, 2, 2), gs::rgb4(6, 10, 13), gs::rgb4(3, 2, 2),
            gs::rgb4(15, 15, 15), gs::rgb4(15, 8, 3), gs::rgb4(8, 3, 2), gs::rgb4(15, 13, 6), gs::rgb4(1, 0, 0)});
    setPal(vdp, PAL_SUB,
           {0, gs::rgb4(10, 13, 12), gs::rgb4(5, 7, 7), gs::rgb4(2, 3, 4), gs::rgb4(1, 1, 1), gs::rgb4(12, 9, 4),
            gs::rgb4(8, 9, 8), gs::rgb4(15, 3, 2), gs::rgb4(14, 15, 15), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 15, 15), gs::rgb4(8, 13, 14), gs::rgb4(15, 14, 12), gs::rgb4(15, 8, 2),
            gs::rgb4(7, 8, 8), gs::rgb4(2, 3, 2), gs::rgb4(15, 12, 4), gs::rgb4(4, 5, 3), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SKY,
           {0, gs::rgb4(11, 12, 13), gs::rgb4(15, 15, 15), gs::rgb4(15, 13, 8), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, shadow});

    loadFont(vdp, art);
    art.hull = paintShip();
    art.ship[0] = gs::uploadMipped(vdp, art.hull);
    art.ship[1] = gs::uploadMipped(vdp, tilt(art.hull, -0.20f));
    art.ship[2] = gs::uploadMipped(vdp, tilt(art.hull, -0.42f));
    art.sub = gs::uploadMipped(vdp, paintSub());
    art.torp = gs::uploadMipped(vdp, paintTorp());
    art.bubble = gs::uploadMipped(vdp, paintBubble());
    art.splash = gs::uploadMipped(vdp, paintSplash());
    art.puff = gs::uploadMipped(vdp, paintPuff());
    art.wave = gs::uploadMipped(vdp, paintWave());
    art.cloud = gs::uploadMipped(vdp, paintCloud());
    art.sun = gs::uploadMipped(vdp, paintSun());
    art.oil = gs::uploadMipped(vdp, paintOil());
    Bitmap dash(10, 2);
    dash.rect(0, 0, 6, 2, 1);
    art.dash = gs::uploadMipped(vdp, dash);
    Bitmap tick(3, 8);
    tick.rect(0, 0, 3, 2, 1);
    tick.rect(0, 6, 3, 2, 1);
    tick.rect(0, 0, 1, 8, 1);
    art.tick = gs::uploadMipped(vdp, tick);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.setFogColor(gs::rgb4(1, 2, 5));
}

}  // namespace torpedo
