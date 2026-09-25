#include "game/art.h"

#include <cmath>
#include <cstdint>

namespace raid {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float kPi = 3.14159265f;

void textPal(gs::VDP& vdp, int pal, uint16_t fg) {
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 1, fg);
    vdp.setColor(pal * 16 + 15, gs::rgb4(0, 0, 0));
}

void colors(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    vdp.setColor(pal * 16 + 0, 0);
    int i = 1;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
}

int makeTile(gs::VDP& vdp, gs::TileAlloc& tiles, const uint8_t px[64]) {
    int t = tiles.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

Bitmap leanOf(const Bitmap& src, float k) {
    Bitmap o(src.w, src.h);
    float cy = src.h * 0.42f;
    for (int y = 0; y < src.h; y++) {
        int shift = int(std::lround((y - cy) * -k));
        for (int x = 0; x < src.w; x++) {
            int c = src.get(x, y);
            if (c) o.set(x + shift, y, c);
        }
    }
    return o;
}

Bitmap rotateUp(const Bitmap& src, float ang) {
    Bitmap o(src.w, src.h);
    float c = std::cos(ang), s = std::sin(ang);
    float cx = (src.w - 1) * 0.5f, cy = (src.h - 1) * 0.5f;
    for (int y = 0; y < src.h; y++) {
        for (int x = 0; x < src.w; x++) {
            float ox = x - cx, oy = y - cy;
            int sx = int(std::lround(cx + ox * c + oy * s));
            int sy = int(std::lround(cy - ox * s + oy * c));
            int col = src.get(sx, sy);
            if (col) o.set(x, y, col);
        }
    }
    return o;
}

Bitmap paintBike() {
    Bitmap b(80, 96);
    b.ellipse(22, 78, 12, 16, 3);
    b.ellipse(58, 78, 12, 16, 3);
    b.ellipse(22, 78, 5, 7, 4);
    b.ellipse(58, 78, 5, 7, 4);
    b.line(40, 64, 22, 78, 4, 3.2f);
    b.line(40, 64, 58, 78, 4, 3.2f);
    b.poly({{40, 42}, {24, 56}, {20, 70}, {60, 70}, {56, 56}}, 3);
    b.ellipse(40, 58, 16, 8, 4);
    b.rect(30, 48, 20, 7, 6);
    b.rect(33, 50, 6, 3, 9);
    b.rect(41, 50, 6, 3, 9);
    b.poly({{40, 22}, {26, 34}, {24, 58}, {56, 58}, {54, 34}}, 1);
    b.poly({{40, 26}, {30, 34}, {50, 34}}, 2);
    b.ellipse(40, 18, 10, 11, 3);
    b.ellipse(40, 20, 7, 5, 7);
    b.line(28, 36, 14, 48, 1, 3.4f);
    b.line(52, 36, 66, 48, 1, 3.4f);
    b.ellipse(14, 48, 3, 3, 4);
    b.ellipse(66, 48, 3, 3, 4);
    b.rect(36, 30, 8, 10, 5);
    b.outline(8, false);
    return b;
}

Bitmap paintTop() {
    Bitmap b(48, 48);
    b.ellipse(24, 8, 5, 7, 3);
    b.ellipse(24, 8, 2, 3, 4);
    b.ellipse(24, 40, 6, 8, 3);
    b.ellipse(24, 40, 2, 4, 4);
    b.rect(21, 12, 6, 20, 4);
    b.ellipse(24, 24, 9, 10, 1);
    b.ellipse(24, 22, 6, 7, 2);
    b.ellipse(24, 16, 6, 6, 3);
    b.ellipse(24, 17, 4, 3, 7);
    b.rect(16, 28, 16, 4, 6);
    b.line(16, 18, 8, 26, 1, 2.4f);
    b.line(32, 18, 40, 26, 1, 2.4f);
    b.outline(8, false);
    return b;
}

Bitmap paintBoard() {
    Bitmap b(52, 40);
    b.line(10, 34, 26, 8, 1, 4.f);
    b.line(42, 34, 26, 8, 1, 4.f);
    b.rect(6, 14, 40, 10, 1);
    for (int i = 0; i < 5; i++) b.rect(8 + i * 8, 14, 4, 10, (i & 1) ? 3 : 4);
    b.rect(8, 32, 8, 4, 2);
    b.rect(36, 32, 8, 4, 2);
    b.outline(2, false);
    return b;
}

Bitmap paintTree() {
    Bitmap b(40, 64);
    b.rect(17, 40, 6, 20, 3);
    b.rect(19, 44, 2, 14, 4);
    b.poly({{20, 4}, {38, 28}, {2, 28}}, 1);
    b.poly({{20, 16}, {36, 42}, {4, 42}}, 2);
    b.poly({{20, 28}, {32, 48}, {8, 48}}, 1);
    b.outline(5, false);
    return b;
}

Bitmap paintPost() {
    Bitmap b(16, 48);
    b.rect(6, 4, 4, 40, 1);
    b.rect(4, 8, 8, 3, 3);
    b.rect(4, 16, 8, 3, 4);
    b.ellipse(8, 4, 3, 3, 6);
    b.outline(2, false);
    return b;
}

Bitmap paintArch() {
    Bitmap b(120, 72);
    b.rect(6, 16, 14, 54, 1);
    b.rect(100, 16, 14, 54, 1);
    b.rect(8, 20, 4, 46, 2);
    b.rect(106, 20, 4, 46, 2);
    b.rect(6, 10, 108, 14, 1);
    b.rect(10, 14, 100, 6, 3);
    for (int i = 0; i < 8; i++) b.rect(16 + i * 12, 14, 6, 6, (i & 1) ? 4 : 3);
    b.ellipse(13, 12, 3, 3, 6);
    b.ellipse(107, 12, 3, 3, 6);
    b.outline(2, false);
    return b;
}

Bitmap paintShack() {
    Bitmap b(48, 40);
    b.rect(8, 18, 32, 18, 1);
    b.poly({{6, 18}, {24, 6}, {42, 18}}, 3);
    b.rect(20, 24, 8, 12, 2);
    b.rect(12, 22, 6, 6, 4);
    b.rect(30, 22, 6, 6, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintSilo() {
    Bitmap b(28, 72);
    b.rect(8, 16, 12, 50, 1);
    b.rect(10, 18, 3, 46, 2);
    b.ellipse(14, 16, 10, 6, 1);
    b.ellipse(14, 14, 6, 4, 3);
    b.rect(13, 4, 2, 12, 4);
    b.ellipse(14, 4, 2, 2, 6);
    b.outline(5, false);
    return b;
}

Bitmap paintCrate() {
    Bitmap b(36, 36);
    b.poly({{6, 14}, {18, 8}, {30, 14}, {18, 20}}, 1);
    b.poly({{6, 14}, {18, 20}, {18, 32}, {6, 26}}, 2);
    b.poly({{30, 14}, {18, 20}, {18, 32}, {30, 26}}, 4);
    b.line(10, 16, 16, 28, 6, 1.6f);
    b.line(14, 14, 22, 30, 6, 1.6f);
    b.outline(5, false);
    return b;
}

Bitmap paintDrum() {
    Bitmap b(32, 36);
    b.ellipse(16, 10, 10, 5, 3);
    b.rect(6, 10, 20, 18, 1);
    b.ellipse(16, 28, 10, 5, 2);
    b.rect(6, 16, 20, 3, 3);
    b.rect(6, 22, 20, 2, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintMast(bool lit) {
    Bitmap b(28, 52);
    b.rect(8, 28, 12, 18, 1);
    b.rect(10, 30, 8, 10, 2);
    b.rect(13, 8, 2, 22, 4);
    b.line(8, 14, 20, 14, 4, 1.4f);
    b.line(6, 18, 22, 18, 4, 1.4f);
    b.ellipse(14, 8, 3, 3, lit ? 6 : 5);
    b.outline(5, false);
    return b;
}

Bitmap paintTruck() {
    Bitmap b(56, 30);
    b.rect(8, 8, 22, 14, 1);
    b.rect(30, 6, 16, 18, 2);
    b.rect(34, 8, 8, 6, 5);
    b.rect(6, 10, 6, 4, 6);
    b.ellipse(16, 22, 5, 5, 3);
    b.ellipse(40, 22, 5, 5, 3);
    b.ellipse(16, 22, 2, 2, 4);
    b.ellipse(40, 22, 2, 2, 4);
    b.rect(18, 4, 10, 3, 6);
    b.outline(7, false);
    return b;
}

Bitmap paintPuff() {
    Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 6, 1);
    b.ellipse(10, 10, 4, 3, 2);
    return b;
}

Bitmap paintShadow() {
    Bitmap b(32, 14);
    b.ellipse(16, 7, 14, 5, 1);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.ellipse(4, 4, 1, 1, 2);
    return b;
}

Bitmap paintSun() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 12, 12, 2);
    b.ellipse(16, 16, 8, 8, 1);
    b.ellipse(14, 14, 3, 3, 3);
    return b;
}

Bitmap paintStars() {
    Bitmap b(240, 48);
    uint32_t r = 0x51A7C0DEu;
    for (int i = 0; i < 56; i++) {
        r = r * 1664525u + 1013904223u;
        int x = int(r >> 16) % b.w;
        r = r * 1664525u + 1013904223u;
        int y = int(r >> 17) % b.h;
        b.set(x, y, (i & 3) == 0 ? 2 : 1);
    }
    return b;
}

Bitmap paintHill() {
    Bitmap b(512, 32);
    for (int x = 0; x < b.w; x++) {
        float n = std::sin(x * 0.017f) * 5.5f + std::sin(x * 0.046f + 1.2f) * 2.6f + std::sin(x * 0.008f) * 3.f;
        int top = int(15 + n);
        if (top < 2) top = 2;
        for (int y = top; y < b.h; y++) b.set(x, y, y < top + 3 ? 1 : 2);
    }
    return b;
}

void loadFont(gs::VDP& vdp, Art& art, gs::TileAlloc& tiles) {
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
        art.font[c - 32] = t;
    }
}

void loadFloor(gs::VDP& vdp, Art& art, gs::TileAlloc& tiles) {
    uint8_t dirt[64] = {};
    uint8_t dirtB[64] = {};
    uint8_t fence[64] = {};
    uint8_t lane[64] = {};
    for (int i = 0; i < 64; i++) {
        dirt[i] = 1;
        dirtB[i] = 2;
    }
    dirt[3 * 8 + 2] = 3;
    dirt[5 * 8 + 6] = 3;
    dirt[1 * 8 + 5] = 2;
    dirtB[2 * 8 + 3] = 3;
    dirtB[6 * 8 + 1] = 1;
    dirtB[4 * 8 + 5] = 3;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) fence[y * 8 + x] = (y == 0 || y == 7) ? 6 : (y & 1) ? 4 : 5;
    }
    for (int i = 0; i < 64; i++) lane[i] = 7;
    for (int x = 1; x < 7; x++) lane[3 * 8 + x] = 8;
    for (int x = 1; x < 7; x++) lane[4 * 8 + x] = 8;
    art.dirt = makeTile(vdp, tiles, dirt);
    art.dirtB = makeTile(vdp, tiles, dirtB);
    art.fence = makeTile(vdp, tiles, fence);
    art.lane = makeTile(vdp, tiles, lane);
}

