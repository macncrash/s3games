#include "game/art.h"

#include <cmath>
#include <cstring>
#include <initializer_list>

namespace outpost {
namespace {

using gs::Bitmap;

void pal(gs::VDP& vdp, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(p * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(p * 16 + i++, 0);
}

int tileOf(gs::VDP& vdp, gs::TileAlloc& tiles, const uint8_t* px) {
    int t = tiles.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
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
        a.font[c - 32] = tileOf(vdp, tiles, px);
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
    uint8_t on[64], dim[64], mark[64];
    for (int i = 0; i < 64; i++) {
        int x = i % 8, y = i / 8;
        bool edge = x == 0 || y == 0 || y == 7;
        on[i] = edge ? 2 : 6;
        dim[i] = edge ? 7 : 3;
        mark[i] = (x >= 3 && x <= 5) ? 1 : 6;
    }
    a.pipOn = tileOf(vdp, tiles, on);
    a.pipDim = tileOf(vdp, tiles, dim);
    a.pipMark = tileOf(vdp, tiles, mark);
}

// 1 coat, 2 shade, 3 skin, 4 cap, 5 wood, 6 steel, 7 boot, 8 belt, 9 eye, 15 line.
Bitmap sentryArt(int step) {
    Bitmap b(40, 48);
    int s = step ? 3 : -2;
    b.ellipse(20, 11, 8, 7, 4);
    b.rect(14, 6, 12, 4, 4);
    b.ellipse(20, 16, 6, 6, 3);
    b.rect(17, 15, 2, 2, 9);
    b.rect(22, 15, 2, 2, 9);
    b.poly({{10, 22}, {32, 22}, {34, 38}, {8, 38}}, 1);
    b.poly({{16, 24}, {26, 24}, {25, 36}, {15, 36}}, 2);
    b.rect(12, 32, 16, 3, 8);
    b.rect(12, 38, 6, 7, 7);
    b.rect(22 + s, 38, 6, 7, 7);
    b.ellipse(15, 46, 5, 2, 7);
    b.ellipse(25 + s, 46, 5, 2, 7);
    b.poly({{22, 26}, {36, 18}, {38, 21}, {24, 30}}, 5);
    b.rect(34, 15, 4, 3, 6);
    b.rect(36, 14, 2, 2, 6);
    b.outline(15, false);
    return b;
}

// 1 hide, 2 shade, 3 rag, 4 eye, 5 claw, 6 tooth, 15 line.
Bitmap stalkerArt(int step) {
    Bitmap b(36, 46);
    int s = step ? 2 : 0;
    b.ellipse(18, 14, 9, 10, 1);
    b.ellipse(18, 16, 6, 7, 2);
    b.rect(13, 12, 3, 2, 4);
    b.rect(20, 12, 3, 2, 4);
    b.rect(16, 18, 4, 2, 6);
    b.poly({{8, 22}, {28, 20}, {30, 36}, {6, 38}}, 1);
    b.poly({{14, 24}, {24, 23}, {23, 34}, {12, 35}}, 2);
    b.rect(6, 24, 5, 12, 3);
    b.rect(26, 22, 5, 13, 3);
    b.rect(5, 34, 4, 3, 5);
    b.rect(28, 33, 4, 3, 5);
    b.rect(10 - s, 36, 5, 8, 2);
    b.rect(20 + s, 36, 5, 8, 2);
    b.ellipse(12 - s, 44, 4, 2, 5);
    b.ellipse(23 + s, 44, 4, 2, 5);
    b.outline(15, false);
    return b;
}

// 1 bulk, 2 shade, 3 plate, 4 eye, 5 horn, 6 claw, 15 line.
Bitmap bruteArt(int step) {
    Bitmap b(48, 52);
    int s = step ? 3 : 0;
    b.ellipse(24, 16, 12, 12, 1);
    b.ellipse(24, 18, 8, 8, 2);
    b.poly({{16, 6}, {20, 16}, {14, 14}}, 5);
    b.poly({{32, 6}, {28, 16}, {34, 14}}, 5);
    b.rect(17, 15, 4, 3, 4);
    b.rect(27, 15, 4, 3, 4);
    b.rect(21, 21, 6, 2, 6);
    b.poly({{8, 24}, {40, 24}, {42, 40}, {6, 40}}, 1);
    b.rect(14, 26, 20, 10, 3);
    b.rect(6, 26, 6, 14, 2);
    b.rect(36, 26, 6, 14, 2);
    b.rect(4, 38, 5, 4, 6);
    b.rect(39, 38, 5, 4, 6);
    b.rect(12, 40, 8, 9, 2);
    b.rect(28 + s, 40, 8, 9, 2);
    b.ellipse(16, 50, 6, 2, 6);
    b.ellipse(32 + s, 50, 6, 2, 6);
    b.outline(15, false);
    return b;
}

// 1 wall, 2 shade, 3 roof, 4 roof dark, 5 window, 6 door, 7 metal, 8 sand, 15 line.
Bitmap hutArt() {
    Bitmap b(52, 44);
    b.poly({{26, 2}, {50, 18}, {2, 18}}, 3);
    b.poly({{26, 6}, {44, 18}, {8, 18}}, 4);
    b.rect(8, 18, 36, 20, 1);
    b.rect(10, 20, 14, 16, 2);
    b.rect(22, 24, 10, 14, 6);
    b.rect(24, 30, 2, 4, 7);
    b.rect(36, 22, 6, 6, 5);
    b.rect(37, 23, 4, 2, 1);
    b.rect(4, 34, 10, 6, 8);
    b.rect(38, 34, 10, 6, 8);
    b.rect(24, 0, 4, 6, 7);
    b.outline(15, false);
    return b;
}

Bitmap mastArt() {
    Bitmap b(20, 46);
    b.rect(9, 8, 2, 36, 7);
    b.rect(4, 10, 12, 2, 7);
    b.rect(2, 8, 3, 6, 2);
    b.ellipse(16, 12, 4, 3, 5);
    b.rect(8, 40, 4, 4, 8);
    b.outline(15, false);
    return b;
}

// 1 dark, 2 mid, 3 light, 4 trunk, 5 shade, 15 line.
Bitmap treeArt() {
    Bitmap b(36, 48);
    b.poly({{18, 2}, {32, 20}, {4, 20}}, 1);
    b.poly({{18, 12}, {34, 30}, {2, 30}}, 2);
    b.poly({{18, 22}, {34, 38}, {2, 38}}, 3);
    b.rect(15, 36, 6, 10, 4);
    b.rect(16, 38, 2, 8, 5);
    b.outline(15, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(10, 18);
    b.rect(4, 0, 2, 16, 7);
    b.rect(2, 14, 6, 3, 8);
    b.rect(3, 3, 4, 1, 6);
    b.outline(15, false);
    return b;
}

Bitmap flareArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 5, 5, 3);
    b.ellipse(8, 8, 3, 3, 2);
    b.ellipse(8, 8, 1.6f, 1.6f, 1);
    b.rect(7, 1, 2, 3, 2);
    b.rect(7, 12, 2, 3, 3);
    b.rect(1, 7, 3, 2, 2);
    b.rect(12, 7, 3, 2, 3);
    return b;
}

Bitmap glowArt() {
    Bitmap b(64, 64);
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            float dx = x - 31.5f, dy = y - 31.5f;
            float d = std::sqrt(dx * dx + dy * dy) / 31.5f;
            if (d > 1.f) continue;
            int c = 0;
            if (d < 0.18f) c = 1;
            else if (d < 0.38f) c = 2;
            else if (d < 0.58f) c = 3;
            else if (d < 0.78f) c = 4;
            else if (((x + y) & 1) == 0) c = 5;
            if (c) b.set(x, y, c);
        }
    }
    return b;
}

