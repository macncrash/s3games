#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <string>

namespace beds {
namespace {

constexpr float PI = 3.14159265f;

uint16_t rgb(int r, int g, int b) { return gs::rgb4(r, g, b); }

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 0x8da6b343u ^ uint32_t(y) * 0xd8163841u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    return h ^ (h >> 15);
}

void pal(gs::VDP& vdp, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(p * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(p * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
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

void paintPalettes(gs::VDP& vdp) {
    pal(vdp, PAL_HUD, {0, rgb(15, 15, 15), rgb(2, 3, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, rgb(1, 2, 5)});
    pal(vdp, PAL_GOLD, {0, rgb(15, 13, 4), rgb(5, 3, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, rgb(4, 2, 0)});
    pal(vdp, PAL_WARN, {0, rgb(15, 5, 3), rgb(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, rgb(5, 0, 1)});
    pal(vdp, PAL_GOOD, {0, rgb(7, 15, 6), rgb(0, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, rgb(0, 3, 1)});
    pal(vdp, PAL_DIM, {0, rgb(9, 10, 12), rgb(2, 2, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, rgb(1, 1, 3)});
    pal(vdp, PAL_WOOD, {0, rgb(3, 2, 1), rgb(13, 9, 4), rgb(10, 6, 2), rgb(6, 4, 1), rgb(8, 8, 7), rgb(4, 8, 2), rgb(15, 12, 6),
                        rgb(5, 3, 1), 0, 0, 0, 0, 0, 0, 0});
    pal(vdp, PAL_DRY, {0, rgb(3, 2, 1), rgb(13, 10, 5), rgb(10, 7, 3), rgb(6, 4, 1), rgb(8, 7, 5), rgb(5, 3, 1), rgb(14, 12, 7), 0,
                       0, 0, 0, 0, 0, 0, 0});
    pal(vdp, PAL_WET, {0, rgb(1, 2, 2), rgb(6, 8, 4), rgb(3, 5, 3), rgb(2, 3, 2), rgb(8, 11, 8), rgb(3, 6, 9), rgb(1, 3, 4), 0, 0,
                       0, 0, 0, 0, 0, 0});
    pal(vdp, PAL_PLANT,
        {0, rgb(1, 2, 1), rgb(9, 14, 4), rgb(4, 11, 3), rgb(2, 7, 2), rgb(3, 8, 2), rgb(14, 3, 3), rgb(15, 12, 2), rgb(15, 14, 12),
         rgb(14, 8, 2), rgb(8, 6, 3), rgb(5, 4, 2), rgb(13, 5, 7), 0, 0, 0});
    pal(vdp, PAL_MAN, {0, rgb(2, 1, 1), rgb(14, 10, 6), rgb(10, 6, 4), rgb(3, 8, 13), rgb(2, 5, 9), rgb(4, 4, 7), rgb(2, 2, 4),
                       rgb(14, 11, 4), rgb(11, 4, 2), rgb(3, 2, 2), rgb(11, 12, 13), rgb(6, 7, 8), rgb(15, 15, 14), rgb(4, 10, 15),
                       rgb(4, 3, 2)});
    pal(vdp, PAL_SUN, {0, rgb(15, 15, 12), rgb(15, 12, 4), rgb(15, 8, 2), rgb(15, 13, 6), rgb(15, 15, 14), rgb(12, 11, 13),
                       rgb(2, 2, 4), rgb(8, 6, 4), 0, 0, 0, 0, 0, 0, 0});
    pal(vdp, PAL_LIGHT, {0, rgb(15, 15, 11), rgb(15, 12, 5), rgb(14, 9, 3), rgb(11, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    pal(vdp, PAL_BRICK, {0, rgb(8, 7, 6), rgb(12, 4, 3), rgb(8, 2, 2), rgb(14, 7, 4), rgb(12, 11, 9), rgb(9, 8, 7), rgb(4, 7, 3),
                         rgb(5, 3, 2), 0, 0, 0, 0, 0, 0, 0});
    pal(vdp, PAL_YARD, {0, rgb(7, 13, 4), rgb(3, 9, 3), rgb(2, 6, 2), rgb(8, 6, 3), rgb(14, 12, 3), rgb(13, 6, 6), rgb(13, 11, 7),
                        rgb(9, 8, 6), rgb(6, 5, 4), rgb(8, 8, 7), 0, 0, 0, 0, 0});
    pal(vdp, PAL_WATER, {0, rgb(12, 15, 15), rgb(3, 10, 15), rgb(1, 5, 12), rgb(2, 3, 5), rgb(8, 13, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
}

gs::Bitmap bedFrame() {
    gs::Bitmap b(60, 44);
    b.rect(2, 3, 56, 6, 2);
    b.rect(2, 3, 56, 2, 7);
    b.rect(2, 8, 5, 32, 3);
    b.rect(53, 8, 5, 32, 4);
    b.rect(2, 30, 56, 5, 2);
    b.rect(2, 35, 56, 5, 3);
    b.rect(2, 8, 56, 4, 4);
    for (int i = 0; i < 4; i++) b.rect(10 + i * 12, 32, 2, 2, 5);
    b.rect(8, 39, 7, 2, 6);
    b.rect(40, 40, 8, 2, 6);
    b.rect(6, 9, 3, 2, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap soilBmp() {
    gs::Bitmap b(46, 22);
    b.rect(1, 1, 44, 20, 3);
    b.ellipse(14, 10, 9, 6, 2);
    b.ellipse(32, 12, 8, 5, 4);
    b.ellipse(24, 7, 5, 3, 2);
    b.rect(18, 14, 4, 2, 5);
    b.rect(8, 6, 3, 2, 6);
    b.rect(36, 8, 3, 2, 5);
    b.rect(12, 16, 5, 2, 6);
    b.outline(1, false);
    return b;
}

void leaf(gs::Bitmap& b, float x, float y, float rx, float ry, int c) {
    if (rx < 0.8f || ry < 0.8f) return;
    b.ellipse(x, y, rx, ry, c);
}

void crop(gs::Bitmap& b, int kind, int stage) {
    const bool wilt = stage == 3;
    const float s = stage == 0 ? 0.55f : stage == 1 ? 0.8f : wilt ? 0.62f : 1.f;
    const int leafC = wilt ? 10 : 3;
    const int leafD = wilt ? 11 : 4;
    const int leafL = wilt ? 10 : 2;
    const int stem = wilt ? 11 : 5;
    const float base = 30.f;
    const float cx = 22.f;
    auto up = [&](float y) { return base - (base - y) * s; };

    if (kind == 0) {
        leaf(b, cx, up(22), 12 * s, 6 * s, leafC);
        leaf(b, cx - 8, up(18), 7 * s, 5 * s, leafD);
        leaf(b, cx + 8, up(18), 7 * s, 5 * s, leafL);
        leaf(b, cx, up(14), 6 * s, 4 * s, leafL);
        if (stage == 2) leaf(b, cx, up(12), 3.5f, 2.5f, 2);
    } else if (kind == 1) {
        b.line(cx, base, cx, up(12), stem, 2.f);
        b.line(cx, up(20), cx - 9 * s, up(14), stem, 1.5f);
        b.line(cx, up(20), cx + 9 * s, up(14), stem, 1.5f);
        leaf(b, cx - 9 * s, up(14), 4 * s, 3 * s, leafC);
        leaf(b, cx + 9 * s, up(14), 4 * s, 3 * s, leafD);
        if (stage >= 1) {
            b.ellipse(cx - 5, up(22), 3.2f * s, 3.2f * s, wilt ? 11 : 6);
            b.ellipse(cx + 6, up(20), 3.f * s, 3.f * s, wilt ? 10 : 6);
        }
        if (stage == 2) b.ellipse(cx, up(13), 3.4f, 3.4f, 6);
    } else if (kind == 2) {
        for (int i = 0; i < 5; i++) {
            float x = cx - 8 + i * 4;
            float h = (8.f + float((i * 3) % 5)) * s;
            b.line(x, base, x + (wilt ? (i - 2) * 1.4f : 0), base - h, i % 2 ? leafC : leafL, 1.6f);
        }
        if (stage == 2) leaf(b, cx, up(14), 4, 2.5f, 2);
    } else if (kind == 3) {
        b.line(cx - 4, base, cx - 6, up(14), stem, 1.4f);
        b.line(cx + 1, base, cx + 2, up(12), stem, 1.4f);
        b.line(cx + 6, base, cx + 8, up(15), stem, 1.4f);
        leaf(b, cx - 7, up(14), 4 * s, 2.4f * s, leafL);
        leaf(b, cx + 2, up(11), 4 * s, 2.4f * s, leafC);
        leaf(b, cx + 9, up(15), 3.5f * s, 2.f * s, leafD);
        if (stage >= 1 && !wilt) {
            b.rect(cx - 5, base - 4, 3, 4, 9);
            b.rect(cx + 3, base - 5, 3, 5, 9);
        }
    } else if (kind == 4) {
        b.rect(cx - 8, up(12), 2, base - up(12), wilt ? 11 : 10);
        b.rect(cx + 7, up(12), 2, base - up(12), wilt ? 11 : 10);
        b.line(cx - 7, up(14), cx + 8, up(14), wilt ? 11 : 10, 1.4f);
        leaf(b, cx - 3, up(18), 4 * s, 3 * s, leafC);
        leaf(b, cx + 4, up(16), 4 * s, 3 * s, leafD);
        leaf(b, cx, up(13), 3.5f * s, 3.f * s, leafL);
        if (stage == 2) {
            b.ellipse(cx - 2, up(20), 2.2f, 2.6f, 12);
            b.ellipse(cx + 5, up(18), 2.f, 2.4f, 12);
        }
    } else {
        b.line(cx - 6, base, cx - 6, up(16), stem, 1.4f);
        b.line(cx, base, cx + 1, up(12), stem, 1.4f);
        b.line(cx + 6, base, cx + 5, up(15), stem, 1.4f);
        int blooms[3] = {wilt ? 10 : 6, wilt ? 11 : 7, wilt ? 10 : 8};
        leaf(b, cx - 6, up(15), 3.5f * s, 3.5f * s, blooms[0]);
        leaf(b, cx + 1, up(11), 4.f * s, 4.f * s, blooms[1]);
        leaf(b, cx + 6, up(15), 3.2f * s, 3.2f * s, blooms[2]);
        if (stage == 2) {
            b.set(int(cx - 6), int(up(15)), 8);
            b.set(int(cx + 1), int(up(11)), 6);
            b.set(int(cx + 6), int(up(15)), 7);
        }
        leaf(b, cx - 2, up(22), 5 * s, 2.5f * s, leafD);
        leaf(b, cx + 4, up(22), 4 * s, 2.2f * s, leafC);
    }
    b.outline(1, false);
}

gs::Bitmap manBmp(int pose) {
    gs::Bitmap b(28, 30);
    b.ellipse(13, 8, 8, 3.2f, 8);
    b.rect(8, 5, 10, 4, 8);
    b.rect(9, 8, 8, 2, 9);
    b.ellipse(13, 13, 4.6f, 4.2f, 2);
    b.set(11, 12, 3);
    b.set(15, 12, 3);
    b.rect(9, 11, 2, 3, 15);
    b.rect(16, 11, 2, 3, 15);
    b.rect(9, 16, 9, 7, 4);
    b.rect(9, 20, 9, 3, 5);
    b.rect(7, 17, 2, 5, 2);
    if (pose == 0 || pose == 1) b.rect(18, 17, 2, 5, 4);
    b.rect(9, 22, 4, 3, 6);
    b.rect(14, 22, 4, 3, pose == 1 ? 7 : 6);
    if (pose == 1) {
        b.rect(8, 25, 4, 2, 10);
        b.rect(15, 26, 5, 2, 10);
    } else {
        b.rect(8, 25, 5, 2, 10);
        b.rect(14, 25, 5, 2, 10);
    }
    if (pose == 2) {
        b.rect(17, 12, 8, 4, 11);
        b.rect(24, 11, 2, 2, 12);
        b.rect(18, 13, 3, 1, 13);
        b.rect(20, 16, 2, 4, 2);
    } else if (pose == 3) {
        b.rect(17, 19, 8, 4, 11);
        b.rect(24, 22, 2, 2, 12);
        b.rect(18, 20, 3, 1, 13);
    } else {
        b.rect(18, 18, 7, 5, 11);
        b.rect(24, 19, 2, 2, 12);
        b.rect(19, 19, 2, 1, 13);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap sunBmp(float ang) {
    gs::Bitmap b(36, 36);
    float cx = 17.5f, cy = 17.5f;
    for (int i = 0; i < 8; i++) {
        float a = ang + i * PI / 4.f;
        float x1 = cx + std::cos(a) * 8.f;
        float y1 = cy + std::sin(a) * 8.f;
        float x2 = cx + std::cos(a) * 16.f;
        float y2 = cy + std::sin(a) * 16.f;
        b.line(x1, y1, x2, y2, 4, 2.f);
    }
    b.ellipse(cx, cy, 8, 8, 3);
    b.ellipse(cx, cy, 6, 6, 2);
    b.ellipse(cx - 1, cy - 1, 3.2f, 3.2f, 1);
    return b;
}

gs::Bitmap cloudBmp() {
    gs::Bitmap b(40, 16);
    b.ellipse(14, 9, 10, 5, 6);
    b.ellipse(24, 8, 12, 6, 5);
    b.ellipse(30, 10, 7, 4, 6);
    b.ellipse(18, 7, 6, 3, 5);
    return b;
}

gs::Bitmap birdBmp(int flap) {
    gs::Bitmap b(14, 8);
    if (flap == 0) {
        b.line(1, 5, 6, 3, 7, 1.4f);
        b.line(6, 3, 12, 5, 7, 1.4f);
    } else {
        b.line(1, 2, 6, 4, 7, 1.4f);
        b.line(6, 4, 12, 2, 7, 1.4f);
    }
    b.set(6, 4, 7);
    return b;
}

gs::Bitmap lightBmp() {
    gs::Bitmap b(320, 176);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x, y * 3 + 1);
            bool edge = x >= 312;
            int dens = edge ? 230 : 78 + (y * 40) / b.h;
            if (int(h & 255) > dens) continue;
            int c = edge ? 1 : 2 + int((h >> 8) % 3);
            b.set(x, y, c);
        }
    }
    return b;
}

gs::Bitmap dropBmp() {
    gs::Bitmap b(6, 8);
    b.ellipse(3, 4, 2.2f, 3.f, 2);
    b.set(3, 2, 1);
    b.set(2, 4, 5);
    b.set(3, 6, 3);
    return b;
}

gs::Bitmap sparkBmp() {
    gs::Bitmap b(7, 7);
    b.set(3, 1, 1);
    b.set(3, 2, 1);
    b.set(3, 3, 2);
    b.set(3, 4, 1);
    b.set(3, 5, 1);
    b.set(1, 3, 1);
    b.set(2, 3, 1);
    b.set(4, 3, 1);
    b.set(5, 3, 1);
    return b;
}

gs::Bitmap barBg() {
    gs::Bitmap b(40, 4);
    b.rect(0, 0, 40, 4, 4);
    b.rect(1, 1, 38, 2, 3);
    return b;
}

gs::Bitmap barFg() {
    gs::Bitmap b(40, 4);
    b.rect(0, 0, 40, 4, 2);
    b.rect(0, 0, 40, 1, 1);
    return b;
}

gs::Bitmap barrelBmp() {
    gs::Bitmap b(22, 26);
    b.ellipse(11, 6, 8, 3, 2);
    b.rect(3, 6, 16, 14, 3);
    b.ellipse(11, 20, 8, 3, 4);
    b.rect(3, 10, 16, 2, 7);
    b.rect(3, 15, 16, 2, 7);
    b.rect(10, 6, 2, 14, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap vineBmp() {
    gs::Bitmap b(18, 72);
    b.line(9, 2, 8, 70, 5, 1.6f);
    for (int i = 0; i < 8; i++) {
        float y = 8.f + i * 8.f;
        float x = 9.f + ((i & 1) ? 4.f : -4.f);
        leaf(b, x, y, 4.f, 2.4f, (i % 3) == 0 ? 2 : 3);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap shadowBmp() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 9, 3, 1);
    return b;
}

void tileGrass(uint8_t* px, int seed) {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            uint32_t h = hash2(x + seed * 5, y + seed * 9);
            int c = 2;
            if ((h % 6) == 0) c = 1;
            else if ((h % 7) == 0) c = 3;
            if ((h % 19) == 0) c = 4;
            if ((h % 23) == 0) c = (h & 1) ? 5 : 6;
            px[y * 8 + x] = uint8_t(c);
        }
}

void tileGravel(uint8_t* px, int seed) {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            uint32_t h = hash2(x + 40 + seed, y + 17);
            int c = 8;
            if ((h % 5) == 0) c = 7;
            else if ((h % 4) == 0) c = 9;
            if ((h % 11) == 0) c = 10;
            px[y * 8 + x] = uint8_t(c);
        }
}

void tileBrick(uint8_t* px, int variant) {
    for (int y = 0; y < 8; y++) {
        bool mortarRow = (y % 4) == 0;
        int mortarCol = (y < 4) ? 7 : 3;
        if (variant) mortarCol = (mortarCol + 4) & 7;
        for (int x = 0; x < 8; x++) {
            int c;
            if (mortarRow || x == mortarCol) c = 1;
            else {
                uint32_t h = hash2(x + variant * 3, y + 9);
                c = (h % 5) == 0 ? 4 : (h % 4) == 0 ? 3 : 2;
            }
            px[y * 8 + x] = uint8_t(c);
        }
    }
}

void tilePier(uint8_t* px) {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            bool edge = x < 2;
            bool crack = (y == 3) || (x == 5 && y > 3);
            int c = edge ? 8 : crack ? 6 : (y < 2 ? 5 : 6);
            if (!edge && ((x + y) % 7) == 0) c = 5;
            px[y * 8 + x] = uint8_t(c);
        }
}

void tileCap(uint8_t* px) {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int c = y < 2 ? 5 : y > 5 ? 1 : 6;
            if (y == 2) c = 5;
            if ((x == 0 || x == 7) && y > 1) c = 8;
            px[y * 8 + x] = uint8_t(c);
        }
}

bool pathAt(int x, int y) {
    if (x >= 256) return false;
    if (y >= 102 && y < 150) return true;
    if (y >= 48 && y < 214) {
        if (x < 16 || x >= 232) return true;
        if (x >= 76 && x < 94) return true;
        if (x >= 154 && x < 172) return true;
    }
    if (y >= 194 && y < 224 && x < 256) return true;
    if (y >= 40 && y < 58 && x < 256) return true;
    return false;
}

void paintYard(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    uint8_t px[64];
    for (int i = 0; i < 4; i++) {
        tileGrass(px, i + 1);
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.grass[i] = t;
    }
    for (int i = 0; i < 2; i++) {
        tileGravel(px, i + 1);
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.gravel[i] = t;
    }
    for (int i = 0; i < 2; i++) {
        tileBrick(px, i);
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.brick[i] = t;
    }
    tilePier(px);
    a.pier = tiles.alloc(1);
    vdp.loadTile(a.pier, px);
    tileCap(px);
    a.cap = tiles.alloc(1);
    vdp.loadTile(a.cap, px);

    vdp.A.clear();
    vdp.B.clear();
    for (int ty = 0; ty < 28; ty++) {
        for (int tx = 0; tx < 40; tx++) {
            int x = tx * 8 + 4;
            int y = ty * 8 + 4;
            if (tx >= 32 && ty >= 5) {
                int tile = ty == 5 ? a.cap : tx == 32 ? a.pier : a.brick[(tx + ty) & 1];
                vdp.A.set(tx, ty, gs::entry(tile, PAL_BRICK));
                continue;
            }
            if (ty < 5) continue;
            bool path = pathAt(x, y);
            int tile = path ? a.gravel[(tx + ty) & 1] : a.grass[(tx * 3 + ty) & 3];
            vdp.B.set(tx, ty, gs::entry(tile, PAL_YARD));
        }
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    paintPalettes(vdp);
    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    paintYard(vdp, art, tiles);

    gs::TextStyle big{4, 1, 0, 15, 1};
    gs::TextStyle mid{2, 1, 0, 15, 1};
    gs::TextStyle small{1, 1, 0, 15, 1};
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("S3 BEDS", big));
    art.sub = gs::uploadMipped(vdp, gs::textBitmap("SIX BEDS", mid));
    art.tag = gs::uploadMipped(vdp, gs::textBitmap("WATER THEM BEFORE THE SUN HITS THE WALL", small));
    art.watered = gs::uploadMipped(vdp, gs::textBitmap("WATERED", big));
    art.late = gs::uploadMipped(vdp, gs::textBitmap("TOO LATE", big));

    art.frame = gs::uploadMipped(vdp, bedFrame());
    art.soil = gs::uploadMipped(vdp, soilBmp());
    for (int c = 0; c < 6; c++)
        for (int s = 0; s < 4; s++) {
            gs::Bitmap b(44, 34);
            crop(b, c, s);
            art.plant[c][s] = gs::uploadMipped(vdp, b);
        }
    for (int p = 0; p < 4; p++) art.man[p] = gs::uploadMipped(vdp, manBmp(p));
    art.shadow = gs::uploadMipped(vdp, shadowBmp());
    for (int i = 0; i < 4; i++) art.sun[i] = gs::uploadMipped(vdp, sunBmp(i * 0.18f));
    art.cloud = gs::uploadMipped(vdp, cloudBmp());
    art.bird[0] = gs::uploadMipped(vdp, birdBmp(0));
    art.bird[1] = gs::uploadMipped(vdp, birdBmp(1));
    art.light = gs::uploadMipped(vdp, lightBmp());
    art.drop = gs::uploadMipped(vdp, dropBmp());
    art.spark = gs::uploadMipped(vdp, sparkBmp());
    art.barBg = gs::uploadMipped(vdp, barBg());
    art.barFg = gs::uploadMipped(vdp, barFg());
    art.barrel = gs::uploadMipped(vdp, barrelBmp());
    art.vine = gs::uploadMipped(vdp, vineBmp());
    vdp.setFogColor(rgb(15, 10, 6));
}

}  // namespace beds
