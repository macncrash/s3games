#include "game/art.h"

#include <string>

namespace mill {
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

void paintLane(gs::VDP& vdp, gs::TileAlloc& tiles) {
    int wheat = makeTile(tiles, vdp, {"33443344", "44334433", "33443343", "43344334", "34433443", "43344334",
                                      "33444333", "44333443"});
    int crust = makeTile(tiles, vdp, {"66666666", "66566656", "33443344", "44334433", "33443343", "43344334",
                                      "34433443", "43344334"});
    int chaff = makeTile(tiles, vdp, {"33443344", "44335433", "33443343", "43344334", "34433443", "53344334",
                                      "33444333", "44333443"});
    int brick = makeTile(tiles, vdp, {"22222222", "22222222", "22322232", "22222222", "22232222", "22222222",
                                      "23222223", "22222222"});
    int eave = makeTile(tiles, vdp, {"22002200", "22002200", "22222222", "22232222", "22222222", "22322232",
                                     "22222222", "22222222"});
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            vdp.A.set(cx, cy, 0);
            vdp.B.set(cx, cy, 0);
        }
    }
    for (int cx = 0; cx < 64; cx++) {
        vdp.A.set(cx, 10, gs::entry(eave, PAL_STONE));
        vdp.A.set(cx, 11, gs::entry(brick, PAL_STONE));
        vdp.A.set(cx, 12, gs::entry(brick, PAL_STONE));
        vdp.A.set(cx, 13, gs::entry(brick, PAL_STONE));
        vdp.B.set(cx, 23, gs::entry(crust, PAL_EARTH));
        for (int cy = 24; cy < 32; cy++) {
            int tile = ((cx * 3 + cy) % 7 == 0) ? chaff : wheat;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_EARTH));
        }
    }
}

// 1 ink 2 coat 3 hi 4 skin 5 hair 6 boot 7 staff 8 flour 9 eye
gs::Bitmap runner(int pose) {
    gs::Bitmap b(32, 46);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose == 3 ? -1 : 0;
    R(12, 5 + bob, 8, 7, 4);
    R(11, 3 + bob, 10, 4, 5);
    R(17, 7 + bob, 2, 2, 9);
    R(18, 8 + bob, 1, 1, 1);
    R(10, 12 + bob, 12, 13, 2);
    R(10, 12 + bob, 4, 13, 3);
    R(12, 16 + bob, 8, 4, 8);
    if (pose == 1) {
        R(10, 25, 4, 12, 2);
        R(17, 27, 4, 9, 2);
        R(9, 36, 6, 4, 6);
        R(17, 35, 6, 4, 6);
    } else if (pose == 2) {
        R(11, 27, 4, 9, 2);
        R(17, 25, 4, 12, 2);
        R(10, 35, 6, 4, 6);
        R(16, 36, 6, 4, 6);
    } else {
        R(11, 25 + bob, 4, 12, 2);
        R(17, 25 + bob, 4, 12, 2);
        R(10, 36 + bob, 6, 4, 6);
        R(16, 36 + bob, 6, 4, 6);
    }
    if (pose == 3) {
        R(20, 14, 9, 3, 2);
        R(27, 13, 3, 2, 7);
        R(8, 15, 3, 8, 4);
    } else {
        R(20, 13 + bob, 3, 10, 4);
        R(22, 7 + bob, 2, 12, 7);
        R(21, 6 + bob, 4, 2, 7);
    }
    b.outline(1, false);
    return b;
}