Bitmap boltArt() {
    Bitmap b(12, 4);
    b.rect(0, 1, 12, 2, 2);
    b.rect(8, 0, 4, 4, 1);
    return b;
}

Bitmap glintArt() {
    Bitmap b(10, 6);
    b.rect(1, 2, 3, 2, 4);
    b.rect(6, 2, 3, 2, 4);
    return b;
}

Bitmap aimArt() {
    Bitmap b(12, 12);
    b.rect(5, 1, 2, 10, 2);
    b.rect(1, 5, 10, 2, 2);
    b.rect(5, 5, 2, 2, 1);
    return b;
}

Bitmap chevArt() {
    Bitmap b(14, 12);
    b.poly({{2, 10}, {7, 1}, {12, 10}, {7, 7}}, 2);
    b.poly({{5, 8}, {7, 3}, {9, 8}}, 1);
    return b;
}

Bitmap sunArt() {
    Bitmap b(18, 18);
    b.ellipse(9, 9, 7, 7, 3);
    b.ellipse(9, 9, 4, 4, 2);
    b.ellipse(9, 9, 2, 2, 1);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(28, 12);
    b.ellipse(14, 6, 12, 4, 1);
    return b;
}

void groundTiles(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    auto fill = [&](int base, int spec, int nspec) {
        uint8_t px[64];
        std::memset(px, uint8_t(base), sizeof px);
        uint32_t h = uint32_t(base * 97 + nspec * 13);
        for (int i = 0; i < nspec; i++) {
            h = h * 1664525u + 1013904223u;
            px[(h >> 8) % 64] = uint8_t(spec);
        }
        return tileOf(vdp, tiles, px);
    };
    a.grass = fill(1, 2, 7);
    a.moss = fill(3, 1, 5);
    a.dirt = fill(4, 5, 6);
    a.yard = fill(6, 5, 4);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    pal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 10), gs::rgb4(14, 10, 3), gs::rgb4(5, 6, 8), gs::rgb4(13, 3, 2),
                       gs::rgb4(6, 12, 6), gs::rgb4(15, 12, 5), gs::rgb4(3, 4, 7), gs::rgb4(8, 9, 10),
                       gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    pal(vdp, PAL_SENT, {0, gs::rgb4(3, 5, 3), gs::rgb4(2, 3, 2), gs::rgb4(12, 8, 6), gs::rgb4(4, 4, 3),
                        gs::rgb4(8, 5, 2), gs::rgb4(11, 12, 13), gs::rgb4(2, 2, 2), gs::rgb4(6, 5, 2),
                        gs::rgb4(2, 2, 1), 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
    pal(vdp, PAL_THEM, {0, gs::rgb4(4, 3, 6), gs::rgb4(2, 1, 3), gs::rgb4(5, 4, 5), gs::rgb4(14, 14, 9),
                        gs::rgb4(8, 7, 6), gs::rgb4(10, 9, 8), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 0, 2)});
    pal(vdp, PAL_BRUTE, {0, gs::rgb4(5, 3, 4), gs::rgb4(3, 1, 2), gs::rgb4(7, 5, 4), gs::rgb4(14, 3, 2),
                         gs::rgb4(8, 6, 4), gs::rgb4(10, 8, 7), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 0, 1)});
    pal(vdp, PAL_FLARE, {0, gs::rgb4(15, 15, 13), gs::rgb4(15, 13, 4), gs::rgb4(15, 8, 2), gs::rgb4(12, 4, 1),
                         gs::rgb4(8, 2, 1), gs::rgb4(15, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 0)});
    pal(vdp, PAL_HUT, {0, gs::rgb4(8, 6, 4), gs::rgb4(5, 4, 3), gs::rgb4(7, 2, 2), gs::rgb4(4, 1, 1),
                       gs::rgb4(14, 12, 5), gs::rgb4(3, 2, 2), gs::rgb4(9, 10, 11), gs::rgb4(7, 6, 4), 0, 0, 0, 0,
                       0, 0, gs::rgb4(1, 1, 1)});
    pal(vdp, PAL_PINE, {0, gs::rgb4(1, 3, 2), gs::rgb4(2, 5, 3), gs::rgb4(4, 7, 4), gs::rgb4(5, 3, 2),
                        gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 1, 1)});
    pal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 12, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    pal(vdp, PAL_GROUND, {0, gs::rgb4(1, 3, 2), gs::rgb4(2, 5, 3), gs::rgb4(0, 2, 1), gs::rgb4(5, 4, 2),
                          gs::rgb4(3, 3, 2), gs::rgb4(6, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0});

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);
    groundTiles(vdp, art, tiles);

    art.sentry[0] = gs::uploadMipped(vdp, sentryArt(0));
    art.sentry[1] = gs::uploadMipped(vdp, sentryArt(1));
    art.stalker[0] = gs::uploadMipped(vdp, stalkerArt(0));
    art.stalker[1] = gs::uploadMipped(vdp, stalkerArt(1));
    art.brute[0] = gs::uploadMipped(vdp, bruteArt(0));
    art.brute[1] = gs::uploadMipped(vdp, bruteArt(1));
    art.hut = gs::uploadMipped(vdp, hutArt());
    art.mast = gs::uploadMipped(vdp, mastArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.flare = gs::uploadMipped(vdp, flareArt());
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.glint = gs::uploadMipped(vdp, glintArt());
    art.aim = gs::uploadMipped(vdp, aimArt());
    art.chev = gs::uploadMipped(vdp, chevArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    vdp.setColor(9 * 16 + 1, gs::rgb4(15, 4, 3));
    vdp.setColor(9 * 16 + 15, gs::rgb4(2, 0, 0));
}

}  // namespace outpost
