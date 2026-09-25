#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace duel {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    gs::TextStyle big{3, 1, 0, 0, 1};
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

gs::Bitmap cowboy(int pose) {
    if (pose == POSE_DOWN) {
        gs::Bitmap b(120, 40);
        b.ellipse(18, 14, 12, 6, 3);
        b.rect(8, 16, 28, 4, 3);
        b.rect(12, 19, 18, 2, 4);
        b.ellipse(34, 18, 8, 7, 5);
        b.rect(40, 12, 52, 14, 1);
        b.rect(46, 15, 16, 8, 6);
        b.rect(78, 20, 28, 7, 2);
        b.ellipse(108, 24, 7, 4, 9);
        b.rect(44, 24, 10, 8, 1);
        b.rect(58, 26, 8, 3, 8);
        b.poly({{48, 16}, {54, 13}, {56, 18}, {50, 21}}, 13);
        b.outline(10, false);
        return b;
    }

    const bool back = pose == POSE_BACK || pose == POSE_STEP;
    const bool aim = pose == POSE_AIM || pose == POSE_FIRE;
    const bool hurt = pose == POSE_HURT;
    gs::Bitmap b(104, 108);
    const int c = 52 + (hurt ? -6 : 0);

    b.ellipse(float(c), 13, 11, 7, 3);
    b.ellipse(float(c), 11, 6, 3, 2);
    b.rect(c - 20, 17, 40, 5, 3);
    b.rect(c - 14, 21, 28, 3, 4);

    b.ellipse(float(c - (back ? 0 : 2)), 32, 8, 9, back ? 11 : 5);
    if (!back) {
        b.ellipse(float(c + 4), 31, 1.5f, 1.5f, 10);
        b.rect(c - 7, 38, 12, 3, 4);
    }

    b.poly({{float(c - 18), 44}, {float(c + 18), 44}, {float(c + 22), 78}, {float(c - 16), 80}}, 1);
    b.poly({{float(c - 5), 48}, {float(c + 8), 48}, {float(c + 6), 72}, {float(c - 3), 72}}, back ? 2 : 6);
    if (!back) b.poly({{float(c - 14), 56}, {float(c - 8), 51}, {float(c - 4), 56}, {float(c - 8), 61}}, 13);

    b.rect(c - 16, 74, 36, 5, 7);
    b.rect(c - 2, 74, 5, 5, 12);

    int lf = 0, rf = 0;
    if (pose == POSE_STEP) {
        lf = -6;
        rf = 7;
    } else if (pose == POSE_BACK) {
        lf = 2;
        rf = -2;
    }
    b.rect(c - 14 + lf, 78, 9, 18, 1);
    b.rect(c + 3 + rf, 78, 9, 18, 2);
    b.ellipse(float(c - 10 + lf), 98, 7, 4, 9);
    b.ellipse(float(c + 7 + rf), 98, 7, 4, 9);

    if (aim) {
        int ext = pose == POSE_FIRE ? 4 : 0;
        b.rect(c + 14, 50, 22 + ext, 6, 1);
        b.rect(c + 34 + ext, 51, 12, 4, 8);
        b.rect(c + 32 + ext, 49, 5, 8, 7);
        b.rect(c - 24, 50, 8, 20, 1);
        if (pose == POSE_FIRE) b.ellipse(float(c + 48 + ext), 53, 6, 4, 14);
    } else if (hurt) {
        b.rect(c - 30, 48, 8, 18, 1);
        b.rect(c + 16, 42, 8, 16, 1);
        b.rect(c - 32, 66, 10, 3, 8);
    } else {
        b.rect(c - 26, 46, 8, 24, 1);
        b.rect(c + 16, 46, 8, 24, 2);
        b.rect(c + 12, 74, 6, 12, 7);
        b.rect(c + 13, 76, 3, 8, 8);
    }
    b.outline(10, false);
    if (pose == POSE_FIRE) b.ellipse(float(c + 50), 53, 4, 3, 14);
    return b;
}

