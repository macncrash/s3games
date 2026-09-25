#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace fair {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 0, 2));
}

Bitmap kidArt(int pose) {
    Bitmap b(52, 88);
    int step = pose ? 5 : 0;
    b.ellipse(26, 18, 11, 12, 4);
    b.ellipse(26, 20, 8, 9, 3);
    b.poly({{14, 12}, {38, 12}, {40, 18}, {12, 18}}, 10);
    b.rect(10, 17, 32, 5, 10);
    b.rect(12, 18, 28, 2, 11);
    b.rect(21, 18, 2, 2, 8);
    b.rect(29, 18, 2, 2, 8);
    b.rect(23, 24, 6, 1, 9);
    b.poly({{14, 30}, {38, 30}, {42, 54}, {10, 54}}, 1);
    b.poly({{22, 31}, {30, 31}, {29, 50}, {23, 50}}, 7);
    for (int i = 0; i < 3; i++) b.rect(23, 34 + i * 5, 6, 2, 10);
    b.rect(8, 32, 6, 18, 1);
    b.rect(38, 32, 6, 16, 1);
    b.ellipse(10, 52, 4, 4, 3);
    b.ellipse(42, 50, 4, 4, 3);
    b.rect(40, 46, 9, 5, 8);
    b.rect(41, 48, 7, 1, 10);
    b.rect(16 - step, 54, 8, 22, 5);
    b.rect(28 + step, 54, 8, 22, 5);
    b.ellipse(18 - step, 78, 7, 4, 6);
    b.ellipse(34 + step, 78, 7, 4, 6);
    b.outline(9, false);
    return b;
}

Bitmap ringArt() {
    Bitmap b(46, 28);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            float dx = (x + 0.5f - 23.f) / 18.f;
            float dy = (y + 0.5f - 14.f) / 10.f;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d <= 1.f && d >= 0.58f) b.set(x, y, y < 13 ? 2 : (d > 0.84f ? 3 : 1));
        }
    }
    return b;
}

Bitmap bottleArt() {
    Bitmap b(30, 56);
    b.rect(12, 2, 6, 4, 4);
    b.rect(11, 6, 8, 10, 1);
    b.rect(11, 12, 8, 3, 3);
    b.ellipse(15, 34, 12, 16, 1);
    b.ellipse(11, 28, 5, 8, 2);
    b.ellipse(15, 44, 7, 4, 6);
    b.outline(5, false);
    return b;
}

Bitmap balloonArt() {
    Bitmap b(40, 58);
    b.ellipse(20, 20, 15, 17, 1);
    b.ellipse(14, 14, 6, 7, 2);
    b.ellipse(20, 32, 5, 4, 3);
    b.poly({{17, 34}, {23, 34}, {20, 40}}, 4);
    b.line(20, 40, 20, 56, 4, 1.4f);
    b.outline(5, false);
    return b;
}

Bitmap dartArt() {
    Bitmap b(44, 16);
    b.poly({{2, 2}, {2, 14}, {16, 8}}, 4);
    b.poly({{4, 4}, {4, 12}, {14, 8}}, 2);
    b.rect(14, 6, 20, 4, 3);
    b.poly({{32, 4}, {42, 8}, {32, 12}}, 1);
    b.outline(5, false);
    return b;
}

Bitmap bellArt() {
    Bitmap b(44, 40);
    b.rect(20, 1, 4, 7, 3);
    b.ellipse(22, 18, 16, 12, 1);
    b.ellipse(22, 16, 8, 6, 2);
    b.rect(6, 26, 32, 6, 1);
    b.rect(8, 26, 28, 2, 2);
    b.ellipse(22, 24, 3, 4, 4);
    b.outline(5, false);
    return b;
}

Bitmap puckArt() {
    Bitmap b(18, 14);
    b.ellipse(9, 7, 8, 5, 1);
    b.ellipse(7, 5, 3, 2, 2);
    b.outline(3, false);
    return b;
}

