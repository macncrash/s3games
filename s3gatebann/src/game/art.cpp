#include "game/art.h"

#include <string>

namespace bann {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

int makeTile(gs::TileAlloc& al, gs::VDP& vdp, std::initializer_list<const char*> rows) {
    uint8_t px[64] = {};
    int y = 0;
    for (const char* row : rows) {
        if (y >= 8) break;
        for (int x = 0; x < 8 && row[x]; x++) {
            char c = row[x];
            int v = 0;
            if (c >= '0' && c <= '9') v = c - '0';
            else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
            px[y * 8 + x] = uint8_t(v);
        }
        y++;
    }
    int t = al.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    gs::TextStyle big{2, 1, 0, 15, 1};
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

void paintYard(gs::VDP& vdp, gs::TileAlloc& tiles) {
    int dirt = makeTile(tiles, vdp, {"22332233", "33223322", "22332232", "32233223", "23322332", "32233223",
                                     "22333222", "33222332"});
    int crust = makeTile(tiles, vdp, {"55555555", "55255525", "22332233", "33223322", "22332232", "32233223",
                                      "23322332", "32233223"});
    int pebble = makeTile(tiles, vdp, {"22332233", "33224322", "22332232", "32233223", "23322332", "42233223",
                                       "22333222", "33222332"});
    int wall = makeTile(tiles, vdp, {"44444444", "44444444", "44244424", "44444444", "44424444", "44444444",
                                     "42444442", "44444444"});
    int cren = makeTile(tiles, vdp, {"44004400", "44004400", "44444444", "44424444", "44444444", "44244424",
                                     "44444444", "44444444"});
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            vdp.A.set(cx, cy, 0);
            vdp.B.set(cx, cy, 0);
        }
    }
    for (int cx = 0; cx < 64; cx++) {
        vdp.A.set(cx, 11, gs::entry(cren, PAL_STONE));
        vdp.A.set(cx, 12, gs::entry(wall, PAL_STONE));
        vdp.A.set(cx, 13, gs::entry(wall, PAL_STONE));
        vdp.A.set(cx, 14, gs::entry(wall, PAL_STONE));
        vdp.B.set(cx, 23, gs::entry(crust, PAL_EARTH));
        for (int cy = 24; cy < 32; cy++) {
            int tile = ((cx + cy) % 5 == 0) ? pebble : dirt;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_EARTH));
        }
    }
}

// 1 ink, 2 coat, 3 coat hi, 4 skin, 5 hair, 6 boot, 7 steel, 8 sash, 9 eye
gs::Bitmap runner(int pose) {
    gs::Bitmap b(30, 46);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose == 3 ? -1 : 0;
    R(11, 6 + bob, 8, 7, 4);
    R(10, 4 + bob, 10, 4, 5);
    R(16, 8 + bob, 2, 2, 9);
    R(17, 9 + bob, 1, 1, 1);
    R(9, 13 + bob, 12, 12, 2);
    R(9, 13 + bob, 4, 12, 3);
    R(11, 16 + bob, 8, 3, 8);
    R(12, 22 + bob, 6, 2, 5);
    if (pose == 1) {
        R(9, 25, 4, 12, 2);
        R(16, 27, 4, 9, 2);
        R(8, 36, 6, 4, 6);
        R(16, 35, 6, 4, 6);
    } else if (pose == 2) {
        R(10, 27, 4, 9, 2);
        R(16, 25, 4, 12, 2);
        R(9, 35, 6, 4, 6);
        R(15, 36, 6, 4, 6);
    } else {
        R(10, 25 + bob, 4, 12, 2);
        R(16, 25 + bob, 4, 12, 2);
        R(9, 36 + bob, 6, 4, 6);
        R(15, 36 + bob, 6, 4, 6);
    }
    if (pose == 3) {
        R(18, 15, 8, 3, 2);
        R(24, 14, 4, 2, 7);
        R(26, 12, 2, 2, 7);
        R(8, 16, 3, 8, 4);
    } else {
        R(19, 14 + bob, 3, 9, 4);
        R(20, 22 + bob, 3, 3, 4);
        R(21, 8 + bob, 2, 10, 7);
        R(20, 7 + bob, 4, 2, 7);
    }
    b.outline(1, false);
    return b;
}