gs::Bitmap saloonArt() {
    gs::Bitmap b(150, 96);
    b.poly({{8, 30}, {75, 4}, {142, 30}}, 7);
    b.rect(12, 26, 126, 10, 1);
    b.rect(34, 32, 82, 16, 5);
    b.rect(14, 48, 122, 40, 1);
    for (int i = 0; i < 7; i++) b.rect(18 + i * 17, 48, 3, 40, 2);
    b.rect(26, 56, 24, 16, 13);
    b.rect(100, 56, 24, 16, 13);
    b.rect(36, 56, 3, 16, 2);
    b.rect(110, 56, 3, 16, 2);
    b.rect(26, 63, 24, 2, 2);
    b.rect(100, 63, 24, 2, 2);
    b.rect(4, 4, 15, 15, 4);
    b.rect(63, 58, 24, 30, 3);
    b.rect(74, 58, 2, 30, 2);
    b.ellipse(70, 74, 2, 2, 9);
    b.ellipse(80, 74, 2, 2, 9);
    b.rect(6, 86, 138, 5, 8);
    b.rect(12, 74, 5, 16, 8);
    b.rect(133, 74, 5, 16, 8);
    b.outline(10, false);
    gs::Bitmap word = gs::textBitmap("SALOON", gs::TextStyle{1, 6, 0, 0, 1});
    b.blit(word, 34 + (82 - word.w) / 2, 32 + (16 - word.h) / 2);
    return b;
}

gs::Bitmap storeArt() {
    gs::Bitmap b(86, 80);
    b.poly({{6, 22}, {43, 3}, {80, 22}}, 7);
    b.rect(10, 20, 66, 54, 1);
    for (int i = 0; i < 4; i++) b.rect(14 + i * 16, 20, 3, 54, 2);
    b.rect(16, 28, 54, 14, 5);
    b.rect(18, 48, 18, 14, 13);
    b.rect(50, 48, 18, 14, 13);
    b.rect(34, 48, 18, 26, 3);
    b.rect(6, 72, 74, 4, 8);
    b.outline(10, false);
    gs::Bitmap word = gs::textBitmap("GOODS", gs::TextStyle{1, 6, 0, 0, 1});
    b.blit(word, 16 + (54 - word.w) / 2, 28 + (14 - word.h) / 2);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(52, 112);
    b.rect(18, 4, 16, 14, 7);
    b.rect(22, 0, 8, 6, 2);
    b.poly({{8, 22}, {26, 8}, {44, 22}}, 7);
    b.rect(10, 22, 32, 82, 1);
    b.rect(6, 22, 40, 6, 2);
    for (int i = 0; i < 4; i++) {
        b.rect(16, 34 + i * 16, 8, 10, 13);
        b.rect(28, 34 + i * 16, 8, 10, 13);
    }
    b.rect(20, 90, 12, 14, 3);
    b.outline(10, false);
    return b;
}

gs::Bitmap hillArt() {
    gs::Bitmap b(320, 52);
    b.poly({{0, 51}, {0, 36}, {36, 22}, {70, 34}, {112, 16}, {156, 32}, {198, 14}, {248, 30}, {292, 18}, {319, 28}, {319, 51}}, 1);
    b.poly({{0, 51}, {0, 42}, {48, 32}, {96, 44}, {150, 30}, {210, 46}, {270, 34}, {319, 40}, {319, 51}}, 2);
    return b;
}

gs::Bitmap cactusArt() {
    gs::Bitmap b(44, 68);
    b.rect(17, 12, 10, 50, 1);
    b.rect(4, 26, 14, 8, 1);
    b.rect(4, 16, 8, 16, 1);
    b.rect(26, 30, 14, 8, 1);
    b.rect(30, 20, 8, 16, 1);
    b.ellipse(22, 12, 6, 4, 1);
    b.ellipse(8, 16, 4, 3, 1);
    b.ellipse(34, 20, 4, 3, 1);
    for (int i = 0; i < 6; i++) {
        b.rect(15, 18 + i * 7, 2, 2, 4);
        b.rect(27, 20 + i * 7, 2, 2, 4);
    }
    b.ellipse(22, 20, 2, 2, 3);
    b.outline(10, false);
    return b;
}