Bitmap hammerArt(int up) {
    Bitmap b(48, 48);
    if (up) {
        b.rect(30, 6, 8, 14, 1);
        b.rect(30, 6, 8, 4, 2);
        b.rect(22, 16, 6, 26, 3);
        b.ellipse(24, 42, 5, 4, 3);
    } else {
        b.rect(8, 28, 16, 8, 1);
        b.rect(8, 28, 16, 3, 2);
        b.rect(20, 18, 6, 22, 3);
        b.ellipse(22, 16, 4, 4, 3);
    }
    b.outline(5, false);
    return b;
}

Bitmap towerArt() {
    Bitmap b(64, 132);
    b.rect(26, 8, 12, 108, 3);
    b.rect(22, 8, 6, 108, 1);
    b.rect(36, 8, 6, 108, 1);
    b.rect(24, 8, 2, 108, 2);
    b.rect(10, 112, 44, 14, 1);
    b.rect(12, 112, 40, 4, 2);
    for (int i = 0; i < 8; i++) b.rect(20, 16 + i * 12, 4, 2, 4);
    b.outline(6, false);
    return b;
}

Bitmap awningArt() {
    Bitmap b(104, 86);
    b.rect(18, 28, 68, 30, 3);
    b.poly({{4, 26}, {100, 26}, {92, 8}, {12, 8}}, 5);
    b.rect(8, 6, 88, 4, 4);
    for (int i = 0; i < 7; i++) {
        int x = 10 + i * 12;
        b.poly({{x, 26.f}, {x + 12.f, 26.f}, {x + 6.f, 38.f}}, (i & 1) ? 4 : 5);
    }
    b.rect(10, 24, 6, 46, 1);
    b.rect(88, 24, 6, 46, 1);
    b.rect(12, 24, 2, 46, 2);
    b.rect(6, 66, 92, 8, 2);
    b.rect(10, 74, 84, 8, 1);
    b.outline(6, false);
    return b;
}

Bitmap gateArt() {
    Bitmap b(96, 112);
    b.rect(8, 40, 10, 68, 1);
    b.rect(78, 40, 10, 68, 1);
    b.rect(10, 40, 3, 68, 2);
    b.rect(80, 40, 3, 68, 2);
    for (int y = 10; y <= 48; y++) {
        for (int x = 8; x <= 88; x++) {
            float dx = (x - 48) / 40.f;
            float dy = (y - 48) / 34.f;
            float d = dx * dx + dy * dy;
            if (d <= 1.f && d >= 0.62f) b.set(x, y, 1);
        }
    }
    b.rect(28, 36, 40, 14, 4);
    b.rect(30, 38, 36, 10, 5);
    b.outline(6, false);
    return b;
}

Bitmap carArt() {
    Bitmap b(24, 22);
    b.rect(4, 4, 16, 4, 3);
    b.poly({{3, 8}, {21, 8}, {19, 18}, {5, 18}}, 1);
    b.rect(7, 10, 4, 4, 4);
    b.rect(13, 10, 4, 4, 4);
    b.outline(5, false);
    return b;
}

Bitmap hubArt() {
    Bitmap b(26, 26);
    b.ellipse(13, 13, 11, 11, 1);
    b.ellipse(13, 13, 5, 5, 3);
    b.ellipse(10, 10, 2, 2, 2);
    b.outline(5, false);
    return b;
}

Bitmap standArt() {
    Bitmap b(96, 100);
    b.poly({{10, 96}, {26, 96}, {54, 8}, {40, 8}}, 1);
    b.poly({{56, 8}, {70, 8}, {86, 96}, {70, 96}}, 1);
    b.rect(18, 48, 60, 7, 2);
    b.rect(44, 8, 8, 88, 3);
    b.outline(5, false);
    return b;
}

Bitmap bulbArt() {
    Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(4, 4, 2, 2, 2);
    return b;
}

Bitmap moonArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 13, 13, 1);
    b.ellipse(22, 14, 9, 9, 0);
    b.ellipse(11, 12, 1.4f, 1.4f, 2);
    b.ellipse(12, 20, 1.2f, 1.2f, 2);
    return b;
}

