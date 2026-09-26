#include "game/art.h"

#include <algorithm>
#include <cmath>

namespace gatedawn {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* c) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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
        int t = tiles.shared(px);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

void paintRoad(gs::VDP& vdp, gs::TileAlloc& tiles) {
    uint8_t cob[64], lip[64], step[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            bool mortar = y == 0 || x == 0 || ((x + y) % 7 == 0);
            int c = mortar ? 2 : ((x * 3 + y) % 5 == 0 ? 3 : 1);
            cob[y * 8 + x] = uint8_t(c);
            lip[y * 8 + x] = uint8_t(y < 2 ? 5 : c);
            bool edge = y == 0 || x == 0;
            step[y * 8 + x] = uint8_t(edge ? 7 : (y < 3 ? 6 : 1));
        }
    int tc = tiles.shared(cob);
    int tl = tiles.shared(lip);
    int ts = tiles.shared(step);
    for (int y = 22; y < 28; y++)
        for (int x = 0; x < 40; x++) vdp.B.set(x, y, gs::entry(y == 22 ? tl : tc, PAL_ROAD));
    for (int y = 22; y <= 23; y++)
        for (int x = 16; x <= 23; x++) vdp.B.set(x, y, gs::entry(ts, PAL_ROAD));
    for (int x = 0; x < 40; x++) vdp.B.set(x, 27, gs::entry(tc, PAL_ROAD));
}

gs::Bitmap gateArt() {
    gs::Bitmap b(320, 128);
    auto brick = [&](int x0, int y0, int w, int h) {
        for (int y = y0; y < y0 + h && y < b.h; y++) {
            int off = ((y / 5) & 1) ? 5 : 0;
            for (int x = x0; x < x0 + w && x < b.w; x++) {
                bool mortar = (y % 5 == 0) || ((x + off) % 10 == 0);
                int c = 2;
                if (mortar) c = 1;
                else if (((x * 5 + y * 3) % 17) == 0) c = 4;
                else if (((x + y) % 13) == 0) c = 3;
                else if (((x * 7 + y) % 29) == 0) c = 5;
                b.set(x, y, c);
            }
        }
    };
    brick(0, 0, 52, 128);
    brick(268, 0, 52, 128);
    brick(40, 36, 240, 92);
    b.rect(48, 0, 4, 128, 9);
    b.rect(268, 0, 4, 128, 9);
    b.rect(0, 0, 52, 5, 8);
    b.rect(268, 0, 52, 5, 8);
    for (int i = 0; i < 8; i++) {
        int x = 64 + i * 24;
        for (int y = 36; y < 50; y++)
            for (int k = 0; k < 10; k++) b.set(x + k, y, 0);
    }
    b.rect(52, 50, 216, 3, 8);
    b.rect(0, 118, 140, 10, 9);
    b.rect(180, 118, 140, 10, 9);
    b.ellipse(160, 96, 26, 20, 6);
    b.rect(142, 96, 36, 32, 6);
    for (int x = 148; x < 176; x += 8) b.line(float(x), 100, float(x), 127, 10, 1.2f);
    b.rect(144, 112, 32, 3, 13);
    for (int y = 104; y < 124; y += 10)
        for (int x = 150; x < 174; x += 10) b.ellipse(float(x), float(y), 1.2f, 1.2f, 13);
    auto window = [&](int x, int y) {
        b.rect(float(x), float(y), 8, 12, 6);
        b.rect(float(x + 2), float(y + 2), 4, 6, 7);
        b.set(x + 3, y + 4, 11);
    };
    window(16, 18);
    window(16, 46);
    window(16, 76);
    window(292, 18);
    window(292, 46);
    window(292, 76);
    b.poly({{160, 46}, {172, 58}, {160, 70}, {148, 58}}, 11);
    b.poly({{160, 52}, {166, 58}, {160, 64}, {154, 58}}, 12);
    b.line(50, 8, 50, 28, 13, 1.3f);
    b.poly({{50, 8}, {82, 16}, {50, 24}}, 12);
    return b;
}