gs::Bitmap horseArt() {
    gs::Bitmap b(92, 64);
    b.poly({{12, 26}, {4, 38}, {8, 50}, {18, 36}}, 3);
    b.ellipse(42, 30, 24, 12, 1);
    b.ellipse(38, 28, 12, 6, 2);
    b.poly({{54, 26}, {70, 12}, {76, 20}, {58, 34}}, 1);
    b.ellipse(78, 16, 8, 5, 1);
    b.rect(82, 14, 8, 4, 1);
    b.rect(68, 10, 8, 4, 3);
    b.ellipse(80, 15, 1.2f, 1.2f, 10);
    b.rect(24, 38, 5, 18, 1);
    b.rect(36, 38, 5, 18, 2);
    b.rect(50, 36, 5, 18, 1);
    b.rect(60, 36, 5, 18, 2);
    b.ellipse(26, 57, 4, 3, 4);
    b.ellipse(38, 57, 4, 3, 4);
    b.ellipse(52, 55, 4, 3, 4);
    b.ellipse(62, 55, 4, 3, 4);
    b.outline(10, false);
    return b;
}

gs::Bitmap fenceArt() {
    gs::Bitmap b(140, 40);
    b.rect(0, 12, 140, 4, 1);
    b.rect(0, 24, 140, 4, 1);
    for (int i = 0; i < 5; i++) b.rect(8 + i * 32, 4, 5, 32, 2);
    b.outline(10, false);
    return b;
}

