#include "game/art.h"

#include <cmath>
#include <string>

namespace s3lock {
namespace {

constexpr float PI = 3.14159265f;

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

// Clockwise in image space (y down). Inverse sample, with a half-pixel bleed so
// the rotated timber doesn't split into cracks.
Bitmap rotateCW(const Bitmap& src, float a) {
    const float ca = std::cos(a), sa = std::sin(a);
    const float c = std::fabs(ca), s = std::fabs(sa);
    const int nw = int(std::ceil(src.w * c + src.h * s)) + 4;
    const int nh = int(std::ceil(src.w * s + src.h * c)) + 4;
    Bitmap o(nw, nh);
    const float cx = src.w * 0.5f, cy = src.h * 0.5f;
    const float ocx = nw * 0.5f, ocy = nh * 0.5f;
    auto at = [&](float x, float y) { return src.get(int(std::lround(x)), int(std::lround(y))); };
    for (int y = 0; y < nh; y++) {
        for (int x = 0; x < nw; x++) {
            float dx = x + 0.5f - ocx, dy = y + 0.5f - ocy;
            // Inverse of clockwise y-down: x' = dx ca - dy sa, y' = dx sa + dy ca.
            float sx = cx + dx * ca + dy * sa;
            float sy = cy - dx * sa + dy * ca;
            int p = at(sx, sy);
            if (!p) p = at(sx + 0.45f, sy);
            if (!p) p = at(sx - 0.45f, sy);
            if (!p) p = at(sx, sy + 0.45f);
            if (!p) p = at(sx, sy - 0.45f);
            if (p) o.set(x, y, p);
        }
    }
    return o;
}

Bitmap narrowboat() {
    Bitmap b(28, BOAT_SRC_H);
    const float cx = 14.f;
    // Hull, nose at the top. The bitmap centre is the boat's centre.
    b.poly({{6, 22}, {cx, 3}, {22, 22}}, 2);
    b.poly({{8, 20}, {cx, 7}, {20, 20}}, 1);
    b.rect(6, 20, 16, 78, 2);
    b.rect(7, 22, 14, 74, 1);
    b.rect(8, 92, 12, 8, 3);
    b.rect(7, 96, 14, 4, 8);
    // Cabin and roof.
    b.rect(7, 40, 14, 34, 4);
    b.rect(8, 42, 12, 26, 5);
    b.rect(9, 46, 4, 4, 6);
    b.rect(15, 46, 4, 4, 6);
    b.rect(9, 54, 4, 4, 6);
    b.rect(15, 54, 4, 4, 6);
    b.rect(11, 62, 6, 4, 10);
    // Chimney and tiller.
    b.rect(12, 32, 4, 9, 7);
    b.rect(11, 30, 6, 3, 8);
    b.line(14, 98, 14, 103, 9, 1.6f);
    b.line(10, 101, 18, 101, 9, 1.6f);
    // Fenders — they stick out, which is what kisses a gate.
    b.ellipse(6, 36, 2.4f, 3.2f, 8);
    b.ellipse(22, 36, 2.4f, 3.2f, 8);
    b.ellipse(6, 86, 2.4f, 3.2f, 8);
    b.ellipse(22, 86, 2.4f, 3.2f, 8);
    b.line(10, 16, 18, 16, 9, 1.2f);
    b.outline(3, false);
    return b;
}

// Horizontal leaf. +x is the tip (yellow), the other end is the hinge.
Bitmap gateLeaf() {
    Bitmap b(72, 16);
    b.rect(2, 3, 66, 10, 1);
    b.rect(2, 3, 66, 3, 2);
    b.rect(6, 5, 7, 6, 3);
    b.rect(28, 5, 6, 6, 3);
    b.rect(48, 4, 16, 8, 4);
    b.rect(50, 7, 12, 2, 5);
    b.ellipse(8, 8, 2.2f, 2.2f, 3);
    b.ellipse(32, 8, 2.0f, 2.0f, 3);
    b.outline(6, false);
    return b;
}

Bitmap stoneCourse() {
    Bitmap b(40, 18);
    b.rect(0, 0, 40, 18, 1);
    b.rect(0, 0, 40, 4, 2);
    b.rect(0, 14, 40, 4, 3);
    for (int x = 4; x < 40; x += 10) b.rect(x, 4, 1, 10, 3);
    b.rect(9, 8, 1, 6, 4);
    b.rect(19, 5, 1, 5, 4);
    b.rect(29, 9, 1, 5, 4);
    // Coping faces the water (right edge of this sprite).
    b.rect(36, 0, 4, 18, 5);
    b.rect(37, 0, 1, 18, 2);
    return b;
}

Bitmap grassBank() {
    Bitmap b(72, 24);
    b.rect(0, 0, 72, 24, 1);
    for (int i = 0; i < 18; i++) {
        int x = (i * 17) % 58;
        int y = (i * 5) % 16;
        b.ellipse(float(x + 4), float(y + 4), 3.5f, 2.4f, (i % 3) ? 2 : 3);
    }
    // Towpath along the canal edge.
    b.rect(58, 0, 14, 24, 4);
    b.rect(58, 0, 2, 24, 5);
    b.rect(68, 0, 2, 24, 6);
    for (int y = 3; y < 24; y += 7) b.rect(62, y, 4, 1, 5);
    return b;
}

Bitmap fieldArt() {
    Bitmap b(48, 20);
    b.rect(0, 0, 48, 20, 1);
    for (int i = 0; i < 9; i++) {
        float x = float((i * 11) % 40 + 4);
        float y = float((i * 5) % 12 + 4);
        b.ellipse(x, y, 3.4f, 2.3f, (i % 2) ? 2 : 3);
    }
    return b;
}

Bitmap treeArt() {
    Bitmap b(32, 40);
    b.rect(14, 26, 4, 12, 4);
    b.ellipse(16, 16, 12, 11, 1);
    b.ellipse(12, 14, 7, 6, 2);
    b.ellipse(20, 18, 5, 4, 3);
    b.outline(5, false);
    return b;
}

Bitmap keeperArt() {
    Bitmap b(20, 32);
    b.ellipse(10, 7, 4, 4, 1);
    b.rect(6, 5, 8, 3, 2);
    b.rect(7, 11, 6, 10, 3);
    b.rect(5, 12, 3, 7, 3);
    b.rect(12, 12, 3, 7, 3);
    b.rect(7, 21, 2, 8, 4);
    b.rect(11, 21, 2, 8, 4);
    b.rect(6, 28, 4, 2, 5);
    b.rect(11, 28, 4, 2, 5);
    b.rect(8, 8, 2, 1, 6);
    b.rect(11, 8, 2, 1, 6);
    return b;
}

Bitmap buoyArt() {
    Bitmap b(14, 18);
    b.rect(6, 1, 2, 4, 2);
    b.ellipse(7, 9, 5, 5, 1);
    b.rect(3, 8, 8, 2, 2);
    b.ellipse(7, 15, 3, 2, 3);
    b.outline(4, false);
    return b;
}

Bitmap rippleArt() {
    Bitmap b(22, 12);
    b.ellipse(11, 6, 9, 4, 2);
    b.ellipse(11, 6, 5, 2, 3);
    // Hollow the middle so it reads as a ring.
    b.ellipse(11, 6, 3, 1.2f, 0);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 9, 10), gs::rgb4(15, 15, 15), gs::rgb4(15, 5, 3),
                          gs::rgb4(5, 14, 6), gs::rgb4(15, 12, 3), gs::rgb4(6, 8, 12), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 8, 2), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 3, 2), gs::rgb4(15, 15, 15), gs::rgb4(4, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(4, 13, 4), gs::rgb4(15, 15, 15), gs::rgb4(1, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, shadow});
    // 1 highlight, 2 hull, 3 dark, 4 cabin, 5 roof, 6 glass, 7 stack, 8 fender, 9 rope, 10 door
    setPal(vdp, PAL_BOAT, {0, gs::rgb4(4, 9, 5), gs::rgb4(2, 6, 3), gs::rgb4(1, 2, 1), gs::rgb4(13, 11, 7),
                           gs::rgb4(8, 5, 3), gs::rgb4(6, 11, 13), gs::rgb4(12, 3, 2), gs::rgb4(2, 2, 2),
                           gs::rgb4(11, 9, 5), gs::rgb4(6, 3, 2), 0, 0, 0, 0, shadow});
    // 1 wood, 2 wood hi, 3 iron, 4 yellow, 5 black band, 6 outline
    setPal(vdp, PAL_GATE, {0, gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 4), gs::rgb4(12, 12, 13), gs::rgb4(15, 13, 2),
                           gs::rgb4(1, 1, 1), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(8, 8, 7), gs::rgb4(12, 12, 11), gs::rgb4(4, 4, 4), gs::rgb4(6, 6, 5),
                            gs::rgb4(14, 14, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(3, 8, 2), gs::rgb4(5, 11, 3), gs::rgb4(2, 5, 2), gs::rgb4(9, 7, 3),
                           gs::rgb4(7, 5, 2), gs::rgb4(5, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(1, 2, 3), gs::rgb4(8, 13, 14), gs::rgb4(12, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, shadow});
    setPal(vdp, PAL_KEEP, {0, gs::rgb4(13, 9, 6), gs::rgb4(2, 2, 2), gs::rgb4(3, 4, 9), gs::rgb4(2, 2, 4),
                           gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    Bitmap boat = narrowboat();
    const float step = (2.f * YAW_SPAN) / float(YAWS - 1);
    for (int i = 0; i < YAWS; i++) {
        float deg = -YAW_SPAN + step * float(i);
        art.boat[i] = gs::uploadMipped(vdp, rotateCW(boat, deg * PI / 180.f));
    }
    Bitmap leaf = gateLeaf();
    for (int i = 0; i < GATE_DIRS; i++) {
        float a = float(i) * (PI * 2.f / float(GATE_DIRS));
        art.leaf[i] = gs::uploadMipped(vdp, rotateCW(leaf, a));
    }
    art.stone = gs::uploadMipped(vdp, stoneCourse());
    art.grass = gs::uploadMipped(vdp, grassBank());
    art.field = gs::uploadMipped(vdp, fieldArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.keeper = gs::uploadMipped(vdp, keeperArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.ripple = gs::uploadMipped(vdp, rippleArt());
    loadFont(vdp, art);
}

}  // namespace s3lock
