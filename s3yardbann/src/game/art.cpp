#include "game/art.h"

#include <initializer_list>
#include <string>

namespace yard {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
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
    int gravel = makeTile(tiles, vdp, {"23232323", "32323232", "23242323", "32323232", "23232323", "32353232",
                                       "23232323", "32323232"});
    int gravel2 = makeTile(tiles, vdp, {"32323232", "23282323", "32323232", "23232342", "32323232", "23232323",
                                        "32823232", "23232323"});
    int oil = makeTile(tiles, vdp, {"25252525", "52555255", "55555555", "25555552", "55555555", "52555525",
                                    "55525555", "25555552"});
    int rust = makeTile(tiles, vdp, {"24242424", "42444242", "24442442", "42424244", "24244242", "42424424",
                                     "24424242", "42424242"});
    int curb = makeTile(tiles, vdp, {"66666666", "77777777", "23232323", "32323232", "23232323", "32323232",
                                     "23232323", "32323232"});
    int skyCar = makeTile(tiles, vdp, {"00000000", "00022000", "00222200", "02222220", "02555220", "02222220",
                                       "02000020", "00000000"});
    int skyPost = makeTile(tiles, vdp, {"00040000", "00040000", "00040000", "00040000", "00040000", "00040000",
                                        "00444000", "04444440"});
    int skyJib = makeTile(tiles, vdp, {"44444444", "04040404", "00000000", "00000000", "00000000", "00000000",
                                       "00000000", "00000000"});

    vdp.A.resize(128, 32);
    vdp.B.resize(128, 32);
    for (int cx = 0; cx < 128; cx++) {
        vdp.B.set(cx, 22, gs::entry(curb, PAL_YARD));
        for (int cy = 23; cy < 32; cy++) {
            int n = (cx * 13 + cy * 5) % 17;
            int tile = gravel;
            if (n == 0) tile = oil;
            else if (n == 1 || n == 8) tile = rust;
            else if (n == 3) tile = gravel2;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_YARD));
        }
        int k = cx % 19;
        if (k == 0) {
            vdp.A.set(cx, 14, gs::entry(skyPost, PAL_SCRAP));
            vdp.A.set(cx, 15, gs::entry(skyPost, PAL_SCRAP));
            vdp.A.set(cx, 16, gs::entry(skyPost, PAL_SCRAP));
        } else if (k == 6 || k == 7) {
            vdp.A.set(cx, 15, gs::entry(skyJib, PAL_SCRAP));
            vdp.A.set(cx, 16, gs::entry(skyCar, PAL_SCRAP));
        } else if (k % 4 == 1) {
            vdp.A.set(cx, 16, gs::entry(skyCar, PAL_SCRAP));
        }
    }
}

void box(gs::Bitmap& b, int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); }

// 1 ink, 2 coverall, 3 hi, 4 skin, 5 cap, 6 boot, 7 pry, 8 glove, 9 vest
gs::Bitmap handArt(int pose) {
    gs::Bitmap b(36, 54);
    int y = pose == 1 ? 1 : 0;
    box(b, 13, 6 + y, 10, 5, 5);
    box(b, 14, 10 + y, 8, 8, 4);
    box(b, 19, 13 + y, 2, 2, 1);
    box(b, 11, 18 + y, 14, 14, 2);
    box(b, 11, 18 + y, 4, 14, 3);
    box(b, 12, 22 + y, 12, 3, 9);
    box(b, 13, 30 + y, 10, 2, 5);
    if (pose == 3) {
        box(b, 22, 21, 10, 3, 4);
        box(b, 24, 22, 4, 3, 8);
        box(b, 30, 20, 4, 3, 7);
        box(b, 12, 20, 3, 8, 4);
    } else {
        box(b, 23, 19 + y, 3, 10, 4);
        box(b, 24, 27 + y, 3, 3, 8);
        box(b, 25, 16 + y, 2, 16, 7);
        box(b, 24, 14 + y, 4, 3, 7);
    }
    if (pose == 1) {
        box(b, 12, 32, 5, 10, 2);
        box(b, 19, 34, 5, 8, 2);
        box(b, 11, 41, 7, 4, 6);
        box(b, 18, 41, 7, 4, 6);
    } else if (pose == 2) {
        box(b, 12, 34, 5, 8, 2);
        box(b, 19, 32, 5, 10, 2);
        box(b, 11, 41, 7, 4, 6);
        box(b, 18, 41, 7, 4, 6);
    } else {
        box(b, 12, 32 + y, 5, 12, 2);
        box(b, 19, 32 + y, 5, 12, 2);
        box(b, 11, 43 + y, 7, 4, 6);
        box(b, 18, 43 + y, 7, 4, 6);
    }
    b.outline(1, false);
    return b;
}