gs::Bitmap barrelArt() {
    gs::Bitmap b(26, 30);
    b.ellipse(13, 8, 10, 4, 1);
    b.rect(3, 8, 20, 16, 1);
    b.ellipse(13, 24, 10, 4, 2);
    b.rect(3, 13, 20, 2, 2);
    b.rect(3, 18, 20, 2, 2);
    b.outline(10, false);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(30, 30);
    b.ellipse(15, 15, 13, 13, 1);
    b.ellipse(15, 15, 3, 3, 2);
    b.line(15, 3, 15, 27, 2, 1.4f);
    b.line(3, 15, 27, 15, 2, 1.4f);
    b.line(6, 6, 24, 24, 2, 1.2f);
    b.line(24, 6, 6, 24, 2, 1.2f);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(48, 48);
    const float tau = 6.2831853f;
    for (int i = 0; i < 8; i++) {
        float a = i * tau / 8.f;
        b.line(24 + std::cos(a) * 10, 24 + std::sin(a) * 10, 24 + std::cos(a) * 22, 24 + std::sin(a) * 22, 1, 2.f);
    }
    b.ellipse(24, 24, 12, 12, 1);
    b.ellipse(24, 24, 7, 7, 2);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(72, 28);
    b.ellipse(26, 16, 18, 8, 1);
    b.ellipse(46, 14, 20, 10, 1);
    b.ellipse(36, 12, 12, 7, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(36, 16);
    b.ellipse(18, 10, 16, 5, 1);
    b.ellipse(12, 8, 7, 3, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(44, 14);
    b.ellipse(22, 7, 18, 5, 1);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(32, 32);
    b.poly({{16, 1}, {20, 12}, {31, 16}, {20, 20}, {16, 31}, {12, 20}, {1, 16}, {12, 12}}, 1);
    b.ellipse(16, 16, 5, 5, 2);
    return b;
}

gs::Bitmap printArt() {
    gs::Bitmap b(14, 18);
    b.ellipse(7, 6, 5, 5, 4);
    b.ellipse(7, 13, 3, 3, 4);
    return b;
}

gs::Bitmap arrowArt() {
    gs::Bitmap b(16, 14);
    b.poly({{8, 13}, {1, 1}, {15, 1}}, 1);
    b.poly({{8, 10}, {4, 3}, {12, 3}}, 2);
    return b;
}

gs::Bitmap birdArt() {
    gs::Bitmap b(18, 8);
    b.line(0, 5, 7, 1, 1, 1.4f);
    b.line(7, 1, 9, 5, 1, 1.4f);
    b.line(9, 5, 11, 1, 1, 1.4f);
    b.line(11, 1, 17, 5, 1, 1.4f);
    return b;
}

gs::Bitmap weedArt() {
    gs::Bitmap b(30, 28);
    b.ellipse(15, 15, 12, 10, 1);
    b.ellipse(15, 15, 6, 4, 2);
    b.line(4, 8, 26, 20, 3, 1.2f);
    b.line(6, 22, 24, 6, 3, 1.2f);
    b.line(8, 6, 22, 22, 3, 1.f);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 1);
    const uint16_t shadow = gs::rgb4(2, 1, 1);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 11, 13), gs::rgb4(15, 13, 7), gs::rgb4(8, 8, 10), 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(12, 9, 4), gs::rgb4(8, 6, 2), gs::rgb4(14, 13, 10), gs::rgb4(6, 3, 1), gs::rgb4(15, 11, 8),
            gs::rgb4(14, 14, 12), gs::rgb4(4, 2, 1), gs::rgb4(6, 6, 7), gs::rgb4(3, 2, 1), ink, gs::rgb4(5, 3, 2),
            gs::rgb4(15, 12, 3), gs::rgb4(14, 14, 15), gs::rgb4(15, 15, 8), shadow});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 1), gs::rgb4(1, 1, 2), gs::rgb4(12, 2, 2), gs::rgb4(13, 9, 6),
            gs::rgb4(8, 1, 1), gs::rgb4(3, 2, 1), gs::rgb4(7, 7, 8), gs::rgb4(2, 1, 1), ink, gs::rgb4(1, 1, 1),
            gs::rgb4(8, 7, 3), gs::rgb4(2, 2, 3), gs::rgb4(15, 14, 6), shadow});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(4, 2, 1), gs::rgb4(15, 12, 5), gs::rgb4(13, 11, 8),
            gs::rgb4(8, 1, 1), gs::rgb4(7, 2, 2), gs::rgb4(8, 6, 3), gs::rgb4(14, 11, 3), ink, gs::rgb4(12, 3, 2),
            gs::rgb4(14, 12, 9), gs::rgb4(8, 10, 12), gs::rgb4(7, 5, 3), shadow});
    setPal(vdp, PAL_PLANT,
           {0, gs::rgb4(3, 9, 3), gs::rgb4(2, 6, 2), gs::rgb4(14, 12, 4), gs::rgb4(13, 13, 9), 0, 0, 0, 0, 0, ink, 0, 0,
            0, 0, shadow});
    setPal(vdp, PAL_HORSE,
           {0, gs::rgb4(8, 5, 2), gs::rgb4(4, 3, 1), gs::rgb4(2, 1, 1), gs::rgb4(3, 2, 2), gs::rgb4(13, 11, 8), 0, 0, 0,
            0, ink, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LAND, {0, gs::rgb4(7, 6, 9), gs::rgb4(9, 6, 3), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 3, 3), gs::rgb4(15, 12, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DUST,
           {0, gs::rgb4(12, 9, 5), gs::rgb4(9, 7, 4), gs::rgb4(6, 4, 2), gs::rgb4(7, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            shadow});
    setPal(vdp, PAL_CLOUD, {0, gs::rgb4(14, 13, 12), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    vdp.setFogColor(gs::rgb4(12, 8, 5));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    for (int v = 0; v < 4; v++) {
        uint8_t px[64];
        for (int i = 0; i < 64; i++) {
            uint32_t h = uint32_t(i * 17 + v * 29 + 3) * 0x9E3779B9u;
            px[i] = (h % 11 == 0) ? 4 : (h % 17 == 0) ? 2 : 1;
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.ground[v] = t;
    }

    for (int i = 0; i < POSE_N; i++) art.pose[i] = gs::uploadMipped(vdp, cowboy(i));
    art.saloon = gs::uploadMipped(vdp, saloonArt());
    art.store = gs::uploadMipped(vdp, storeArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.cactus = gs::uploadMipped(vdp, cactusArt());
    art.horse = gs::uploadMipped(vdp, horseArt());
    art.fence = gs::uploadMipped(vdp, fenceArt());
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.print = gs::uploadMipped(vdp, printArt());
    art.arrow = gs::uploadMipped(vdp, arrowArt());
    art.bird = gs::uploadMipped(vdp, birdArt());
    art.weed = gs::uploadMipped(vdp, weedArt());
}

}  // namespace duel
