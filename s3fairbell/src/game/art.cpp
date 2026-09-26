#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace fairbell {
namespace {

constexpr float PI = 3.14159265f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap kidArt(bool step) {
    gs::Bitmap b(18, 34);
    b.ellipse(9, 8, 4.2f, 4.4f, 1);
    b.rect(5, 2, 8, 3, 7);
    b.rect(4, 4, 3, 2, 4);
    b.rect(6, 12, 6, 9, 2);
    b.rect(4, 13, 2, 6, 5);
    b.rect(12, 13, 2, 6, 5);
    b.rect(7, 14, 4, 2, 6);
    if (step) {
        b.rect(5, 21, 3, 9, 3);
        b.rect(10, 22, 3, 8, 3);
        b.rect(4, 29, 4, 3, 4);
        b.rect(10, 29, 4, 3, 4);
    } else {
        b.rect(6, 21, 3, 9, 3);
        b.rect(10, 21, 3, 9, 3);
        b.rect(5, 29, 4, 3, 4);
        b.rect(10, 29, 4, 3, 4);
    }
    b.set(7, 8, 4);
    b.set(11, 8, 4);
    b.outline(4, false);
    return b;
}

gs::Bitmap hammerArt(bool up) {
    gs::Bitmap b(28, 28);
    if (up) {
        b.rect(16, 3, 8, 8, 1);
        b.rect(16, 3, 8, 3, 2);
        b.rect(12, 10, 5, 14, 4);
        b.rect(13, 10, 2, 14, 5);
    } else {
        b.rect(4, 16, 12, 7, 1);
        b.rect(4, 16, 12, 2, 2);
        b.rect(14, 8, 5, 12, 4);
        b.rect(15, 8, 2, 12, 5);
    }
    b.outline(3, false);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(26, 24);
    b.rect(11, 1, 4, 5, 3);
    b.ellipse(13, 12, 10.2f, 8.2f, 1);
    b.ellipse(10, 10, 3.2f, 2.6f, 2);
    b.rect(3, 16, 20, 5, 1);
    b.rect(5, 16, 16, 2, 2);
    b.ellipse(13, 14, 1.8f, 2.6f, 3);
    b.outline(3, false);
    return b;
}

gs::Bitmap puckArt() {
    gs::Bitmap b(14, 12);
    b.ellipse(7, 6, 6.2f, 4.6f, 1);
    b.ellipse(5, 4, 2.2f, 1.6f, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(kTowerImgW, kTowerImgH);
    for (int y = 0; y < kTowerImgH; y++) {
        for (int x = 2; x <= 13; x++) b.set(x, y, x < 5 ? 2 : 1);
        for (int x = 26; x <= 37; x++) b.set(x, y, x > 34 ? 3 : 1);
    }
    b.rect(16, 0, 8, 4, 2);
    for (int y = kCapTop; y <= kCapBot; y++)
        for (int x = 8; x <= 31; x++) b.set(x, y, 5);
    for (int y = kGoldTop; y <= kGoldBot; y++)
        for (int x = 8; x <= 31; x++) b.set(x, y, 4);
    for (int y = kGoldBot + 1; y <= 95; y++) {
        b.set(13, y, 3);
        b.set(26, y, 3);
        if ((y % 14) == 0) {
            b.set(6, y, 6);
            b.set(33, y, 6);
        }
    }
    for (int y = 96; y < kTowerImgH; y++)
        for (int x = 0; x < kTowerImgW; x++) b.set(x, y, y < 101 ? 2 : (y > 106 ? 3 : 1));
    b.rect(0, 100, kTowerImgW, 3, 6);
    return b;
}

gs::Bitmap awningArt() {
    gs::Bitmap b(48, 16);
    for (int x = 0; x < 48; x++) {
        int stripe = (x / 6) % 3;
        int c = stripe == 0 ? 1 : (stripe == 1 ? 2 : 3);
        for (int y = 0; y < 9; y++) b.set(x, y, c);
        int scallop = x % 8;
        int cut = scallop < 4 ? scallop : 7 - scallop;
        for (int y = 9; y < 9 + cut && y < 16; y++) b.set(x, y, c);
    }
    b.rect(0, 0, 48, 2, 4);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(32, 56);
    b.rect(2, 12, 5, 44, 1);
    b.rect(25, 12, 5, 44, 1);
    b.rect(3, 12, 2, 44, 2);
    b.rect(26, 12, 2, 44, 2);
    b.rect(1, 6, 30, 7, 1);
    b.rect(3, 8, 26, 2, 4);
    b.rect(8, 16, 16, 8, 3);
    b.rect(10, 18, 12, 4, 4);
    return b;
}

gs::Bitmap bulbArt() {
    gs::Bitmap b(8, 10);
    b.ellipse(4, 4, 3.1f, 3.2f, 2);
    b.ellipse(3, 3, 1.2f, 1.2f, 1);
    b.rect(3, 7, 2, 2, 3);
    return b;
}

gs::Bitmap pennantArt() {
    gs::Bitmap b(10, 14);
    b.poly({{5, 1}, {9, 12}, {1, 12}}, 2);
    b.poly({{5, 4}, {7, 11}, {3, 11}}, 1);
    b.rect(4, 0, 2, 2, 3);
    return b;
}

gs::Bitmap balloonArt() {
    gs::Bitmap b(14, 18);
    b.ellipse(7, 7, 5.6f, 6.2f, 2);
    b.ellipse(5, 5, 2.f, 2.2f, 1);
    b.poly({{7, 12}, {9, 15}, {5, 15}}, 3);
    b.line(7, 15, 8, 17, 3, 1);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6.4f, 6.4f, 1);
    b.ellipse(11, 7, 5.f, 5.f, 0);
    b.set(5, 6, 2);
    b.set(7, 10, 2);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(7, 7);
    b.line(3, 0, 3, 6, 1, 1);
    b.line(0, 3, 6, 3, 1, 1);
    b.set(3, 3, 2);
    return b;
}

gs::Bitmap burstArt() {
    gs::Bitmap b(16, 16);
    b.line(8, 1, 8, 15, 1, 1);
    b.line(1, 8, 15, 8, 1, 1);
    b.line(3, 3, 13, 13, 2, 1);
    b.line(13, 3, 3, 13, 2, 1);
    b.ellipse(8, 8, 2.2f, 2.2f, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7.f, 2.f, 1);
    return b;
}

gs::Bitmap carArt() {
    gs::Bitmap b(12, 10);
    b.rect(1, 2, 10, 6, 1);
    b.rect(2, 3, 3, 3, 2);
    b.rect(7, 3, 3, 3, 2);
    b.rect(5, 0, 2, 2, 3);
    return b;
}

gs::Bitmap hubArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 10.f, 10.f, 1);
    b.ellipse(11, 11, 7.4f, 7.4f, 0);
    for (int i = 0; i < 6; i++) {
        float a = i * PI / 3.f;
        b.line(11, 11, 11 + std::cos(a) * 9.f, 11 + std::sin(a) * 9.f, 3, 1.1f);
    }
    b.ellipse(11, 11, 2.4f, 2.4f, 2);
    return b;
}