// 1 ink, 2 mail, 3 mail hi, 4 tabard, 5 tabard dark, 6 skin, 7 pike wood, 8 pike steel, 9 plume
gs::Bitmap sentry(int pose) {
    gs::Bitmap b(44, 50);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int y0 = pose == 2 ? 4 : 0;
    R(14, 8 + y0, 8, 6, 6);
    R(13, 5 + y0, 10, 4, 2);
    R(16, 2 + y0, 4, 4, 9);
    R(18, 10 + y0, 2, 2, 1);
    R(13, 14 + y0, 12, 12, 4);
    R(13, 14 + y0, 4, 12, 5);
    R(15, 16 + y0, 8, 8, 2);
    R(16, 17 + y0, 3, 6, 3);
    if (pose == 2) {
        R(14, 28, 4, 8, 2);
        R(20, 30, 4, 6, 2);
        R(13, 36, 6, 3, 1);
        R(19, 36, 6, 3, 1);
        R(22, 18, 3, 8, 6);
    } else if (pose == 1) {
        R(14, 26, 4, 12, 2);
        R(19, 26, 4, 12, 2);
        R(13, 37, 6, 4, 1);
        R(18, 37, 6, 4, 1);
        R(24, 18, 16, 2, 7);
        R(38, 17, 4, 4, 8);
        R(12, 17, 4, 3, 6);
    } else {
        R(14, 26, 4, 12, 2);
        R(20, 26, 4, 12, 2);
        R(13, 37, 6, 4, 1);
        R(19, 37, 6, 4, 1);
        R(24, 6, 2, 28, 7);
        R(22, 4, 6, 3, 8);
    }
    b.outline(1, false);
    return b;
}

