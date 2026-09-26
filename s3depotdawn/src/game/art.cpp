#include "game/art.h"

#include <algorithm>

namespace depotdawn {
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

// Shed occupies screen y 40..136. The open bay is x 136..184.
gs::Bitmap shedArt() {
    gs::Bitmap b(320, 96);
    for (int y = 0; y < 12; y++)
        for (int x = 0; x < 320; x++) {
            bool merlon = y < 7 && ((x / 12) % 2 == 0);
            if (y >= 7 || merlon) b.set(x, y, y < 7 ? 9 : 8);
        }
    for (int y = 12; y < 96; y++) {
        int tileY = y >> 3;
        int phase = (tileY & 1) ? 4 : 0;
        for (int x = 0; x < 320; x++) {
            int tileX = x >> 3;
            bool mortar = (y & 7) == 0 || ((x + phase) & 7) == 0;
            int shade = 1;
            if (((tileX * 3 + tileY * 5) % 11) == 0) shade = 2;
            else if (((tileX + tileY * 2) % 9) == 0) shade = 3;
            b.set(x, y, mortar ? 4 : shade);
        }
    }
    b.rect(0, 32, 320, 8, 7);
    b.rect(0, 32, 320, 2, 9);
    auto closedBay = [&](int x) {
        b.rect(float(x), 42, 48, 54, 5);
        b.rect(float(x + 5), 46, 38, 46, 15);
        b.rect(float(x + 22), 46, 2, 46, 4);
        b.rect(float(x + 8), 62, 12, 16, 6);
        b.rect(float(x + 26), 62, 12, 16, 6);
    };
    closedBay(24);
    closedBay(248);
    b.rect(136, 42, 48, 54, 0);
    b.rect(132, 40, 4, 56, 7);
    b.rect(184, 40, 4, 56, 7);
    auto window = [&](int x, int y) {
        b.rect(float(x), float(y), 16, 12, 7);
        b.rect(float(x + 2), float(y + 2), 12, 8, 6);
        b.rect(float(x + 7), float(y + 2), 2, 8, 7);
        b.set(x + 4, y + 5, 13);
    };
    window(36, 16);
    window(92, 16);
    window(214, 16);
    window(274, 16);
    b.rect(116, 14, 88, 16, 11);
    b.rect(116, 14, 88, 2, 10);
    gs::TextStyle st{2, 10, 0, 0, 1};
    gs::Bitmap word = gs::textBitmap("DEPOT", st);
    b.blit(word, 160 - word.w / 2, 16);
    b.ellipse(302, 22, 7, 7, 9);
    b.ellipse(302, 22, 5, 5, 5);
    b.line(302, 22, 302, 18, 10, 1.1f);
    b.line(302, 22, 305, 24, 10, 1.1f);
    b.rect(16, 0, 14, 26, 2);
    b.rect(14, 0, 18, 4, 9);
    b.rect(20, 6, 4, 5, 5);
    b.rect(0, 90, 320, 6, 14);
    b.rect(136, 88, 48, 8, 0);
    return b;
}

void paintDock(gs::VDP& vdp, gs::TileAlloc& tiles) {
    uint8_t lip[64], plank[64], rail[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            bool gap = (y & 3) == 3;
            bool nail = (x == 2 || x == 6) && (y == 1 || y == 5);
            int c = gap ? 3 : (nail ? 5 : ((x + y) & 1 ? 1 : 2));
            plank[y * 8 + x] = uint8_t(c);
            lip[y * 8 + x] = uint8_t(y < 2 ? 4 : c);
            int r = 1;
            if (y == 2 || y == 5) r = 3;
            else if (y == 3 || y == 6) r = 2;
            if (x == 1 || x == 2) r = 5;
            rail[y * 8 + x] = uint8_t(r);
        }
    int tl = tiles.shared(lip);
    int tp = tiles.shared(plank);
    int tr = tiles.shared(rail);
    for (int x = 0; x < 40; x++) {
        vdp.B.set(x, 22, gs::entry(tr, PAL_IRON));
        vdp.B.set(x, 23, gs::entry(tl, PAL_DOCK));
        for (int y = 24; y < 28; y++) vdp.B.set(x, y, gs::entry(tp, PAL_DOCK));
    }
}

gs::Bitmap manArt(int frame) {
    gs::Bitmap b(32, 48);
    b.rect(8, 2, 14, 3, 5);
    b.rect(6, 5, 18, 3, 5);
    b.rect(9, 4, 12, 2, 9);
    b.rect(10, 8, 10, 7, 4);
    b.set(13, 10, 11);
    b.set(17, 10, 11);
    b.rect(14, 13, 3, 1, 13);
    b.poly({{6, 15}, {24, 15}, {27, 34}, {4, 34}}, 1);
    b.rect(14, 16, 2, 14, 2);
    b.rect(8, 16, 5, 3, 9);
    b.line(22, 20, 29, 14, 4, 2.2f);
    b.rect(26, 11, 5, 6, 6);
    b.rect(27, 12, 3, 3, 7);
    b.line(8, 22, 2, 28, 4, 2.f);
    b.rect(0, 26, 5, 6, 8);
    b.rect(1, 25, 3, 2, 3);
    if (frame == 0) {
        b.rect(8, 34, 5, 9, 3);
        b.rect(17, 34, 5, 9, 3);
        b.rect(7, 42, 7, 4, 10);
        b.rect(16, 42, 7, 4, 10);
    } else {
        b.rect(10, 34, 5, 9, 3);
        b.rect(15, 34, 5, 8, 3);
        b.rect(9, 42, 7, 4, 10);
        b.rect(14, 41, 7, 4, 10);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap potArt() {
    gs::Bitmap b(18, 16);
    b.rect(3, 1, 12, 3, 3);
    b.poly({{4, 4}, {14, 4}, {16, 14}, {2, 14}}, 1);
    b.rect(4, 6, 2, 7, 2);
    b.rect(6, 7, 6, 3, 8);
    b.set(8, 8, 9);
    b.set(10, 8, 9);
    b.line(5, 11, 13, 11, 4, 1.f);
    b.outline(15, false);
    return b;
}

gs::Bitmap flameArt(int frame) {
    gs::Bitmap b(14, 22);
    if (frame == 0) {
        b.poly({{7, 1}, {13, 14}, {1, 14}}, 2);
        b.ellipse(7, 16, 5, 4, 3);
        b.poly({{7, 6}, {11, 18}, {3, 18}}, 4);
        b.poly({{7, 10}, {9, 19}, {5, 19}}, 5);
    } else {
        b.poly({{8, 2}, {13, 15}, {2, 13}}, 2);
        b.ellipse(7, 16, 4, 4, 3);
        b.poly({{7, 7}, {11, 18}, {3, 16}}, 4);
        b.ellipse(7, 12, 2, 4, 5);
    }
    return b;
}

gs::Bitmap hoodArt() {
    gs::Bitmap b(22, 10);
    b.poly({{1, 8}, {11, 1}, {21, 8}}, 1);
    b.rect(2, 7, 18, 2, 3);
    b.line(5, 8, 5, 4, 6, 1.f);
    b.line(16, 8, 16, 4, 6, 1.f);
    b.outline(15, false);
    return b;
}

gs::Bitmap locoArt(int frame) {
    gs::Bitmap b(88, 40);
    int wy = frame ? 30 : 31;
    b.ellipse(18, float(wy), 7, 7, 4);
    b.ellipse(40, float(wy), 7, 7, 4);
    b.ellipse(62, float(wy), 6, 6, 4);
    b.ellipse(18, float(wy), 3, 3, 5);
    b.ellipse(40, float(wy), 3, 3, 5);
    b.ellipse(62, float(wy), 2, 2, 5);
    if (frame == 0) b.line(18, 30, 62, 26, 5, 2.f);
    else b.line(18, 26, 62, 32, 5, 2.f);
    b.rect(10, 14, 52, 14, 1);
    b.rect(10, 14, 52, 3, 2);
    b.rect(52, 6, 22, 22, 3);
    b.rect(56, 9, 10, 8, 8);
    b.rect(58, 11, 6, 4, 9);
    b.rect(16, 4, 8, 10, 1);
    b.rect(14, 2, 12, 4, 3);
    b.ellipse(36, 12, 5, 4, 2);
    b.rect(74, 14, 8, 8, 6);
    b.rect(80, 16, 6, 4, 6);
    b.rect(76, 16, 4, 4, 9);
    b.rect(78, 24, 8, 6, 7);
    b.rect(8, 26, 6, 5, 7);
    b.rect(12, 26, 64, 3, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap wagonArt() {
    gs::Bitmap b(52, 32);
    b.rect(4, 4, 44, 16, 10);
    b.rect(8, 7, 14, 10, 11);
    b.rect(28, 7, 14, 10, 3);
    b.rect(2, 18, 48, 4, 1);
    b.ellipse(14, 25, 5, 5, 4);
    b.ellipse(38, 25, 5, 5, 4);
    b.rect(0, 12, 4, 6, 7);
    b.rect(48, 12, 4, 6, 7);
    b.outline(15, false);
    return b;
}

gs::Bitmap bufferArt() {
    gs::Bitmap b(20, 24);
    b.rect(2, 6, 3, 16, 5);
    b.rect(15, 6, 3, 16, 5);
    b.rect(1, 8, 18, 5, 4);
    b.rect(1, 9, 18, 2, 3);
    b.rect(8, 13, 4, 8, 1);
    b.outline(15, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 18);
    b.line(5, 0, 5, 5, 7, 1.2f);
    b.poly({{2, 6}, {8, 6}, {9, 12}, {1, 12}}, 12);
    b.rect(3, 7, 4, 4, 13);
    b.rect(2, 12, 6, 2, 7);
    return b;
}

gs::Bitmap rainArt() {
    gs::Bitmap b(6, 14);
    b.line(1, 0, 2, 13, 1, 1.f);
    b.line(4, 2, 5, 12, 2, 1.f);
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

gs::Bitmap sparkArt() {
    gs::Bitmap b(3, 3);
    b.set(1, 0, 1);
    b.set(0, 1, 1);
    b.set(1, 1, 1);
    b.set(2, 1, 1);
    b.set(1, 2, 1);
    return b;
}

gs::Bitmap shadeArt() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 9, 3, 1);
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    auto C = gs::rgb4;
    uint16_t hud[16] = {};
    hud[1] = C(15, 14, 12);
    hud[15] = C(2, 1, 2);
    setPal(vdp, PAL_HUD, hud);

    uint16_t shed[16] = {};
    shed[1] = C(7, 3, 2);
    shed[2] = C(5, 2, 2);
    shed[3] = C(9, 4, 3);
    shed[4] = C(3, 2, 2);
    shed[5] = C(1, 1, 2);
    shed[6] = C(12, 8, 3);
    shed[7] = C(4, 3, 2);
    shed[8] = C(2, 2, 3);
    shed[9] = C(6, 6, 7);
    shed[10] = C(14, 12, 5);
    shed[11] = C(6, 1, 2);
    shed[12] = C(8, 6, 2);
    shed[13] = C(15, 13, 6);
    shed[14] = C(5, 5, 5);
    shed[15] = C(1, 1, 1);
    setPal(vdp, PAL_SHED, shed);

    uint16_t dock[16] = {};
    dock[1] = C(6, 4, 2);
    dock[2] = C(4, 3, 2);
    dock[3] = C(2, 2, 2);
    dock[4] = C(12, 10, 3);
    dock[5] = C(8, 8, 7);
    setPal(vdp, PAL_DOCK, dock);

    uint16_t man[16] = {};
    man[1] = C(2, 3, 6);
    man[2] = C(4, 5, 9);
    man[3] = C(2, 2, 3);
    man[4] = C(12, 8, 6);
    man[5] = C(1, 1, 2);
    man[6] = C(10, 8, 3);
    man[7] = C(15, 13, 5);
    man[8] = C(5, 4, 2);
    man[9] = C(12, 10, 3);
    man[10] = C(1, 1, 1);
    man[11] = C(1, 1, 2);
    man[13] = C(8, 4, 3);
    man[15] = C(1, 1, 2);
    setPal(vdp, PAL_MAN, man);

    uint16_t fire[16] = {};
    fire[1] = C(12, 4, 1);
    fire[2] = C(12, 2, 0);
    fire[3] = C(14, 6, 1);
    fire[4] = C(15, 11, 2);
    fire[5] = C(15, 15, 10);
    fire[6] = C(8, 2, 1);
    fire[15] = C(3, 1, 0);
    setPal(vdp, PAL_FIRE, fire);

    uint16_t ember[16] = {};
    ember[1] = C(8, 2, 1);
    ember[2] = C(8, 1, 0);
    ember[3] = C(10, 3, 1);
    ember[4] = C(12, 6, 1);
    ember[5] = C(14, 9, 3);
    ember[6] = C(5, 1, 1);
    ember[15] = C(2, 0, 0);
    setPal(vdp, PAL_EMBER, ember);

    uint16_t iron[16] = {};
    iron[1] = C(4, 4, 5);
    iron[2] = C(7, 7, 8);
    iron[3] = C(10, 11, 12);
    iron[4] = C(11, 3, 2);
    iron[5] = C(5, 3, 2);
    iron[6] = C(11, 9, 4);
    iron[7] = C(14, 12, 6);
    iron[8] = C(2, 2, 2);
    iron[9] = C(13, 7, 2);
    iron[15] = C(1, 1, 1);
    setPal(vdp, PAL_IRON, iron);

    uint16_t loco[16] = {};
    loco[1] = C(1, 3, 2);
    loco[2] = C(3, 6, 4);
    loco[3] = C(1, 1, 2);
    loco[4] = C(2, 2, 2);
    loco[5] = C(8, 8, 9);
    loco[6] = C(12, 10, 4);
    loco[7] = C(12, 2, 2);
    loco[8] = C(6, 8, 10);
    loco[9] = C(14, 14, 12);
    loco[10] = C(6, 3, 2);
    loco[11] = C(3, 2, 2);
    loco[15] = C(1, 1, 1);
    setPal(vdp, PAL_LOCO, loco);

    uint16_t smoke[16] = {};
    smoke[1] = C(6, 6, 7);
    smoke[2] = C(4, 4, 5);
    smoke[3] = C(8, 8, 9);
    setPal(vdp, PAL_SMOKE, smoke);

    uint16_t rain[16] = {};
    rain[1] = C(8, 10, 13);
    rain[2] = C(12, 13, 15);
    setPal(vdp, PAL_RAIN, rain);

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

    uint16_t yard[16] = {};
    yard[1] = C(4, 4, 4);
    yard[2] = C(3, 3, 3);
    yard[3] = C(5, 5, 4);
    yard[4] = C(5, 4, 3);
    yard[5] = C(3, 3, 2);
    yard[6] = C(2, 2, 3);
    yard[7] = C(3, 3, 4);
    yard[8] = C(6, 6, 5);
    yard[9] = C(11, 12, 13);
    yard[10] = C(6, 4, 2);
    yard[14] = C(12, 10, 3);
    yard[15] = C(7, 7, 6);
    setPal(vdp, PAL_YARD, yard);

    uint16_t gold[16] = {};
    gold[1] = C(15, 13, 5);
    gold[15] = C(3, 2, 0);
    setPal(vdp, PAL_GOLD, gold);

    uint16_t alert[16] = {};
    alert[1] = C(15, 8, 6);
    alert[15] = C(4, 0, 0);
    setPal(vdp, PAL_ALERT, alert);

    uint16_t pip[16] = {};
    pip[1] = C(4, 4, 6);
    setPal(vdp, PAL_PIP, pip);

    vdp.setFogColor(C(1, 1, 3));
    vdp.A.enabled = true;
    vdp.B.enabled = true;

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    paintDock(vdp, tiles);
    gs::bitmapToPlane(tiles, vdp.A, 0, 5, shedArt(), PAL_SHED);

    art.man[0] = gs::uploadMipped(vdp, manArt(0));
    art.man[1] = gs::uploadMipped(vdp, manArt(1));
    art.pot = gs::uploadMipped(vdp, potArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.hood = gs::uploadMipped(vdp, hoodArt());
    art.loco[0] = gs::uploadMipped(vdp, locoArt(0));
    art.loco[1] = gs::uploadMipped(vdp, locoArt(1));
    art.wagon = gs::uploadMipped(vdp, wagonArt());
    art.buffer = gs::uploadMipped(vdp, bufferArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.rain = gs::uploadMipped(vdp, rainArt());
    art.smoke[0] = gs::uploadMipped(vdp, smokeArt(0));
    art.smoke[1] = gs::uploadMipped(vdp, smokeArt(1));
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.star = gs::uploadMipped(vdp, starArt());
}

}  // namespace depotdawn
