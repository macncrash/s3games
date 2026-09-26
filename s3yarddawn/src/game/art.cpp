#include "game/art.h"

namespace yarddawn {
namespace {

uint16_t C(int r, int g, int b) { return gs::rgb4(r, g, b); }

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

// Placed at tile row 6, so bitmap y maps to screen y + 48. Feet sit near y 118.
gs::Bitmap yardArt() {
    gs::Bitmap b(320, 120);
    auto fence = [&]() {
        for (int x = 0; x < 320; x += 14) b.rect(float(x), 74, 2, 16, 7);
        b.rect(0, 80, 320, 2, 8);
    };
    fence();

    auto stack = [&](int x, int y, int w, int courses) {
        for (int i = 0; i < courses; i++) {
            int yy = y + i * 6;
            int c = (i & 1) ? 2 : 1;
            b.rect(float(x), float(yy), float(w), 5, c);
            b.rect(float(x), float(yy), float(w), 1, 3);
            for (int k = 6; k < w - 2; k += 9) b.set(x + k, yy + 3, 3);
        }
    };
    stack(6, 52, 64, 11);
    stack(18, 78, 40, 6);

    b.rect(20, 2, 196, 8, 9);
    b.rect(20, 2, 196, 2, 8);
    b.rect(26, 2, 5, 112, 7);
    b.rect(26, 2, 2, 112, 8);
    b.rect(206, 2, 5, 114, 7);
    b.rect(206, 2, 2, 114, 8);
    b.line(148, 10, 148, 46, 7, 1.2f);
    b.rect(142, 44, 12, 6, 8);
    b.rect(146, 50, 4, 6, 7);
    b.set(214, 4, 4);

    b.rect(92, 58, 108, 58, 11);
    b.rect(92, 58, 108, 7, 12);
    b.rect(92, 58, 4, 58, 7);
    b.rect(196, 58, 4, 58, 7);
    b.rect(100, 70, 24, 16, 14);
    b.rect(132, 70, 24, 16, 14);
    b.rect(104, 74, 16, 8, 10);
    b.rect(136, 74, 16, 8, 10);
    b.rect(164, 72, 28, 32, 7);
    b.rect(168, 76, 20, 24, 14);
    b.line(100, 66, 190, 108, 9, 2.2f);
    b.ellipse(118, 112, 9, 9, 7);
    b.ellipse(176, 112, 9, 9, 7);
    b.ellipse(118, 112, 3, 3, 8);
    b.ellipse(176, 112, 3, 3, 8);

    b.poly({{232, 28}, {316, 28}, {320, 42}, {228, 42}}, 5);
    b.rect(230, 40, 90, 4, 4);
    for (int y = 44; y < 118; y++) {
        int phase = ((y >> 3) & 1) ? 4 : 0;
        for (int x = 234; x < 318; x++) {
            bool mortar = (y & 7) == 0 || ((x + phase) & 7) == 0;
            int shade = 4;
            if (((x * 3 + y) % 19) == 0) shade = 5;
            b.set(x, y, mortar ? 6 : shade);
        }
    }
    b.rect(258, 62, 42, 56, 14);
    b.rect(254, 58, 4, 60, 7);
    b.rect(300, 58, 4, 60, 7);
    b.rect(270, 70, 18, 14, 10);
    b.rect(238, 48, 12, 10, 7);
    b.rect(240, 50, 8, 6, 10);
    b.rect(306, 48, 10, 10, 7);
    b.rect(308, 50, 6, 6, 10);
    b.rect(250, 44, 58, 20, 14);
    gs::TextStyle st{2, 13, 0, 0, 1};
    gs::Bitmap word = gs::textBitmap("YARD", st);
    b.blit(word, 254, 47);

    b.ellipse(214, 100, 8, 8, 5);
    b.ellipse(214, 100, 5, 5, 3);
    b.ellipse(228, 108, 7, 7, 5);
    b.rect(210, 92, 10, 3, 7);

    for (int x = 0; x < 320; x += 22) {
        if (x > 70 && x < 210) continue;
        if (x > 240 && x < 300) continue;
        b.rect(float(x), 108, 3, 12, 7);
    }
    b.rect(0, 112, 78, 2, 8);
    b.rect(200, 112, 36, 2, 8);
    return b;
}

void paintGround(gs::VDP& vdp, gs::TileAlloc& tiles) {
    auto tile = [&](int variant, int kind) {
        uint8_t px[64];
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++) {
                int h = (x * 3 + y * 5 + variant * 7) % 11;
                int c = h == 0 ? 3 : (h < 3 ? 2 : 1);
                if (variant > 2 && ((x + y) % 13) == 0) c = 8;
                if (kind == 1 && (y == 5 || y == 6)) c = 6;
                if (kind == 2 && (x % 8) < 2 && y > 1 && y < 7) c = 7;
                if (kind == 3 && (y == 1 || y == 2)) c = 6;
                if (kind == 4) c = (y < 2) ? 5 : c;
                px[y * 8 + x] = uint8_t(c);
            }
        return tiles.shared(px);
    };
    int gravel[4], lip, railTop, ties, railBot;
    for (int i = 0; i < 4; i++) gravel[i] = tile(i, 0);
    lip = tile(1, 4);
    railTop = tile(2, 1);
    ties = tile(3, 2);
    railBot = tile(4, 3);
    for (int y = 16; y < 28; y++)
        for (int x = 0; x < 40; x++) {
            int t = gravel[(x + y * 3) & 3];
            if (y == 16) t = lip;
            else if (y == 22) t = railTop;
            else if (y == 23) t = ties;
            else if (y == 24) t = railBot;
            vdp.B.set(x, y, gs::entry(t, PAL_GRAVEL));
        }
}

gs::Bitmap manArt(int frame) {
    gs::Bitmap b(36, 48);
    b.rect(11, 1, 14, 4, 4);
    b.rect(8, 4, 20, 3, 4);
    b.rect(8, 6, 22, 2, 8);
    b.rect(12, 7, 11, 8, 3);
    b.set(15, 10, 9);
    b.set(20, 10, 9);
    b.rect(14, 13, 3, 1, 6);
    b.poly({{7, 15}, {27, 15}, {31, 34}, {5, 34}}, 1);
    b.rect(16, 16, 3, 16, 2);
    b.rect(8, 15, 8, 3, 10);
    b.rect(24, 18, 5, 8, 1);
    b.rect(28, 16, 6, 9, 5);
    b.rect(29, 14, 4, 3, 6);
    b.rect(33, 15, 3, 2, 6);
    if (frame == 0) {
        b.rect(10, 34, 5, 10, 7);
        b.rect(20, 34, 5, 10, 7);
        b.rect(9, 43, 7, 3, 8);
        b.rect(19, 43, 7, 3, 8);
    } else {
        b.rect(12, 34, 5, 10, 7);
        b.rect(19, 34, 5, 9, 7);
        b.rect(11, 43, 7, 3, 8);
        b.rect(18, 42, 7, 3, 8);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap potArt() {
    gs::Bitmap b(26, 18);
    b.rect(3, 13, 3, 4, 2);
    b.rect(12, 13, 3, 4, 2);
    b.rect(20, 13, 3, 4, 2);
    b.poly({{2, 5}, {23, 5}, {20, 13}, {5, 13}}, 1);
    b.rect(1, 3, 24, 3, 3);
    b.rect(4, 6, 18, 3, 4);
    b.set(7, 4, 5);
    b.set(13, 4, 5);
    b.set(18, 4, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap flameArt(int frame) {
    gs::Bitmap b(16, 26);
    if (frame == 0) {
        b.poly({{8, 1}, {15, 16}, {1, 16}}, 2);
        b.ellipse(8, 18, 6, 5, 1);
        b.poly({{8, 8}, {12, 22}, {4, 22}}, 3);
        b.poly({{8, 13}, {10, 23}, {6, 23}}, 4);
    } else {
        b.poly({{7, 2}, {14, 15}, {2, 18}}, 2);
        b.ellipse(8, 19, 5, 4, 1);
        b.poly({{8, 9}, {12, 22}, {3, 20}}, 3);
        b.ellipse(8, 16, 2, 4, 4);
    }
    return b;
}

gs::Bitmap canvasArt() {
    gs::Bitmap b(32, 18);
    b.poly({{2, 6}, {28, 2}, {30, 12}, {4, 16}}, 1);
    b.line(5, 8, 26, 5, 4, 1.f);
    b.line(6, 12, 28, 9, 2, 1.f);
    b.rect(1, 5, 3, 3, 3);
    b.rect(26, 2, 3, 3, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap drumArt(int frame) {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 1);
    b.ellipse(11, 11, 7, 7, 2);
    int y = frame ? 6 : 13;
    b.rect(2, float(y), 18, 3, 3);
    b.ellipse(11, 11, 2, 2, 5);
    b.rect(5, 4, 4, 2, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap smokeArt(int frame) {
    gs::Bitmap b(16, 16);
    if (frame == 0) {
        b.ellipse(8, 9, 6, 5, 1);
        b.ellipse(6, 6, 3, 3, 2);
    } else {
        b.ellipse(8, 8, 5, 4, 2);
        b.ellipse(10, 11, 3, 2, 1);
    }
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(4, 4);
    b.set(1, 0, 1);
    b.set(0, 1, 1);
    b.set(1, 1, 2);
    b.set(2, 1, 1);
    b.set(1, 2, 1);
    return b;
}

gs::Bitmap shadeArt() {
    gs::Bitmap b(20, 6);
    b.ellipse(10, 3, 9, 2, 1);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 1);
    b.ellipse(12, 7, 2, 2, 2);
    b.ellipse(7, 11, 1, 1, 2);
    b.set(6, 6, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 8, 8, 2);
    b.ellipse(14, 14, 5, 5, 1);
    b.line(14, 1, 14, 5, 3, 1.2f);
    b.line(14, 23, 14, 27, 3, 1.2f);
    b.line(1, 14, 5, 14, 3, 1.2f);
    b.line(23, 14, 27, 14, 3, 1.2f);
    b.line(4, 4, 7, 7, 4, 1.f);
    b.line(21, 21, 24, 24, 4, 1.f);
    b.line(24, 4, 21, 7, 4, 1.f);
    b.line(7, 21, 4, 24, 4, 1.f);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 1, 1);
    b.set(0, 2, 1);
    b.set(1, 2, 1);
    b.set(2, 2, 2);
    b.set(3, 2, 1);
    b.set(4, 2, 1);
    b.set(2, 3, 1);
    b.set(2, 4, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 16);
    b.rect(5, 0, 2, 4, 1);
    b.ellipse(6, 10, 5, 5, 2);
    b.ellipse(6, 10, 2, 2, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    uint16_t hud[16] = {};
    hud[1] = C(14, 13, 11);
    hud[15] = C(1, 1, 2);
    setPal(vdp, PAL_HUD, hud);

    uint16_t yard[16] = {};
    yard[1] = C(11, 7, 3);
    yard[2] = C(8, 5, 2);
    yard[3] = C(5, 3, 1);
    yard[4] = C(10, 4, 3);
    yard[5] = C(6, 2, 2);
    yard[6] = C(13, 11, 9);
    yard[7] = C(5, 6, 7);
    yard[8] = C(11, 12, 13);
    yard[9] = C(13, 10, 2);
    yard[10] = C(15, 13, 6);
    yard[11] = C(3, 5, 4);
    yard[12] = C(6, 8, 6);
    yard[13] = C(15, 14, 8);
    yard[14] = C(1, 1, 2);
    yard[15] = C(12, 12, 9);
    setPal(vdp, PAL_YARD, yard);

    uint16_t gravel[16] = {};
    gravel[1] = C(5, 5, 4);
    gravel[2] = C(3, 3, 3);
    gravel[3] = C(7, 7, 6);
    gravel[4] = C(8, 6, 3);
    gravel[5] = C(4, 4, 3);
    gravel[6] = C(9, 9, 10);
    gravel[7] = C(4, 3, 2);
    gravel[8] = C(2, 2, 2);
    setPal(vdp, PAL_GRAVEL, gravel);

    uint16_t man[16] = {};
    man[1] = C(2, 3, 7);
    man[2] = C(4, 6, 11);
    man[3] = C(13, 9, 6);
    man[4] = C(1, 1, 2);
    man[5] = C(12, 10, 4);
    man[6] = C(8, 6, 2);
    man[7] = C(2, 2, 3);
    man[8] = C(1, 1, 1);
    man[9] = C(1, 1, 2);
    man[10] = C(12, 12, 10);
    man[15] = C(0, 0, 1);
    setPal(vdp, PAL_MAN, man);

    uint16_t fire[16] = {};
    fire[1] = C(12, 2, 1);
    fire[2] = C(15, 8, 1);
    fire[3] = C(15, 13, 2);
    fire[4] = C(15, 15, 11);
    setPal(vdp, PAL_FIRE, fire);

    uint16_t ember[16] = {};
    ember[1] = C(8, 1, 1);
    ember[2] = C(12, 4, 1);
    ember[3] = C(14, 8, 2);
    ember[4] = C(15, 12, 6);
    setPal(vdp, PAL_EMBER, ember);

    uint16_t iron[16] = {};
    iron[1] = C(5, 5, 6);
    iron[2] = C(2, 2, 3);
    iron[3] = C(9, 9, 10);
    iron[4] = C(12, 5, 2);
    iron[5] = C(12, 11, 8);
    iron[15] = C(1, 1, 2);
    setPal(vdp, PAL_IRON, iron);

    uint16_t canvas[16] = {};
    canvas[1] = C(12, 12, 9);
    canvas[2] = C(8, 8, 6);
    canvas[3] = C(4, 4, 4);
    canvas[4] = C(14, 13, 8);
    canvas[15] = C(2, 2, 2);
    setPal(vdp, PAL_CANVAS, canvas);

    uint16_t drum[16] = {};
    drum[1] = C(10, 5, 2);
    drum[2] = C(6, 3, 1);
    drum[3] = C(4, 4, 5);
    drum[4] = C(13, 8, 4);
    drum[5] = C(1, 1, 1);
    drum[15] = C(2, 1, 1);
    setPal(vdp, PAL_DRUM, drum);

    uint16_t smoke[16] = {};
    smoke[1] = C(5, 5, 6);
    smoke[2] = C(8, 8, 9);
    setPal(vdp, PAL_SMOKE, smoke);

    uint16_t moon[16] = {};
    moon[1] = C(14, 14, 12);
    moon[2] = C(10, 10, 9);
    setPal(vdp, PAL_MOON, moon);

    uint16_t sun[16] = {};
    sun[1] = C(15, 15, 12);
    sun[2] = C(15, 12, 4);
    sun[3] = C(15, 14, 8);
    sun[4] = C(15, 10, 3);
    setPal(vdp, PAL_SUN, sun);

    uint16_t gold[16] = {};
    gold[1] = C(15, 13, 4);
    gold[15] = C(4, 2, 0);
    setPal(vdp, PAL_GOLD, gold);

    uint16_t alert[16] = {};
    alert[1] = C(15, 6, 4);
    alert[15] = C(4, 0, 0);
    setPal(vdp, PAL_ALERT, alert);

    uint16_t pip[16] = {};
    pip[1] = C(4, 4, 5);
    setPal(vdp, PAL_PIP, pip);

    uint16_t lamp[16] = {};
    lamp[1] = C(3, 3, 4);
    lamp[2] = C(14, 11, 3);
    lamp[3] = C(15, 15, 10);
    setPal(vdp, PAL_LAMP, lamp);

    vdp.setFogColor(C(1, 1, 3));
    vdp.A.enabled = true;
    vdp.B.enabled = true;

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    uint8_t solid[64];
    for (int i = 0; i < 64; i++) solid[i] = 15;
    art.bar = tiles.shared(solid);
    paintGround(vdp, tiles);
    gs::bitmapToPlane(tiles, vdp.A, 0, 6, yardArt(), PAL_YARD);

    art.man[0] = gs::uploadMipped(vdp, manArt(0));
    art.man[1] = gs::uploadMipped(vdp, manArt(1));
    art.pot = gs::uploadMipped(vdp, potArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.canvas = gs::uploadMipped(vdp, canvasArt());
    art.drum[0] = gs::uploadMipped(vdp, drumArt(0));
    art.drum[1] = gs::uploadMipped(vdp, drumArt(1));
    art.smoke[0] = gs::uploadMipped(vdp, smokeArt(0));
    art.smoke[1] = gs::uploadMipped(vdp, smokeArt(1));
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
}

}  // namespace yarddawn
