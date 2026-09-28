#include "game/art.h"

#include <algorithm>
#include <cmath>

namespace lanternchime {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* c) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

void solid(uint8_t* px, int c) {
    for (int i = 0; i < 64; i++) px[i] = uint8_t(c);
}

gs::Bitmap bearerArt() {
    gs::Bitmap b(28, 46);
    b.ellipse(14, 8, 6, 6, 2);
    b.rect(11, 13, 6, 3, 3);
    b.poly({{6, 16}, {22, 16}, {24, 34}, {4, 34}}, 1);
    b.rect(8, 18, 5, 8, 4);
    b.rect(12, 33, 5, 10, 5);
    b.rect(17, 33, 4, 10, 6);
    b.rect(10, 42, 7, 3, 7);
    b.rect(16, 42, 6, 3, 7);
    b.rect(20, 18, 4, 12, 1);
    b.ellipse(14, 7, 2, 1, 8);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(16, 28);
    b.line(8, 0, 8, 22, 1, 1.4f);
    b.line(8, 4, 3, 10, 1, 1.2f);
    b.ellipse(3, 12, 3, 4, 2);
    b.ellipse(3, 12, 1.4f, 2.2f, 3);
    b.ellipse(8, 24, 2, 2, 4);
    return b;
}

gs::Bitmap cageArt() {
    gs::Bitmap b(26, 40);
    b.rect(11, 0, 4, 5, 1);
    b.poly({{6, 5}, {20, 5}, {23, 10}, {3, 10}}, 2);
    b.rect(4, 10, 18, 18, 3);
    b.rect(7, 13, 12, 12, 4);
    b.line(4, 16, 22, 16, 1, 1.1f);
    b.line(4, 22, 22, 22, 1, 1.1f);
    b.line(13, 10, 13, 28, 1, 1.1f);
    b.poly({{5, 28}, {21, 28}, {17, 33}, {9, 33}}, 2);
    b.rect(12, 33, 2, 7, 1);
    return b;
}

gs::Bitmap flameArt(int frame) {
    gs::Bitmap b(12, 16);
    if (frame == 0) {
        b.poly({{6, 1}, {11, 11}, {1, 11}}, 1);
        b.ellipse(6, 12, 4, 3, 1);
        b.poly({{6, 5}, {9, 13}, {3, 13}}, 2);
        b.ellipse(6, 11, 1.4f, 2.2f, 3);
    } else {
        b.poly({{7, 2}, {11, 12}, {2, 11}}, 1);
        b.ellipse(6, 12, 4, 3, 1);
        b.ellipse(6, 10, 2, 3, 2);
        b.set(6, 8, 3);
    }
    return b;
}

gs::Bitmap glowArt() {
    gs::Bitmap b(48, 48);
    b.ellipse(24, 26, 22, 18, 1);
    b.ellipse(24, 26, 12, 10, 2);
    b.ellipse(24, 26, 5, 4, 3);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(28, 30);
    b.rect(13, 0, 2, 4, 1);
    b.poly({{6, 6}, {22, 6}, {24, 20}, {4, 20}}, 2);
    b.ellipse(14, 20, 12, 5, 2);
    b.ellipse(14, 20, 8, 3, 3);
    b.ellipse(14, 16, 2, 3, 4);
    b.line(6, 10, 22, 10, 3, 1);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(8, 64);
    for (int y = 0; y < 64; y++) {
        int x = 3 + ((y / 4) % 2);
        b.set(x, y, 1);
        b.set(x + 1, y, 2);
    }
    b.ellipse(4, 60, 3, 3, 3);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(18, 12, 9, 9, 0);
    b.ellipse(10, 16, 1.2f, 1.2f, 2);
    b.ellipse(13, 20, 1.f, 1.2f, 2);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(90, 110);
    b.poly({{45, 2}, {78, 28}, {12, 28}}, 1);
    b.rect(18, 26, 54, 80, 2);
    b.rect(22, 26, 46, 6, 3);
    b.rect(34, 78, 22, 28, 4);
    b.rect(40, 86, 10, 20, 5);
    b.rect(28, 40, 12, 16, 6);
    b.rect(50, 40, 12, 16, 6);
    b.rect(39, 48, 12, 8, 6);
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(64, 64);
    b.ellipse(32, 32, 30, 30, 1);
    b.ellipse(32, 32, 26, 26, 2);
    b.ellipse(32, 32, 3, 3, 3);
    for (int i = 0; i < 12; i++) {
        float a = float(i) * 3.1415926f / 6.f;
        float x0 = 32 + std::sin(a) * 20.f;
        float y0 = 32 - std::cos(a) * 20.f;
        float x1 = 32 + std::sin(a) * 24.f;
        float y1 = 32 - std::cos(a) * 24.f;
        b.line(x0, y0, x1, y1, 4, i % 3 == 0 ? 2.f : 1.2f);
    }
    return b;
}