// 1 ink 2 shirt 3 hi 4 apron 5 dark 6 skin 7 rake 8 iron 9 cap
gs::Bitmap millHand(int pose) {
    gs::Bitmap b(44, 50);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int y0 = pose == 2 ? 4 : 0;
    R(14, 8 + y0, 8, 6, 6);
    R(13, 4 + y0, 10, 5, 9);
    R(18, 10 + y0, 2, 2, 1);
    R(13, 14 + y0, 12, 14, 2);
    R(13, 14 + y0, 4, 14, 3);
    R(15, 18 + y0, 8, 10, 4);
    R(15, 18 + y0, 3, 10, 5);
    if (pose == 2) {
        R(14, 30, 4, 8, 2);
        R(20, 32, 4, 6, 2);
        R(13, 37, 6, 3, 1);
        R(19, 37, 6, 3, 1);
    } else if (pose == 1) {
        R(14, 28, 4, 12, 2);
        R(19, 28, 4, 12, 2);
        R(13, 39, 6, 3, 1);
        R(18, 39, 6, 3, 1);
        R(24, 20, 16, 2, 7);
        R(38, 18, 4, 4, 8);
    } else {
        R(14, 28, 4, 12, 2);
        R(20, 28, 4, 12, 2);
        R(13, 39, 6, 3, 1);
        R(19, 39, 6, 3, 1);
        R(24, 8, 2, 26, 7);
        R(22, 6, 6, 3, 8);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap bannerCloth(int frame) {
    gs::Bitmap b(30, 52);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int sway = frame ? 2 : 0;
    R(4, 2, 3, 46, 6);
    R(5, 2, 1, 46, 4);
    R(4, 2, 8, 3, 4);
    R(7, 6, 18, 30, 2);
    R(7, 6, 5, 30, 3);
    R(14 + sway, 14, 3, 14, 4);
    R(9 + sway, 19, 12, 3, 4);
    R(8, 34, 16, 3, 5);
    R(11, 38, 3, 8, 3);
    R(17, 38, 3, 6, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(10, 58);
    b.rect(3, 2, 3, 52, 6);
    b.rect(4, 2, 1, 52, 4);
    b.rect(1, 2, 8, 3, 4);
    b.rect(2, 50, 6, 4, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap millArt() {
    gs::Bitmap b(64, 96);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(8, 18, 48, 70, 2);
    R(10, 20, 10, 64, 3);
    R(4, 14, 56, 8, 4);
    R(18, 4, 28, 14, 5);
    R(22, 2, 8, 8, 4);
    R(36, 2, 6, 8, 4);
    R(16, 36, 10, 14, 6);
    R(18, 40, 6, 6, 1);
    R(36, 48, 12, 22, 7);
    R(38, 62, 4, 4, 4);
    R(6, 82, 52, 6, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap wheelArt(int frame) {
    gs::Bitmap b(40, 40);
    b.ellipse(20, 20, 16, 16, 2);
    b.ellipse(20, 20, 6, 6, 3);
    if (frame == 0) {
        b.rect(18, 4, 4, 32, 4);
        b.rect(4, 18, 32, 4, 4);
    } else {
        b.line(8, 8, 32, 32, 4, 3);
        b.line(32, 8, 8, 32, 4, 3);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(22, 26);
    b.poly({{4, 8}, {18, 8}, {16, 22}, {6, 22}}, 2);
    b.rect(6, 4, 10, 6, 3);
    b.rect(8, 12, 6, 2, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(14, 32);
    b.line(4, 28, 6, 4, 3, 2);
    b.line(8, 28, 7, 6, 2, 2);
    b.line(10, 28, 12, 8, 4, 2);
    b.rect(2, 26, 10, 3, 5);
    return b;
}

gs::Bitmap raceArt() {
    gs::Bitmap b(48, 16);
    b.rect(0, 4, 48, 8, 2);
    b.rect(4, 6, 12, 3, 3);
    b.rect(24, 7, 10, 2, 4);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 10), gs::rgb4(12, 9, 3), gs::rgb4(5, 6, 7), gs::rgb4(14, 4, 2),
                          gs::rgb4(5, 12, 6), gs::rgb4(8, 7, 6), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(2, 2, 2), gs::rgb4(8, 7, 6), gs::rgb4(12, 11, 9), gs::rgb4(5, 3, 2),
                            gs::rgb4(9, 5, 3), gs::rgb4(3, 4, 6), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_EARTH, {0, gs::rgb4(3, 2, 1), gs::rgb4(7, 5, 2), gs::rgb4(10, 8, 3), gs::rgb4(12, 10, 5),
                            gs::rgb4(4, 6, 2), gs::rgb4(5, 4, 2)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(2, 1, 1), gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(14, 12, 4),
                             gs::rgb4(15, 14, 8), gs::rgb4(5, 4, 3), gs::rgb4(9, 7, 4)});
    setPal(vdp, PAL_PLAYER, {0, gs::rgb4(1, 1, 1), gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 3), gs::rgb4(13, 9, 6),
                             gs::rgb4(3, 2, 1), gs::rgb4(3, 2, 2), gs::rgb4(8, 6, 3), gs::rgb4(14, 13, 11),
                             gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(1, 1, 1), gs::rgb4(4, 6, 8), gs::rgb4(7, 9, 11), gs::rgb4(13, 12, 9),
                           gs::rgb4(8, 7, 5), gs::rgb4(12, 8, 6), gs::rgb4(6, 4, 2), gs::rgb4(10, 11, 12),
                           gs::rgb4(5, 3, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(2, 1, 1), gs::rgb4(7, 4, 2), gs::rgb4(11, 7, 3), gs::rgb4(4, 3, 2),
                           gs::rgb4(5, 8, 10), gs::rgb4(3, 6, 8), gs::rgb4(9, 12, 13)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 12, 5), gs::rgb4(12, 6, 2), gs::rgb4(15, 13, 7), gs::rgb4(15, 14, 9),
                         gs::rgb4(6, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);
    paintLane(vdp, tiles);

    art.stand = gs::uploadMipped(vdp, runner(0));
    art.runA = gs::uploadMipped(vdp, runner(1));
    art.runB = gs::uploadMipped(vdp, runner(2));
    art.swing = gs::uploadMipped(vdp, runner(3));
    for (int i = 0; i < 3; i++) art.hand[i] = gs::uploadMipped(vdp, millHand(i));
    art.banner[0] = gs::uploadMipped(vdp, bannerCloth(0));
    art.banner[1] = gs::uploadMipped(vdp, bannerCloth(1));
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.mill = gs::uploadMipped(vdp, millArt());
    art.wheel[0] = gs::uploadMipped(vdp, wheelArt(0));
    art.wheel[1] = gs::uploadMipped(vdp, wheelArt(1));
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.race = gs::uploadMipped(vdp, raceArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    vdp.setFogColor(gs::rgb4(6, 4, 2));
}

}  // namespace mill