void stampYard(gs::VDP& vdp, const Art& art) {
    vdp.A.clear();
    for (int cy = 0; cy < 28; cy++) {
        for (int cx = 0; cx < 40; cx++) {
            float wx = ((cx * 8 + 4) - kYardCx) / kYardScale;
            float wy = ((cy * 8 + 4) - kYardCy) / kYardScale;
            float ax = std::fabs(wx), ay = std::fabs(wy);
            if (ax > kYardLimX || ay > kYardLimY) continue;
            bool inner = ax <= kYardInX && ay <= kYardInY;
            int tile;
            if (!inner) tile = art.fence;
            else if (std::fabs(wy) < 0.32f) tile = art.lane;
            else tile = ((cx + cy) & 1) ? art.dirt : art.dirtB;
            vdp.A.set(cx, cy, gs::entry(tile, PAL_FLOOR));
        }
    }
}

void roadPalettes(gs::VDP& vdp) {
    // Generator indices: 1-3 ground, 4-5 verge, 6-7 surface, 8 grit, 9 tracks, 10 crest, 14 paint.
    colors(vdp, PAL_ROAD,
           {gs::rgb4(3, 8, 2), gs::rgb4(2, 5, 1), gs::rgb4(6, 7, 3), gs::rgb4(7, 6, 3), gs::rgb4(5, 4, 2),
            gs::rgb4(9, 7, 4), gs::rgb4(6, 5, 3), gs::rgb4(5, 4, 3), gs::rgb4(4, 3, 2), gs::rgb4(11, 9, 5),
            gs::rgb4(3, 5, 8), gs::rgb4(2, 4, 7), gs::rgb4(5, 8, 10), gs::rgb4(13, 11, 4), gs::rgb4(10, 8, 5)});
    colors(vdp, PAL_YARD,
           {gs::rgb4(6, 6, 4), gs::rgb4(4, 4, 3), gs::rgb4(8, 7, 5), gs::rgb4(7, 6, 4), gs::rgb4(5, 5, 3),
            gs::rgb4(8, 7, 5), gs::rgb4(5, 5, 4), gs::rgb4(9, 8, 6), gs::rgb4(4, 4, 3), gs::rgb4(10, 9, 6),
            gs::rgb4(4, 5, 6), gs::rgb4(3, 4, 5), gs::rgb4(6, 7, 8), gs::rgb4(12, 10, 3), gs::rgb4(7, 6, 4)});
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_TEXT, gs::rgb4(15, 15, 15));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 8));
    vdp.setColor(PAL_TEXT * 16 + 2, gs::rgb4(11, 12, 14));

    colors(vdp, PAL_BIKE,
           {gs::rgb4(6, 8, 3), gs::rgb4(3, 5, 2), gs::rgb4(1, 1, 1), gs::rgb4(8, 8, 9), gs::rgb4(12, 8, 5),
            gs::rgb4(14, 2, 2), gs::rgb4(14, 10, 3), gs::rgb4(0, 0, 0), gs::rgb4(15, 13, 8)});
    colors(vdp, PAL_WOOD,
           {gs::rgb4(10, 7, 3), gs::rgb4(5, 3, 1), gs::rgb4(14, 3, 2), gs::rgb4(14, 13, 10), gs::rgb4(2, 1, 1)});
    colors(vdp, PAL_TREE,
           {gs::rgb4(3, 8, 2), gs::rgb4(2, 5, 1), gs::rgb4(7, 5, 2), gs::rgb4(4, 3, 1), gs::rgb4(1, 2, 1)});
    colors(vdp, PAL_STEEL,
           {gs::rgb4(9, 9, 10), gs::rgb4(5, 5, 6), gs::rgb4(13, 3, 2), gs::rgb4(6, 6, 7), gs::rgb4(4, 7, 10),
            gs::rgb4(15, 11, 3), gs::rgb4(1, 1, 2)});
    colors(vdp, PAL_FIRE,
           {gs::rgb4(15, 13, 4), gs::rgb4(14, 7, 2), gs::rgb4(15, 15, 13), gs::rgb4(8, 4, 2)});
    colors(vdp, PAL_MARK,
           {gs::rgb4(11, 8, 4), gs::rgb4(7, 5, 2), gs::rgb4(10, 10, 11), gs::rgb4(6, 4, 2), gs::rgb4(2, 2, 2),
            gs::rgb4(14, 3, 2), gs::rgb4(8, 8, 9)});
    colors(vdp, PAL_HILL, {gs::rgb4(4, 2, 6), gs::rgb4(2, 1, 3)});
    colors(vdp, PAL_LOGO, {gs::rgb4(15, 12, 4), gs::rgb4(5, 2, 1), gs::rgb4(15, 15, 14)});
    colors(vdp, PAL_FLOOR,
           {gs::rgb4(8, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(10, 8, 5), gs::rgb4(7, 5, 2), gs::rgb4(4, 3, 1),
            gs::rgb4(13, 3, 2), gs::rgb4(4, 4, 3), gs::rgb4(13, 11, 3)});
    roadPalettes(vdp);
    vdp.setFogColor(gs::rgb4(8, 5, 4));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    loadFloor(vdp, art, tiles);
    gs::bitmapToPlane(tiles, vdp.B, 0, 8, paintHill(), PAL_HILL);
    stampYard(vdp, art);

    Bitmap bike = paintBike();
    art.bike[0] = gs::uploadMipped(vdp, leanOf(bike, -0.16f));
    art.bike[1] = gs::uploadMipped(vdp, bike);
    art.bike[2] = gs::uploadMipped(vdp, leanOf(bike, 0.16f));
    Bitmap top = paintTop();
    for (int i = 0; i < 8; i++) art.top[i] = gs::uploadMipped(vdp, rotateUp(top, i * kPi * 0.25f));

    art.board = gs::uploadMipped(vdp, paintBoard());
    art.tree = gs::uploadMipped(vdp, paintTree());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.arch = gs::uploadMipped(vdp, paintArch());
    art.shack = gs::uploadMipped(vdp, paintShack());
    art.silo = gs::uploadMipped(vdp, paintSilo());
    art.crate = gs::uploadMipped(vdp, paintCrate());
    art.drum = gs::uploadMipped(vdp, paintDrum());
    art.mast[0] = gs::uploadMipped(vdp, paintMast(false));
    art.mast[1] = gs::uploadMipped(vdp, paintMast(true));
    art.truck = gs::uploadMipped(vdp, paintTruck());
    art.bullet = gs::uploadMipped(vdp, paintLamp());
    art.puff = gs::uploadMipped(vdp, paintPuff());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.sun = gs::uploadMipped(vdp, paintSun());
    art.stars = gs::uploadMipped(vdp, paintStars());

    gs::TextStyle st{3, 1, 2, 0, 1};
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("S3 RAID", st));
}

}  // namespace raid