// Pole on the left, cloth to the right. A black arch is cut into the red.
gs::Bitmap bannerCloth(int frame) {
    gs::Bitmap b(28, 52);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int sway = frame ? 2 : 0;
    R(4, 2, 3, 46, 6);
    R(5, 2, 1, 46, 4);
    R(4, 2, 8, 3, 4);
    R(7, 6, 16, 28, 2);
    R(7, 6, 4, 28, 3);
    R(9 + sway, 10, 10, 8, 1);
    R(11 + sway, 12, 6, 10, 2);
    R(12 + sway, 14, 4, 4, 1);
    R(8, 30, 14, 3, 4);
    R(10, 34, 3, 8, 3);
    R(16, 34, 3, 6, 3);
    R(13, 36, 2, 5, 2);
    b.outline(1, false);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(10, 56);
    b.rect(3, 2, 3, 50, 6);
    b.rect(4, 2, 1, 50, 4);
    b.rect(1, 2, 8, 3, 4);
    b.rect(2, 48, 6, 4, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(36, 90);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    for (int i = 0; i < 4; i++) R(3 + i * 8, 2, 6, 8, 2);
    R(3, 10, 30, 74, 2);
    R(5, 12, 6, 68, 3);
    R(8, 24, 8, 14, 4);
    R(10, 26, 4, 8, 1);
    R(20, 40, 8, 16, 4);
    R(22, 44, 4, 8, 1);
    R(6, 62, 24, 3, 5);
    R(4, 78, 28, 4, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap lintelArt() {
    gs::Bitmap b(72, 18);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(2, 5, 68, 8, 2);
    R(2, 5, 68, 3, 3);
    R(30, 2, 12, 14, 2);
    R(33, 4, 6, 8, 3);
    R(4, 13, 64, 2, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap barsArt() {
    gs::Bitmap b(56, 22);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(2, 2, 52, 4, 2);
    for (int i = 0; i < 7; i++) R(4 + i * 7, 4, 2, 16, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap doorArt() {
    gs::Bitmap b(30, 52);
    b.poly({{6, 3}, {24, 8}, {22, 46}, {3, 42}}, 2);
    b.poly({{8, 8}, {18, 11}, {17, 40}, {6, 37}}, 3);
    b.rect(8, 18, 12, 2, 4);
    b.rect(8, 30, 12, 2, 4);
    b.rect(16, 22, 3, 3, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap pennantArt() {
    gs::Bitmap b(18, 16);
    b.poly({{2, 2}, {4, 2}, {14, 7}, {4, 13}, {2, 13}}, 2);
    b.poly({{4, 4}, {4, 11}, {11, 7}}, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap slabArt() {
    gs::Bitmap b(48, 12);
    b.rect(2, 3, 44, 6, 3);
    b.rect(2, 3, 44, 2, 2);
    b.rect(8, 6, 6, 2, 4);
    b.rect(28, 6, 8, 2, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(16, 36);
    b.rect(6, 8, 3, 24, 2);
    b.rect(7, 8, 1, 24, 3);
    b.poly({{8, 4}, {14, 8}, {8, 14}}, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap tentArt() {
    gs::Bitmap b(40, 32);
    b.poly({{20, 3}, {36, 26}, {4, 26}}, 2);
    b.poly({{20, 8}, {30, 24}, {10, 24}}, 3);
    b.rect(17, 16, 6, 10, 4);
    b.rect(2, 24, 36, 4, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap brazierArt() {
    gs::Bitmap b(18, 24);
    b.poly({{4, 6}, {14, 6}, {16, 12}, {2, 12}}, 4);
    b.rect(8, 12, 2, 8, 2);
    b.rect(4, 19, 10, 3, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap flameArt(int frame) {
    gs::Bitmap b(12, 16);
    if (frame == 0) {
        b.poly({{6, 1}, {10, 8}, {8, 14}, {4, 14}, {2, 7}}, 3);
        b.poly({{6, 5}, {8, 10}, {6, 13}, {4, 9}}, 4);
    } else {
        b.poly({{7, 2}, {11, 9}, {7, 14}, {3, 13}, {2, 6}}, 3);
        b.poly({{7, 6}, {8, 10}, {6, 12}, {5, 8}}, 2);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 7, 7, 4);
    b.ellipse(11, 11, 4, 4, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 8, 3, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 11), gs::rgb4(14, 11, 4), gs::rgb4(6, 6, 8), gs::rgb4(14, 4, 3),
                          gs::rgb4(6, 13, 7), gs::rgb4(8, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 2)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(2, 2, 3), gs::rgb4(7, 7, 8), gs::rgb4(11, 11, 12), gs::rgb4(3, 3, 4),
                            gs::rgb4(3, 5, 3), gs::rgb4(5, 5, 6)});
    setPal(vdp, PAL_EARTH, {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(8, 6, 3), gs::rgb4(4, 4, 4),
                            gs::rgb4(3, 5, 2)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(2, 1, 1), gs::rgb4(12, 2, 2), gs::rgb4(7, 1, 1), gs::rgb4(13, 10, 3),
                             gs::rgb4(15, 13, 7), gs::rgb4(5, 4, 3), gs::rgb4(9, 8, 6)});
    setPal(vdp, PAL_PLAYER, {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 3, 8), gs::rgb4(4, 6, 12), gs::rgb4(13, 9, 7),
                             gs::rgb4(2, 1, 1), gs::rgb4(3, 2, 2), gs::rgb4(12, 13, 14), gs::rgb4(12, 9, 2),
                             gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_SENTRY, {0, gs::rgb4(1, 1, 1), gs::rgb4(6, 6, 7), gs::rgb4(10, 10, 12), gs::rgb4(9, 1, 2),
                             gs::rgb4(5, 0, 1), gs::rgb4(12, 8, 6), gs::rgb4(6, 4, 2), gs::rgb4(11, 12, 13),
                             gs::rgb4(12, 2, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(2, 1, 1), gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(3, 2, 1),
                           gs::rgb4(8, 8, 9)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 13, 6), gs::rgb4(12, 6, 2), gs::rgb4(15, 12, 6), gs::rgb4(15, 14, 8),
                         gs::rgb4(8, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);
    paintYard(vdp, tiles);

    art.stand = gs::uploadMipped(vdp, runner(0));
    art.runA = gs::uploadMipped(vdp, runner(1));
    art.runB = gs::uploadMipped(vdp, runner(2));
    art.swing = gs::uploadMipped(vdp, runner(3));
    for (int i = 0; i < 3; i++) art.sentry[i] = gs::uploadMipped(vdp, sentry(i));
    art.banner[0] = gs::uploadMipped(vdp, bannerCloth(0));
    art.banner[1] = gs::uploadMipped(vdp, bannerCloth(1));
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.lintel = gs::uploadMipped(vdp, lintelArt());
    art.bars = gs::uploadMipped(vdp, barsArt());
    art.door = gs::uploadMipped(vdp, doorArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.slab = gs::uploadMipped(vdp, slabArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.tent = gs::uploadMipped(vdp, tentArt());
    art.brazier = gs::uploadMipped(vdp, brazierArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
}

}  // namespace bann
