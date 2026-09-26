#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace standard {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; ++i) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void tileFrom(uint8_t px[64], const char* const rows[8]) {
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x) {
            char c = rows[y][x];
            px[y * 8 + x] = c == '.' ? 0 : uint8_t(c - '0');
        }
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{2, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
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

// Rear of a dispatch bike. 1 coat, 2 shade, 3 skin, 4 helmet, 5 tyre, 6 metal,
// 7 lamp, 8 satchel, 9 frame, 10 strap, 11 boot, 12 visor.
gs::Bitmap bikeArt() {
    gs::Bitmap b(34, 50);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    b.ellipse(8, 40, 6, 6, 5);
    b.ellipse(25, 40, 6, 6, 5);
    b.ellipse(8, 40, 2.2f, 2.2f, 6);
    b.ellipse(25, 40, 2.2f, 2.2f, 6);
    R(7, 33, 20, 4, 9);
    R(10, 31, 14, 4, 6);
    R(13, 29, 8, 3, 7);
    R(14, 28, 6, 2, 1);
    R(11, 16, 12, 14, 1);
    R(11, 16, 4, 14, 2);
    R(20, 18, 6, 8, 8);
    R(12, 28, 10, 2, 10);
    R(6, 20, 5, 9, 1);
    R(23, 20, 5, 9, 1);
    R(5, 28, 5, 3, 3);
    R(24, 28, 5, 3, 3);
    R(12, 36, 4, 6, 11);
    R(18, 36, 4, 6, 11);
    b.ellipse(17, 11, 6, 6, 4);
    R(12, 9, 10, 4, 4);
    R(13, 12, 8, 3, 12);
    R(15, 13, 4, 1, 9);
    b.outline(15, false);
    return b;
}

// 1 body, 2 shade, 3 canvas, 4 canvas dark, 5 glass, 6 lamp, 7 tyre, 8 bumper, 10 tail, 11 metal.
gs::Bitmap lorryFront() {
    gs::Bitmap b(36, 46);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(8, 4, 20, 8, 3);
    R(8, 4, 6, 8, 4);
    R(6, 12, 24, 14, 1);
    R(6, 12, 7, 14, 2);
    R(9, 14, 18, 7, 5);
    R(17, 14, 2, 7, 11);
    R(8, 22, 5, 3, 6);
    R(23, 22, 5, 3, 6);
    R(4, 26, 28, 8, 2);
    R(6, 28, 24, 3, 8);
    b.ellipse(9, 38, 5, 5, 7);
    b.ellipse(27, 38, 5, 5, 7);
    b.ellipse(9, 38, 2, 2, 11);
    b.ellipse(27, 38, 2, 2, 11);
    R(3, 30, 3, 6, 6);
    R(30, 30, 3, 6, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap lorryRear() {
    gs::Bitmap b(36, 46);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(6, 3, 24, 16, 3);
    R(6, 3, 7, 16, 4);
    R(8, 18, 20, 12, 1);
    R(8, 18, 5, 12, 2);
    R(10, 20, 16, 6, 11);
    R(9, 28, 5, 3, 10);
    R(22, 28, 5, 3, 10);
    R(5, 31, 26, 4, 8);
    b.ellipse(9, 39, 5, 5, 7);
    b.ellipse(27, 39, 5, 5, 7);
    R(4, 33, 3, 5, 10);
    R(29, 33, 3, 5, 10);
    b.outline(15, false);
    return b;
}

gs::Bitmap carFront() {
    gs::Bitmap b(30, 36);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(8, 6, 14, 7, 2);
    R(7, 12, 16, 8, 5);
    R(14, 12, 2, 8, 11);
    R(6, 19, 18, 7, 1);
    R(6, 19, 5, 7, 2);
    R(7, 21, 4, 2, 6);
    R(19, 21, 4, 2, 6);
    R(5, 25, 20, 3, 8);
    b.ellipse(8, 30, 4, 4, 7);
    b.ellipse(22, 30, 4, 4, 7);
    b.outline(15, false);
    return b;
}

gs::Bitmap carRear() {
    gs::Bitmap b(30, 36);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(7, 7, 16, 8, 5);
    R(8, 15, 14, 8, 1);
    R(8, 15, 4, 8, 2);
    R(8, 24, 4, 2, 10);
    R(18, 24, 4, 2, 10);
    R(6, 26, 18, 3, 8);
    b.ellipse(8, 31, 4, 4, 7);
    b.ellipse(22, 31, 4, 4, 7);
    b.outline(15, false);
    return b;
}

// Crimson colour with a gold cross. Pole stays put; the cloth shifts.
gs::Bitmap flagArt(int flutter) {
    gs::Bitmap b(30, 62);
    int s = flutter ? 2 : 0;
    b.rect(4, 4, 3, 54, 6);
    b.rect(4, 4, 1, 54, 3);
    b.ellipse(5.5f, 4, 3.2f, 3.2f, 3);
    b.rect(8 + s, 8, 18, 30, 1);
    b.rect(8 + s, 8, 18, 3, 3);
    b.rect(8 + s, 35, 18, 3, 3);
    b.rect(8 + s, 8, 3, 30, 3);
    b.rect(23 + s, 8, 3, 30, 3);
    b.rect(15 + s, 10, 3, 26, 3);
    b.rect(10 + s, 20, 14, 3, 3);
    b.rect(14 + s, 18, 5, 7, 8);
    for (int i = 0; i < 7; ++i) b.rect(9 + s + i * 2, 38, 1, 5, (i & 1) ? 7 : 3);
    b.outline(9, false);
    return b;
}

gs::Bitmap poplarArt() {
    gs::Bitmap b(18, 52);
    b.rect(8, 30, 3, 20, 4);
    b.rect(8, 30, 1, 20, 5);
    b.ellipse(9, 24, 5, 10, 2);
    b.ellipse(9, 16, 4, 9, 1);
    b.ellipse(9, 10, 3, 6, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 40);
    b.rect(13, 22, 3, 16, 4);
    b.rect(13, 22, 1, 16, 5);
    b.ellipse(14, 16, 10, 9, 2);
    b.ellipse(11, 14, 6, 6, 1);
    b.ellipse(17, 18, 5, 5, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(16, 42);
    b.rect(6, 6, 4, 34, 7);
    b.rect(6, 6, 1, 34, 3);
    b.rect(5, 16, 6, 4, 4);
    b.rect(10, 8, 5, 10, 4);
    b.rect(10, 8, 5, 3, 5);
    b.rect(6, 4, 4, 3, 6);
    b.outline(9, false);
    return b;
}

gs::Bitmap tentArt() {
    gs::Bitmap b(40, 28);
    b.poly({{2, 26}, {20, 4}, {38, 26}}, 1);
    b.poly({{8, 26}, {20, 10}, {22, 26}}, 2);
    b.rect(18, 6, 3, 20, 3);
    b.rect(6, 24, 28, 3, 10);
    b.rect(17, 16, 6, 8, 9);
    b.outline(9, false);
    return b;
}

gs::Bitmap tapeArt() {
    gs::Bitmap b(32, 8);
    for (int x = 0; x < 32; x += 4) {
        b.rect(float(x), 1, 4, 6, (x / 4) & 1 ? 8 : 7);
    }
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 10);
    b.ellipse(8, 5, 7, 3, 2);
    b.ellipse(6, 5, 3, 2, 1);
    return b;
}

gs::Bitmap burstArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 8, 8, 4);
    b.ellipse(11, 11, 5, 5, 2);
    b.ellipse(11, 11, 2, 2, 1);
    b.rect(10, 1, 2, 4, 3);
    b.rect(10, 17, 2, 4, 3);
    b.rect(1, 10, 4, 2, 3);
    b.rect(17, 10, 4, 2, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(24, 8);
    b.ellipse(12, 4, 11, 3, 1);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(36, 14);
    b.ellipse(14, 8, 10, 4, 2);
    b.ellipse(22, 7, 9, 5, 1);
    b.ellipse(18, 9, 6, 3, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 2);
    b.ellipse(7, 7, 3, 3, 1);
    return b;
}

void paintHills(gs::VDP& vdp, const Art& art) {
    vdp.A.clear();
    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.B.clear();
    for (int cy = 0; cy < 12; ++cy) {
        for (int cx = 0; cx < 40; ++cx) {
            float ridge = 4.7f + std::sin(cx * 0.52f) * 0.75f + std::sin(cx * 0.19f + 0.6f) * 0.4f;
            if (float(cy) + 0.15f < ridge || cy > 9) continue;
            int tile = (float(cy) < ridge + 0.8f) ? art.hillHi : art.hill;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_HILL));
        }
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_TEXT, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(8, 15, 7));

    const uint16_t ink = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_BIKE, {0, gs::rgb4(6, 8, 3), gs::rgb4(3, 5, 2), gs::rgb4(13, 9, 6), gs::rgb4(8, 8, 5),
                           gs::rgb4(2, 2, 2), gs::rgb4(10, 10, 9), gs::rgb4(14, 2, 2), gs::rgb4(8, 6, 3),
                           gs::rgb4(3, 3, 2), gs::rgb4(11, 10, 5), gs::rgb4(4, 3, 2), gs::rgb4(4, 5, 6),
                           gs::rgb4(2, 3, 2), gs::rgb4(7, 7, 6), ink});
    setPal(vdp, PAL_LORRY, {0, gs::rgb4(9, 8, 5), gs::rgb4(5, 5, 3), gs::rgb4(7, 8, 4), gs::rgb4(4, 5, 2),
                            gs::rgb4(5, 7, 8), gs::rgb4(15, 14, 8), gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 7),
                            gs::rgb4(1, 1, 1), gs::rgb4(13, 2, 2), gs::rgb4(12, 12, 11), gs::rgb4(6, 6, 5),
                            0, 0, ink});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(12, 2, 2), gs::rgb4(7, 1, 2), gs::rgb4(14, 11, 3), gs::rgb4(4, 2, 1),
                           gs::rgb4(2, 2, 6), gs::rgb4(7, 5, 2), gs::rgb4(13, 9, 3), gs::rgb4(15, 14, 11),
                           gs::rgb4(1, 1, 2), gs::rgb4(3, 1, 1), 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(7, 11, 3), gs::rgb4(3, 7, 2), gs::rgb4(2, 4, 2), gs::rgb4(6, 4, 2),
                           gs::rgb4(4, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CAMP, {0, gs::rgb4(12, 11, 8), gs::rgb4(8, 7, 5), gs::rgb4(6, 4, 2), gs::rgb4(12, 2, 2),
                           gs::rgb4(7, 1, 2), gs::rgb4(13, 11, 4), gs::rgb4(14, 14, 12), gs::rgb4(3, 3, 4),
                           gs::rgb4(1, 1, 1), gs::rgb4(5, 4, 2), 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 13), gs::rgb4(15, 13, 6), gs::rgb4(15, 8, 2), gs::rgb4(12, 4, 2),
                         gs::rgb4(8, 7, 5), gs::rgb4(4, 4, 3), gs::rgb4(15, 5, 2), gs::rgb4(10, 8, 6), 0, 0, 0, 0, 0,
                         0, ink});
    setPal(vdp, PAL_HILL, {0, gs::rgb4(8, 5, 4), gs::rgb4(5, 3, 3), gs::rgb4(3, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(4, 7, 2), gs::rgb4(2, 5, 2), gs::rgb4(6, 8, 3), gs::rgb4(7, 6, 3),
                           gs::rgb4(5, 4, 2), gs::rgb4(4, 4, 4), gs::rgb4(6, 6, 6), gs::rgb4(8, 8, 7),
                           gs::rgb4(5, 5, 5), gs::rgb4(7, 7, 6), gs::rgb4(3, 5, 7), gs::rgb4(4, 6, 8),
                           gs::rgb4(5, 7, 9), gs::rgb4(13, 12, 6), gs::rgb4(9, 9, 8)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    const char* hi[8] = {"11111111", "11121111", "11211121", "12111211", "11121111", "21111112", "11112111",
                          "12111111"};
    const char* lo[8] = {"22222222", "22232222", "22322232", "23222322", "22232222", "32222223", "22223222",
                          "23222222"};
    uint8_t px[64];
    tileFrom(px, hi);
    art.hillHi = tiles.shared(px);
    tileFrom(px, lo);
    art.hill = tiles.shared(px);
    paintHills(vdp, art);

    art.bike = gs::uploadMipped(vdp, bikeArt());
    art.lorryF = gs::uploadMipped(vdp, lorryFront());
    art.lorryR = gs::uploadMipped(vdp, lorryRear());
    art.carF = gs::uploadMipped(vdp, carFront());
    art.carR = gs::uploadMipped(vdp, carRear());
    art.flag[0] = gs::uploadMipped(vdp, flagArt(0));
    art.flag[1] = gs::uploadMipped(vdp, flagArt(1));
    art.poplar = gs::uploadMipped(vdp, poplarArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.tent = gs::uploadMipped(vdp, tentArt());
    art.tape = gs::uploadMipped(vdp, tapeArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.burst = gs::uploadMipped(vdp, burstArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
}

}  // namespace standard