// Head leads to the right. 1 ink, 2 brown, 3 hi, 4 ear, 5 muzzle, 6 eye, 7 tongue, 8 collar
gs::Bitmap dogArt(int pose) {
    gs::Bitmap b(44, pose == 2 ? 24 : 30);
    if (pose == 2) {
        box(b, 6, 12, 22, 8, 2);
        box(b, 6, 12, 8, 3, 3);
        box(b, 26, 10, 10, 8, 2);
        box(b, 30, 8, 6, 4, 4);
        box(b, 33, 13, 4, 3, 5);
        box(b, 34, 12, 2, 2, 6);
        box(b, 32, 16, 3, 2, 7);
        box(b, 14, 14, 4, 3, 8);
        box(b, 8, 18, 3, 3, 2);
        box(b, 16, 18, 3, 3, 2);
        box(b, 22, 18, 3, 3, 4);
    } else {
        int lift = pose == 1 ? 2 : 0;
        box(b, 4, 8, 6, 4, 4);
        box(b, 8, 10, 18, 10, 2);
        box(b, 8, 10, 8, 4, 3);
        box(b, 22, 12, 4, 4, 8);
        box(b, 24, 8, 12, 10, 2);
        box(b, 30, 6, 6, 5, 4);
        box(b, 32, 12, 5, 4, 5);
        box(b, 35, 11, 2, 2, 6);
        box(b, 33, 16, 3, 2, 7);
        box(b, 10, 19, 4, 7, 2);
        box(b, 16, 19 + lift, 4, 7 - lift, 2);
        box(b, 22, 19, 3, 6, 4);
        box(b, 28, 20 - lift, 3, 5, 2);
        box(b, 9, 25, 5, 2, 4);
        box(b, 15, 25, 5, 2, 4);
    }
    b.outline(1, false);
    return b;
}

