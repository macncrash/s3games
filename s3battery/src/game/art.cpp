#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace battery {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
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
    gs::TextStyle big{3, 1, 0, 15, 1};
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

gs::Bitmap carArt() {
    gs::Bitmap b(40, 36);
    b.rect(18, 2, 2, 6, 10);
    b.rect(20, 2, 6, 4, 10);
    b.ellipse(20, 10, 11, 6, 1);
    b.rect(11, 12, 18, 7, 4);
    b.rect(12, 13, 7, 4, 5);
    b.rect(21, 13, 7, 4, 5);
    b.rect(8, 18, 24, 9, 2);
    b.rect(9, 18, 22, 3, 1);
    b.ellipse(12, 22, 3, 2, 6);
    b.ellipse(28, 22, 3, 2, 6);
    b.rect(15, 21, 10, 5, 3);
    b.rect(7, 26, 26, 3, 7);
    b.rect(6, 20, 4, 10, 8);
    b.rect(30, 20, 4, 10, 8);
    b.rect(6, 28, 4, 3, 9);
    b.rect(30, 28, 4, 3, 9);
    b.outline(15, false);
    return b;
}

gs::Bitmap truckArt() {
    gs::Bitmap b(44, 56);
    b.ellipse(22, 10, 14, 9, 1);
    b.rect(9, 8, 26, 16, 2);
    b.rect(10, 8, 24, 4, 1);
    b.rect(11, 20, 22, 4, 3);
    b.rect(10, 24, 24, 10, 4);
    b.rect(12, 26, 20, 6, 6);
    b.rect(13, 27, 8, 4, 7);
    b.rect(9, 33, 26, 8, 5);
    b.ellipse(13, 37, 3, 2, 8);
    b.ellipse(31, 37, 3, 2, 8);
    b.rect(16, 35, 12, 4, 3);
    b.rect(8, 40, 28, 4, 9);
    b.rect(6, 32, 5, 14, 11);
    b.rect(33, 32, 5, 14, 11);
    b.rect(6, 44, 5, 4, 3);
    b.rect(33, 44, 5, 4, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap tankerArt() {
    gs::Bitmap b(44, 58);
    b.ellipse(22, 14, 15, 12, 2);
    b.ellipse(22, 12, 10, 8, 1);
    b.rect(8, 16, 28, 6, 10);
    b.rect(10, 22, 24, 8, 4);
    b.rect(12, 24, 20, 5, 6);
    b.rect(9, 30, 26, 10, 5);
    b.ellipse(13, 35, 3, 2, 8);
    b.ellipse(31, 35, 3, 2, 8);
    b.rect(7, 39, 30, 4, 9);
    b.rect(6, 30, 5, 16, 11);
    b.rect(33, 30, 5, 16, 11);
    b.rect(6, 44, 5, 4, 3);
    b.rect(33, 44, 5, 4, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap armorArt() {
    gs::Bitmap b(46, 42);
    b.poly({{8, 14}, {38, 14}, {42, 28}, {4, 28}}, 2);
    b.poly({{12, 16}, {34, 16}, {36, 24}, {10, 24}}, 1);
    b.rect(16, 8, 14, 8, 3);
    b.rect(20, 4, 6, 6, 8);
    b.rect(18, 18, 10, 3, 4);
    b.ellipse(23, 22, 2, 2, 7);
    b.rect(4, 16, 6, 18, 5);
    b.rect(36, 16, 6, 18, 5);
    b.rect(5, 18, 4, 4, 6);
    b.rect(37, 18, 4, 4, 6);
    b.rect(6, 30, 34, 4, 9);
    b.rect(3, 32, 6, 6, 5);
    b.rect(37, 32, 6, 6, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap wreckArt() {
    gs::Bitmap b(44, 32);
    b.rect(8, 14, 28, 10, 9);
    b.rect(10, 12, 16, 6, 11);
    b.ellipse(18, 12, 7, 6, 4);
    b.ellipse(16, 10, 4, 3, 10);
    b.ellipse(22, 9, 2, 2, 3);
    b.rect(6, 20, 5, 8, 9);
    b.rect(33, 20, 5, 8, 9);
    b.rect(7, 26, 30, 3, 15);
    b.outline(15, false);
    return b;
}

gs::Bitmap gunArt() {
    gs::Bitmap b(52, 64);
    b.rect(23, 3, 6, 26, 6);
    b.rect(21, 2, 10, 5, 4);
    b.rect(22, 14, 8, 3, 5);
    b.rect(12, 26, 28, 16, 7);
    b.rect(14, 28, 24, 5, 8);
    b.rect(21, 30, 10, 3, 4);
    b.ellipse(12, 40, 8, 8, 6);
    b.ellipse(40, 40, 8, 8, 6);
    b.ellipse(12, 40, 3, 3, 4);
    b.ellipse(40, 40, 3, 3, 4);
    b.line(18, 44, 8, 60, 11, 3.2f);
    b.line(34, 44, 44, 60, 11, 3.2f);
    b.rect(5, 57, 8, 4, 11);
    b.rect(39, 57, 8, 4, 11);
    b.outline(15, false);
    return b;
}

gs::Bitmap crewArt() {
    gs::Bitmap b(24, 28);
    b.ellipse(12, 8, 6, 6, 10);
    b.rect(7, 5, 10, 3, 10);
    b.ellipse(12, 11, 4, 3, 9);
    b.rect(6, 14, 12, 8, 7);
    b.rect(3, 16, 4, 7, 8);
    b.rect(17, 16, 4, 7, 8);
    b.outline(15, false);
    return b;
}

gs::Bitmap bagArt() {
    gs::Bitmap b(80, 28);
    for (int i = 0; i < 5; ++i) {
        float x = 10.f + float(i) * 15.f;
        b.ellipse(x, 17, 9, 7, (i & 1) ? 2 : 1);
        b.ellipse(x - 2.f, 15, 4, 3, 1);
    }
    for (int i = 0; i < 4; ++i) {
        float x = 18.f + float(i) * 15.f;
        b.ellipse(x, 9, 8, 5, (i & 1) ? 1 : 2);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(32, 48);
    b.rect(14, 30, 5, 14, 4);
    b.rect(13, 42, 7, 3, 5);
    b.ellipse(16, 22, 13, 12, 2);
    b.ellipse(12, 18, 7, 6, 1);
    b.ellipse(20, 24, 6, 5, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap pineArt() {
    gs::Bitmap b(30, 52);
    b.poly({{15, 2}, {27, 22}, {3, 22}}, 2);
    b.poly({{15, 12}, {26, 32}, {4, 32}}, 1);
    b.poly({{15, 22}, {24, 40}, {6, 40}}, 3);
    b.rect(13, 36, 5, 12, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 44);
    for (int y = 2; y < 36; ++y) b.rect(4, float(y), 4, 1, ((y / 5) & 1) ? 2 : 1);
    b.rect(3, 36, 6, 5, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap tapeArt() {
    gs::Bitmap b(32, 8);
    for (int x = 0; x < 32; ++x) b.rect(float(x), 2, 1, 4, ((x / 4) & 1) ? 2 : 1);
    return b;
}

gs::Bitmap boomArt() {
    gs::Bitmap b(40, 40);
    b.ellipse(20, 22, 16, 13, 4);
    b.ellipse(20, 20, 11, 9, 3);
    b.ellipse(18, 18, 6, 5, 2);
    b.ellipse(16, 16, 3, 2.4f, 1);
    return b;
}

gs::Bitmap smokeArt() {
    gs::Bitmap b(28, 24);
    b.ellipse(14, 14, 10, 7, 5);
    b.ellipse(10, 11, 6, 5, 6);
    b.ellipse(18, 10, 5, 4, 1);
    return b;
}

gs::Bitmap shellArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 2);
    b.ellipse(4, 4, 2, 2, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 1);
    return b;
}

gs::Bitmap sightArt() {
    gs::Bitmap b(28, 28);
    b.rect(2, 8, 7, 2, 7);
    b.rect(2, 8, 2, 7, 7);
    b.rect(19, 8, 7, 2, 7);
    b.rect(24, 8, 2, 7, 7);
    b.rect(2, 18, 7, 2, 7);
    b.rect(2, 13, 2, 7, 7);
    b.rect(19, 18, 7, 2, 7);
    b.rect(24, 13, 2, 7, 7);
    b.rect(12, 13, 4, 2, 1);
    b.rect(13, 11, 2, 6, 1);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(40, 16);
    b.ellipse(14, 9, 10, 5, 6);
    b.ellipse(24, 8, 12, 6, 1);
    b.ellipse(20, 10, 8, 4, 6);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 2);
    b.ellipse(8, 8, 4, 4, 1);
    return b;
}

void paintHills(gs::VDP& vdp, const Art& art) {
    vdp.A.clear();
    vdp.A.enabled = false;
    vdp.B.clear();
    for (int cy = 0; cy < 12; ++cy) {
        for (int cx = 0; cx < 40; ++cx) {
            float ridge = 5.15f + std::sin(cx * 0.46f) * 1.15f + std::sin(cx * 0.17f + 0.6f) * 0.7f;
            if (float(cy) + 0.15f < ridge || cy > 8) continue;
            int tile = (float(cy) < ridge + 0.85f) ? art.hillHi : art.hill;
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
    setPal(vdp, PAL_CAR, {0, gs::rgb4(9, 11, 6), gs::rgb4(5, 7, 3), gs::rgb4(3, 4, 2), gs::rgb4(6, 8, 10),
                          gs::rgb4(11, 13, 14), gs::rgb4(15, 14, 6), gs::rgb4(12, 12, 11), gs::rgb4(2, 2, 2),
                          gs::rgb4(7, 7, 6), gs::rgb4(13, 3, 2), 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TRUCK, {0, gs::rgb4(11, 10, 7), gs::rgb4(7, 7, 4), gs::rgb4(4, 4, 2), gs::rgb4(5, 6, 3),
                            gs::rgb4(3, 4, 2), gs::rgb4(5, 7, 9), gs::rgb4(12, 14, 15), gs::rgb4(15, 14, 5),
                            gs::rgb4(8, 8, 7), gs::rgb4(13, 2, 2), gs::rgb4(2, 2, 2), 0, 0, 0, ink});
    setPal(vdp, PAL_ARMOR, {0, gs::rgb4(8, 9, 7), gs::rgb4(5, 6, 4), gs::rgb4(3, 4, 3), gs::rgb4(2, 3, 2),
                            gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 4), gs::rgb4(15, 13, 5), gs::rgb4(7, 7, 6),
                            gs::rgb4(8, 8, 7), gs::rgb4(6, 5, 3), 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GUN, {0, gs::rgb4(12, 10, 6), gs::rgb4(9, 7, 4), gs::rgb4(6, 4, 3), gs::rgb4(9, 9, 8),
                          gs::rgb4(4, 4, 4), gs::rgb4(2, 2, 2), gs::rgb4(5, 7, 3), gs::rgb4(3, 4, 2),
                          gs::rgb4(12, 8, 5), gs::rgb4(4, 5, 3), gs::rgb4(7, 5, 2), 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 15), gs::rgb4(15, 14, 4), gs::rgb4(15, 8, 1), gs::rgb4(12, 4, 1),
                         gs::rgb4(7, 7, 6), gs::rgb4(12, 12, 11), gs::rgb4(15, 3, 2), gs::rgb4(10, 8, 5),
                         gs::rgb4(3, 3, 3), gs::rgb4(15, 5, 1), gs::rgb4(8, 8, 7), 0, 0, 0, ink});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(8, 12, 4), gs::rgb4(4, 8, 3), gs::rgb4(2, 5, 2), gs::rgb4(6, 4, 2),
                           gs::rgb4(4, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(14, 14, 13), gs::rgb4(13, 2, 2), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_HILL, {0, gs::rgb4(6, 7, 8), gs::rgb4(4, 5, 6), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           ink});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(6, 9, 3), gs::rgb4(3, 6, 2), gs::rgb4(8, 10, 4), gs::rgb4(9, 7, 4),
                           gs::rgb4(6, 4, 2), gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 4), gs::rgb4(10, 9, 6),
                           gs::rgb4(4, 4, 3), gs::rgb4(6, 6, 5), gs::rgb4(2, 5, 8), gs::rgb4(3, 6, 9),
                           gs::rgb4(5, 8, 11), gs::rgb4(14, 13, 6), gs::rgb4(9, 9, 8)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);

    const char* hi[8] = {"33333333", "33313333", "31111113", "11111111", "11121111", "11211121", "12111112",
                          "21111112"};
    const char* lo[8] = {"22222222", "22122222", "12222122", "22212222", "22122212", "12222221", "22221222",
                          "21222222"};
    uint8_t px[64];
    tileFrom(px, hi);
    art.hillHi = tiles.shared(px);
    tileFrom(px, lo);
    art.hill = tiles.shared(px);
    paintHills(vdp, art);

    art.car = gs::uploadMipped(vdp, carArt());
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.tanker = gs::uploadMipped(vdp, tankerArt());
    art.armor = gs::uploadMipped(vdp, armorArt());
    art.wreck = gs::uploadMipped(vdp, wreckArt());
    art.gun = gs::uploadMipped(vdp, gunArt());
    art.crew = gs::uploadMipped(vdp, crewArt());
    art.bags = gs::uploadMipped(vdp, bagArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.tape = gs::uploadMipped(vdp, tapeArt());
    art.boom = gs::uploadMipped(vdp, boomArt());
    art.smoke = gs::uploadMipped(vdp, smokeArt());
    art.shell = gs::uploadMipped(vdp, shellArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
}

}  // namespace battery