gs::Bitmap handArt(float deg, float len, int color) {
    gs::Bitmap b(40, 40);
    float a = deg * 3.1415926f / 180.f;
    float x1 = 20.f + std::sin(a) * len;
    float y1 = 20.f - std::cos(a) * len;
    b.line(20, 20, x1, y1, color, 1.7f);
    b.ellipse(20, 20, 2.2f, 2.2f, color);
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

void paintYard(gs::VDP& vdp, gs::TileAlloc& tiles) {
    uint8_t sky[14][64];
    for (int row = 0; row < 14; row++) {
        int c = 1 + row / 5;
        if (c > 3) c = 3;
        solid(sky[row], c);
        int id = tiles.shared(sky[row]);
        for (int y = row * 2; y < row * 2 + 2 && y < 18; y++)
            for (int x = 0; x < 64; x++) vdp.B.set(x, y, gs::entry(id, PAL_SKY));
    }
    uint8_t stone[64];
    uint8_t path[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            bool grout = (y == 0) || (x % 4 == 0);
            stone[y * 8 + x] = uint8_t(grout ? 1 : ((x + y) % 5 == 0 ? 3 : 2));
            path[y * 8 + x] = uint8_t(grout ? 4 : 5);
        }
    int ts = tiles.shared(stone);
    int tp = tiles.shared(path);
    for (int y = 18; y < 32; y++)
        for (int x = 0; x < 64; x++) {
            bool walk = y >= 22 && y <= 26;
            vdp.B.set(x, y, gs::entry(walk ? tp : ts, PAL_YARD));
        }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    auto C = gs::rgb4;
    uint16_t hud[16] = {};
    hud[1] = C(15, 14, 10);
    hud[2] = C(6, 4, 2);
    setPal(vdp, PAL_HUD, hud);

    uint16_t sky[16] = {};
    sky[1] = C(1, 1, 4);
    sky[2] = C(1, 2, 6);
    sky[3] = C(2, 3, 8);
    setPal(vdp, PAL_SKY, sky);

    uint16_t yard[16] = {};
    yard[1] = C(2, 2, 3);
    yard[2] = C(4, 4, 5);
    yard[3] = C(6, 6, 7);
    yard[4] = C(3, 3, 3);
    yard[5] = C(5, 5, 4);
    setPal(vdp, PAL_YARD, yard);

    uint16_t fire[16] = {};
    fire[1] = C(15, 8, 1);
    fire[2] = C(15, 13, 3);
    fire[3] = C(15, 15, 12);
    setPal(vdp, PAL_FIRE, fire);

    uint16_t body[16] = {};
    body[1] = C(3, 3, 6);
    body[2] = C(12, 8, 5);
    body[3] = C(8, 4, 3);
    body[4] = C(15, 12, 6);
    body[5] = C(2, 2, 4);
    body[6] = C(1, 1, 3);
    body[7] = C(1, 1, 2);
    body[8] = C(4, 2, 2);
    setPal(vdp, PAL_BODY, body);

    uint16_t iron[16] = {};
    iron[1] = C(3, 3, 4);
    iron[2] = C(6, 6, 7);
    iron[3] = C(2, 3, 5);
    iron[4] = C(10, 8, 3);
    setPal(vdp, PAL_IRON, iron);

    uint16_t clock[16] = {};
    clock[1] = C(5, 4, 3);
    clock[2] = C(4, 4, 6);
    clock[3] = C(8, 7, 5);
    clock[4] = C(2, 2, 3);
    clock[5] = C(1, 1, 2);
    clock[6] = C(12, 10, 4);
    setPal(vdp, PAL_CLOCK, clock);

    uint16_t glow[16] = {};
    glow[1] = C(6, 3, 1);
    glow[2] = C(10, 6, 1);
    glow[3] = C(14, 10, 3);
    setPal(vdp, PAL_GLOW, glow);

    vdp.setFogColor(C(1, 1, 3));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    paintYard(vdp, tiles);

    art.bearer = gs::uploadMipped(vdp, bearerArt());
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.cage = gs::uploadMipped(vdp, cageArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.face = gs::uploadMipped(vdp, faceArt());
    art.hourHand = gs::uploadMipped(vdp, handArt(0.f, 10.f, 3));
    for (int i = 0; i < kHands; i++) art.hand[i] = gs::uploadMipped(vdp, handArt(float(i) * (360.f / kHands), 16.f, 4));
}

}  // namespace lanternchime