gs::Bitmap standArt() {
    gs::Bitmap b(14, 40);
    b.poly({{7, 2}, {12, 38}, {2, 38}}, 2);
    b.poly({{7, 8}, {10, 36}, {4, 36}}, 1);
    b.rect(4, 36, 6, 4, 3);
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
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

void loadGround(gs::VDP& vdp, gs::TileAlloc& tiles) {
    uint8_t lip[64] = {}, plank[64] = {}, seam[64] = {}, dirt[64] = {};
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            lip[y * 8 + x] = y < 2 ? 5 : 1;
            plank[y * 8 + x] = ((x + y) % 5 == 0) ? 2 : 1;
            seam[y * 8 + x] = (y == 0 || y == 4) ? 2 : 1;
            dirt[y * 8 + x] = ((x * 3 + y) % 4 == 0) ? 4 : 3;
        }
    }
    auto tile = [&](const uint8_t* px) {
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        return t;
    };
    int lipT = tile(lip), plankT = tile(plank), seamT = tile(seam), dirtT = tile(dirt);
    for (int cy = 24; cy < 28; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int t = cy == 24 ? lipT : (cy == 27 ? dirtT : (((cx + cy) & 1) ? seamT : plankT));
            vdp.B.set(cx, cy, gs::entry(t, PAL_GROUND));
        }
    }
}