Bitmap fluffArt() {
    Bitmap b(36, 48);
    b.ellipse(18, 16, 14, 12, 1);
    b.ellipse(11, 14, 7, 7, 2);
    b.ellipse(24, 18, 6, 6, 3);
    b.rect(16, 26, 4, 18, 4);
    b.outline(5, false);
    return b;
}

Bitmap burstArt() {
    Bitmap b(32, 32);
    b.poly({{16, 1}, {19, 12}, {31, 12}, {21, 19}, {25, 31}, {16, 23}, {7, 31}, {11, 19}, {1, 12}, {13, 12}}, 1);
    b.ellipse(16, 16, 5, 5, 2);
    return b;
}

Bitmap buntArt() {
    Bitmap b(14, 16);
    b.poly({{7, 15}, {1, 2}, {13, 2}}, 1);
    b.poly({{7, 12}, {4, 4}, {10, 4}}, 2);
    return b;
}

Bitmap boardArt() {
    Bitmap b(200, 96);
    b.rect(0, 0, 200, 96, 1);
    b.rect(8, 8, 184, 80, 3);
    b.rect(14, 14, 172, 68, 2);
    for (int i = 0; i < 6; i++) b.rect(20 + i * 28, 20, 2, 56, 1);
    b.outline(6, false);
    return b;
}