gs::Bitmap manArt(int frame) {
    gs::Bitmap b(34, 46);
    b.rect(8, 2, 14, 3, 5);
    b.rect(6, 5, 18, 2, 6);
    b.rect(11, 7, 8, 7, 4);
    b.set(13, 9, 12);
    b.set(17, 9, 12);
    b.set(15, 12, 13);
    b.poly({{7, 14}, {24, 14}, {26, 33}, {5, 33}}, 1);
    b.rect(9, 15, 5, 14, 2);
    b.rect(15, 18, 2, 10, 8);
    b.set(16, 20, 9);
    b.set(16, 24, 9);
    b.set(16, 28, 9);
    b.rect(7, 16, 5, 3, 14);
    b.line(20, 20, 29, 12, 7, 2.f);
    b.rect(27, 10, 3, 6, 7);
    b.rect(22, 18, 4, 3, 11);
    if (frame == 0) {
        b.rect(8, 33, 5, 9, 3);
        b.rect(18, 33, 5, 9, 3);
        b.rect(7, 41, 7, 3, 10);
        b.rect(17, 41, 7, 3, 10);
    } else {
        b.rect(10, 33, 5, 9, 3);
        b.rect(16, 33, 5, 8, 3);
        b.rect(9, 41, 7, 3, 10);
        b.rect(15, 40, 7, 3, 10);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap sneakArt(int frame) {
    gs::Bitmap b(20, 32);
    b.poly({{10, 1}, {16, 8}, {4, 8}}, 1);
    b.rect(5, 6, 10, 6, 1);
    b.set(8, 8, 4);
    b.set(12, 8, 4);
    b.rect(5, 12, 10, 12, 2);
    b.rect(6, 13, 3, 8, 3);
    int ay = frame ? 3 : 6;
    b.line(13, 14, 17, float(ay + 2), 8, 1.4f);
    b.rect(14, float(ay), 5, 4, 5);
    b.rect(15, float(ay + 1), 3, 2, 6);
    b.rect(6, 24, 3, 6, 7);
    b.rect(11, 24, 3, 6, 7);
    b.outline(15, false);
    return b;
}

gs::Bitmap flameArt(int frame) {
    gs::Bitmap b(16, 24);
    if (frame == 0) {
        b.poly({{8, 1}, {14, 16}, {2, 16}}, 2);
        b.ellipse(8, 17, 5, 4, 2);
        b.poly({{8, 7}, {12, 20}, {4, 20}}, 3);
        b.poly({{8, 11}, {10, 21}, {6, 21}}, 4);
        b.ellipse(8, 15, 2, 3, 5);
    } else {
        b.poly({{9, 2}, {14, 17}, {3, 15}}, 2);
        b.ellipse(8, 18, 4, 3, 3);
        b.poly({{8, 8}, {12, 21}, {4, 19}}, 4);
        b.ellipse(8, 14, 2, 3, 5);
    }
    return b;
}

gs::Bitmap basketArt() {
    gs::Bitmap b(20, 14);
    b.poly({{2, 3}, {17, 3}, {15, 12}, {4, 12}}, 1);
    b.rect(3, 4, 14, 3, 5);
    b.rect(2, 2, 16, 2, 3);
    b.line(5, 5, 5, 11, 2, 1.f);
    b.line(10, 5, 10, 11, 2, 1.f);
    b.line(14, 5, 14, 11, 2, 1.f);
    b.set(6, 6, 4);
    b.set(12, 6, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(8, 48);
    b.rect(3, 0, 3, 48, 1);
    b.rect(3, 0, 1, 48, 3);
    b.rect(2, 0, 4, 3, 6);
    return b;
}

gs::Bitmap gustArt() {
    gs::Bitmap b(36, 16);
    b.ellipse(10, 9, 8, 4, 2);
    b.ellipse(22, 7, 10, 5, 1);
    b.ellipse(28, 10, 6, 3, 4);
    b.line(2, 11, 16, 11, 3, 1.f);
    b.line(8, 4, 22, 4, 3, 1.f);
    b.line(18, 12, 32, 11, 3, 1.f);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(12, 14, 10, 10, 1);
    b.ellipse(18, 12, 8, 8, 0);
    b.ellipse(9, 13, 2.2f, 1.4f, 2);
    b.ellipse(11, 18, 1.6f, 1.2f, 2);
    b.set(8, 10, 3);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(32, 32);
    b.line(16, 1, 16, 8, 4, 1.3f);
    b.line(16, 24, 16, 31, 4, 1.3f);
    b.line(1, 16, 8, 16, 4, 1.3f);
    b.line(24, 16, 31, 16, 4, 1.3f);
    b.line(5, 5, 10, 10, 4, 1.2f);
    b.line(22, 22, 27, 27, 4, 1.2f);
    b.line(27, 5, 22, 10, 4, 1.2f);
    b.line(10, 22, 5, 27, 4, 1.2f);
    b.ellipse(16, 16, 11, 11, 3);
    b.ellipse(16, 16, 7, 7, 2);
    b.ellipse(16, 16, 3, 3, 1);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(5, 5);
    for (int i = 0; i < 5; i++) b.set(2, i, 1);
    for (int i = 0; i < 5; i++) b.set(i, 2, 1);
    b.set(2, 2, 3);
    return b;
}

gs::Bitmap smokeArt(int frame) {
    gs::Bitmap b(16, 16);
    float ox = frame ? 2.f : 0.f;
    b.ellipse(8 + ox, 10, 5, 4, 1);
    b.ellipse(6, 6, 3, 3, 2);
    b.ellipse(11, 7, 2.4f, 2.2f, 3);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(5, 5);
    b.rect(1, 0, 3, 5, 1);
    b.rect(0, 1, 5, 3, 1);
    return b;
}

gs::Bitmap shadeArt() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 9, 3, 1);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(3, 3);
    b.set(1, 0, 5);
    b.set(0, 1, 4);
    b.set(1, 1, 5);
    b.set(2, 1, 3);
    b.set(1, 2, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    auto C = gs::rgb4;
    uint16_t hud[16] = {};
    hud[1] = C(15, 14, 12);
    hud[15] = C(2, 1, 3);
    setPal(vdp, PAL_HUD, hud);

    uint16_t stone[16] = {};
    stone[1] = C(3, 3, 4);
    stone[2] = C(6, 6, 7);
    stone[3] = C(9, 9, 10);
    stone[4] = C(4, 4, 5);
    stone[5] = C(3, 6, 4);
    stone[6] = C(1, 1, 2);
    stone[7] = C(12, 7, 3);
    stone[8] = C(10, 10, 11);
    stone[9] = C(3, 3, 5);
    stone[10] = C(5, 4, 3);
    stone[11] = C(12, 10, 4);
    stone[12] = C(8, 3, 2);
    stone[13] = C(8, 8, 9);
    stone[14] = C(2, 5, 3);
    stone[15] = C(1, 1, 2);
    setPal(vdp, PAL_STONE, stone);

    uint16_t road[16] = {};
    road[1] = C(5, 5, 6);
    road[2] = C(2, 2, 3);
    road[3] = C(3, 3, 4);
    road[4] = C(2, 2, 3);
    road[5] = C(8, 8, 9);
    road[6] = C(7, 7, 8);
    road[7] = C(4, 4, 5);
    setPal(vdp, PAL_ROAD, road);

    uint16_t man[16] = {};
    man[1] = C(3, 4, 7);
    man[2] = C(5, 7, 11);
    man[3] = C(2, 2, 4);
    man[4] = C(13, 8, 6);
    man[5] = C(2, 2, 3);
    man[6] = C(5, 4, 4);
    man[7] = C(8, 5, 2);
    man[8] = C(10, 8, 3);
    man[9] = C(14, 12, 6);
    man[10] = C(2, 2, 2);
    man[11] = C(6, 4, 3);
    man[12] = C(1, 1, 2);
    man[13] = C(12, 6, 5);
    man[14] = C(6, 2, 2);
    man[15] = C(1, 1, 2);
    setPal(vdp, PAL_MAN, man);

    uint16_t fire[16] = {};
    fire[1] = C(10, 1, 0);
    fire[2] = C(14, 4, 1);
    fire[3] = C(15, 8, 1);
    fire[4] = C(15, 12, 3);
    fire[5] = C(15, 15, 12);
    fire[6] = C(12, 3, 1);
    setPal(vdp, PAL_FIRE, fire);

    uint16_t ember[16] = {};
    ember[1] = C(6, 1, 1);
    ember[2] = C(10, 2, 1);
    ember[3] = C(12, 4, 1);
    ember[4] = C(14, 7, 2);
    ember[5] = C(12, 8, 4);
    ember[6] = C(6, 2, 1);
    setPal(vdp, PAL_EMBER, ember);

    uint16_t iron[16] = {};
    iron[1] = C(5, 5, 6);
    iron[2] = C(2, 2, 3);
    iron[3] = C(9, 9, 10);
    iron[4] = C(3, 2, 2);
    iron[5] = C(12, 5, 1);
    iron[6] = C(10, 8, 4);
    iron[15] = C(1, 1, 2);
    setPal(vdp, PAL_IRON, iron);

    uint16_t sneak[16] = {};
    sneak[1] = C(2, 3, 3);
    sneak[2] = C(3, 5, 4);
    sneak[3] = C(1, 2, 2);
    sneak[4] = C(14, 15, 8);
    sneak[5] = C(5, 6, 7);
    sneak[6] = C(6, 9, 10);
    sneak[7] = C(8, 6, 5);
    sneak[8] = C(6, 5, 3);
    sneak[15] = C(1, 1, 1);
    setPal(vdp, PAL_SNEAK, sneak);

    uint16_t wind[16] = {};
    wind[1] = C(8, 9, 12);
    wind[2] = C(4, 5, 8);
    wind[3] = C(13, 14, 15);
    wind[4] = C(6, 7, 10);
    setPal(vdp, PAL_WIND, wind);

    uint16_t moon[16] = {};
    moon[1] = C(14, 14, 12);
    moon[2] = C(10, 10, 8);
    moon[3] = C(15, 15, 14);
    setPal(vdp, PAL_MOON, moon);

    uint16_t sun[16] = {};
    sun[1] = C(15, 15, 12);
    sun[2] = C(15, 12, 4);
    sun[3] = C(15, 8, 2);
    sun[4] = C(15, 14, 8);
    setPal(vdp, PAL_SUN, sun);

    uint16_t smoke[16] = {};
    smoke[1] = C(6, 6, 7);
    smoke[2] = C(4, 4, 5);
    smoke[3] = C(8, 8, 9);
    setPal(vdp, PAL_SMOKE, smoke);

    uint16_t alert[16] = {};
    alert[1] = C(15, 6, 5);
    alert[15] = C(4, 0, 0);
    setPal(vdp, PAL_ALERT, alert);

    uint16_t gold[16] = {};
    gold[1] = C(15, 13, 6);
    gold[15] = C(4, 2, 0);
    setPal(vdp, PAL_GOLD, gold);

    uint16_t pip[16] = {};
    pip[1] = C(7, 8, 11);
    setPal(vdp, PAL_PIP, pip);

    vdp.setFogColor(C(2, 2, 5));
    vdp.A.enabled = true;
    vdp.B.enabled = true;

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    paintRoad(vdp, tiles);
    gs::bitmapToPlane(tiles, vdp.A, 0, 6, gateArt(), PAL_STONE);

    art.man[0] = gs::uploadMipped(vdp, manArt(0));
    art.man[1] = gs::uploadMipped(vdp, manArt(1));
    art.sneak[0] = gs::uploadMipped(vdp, sneakArt(0));
    art.sneak[1] = gs::uploadMipped(vdp, sneakArt(1));
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.basket = gs::uploadMipped(vdp, basketArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.gust = gs::uploadMipped(vdp, gustArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.smoke[0] = gs::uploadMipped(vdp, smokeArt(0));
    art.smoke[1] = gs::uploadMipped(vdp, smokeArt(1));
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
}

}  // namespace gatedawn