gs::Image phrase(gs::VDP& vdp, const char* s, int scale) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, 1, 2, 0, 1}));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 8, 11), gs::rgb4(15, 12, 6), gs::rgb4(15, 6, 5)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 12), gs::rgb4(8, 5, 1), gs::rgb4(15, 10, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 4), gs::rgb4(15, 14, 12), gs::rgb4(6, 0, 1), gs::rgb4(15, 8, 6)});
    setPal(vdp, PAL_BLUE, {0, gs::rgb4(4, 7, 14), gs::rgb4(12, 14, 15), gs::rgb4(1, 2, 6), gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_KID, {0, gs::rgb4(15, 12, 9), gs::rgb4(12, 3, 4), gs::rgb4(2, 3, 8), gs::rgb4(3, 2, 1),
                          gs::rgb4(11, 8, 6), gs::rgb4(15, 15, 14), gs::rgb4(14, 10, 3)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(4, 2, 1), gs::rgb4(15, 12, 4),
                           gs::rgb4(8, 1, 2), gs::rgb4(15, 14, 10)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(13, 10, 4), gs::rgb4(15, 14, 8), gs::rgb4(5, 3, 1), gs::rgb4(10, 6, 3),
                            gs::rgb4(5, 3, 1), gs::rgb4(12, 3, 3)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(7, 7, 11), gs::rgb4(3, 3, 6), gs::rgb4(13, 12, 10), gs::rgb4(2, 2, 4)});
    setPal(vdp, PAL_PINK, {0, gs::rgb4(14, 6, 9), gs::rgb4(15, 12, 14), gs::rgb4(8, 2, 5)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 14, 11), gs::rgb4(12, 10, 7), gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_BULB, {0, gs::rgb4(15, 15, 13), gs::rgb4(15, 13, 5), gs::rgb4(6, 5, 2), gs::rgb4(15, 8, 3)});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 13, 5), gs::rgb4(4, 1, 2), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_GROUND, {0, gs::rgb4(11, 7, 3), gs::rgb4(7, 4, 2), gs::rgb4(3, 6, 2), gs::rgb4(5, 3, 1),
                             gs::rgb4(14, 10, 6)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 8, 6), gs::rgb4(5, 1, 1), gs::rgb4(15, 14, 10)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(12, 8, 4), gs::rgb4(7, 4, 2), gs::rgb4(13, 2, 3), gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_AWN, {0, gs::rgb4(13, 2, 3), gs::rgb4(15, 13, 8), gs::rgb4(15, 11, 3), gs::rgb4(6, 1, 1)});
    vdp.setFogColor(gs::rgb4(2, 1, 4));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    loadGround(vdp, tiles);

    art.kid[0] = gs::uploadMipped(vdp, kidArt(false));
    art.kid[1] = gs::uploadMipped(vdp, kidArt(true));
    art.hammer[0] = gs::uploadMipped(vdp, hammerArt(false));
    art.hammer[1] = gs::uploadMipped(vdp, hammerArt(true));
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.puck = gs::uploadMipped(vdp, puckArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.awning = gs::uploadMipped(vdp, awningArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.bulb = gs::uploadMipped(vdp, bulbArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.balloon = gs::uploadMipped(vdp, balloonArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.burst = gs::uploadMipped(vdp, burstArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.car = gs::uploadMipped(vdp, carArt());
    art.hub = gs::uploadMipped(vdp, hubArt());
    art.stand = gs::uploadMipped(vdp, standArt());
    art.logo = phrase(vdp, "S3 FAIRBELL", 2);
}

}  // namespace fairbell