Bitmap plankArt() {
    Bitmap b(160, 24);
    b.rect(0, 0, 160, 24, 1);
    b.rect(0, 0, 160, 4, 2);
    b.rect(0, 20, 160, 4, 3);
    for (int i = 0; i < 5; i++) b.ellipse(16.f + i * 32, 12, 2, 2, 3);
    b.outline(6, false);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(36, 14);
    b.ellipse(18, 7, 16, 5, 1);
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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

void paintTile(uint8_t* px, int c) {
    for (int i = 0; i < 64; i++) px[i] = uint8_t(c);
}

int tileOf(gs::TileAlloc& tiles, gs::VDP& vdp, const uint8_t px[64]) {
    int t = tiles.shared(px);
    if (t > 0) vdp.loadTile(t, px);
    return t;
}

void loadGround(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    uint8_t edge[64], plank[64], plank2[64], grass[64], grass2[64];
    paintTile(edge, 1);
    paintTile(plank, 1);
    paintTile(plank2, 2);
    paintTile(grass, 3);
    paintTile(grass2, 3);
    for (int x = 0; x < 8; x++) {
        edge[0 * 8 + x] = 3;
        edge[1 * 8 + x] = 3;
        edge[2 * 8 + x] = (x & 1) ? 4 : 3;
        edge[3 * 8 + x] = 5;
        plank[3 * 8 + x] = 5;
        plank[7 * 8 + x] = 5;
        plank2[2 * 8 + x] = 5;
        plank2[6 * 8 + x] = 5;
    }
    plank[1 * 8 + 2] = 2;
    plank[5 * 8 + 6] = 2;
    plank2[4 * 8 + 1] = 1;
    plank2[1 * 8 + 5] = 6;
    grass[2 * 8 + 3] = 4;
    grass[5 * 8 + 6] = 4;
    grass[4 * 8 + 1] = 6;
    grass2[1 * 8 + 5] = 4;
    grass2[6 * 8 + 2] = 7;
    grass2[3 * 8 + 6] = 4;
    a.tileEdge = tileOf(tiles, vdp, edge);
    a.tilePlank = tileOf(tiles, vdp, plank);
    a.tilePlank2 = tileOf(tiles, vdp, plank2);
    a.tileGrass = tileOf(tiles, vdp, grass);
    a.tileGrass2 = tileOf(tiles, vdp, grass2);

    vdp.B.clear();
    for (int cy = 19; cy < 28; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int tile = a.tilePlank;
            if (cy == 19) tile = a.tileEdge;
            else if (cy >= 24) tile = ((cx + cy) & 1) ? a.tileGrass : a.tileGrass2;
            else if ((cx + cy) % 3 == 0) tile = a.tilePlank2;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_GROUND));
        }
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 0, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 11, 13), gs::rgb4(15, 13, 8), gs::rgb4(15, 5, 4),
                          gs::rgb4(6, 6, 8), gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(14, 10, 2), gs::rgb4(15, 14, 7), gs::rgb4(8, 5, 1), gs::rgb4(4, 2, 1),
                           gs::rgb4(2, 1, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(13, 2, 3), gs::rgb4(15, 8, 7), gs::rgb4(7, 0, 1), gs::rgb4(15, 14, 12),
                          gs::rgb4(3, 0, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BLUE, {0, gs::rgb4(3, 6, 13), gs::rgb4(8, 11, 15), gs::rgb4(1, 2, 7), gs::rgb4(14, 15, 15),
                           gs::rgb4(0, 1, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PLAYER,
           {0, gs::rgb4(2, 10, 9), gs::rgb4(1, 6, 6), gs::rgb4(14, 10, 7), gs::rgb4(4, 2, 1), gs::rgb4(2, 2, 6),
            gs::rgb4(3, 1, 1), gs::rgb4(14, 13, 9), gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2), gs::rgb4(13, 2, 3),
            gs::rgb4(6, 1, 2), 0, 0, 0, shadow});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(9, 5, 2), gs::rgb4(12, 8, 4), gs::rgb4(5, 2, 1), gs::rgb4(14, 12, 8),
                           gs::rgb4(12, 2, 3), gs::rgb4(2, 1, 0), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(12, 9, 3), gs::rgb4(15, 13, 7), gs::rgb4(6, 4, 1), gs::rgb4(13, 2, 2),
                            gs::rgb4(2, 1, 0), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GROUND, {0, gs::rgb4(10, 6, 3), gs::rgb4(8, 5, 2), gs::rgb4(2, 7, 2), gs::rgb4(5, 11, 4),
                             gs::rgb4(4, 2, 1), gs::rgb4(12, 4, 6), gs::rgb4(13, 11, 3), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(14, 13, 11), gs::rgb4(15, 15, 14), gs::rgb4(12, 2, 2), gs::rgb4(14, 11, 3),
                            gs::rgb4(3, 2, 2), gs::rgb4(8, 7, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(3, 3, 6), gs::rgb4(6, 6, 10), gs::rgb4(1, 1, 3), gs::rgb4(15, 12, 6),
                            gs::rgb4(2, 1, 2), gs::rgb4(8, 8, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PINK, {0, gs::rgb4(13, 5, 8), gs::rgb4(15, 10, 13), gs::rgb4(8, 2, 5), gs::rgb4(12, 8, 4),
                           gs::rgb4(3, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    vdp.setFogColor(gs::rgb4(2, 0, 4));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    loadGround(vdp, tiles, art);

    art.kid[0] = gs::uploadMipped(vdp, kidArt(0));
    art.kid[1] = gs::uploadMipped(vdp, kidArt(1));
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.bottle = gs::uploadMipped(vdp, bottleArt());
    art.balloon = gs::uploadMipped(vdp, balloonArt());
    art.dart = gs::uploadMipped(vdp, dartArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.puck = gs::uploadMipped(vdp, puckArt());
    art.hammer[0] = gs::uploadMipped(vdp, hammerArt(0));
    art.hammer[1] = gs::uploadMipped(vdp, hammerArt(1));
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.awning = gs::uploadMipped(vdp, awningArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.car = gs::uploadMipped(vdp, carArt());
    art.hub = gs::uploadMipped(vdp, hubArt());
    art.stand = gs::uploadMipped(vdp, standArt());
    art.bulb = gs::uploadMipped(vdp, bulbArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.fluff = gs::uploadMipped(vdp, fluffArt());
    art.burst = gs::uploadMipped(vdp, burstArt());
    art.bunt = gs::uploadMipped(vdp, buntArt());
    art.board = gs::uploadMipped(vdp, boardArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
}

}  // namespace fair