// Pole on the left, mustard cloth to the right, black chevron.
gs::Bitmap bannerArt(int frame) {
    gs::Bitmap b(30, 56);
    int sway = frame ? 2 : 0;
    box(b, 4, 2, 3, 50, 6);
    box(b, 5, 2, 1, 50, 7);
    box(b, 3, 2, 8, 3, 7);
    box(b, 7, 6, 18, 30, 2);
    box(b, 7, 6, 5, 30, 3);
    box(b, 10 + sway, 12, 10, 4, 4);
    box(b, 12 + sway, 16, 8, 4, 4);
    box(b, 14 + sway, 20, 6, 4, 4);
    box(b, 8, 32, 16, 3, 5);
    box(b, 11, 36, 3, 8, 3);
    box(b, 18, 36, 3, 6, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap magnetArt() {
    gs::Bitmap b(36, 30);
    box(b, 8, 4, 20, 8, 4);
    box(b, 10, 6, 16, 4, 3);
    box(b, 12, 5, 12, 2, 6);
    box(b, 4, 10, 8, 16, 2);
    box(b, 24, 10, 8, 16, 2);
    box(b, 6, 12, 4, 12, 3);
    box(b, 26, 12, 4, 12, 3);
    box(b, 4, 22, 8, 4, 5);
    box(b, 24, 22, 8, 4, 5);
    box(b, 14, 12, 8, 3, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap cableArt() {
    gs::Bitmap b(4, 36);
    for (int y = 0; y < 36; y++) {
        int x = 1;
        b.set(x, y, 8);
        b.set(x + 1, y, 4);
    }
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(14, 96);
    box(b, 4, 2, 6, 90, 4);
    box(b, 5, 2, 2, 90, 2);
    for (int y = 8; y < 86; y += 10) {
        box(b, 2, y, 10, 2, 3);
        box(b, 6, y, 2, 8, 5);
    }
    box(b, 2, 88, 10, 4, 4);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(84, 12);
    box(b, 2, 2, 80, 8, 2);
    box(b, 2, 2, 80, 2, 3);
    box(b, 2, 8, 80, 2, 4);
    for (int x = 6; x < 78; x += 12) box(b, x, 4, 6, 4, 6);
    return b;
}

gs::Bitmap jawArt() {
    gs::Bitmap b(28, 46);
    box(b, 4, 4, 20, 38, 2);
    box(b, 4, 4, 6, 38, 3);
    box(b, 6, 8, 16, 4, 6);
    box(b, 6, 16, 16, 4, 7);
    box(b, 6, 24, 16, 4, 6);
    box(b, 6, 32, 16, 4, 7);
    box(b, 8, 40, 12, 3, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap carArt(int kind) {
    gs::Bitmap b(42, 46);
    int body = kind ? 6 : 2;
    int hi = kind ? 4 : 3;
    box(b, 4, 24, 34, 14, body);
    box(b, 6, 26, 12, 5, 7);
    box(b, 22, 26, 12, 5, hi);
    box(b, 6, 34, 8, 4, 8);
    box(b, 26, 34, 8, 4, 8);
    box(b, 8, 8, 28, 14, hi);
    box(b, 10, 10, 10, 5, 7);
    box(b, 22, 10, 10, 5, body);
    box(b, 12, 20, 6, 3, 8);
    box(b, 24, 20, 6, 3, 8);
    box(b, 2, 16, 8, 3, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap shackArt() {
    gs::Bitmap b(56, 68);
    b.poly({{6.f, 26.f}, {28.f, 8.f}, {50.f, 26.f}}, 10);
    box(b, 8, 26, 40, 34, 9);
    box(b, 10, 28, 14, 12, 4);
    box(b, 32, 28, 12, 10, 7);
    box(b, 34, 30, 8, 6, 12);
    box(b, 22, 40, 12, 20, 11);
    box(b, 26, 48, 2, 8, 7);
    box(b, 8, 58, 40, 4, 14);
    box(b, 26, 6, 3, 6, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap hoistArt() {
    gs::Bitmap b(48, 92);
    box(b, 4, 16, 5, 70, 4);
    box(b, 6, 16, 2, 70, 2);
    box(b, 38, 16, 5, 70, 4);
    box(b, 39, 16, 2, 70, 2);
    box(b, 4, 14, 40, 6, 2);
    box(b, 4, 14, 40, 2, 3);
    box(b, 8, 20, 14, 2, 5);
    box(b, 26, 24, 12, 2, 5);
    box(b, 22, 18, 4, 16, 4);
    box(b, 20, 32, 8, 4, 6);
    box(b, 23, 36, 2, 10, 7);
    box(b, 4, 82, 8, 4, 4);
    box(b, 36, 82, 8, 4, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap drumArt() {
    gs::Bitmap b(20, 26);
    box(b, 4, 4, 12, 18, 6);
    box(b, 4, 4, 12, 3, 7);
    box(b, 4, 12, 12, 3, 7);
    box(b, 4, 19, 12, 3, 5);
    box(b, 6, 2, 8, 3, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap tireArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7, 7, 8);
    b.ellipse(9, 9, 3, 3, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 44);
    box(b, 6, 10, 2, 30, 4);
    box(b, 3, 4, 8, 7, 1);
    box(b, 4, 5, 6, 5, 12);
    box(b, 4, 38, 6, 3, 14);
    return b;
}

gs::Bitmap plateArt() {
    gs::Bitmap b(48, 10);
    for (int x = 2; x < 46; x += 8) {
        box(b, x, 2, 4, 6, 6);
        box(b, x + 4, 2, 4, 6, 7);
    }
    return b;
}

gs::Bitmap chevronArt() {
    gs::Bitmap b(16, 12);
    b.poly({{12.f, 2.f}, {4.f, 6.f}, {12.f, 10.f}}, 2);
    b.poly({{10.f, 2.f}, {2.f, 6.f}, {10.f, 10.f}}, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap matArt() {
    gs::Bitmap b(72, 12);
    box(b, 2, 3, 68, 6, 2);
    box(b, 2, 3, 68, 2, 5);
    box(b, 8, 5, 8, 3, 4);
    box(b, 24, 5, 8, 3, 4);
    box(b, 40, 5, 8, 3, 4);
    box(b, 56, 5, 8, 3, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap sparkArt(int frame) {
    gs::Bitmap b(10, 10);
    if (frame == 0) {
        box(b, 4, 1, 2, 8, 3);
        box(b, 1, 4, 8, 2, 2);
    } else {
        box(b, 2, 2, 2, 2, 2);
        box(b, 6, 2, 2, 2, 3);
        box(b, 4, 4, 2, 2, 2);
        box(b, 2, 6, 2, 2, 3);
        box(b, 6, 6, 2, 2, 2);
    }
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(22, 8);
    b.ellipse(11, 4, 8, 2, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 8, 1);
    b.ellipse(10, 10, 4, 4, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 11), gs::rgb4(14, 11, 4), gs::rgb4(6, 6, 8), gs::rgb4(14, 4, 3),
                          gs::rgb4(6, 13, 7), gs::rgb4(8, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 2)});
    setPal(vdp, PAL_YARD, {0, gs::rgb4(2, 2, 2), gs::rgb4(5, 4, 3), gs::rgb4(8, 7, 5), gs::rgb4(8, 3, 1),
                           gs::rgb4(2, 2, 2), gs::rgb4(10, 9, 8), gs::rgb4(4, 4, 4), gs::rgb4(3, 5, 2)});
    setPal(vdp, PAL_SCRAP, {0, gs::rgb4(1, 1, 1), gs::rgb4(8, 2, 1), gs::rgb4(12, 4, 2), gs::rgb4(5, 6, 7),
                            gs::rgb4(9, 10, 12), gs::rgb4(3, 5, 2), gs::rgb4(6, 8, 10), gs::rgb4(2, 2, 2),
                            gs::rgb4(8, 7, 6), gs::rgb4(6, 3, 2), gs::rgb4(3, 2, 2), gs::rgb4(14, 12, 4),
                            gs::rgb4(12, 9, 2), gs::rgb4(3, 3, 4), gs::rgb4(10, 5, 2)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(1, 1, 1), gs::rgb4(13, 10, 2), gs::rgb4(8, 6, 1), gs::rgb4(2, 2, 1),
                             gs::rgb4(15, 14, 8), gs::rgb4(5, 4, 3), gs::rgb4(14, 12, 6), gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(1, 1, 1), gs::rgb4(12, 5, 1), gs::rgb4(15, 8, 3), gs::rgb4(13, 9, 6),
                           gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 2), gs::rgb4(12, 13, 14), gs::rgb4(6, 3, 2),
                           gs::rgb4(14, 11, 2)});
    setPal(vdp, PAL_DOG, {0, gs::rgb4(1, 1, 1), gs::rgb4(7, 4, 2), gs::rgb4(10, 6, 3), gs::rgb4(3, 2, 1),
                          gs::rgb4(12, 9, 6), gs::rgb4(14, 12, 3), gs::rgb4(11, 3, 3), gs::rgb4(13, 10, 2)});
    setPal(vdp, PAL_STEEL, {0, gs::rgb4(1, 1, 2), gs::rgb4(6, 7, 8), gs::rgb4(12, 13, 14), gs::rgb4(3, 3, 5),
                            gs::rgb4(8, 4, 2), gs::rgb4(13, 11, 2), gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 6)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 13), gs::rgb4(15, 8, 2), gs::rgb4(8, 5, 2)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 0, 0)});
    vdp.setFogColor(gs::rgb4(4, 3, 2));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);
    paintYard(vdp, tiles);

    art.stand = gs::uploadMipped(vdp, handArt(0));
    art.runA = gs::uploadMipped(vdp, handArt(1));
    art.runB = gs::uploadMipped(vdp, handArt(2));
    art.pry = gs::uploadMipped(vdp, handArt(3));
    for (int i = 0; i < 3; i++) art.dog[i] = gs::uploadMipped(vdp, dogArt(i));
    art.banner[0] = gs::uploadMipped(vdp, bannerArt(0));
    art.banner[1] = gs::uploadMipped(vdp, bannerArt(1));
    art.magnet = gs::uploadMipped(vdp, magnetArt());
    art.cable = gs::uploadMipped(vdp, cableArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.jaw = gs::uploadMipped(vdp, jawArt());
    art.car[0] = gs::uploadMipped(vdp, carArt(0));
    art.car[1] = gs::uploadMipped(vdp, carArt(1));
    art.shack = gs::uploadMipped(vdp, shackArt());
    art.hoist = gs::uploadMipped(vdp, hoistArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.tire = gs::uploadMipped(vdp, tireArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.plate = gs::uploadMipped(vdp, plateArt());
    art.chevron = gs::uploadMipped(vdp, chevronArt());
    art.mat = gs::uploadMipped(vdp, matArt());
    art.spark = gs::uploadMipped(vdp, sparkArt(0));
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
}

}  // namespace yard
