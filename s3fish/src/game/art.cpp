#include "game/art.h"

#include <cmath>
#include <cstring>
#include <initializer_list>
#include <string>

namespace fish {
namespace {

constexpr float TAU = 6.2831853f;

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void eye(Bitmap& b, float x, float y, float r) {
    b.ellipse(x, y, r, r, 6);
    b.ellipse(x + r * 0.28f, y - r * 0.1f, r * 0.42f, r * 0.42f, 7);
}

void goldMark(Bitmap& b, float x, float y) {
    b.poly({{x, y - 5}, {x + 6, y}, {x, y + 5}, {x - 6, y}}, 9);
    b.poly({{x, y - 2.4f}, {x + 2.6f, y}, {x, y + 2.4f}, {x - 2.6f, y}}, 1);
}

void fishPal(gs::VDP& vdp, int pal, uint16_t hi, uint16_t body, uint16_t belly, uint16_t fin, uint16_t dark) {
    const uint16_t sh = gs::rgb4(1, 1, 2);
    setPal(vdp, pal,
           {0, hi, body, belly, fin, dark, gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 2), gs::rgb4(4, 3, 2),
            gs::rgb4(15, 13, 2), 0, 0, 0, 0, 0, sh});
}

Bitmap bass(bool gold) {
    Bitmap b(68, 36);
    b.poly({{8, 18}, {18, 7}, {18, 29}}, 4);
    b.poly({{10, 18}, {18, 12}, {18, 24}}, 2);
    b.ellipse(40, 18, 20, 11, 2);
    b.ellipse(42, 22, 14, 6, 3);
    b.ellipse(52, 16, 9, 8, 2);
    b.poly({{30, 10}, {36, 2}, {46, 9}, {40, 12}}, 4);
    b.poly({{44, 22}, {56, 30}, {40, 24}}, 4);
    eye(b, 55, 15, 3.1f);
    b.line(58, 18, 66, 22, 5, 1.5f);
    b.line(58, 17, 64, 15, 5, 1.2f);
    if (gold) goldMark(b, 36, 17);
    b.outline(5, false);
    return b;
}

Bitmap trout(bool gold) {
    Bitmap b(74, 30);
    b.poly({{6, 15}, {16, 5}, {16, 15}}, 4);
    b.poly({{6, 15}, {16, 25}, {16, 15}}, 4);
    b.poly({{9, 15}, {16, 9}, {16, 21}}, 2);
    b.ellipse(42, 15, 22, 8, 2);
    b.ellipse(44, 18, 14, 4, 3);
    b.poly({{34, 8}, {42, 2}, {48, 9}}, 4);
    b.poly({{24, 9}, {30, 5}, {32, 11}}, 4);
    const float spots[][2] = {{30, 13}, {38, 12}, {48, 14}, {34, 18}, {44, 17}, {52, 16}};
    for (const auto& s : spots) b.ellipse(s[0], s[1], 1.3f, 1.0f, 1);
    eye(b, 58, 13, 2.5f);
    b.line(62, 16, 70, 18, 5, 1.3f);
    if (gold) goldMark(b, 40, 14);
    b.outline(5, false);
    return b;
}

Bitmap pike(bool gold) {
    Bitmap b(96, 28);
    b.ellipse(46, 14, 30, 7, 2);
    b.ellipse(58, 16, 18, 4, 3);
    b.poly({{72, 12}, {94, 15}, {72, 18}}, 2);
    b.line(76, 16, 93, 18, 5, 1.2f);
    b.poly({{10, 14}, {2, 4}, {18, 12}}, 4);
    b.poly({{10, 14}, {2, 24}, {18, 16}}, 4);
    b.poly({{28, 8}, {38, 3}, {42, 10}}, 4);
    b.poly({{30, 19}, {40, 25}, {36, 18}}, 4);
    b.line(58, 10, 66, 18, 1, 1.1f);
    b.line(64, 10, 70, 17, 1, 1.1f);
    eye(b, 76, 12, 2.1f);
    if (gold) goldMark(b, 48, 13);
    b.outline(5, false);
    return b;
}

Bitmap walleye(bool gold) {
    Bitmap b(72, 32);
    b.ellipse(38, 17, 20, 9, 2);
    b.ellipse(46, 19, 12, 5, 3);
    b.poly({{12, 16}, {2, 6}, {16, 14}}, 4);
    b.poly({{12, 16}, {2, 26}, {16, 18}}, 4);
    b.poly({{26, 10}, {30, 1}, {34, 11}}, 4);
    b.poly({{34, 10}, {44, 3}, {48, 11}}, 4);
    b.poly({{40, 22}, {50, 28}, {42, 22}}, 4);
    eye(b, 54, 14, 4.0f);
    b.line(60, 18, 68, 20, 5, 1.3f);
    if (gold) goldMark(b, 36, 16);
    b.outline(5, false);
    return b;
}

Bitmap catfish(bool gold) {
    Bitmap b(76, 34);
    b.ellipse(36, 18, 20, 11, 2);
    b.ellipse(50, 18, 13, 10, 2);
    b.ellipse(38, 22, 14, 6, 3);
    b.ellipse(14, 18, 8, 7, 4);
    b.poly({{32, 9}, {36, 2}, {40, 10}}, 4);
    b.line(58, 16, 74, 8, 8, 1.3f);
    b.line(60, 18, 74, 16, 8, 1.3f);
    b.line(60, 21, 74, 26, 8, 1.3f);
    eye(b, 56, 15, 2.0f);
    if (gold) goldMark(b, 34, 16);
    b.outline(5, false);
    return b;
}

Bitmap anglerArt() {
    Bitmap b(40, 48);
    b.rect(16, 3, 10, 5, 4);
    b.ellipse(21, 8, 10, 3.2f, 4);
    b.ellipse(21, 14, 5, 5.2f, 3);
    b.rect(15, 19, 13, 13, 2);
    b.rect(17, 21, 8, 8, 1);
    b.line(26, 22, 34, 14, 3, 2.4f);
    b.line(30, 16, 38, 3, 5, 1.7f);
    b.rect(16, 31, 5, 10, 6);
    b.rect(23, 31, 5, 10, 6);
    b.rect(15, 40, 7, 4, 7);
    b.rect(22, 40, 7, 4, 7);
    b.outline(5, false);
    return b;
}

Bitmap boatArt() {
    Bitmap b(64, 22);
    b.poly({{4, 8}, {60, 8}, {52, 18}, {12, 18}}, 4);
    b.poly({{10, 8}, {54, 8}, {48, 14}, {16, 14}}, 1);
    b.rect(24, 6, 16, 3, 7);
    b.rect(31, 3, 3, 6, 5);
    b.outline(5, false);
    return b;
}

Bitmap lureArt() {
    Bitmap b(16, 22);
    b.ellipse(8, 5, 4.2f, 4.2f, 1);
    b.ellipse(7, 4, 1.6f, 1.4f, 2);
    b.line(8, 9, 8, 16, 3, 1.6f);
    b.line(8, 16, 13, 12, 3, 1.5f);
    b.line(13, 12, 12, 8, 3, 1.4f);
    return b;
}

Bitmap dotArt() {
    Bitmap b(6, 6);
    b.ellipse(3, 3, 2.1f, 2.1f, 1);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(14, 14);
    b.poly({{7, 0}, {9, 5}, {14, 7}, {9, 9}, {7, 14}, {5, 9}, {0, 7}, {5, 5}}, 1);
    b.ellipse(7, 7, 2.2f, 2.2f, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(40, 40);
    for (int i = 0; i < 8; i++) {
        float a = i * TAU / 8.f;
        b.line(20 + std::cos(a) * 11, 20 + std::sin(a) * 11, 20 + std::cos(a) * 18, 20 + std::sin(a) * 18, 4, 2.f);
    }
    b.ellipse(20, 20, 10, 10, 2);
    b.ellipse(20, 20, 6, 6, 1);
    return b;
}

Bitmap moonArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 11, 11, 1);
    b.ellipse(18, 12, 8, 8, 0);
    b.ellipse(10, 16, 2.2f, 2.0f, 2);
    b.ellipse(15, 18, 1.4f, 1.2f, 2);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(64, 24);
    b.ellipse(20, 14, 14, 8, 1);
    b.ellipse(36, 12, 16, 9, 1);
    b.ellipse(50, 15, 10, 6, 1);
    b.ellipse(32, 12, 8, 5, 2);
    return b;
}

Bitmap treeArt() {
    Bitmap b(40, 48);
    b.rect(17, 28, 6, 16, 4);
    b.ellipse(20, 22, 16, 14, 2);
    b.ellipse(14, 18, 8, 8, 1);
    b.ellipse(26, 20, 7, 6, 3);
    b.outline(5, false);
    return b;
}

Bitmap pineArt() {
    Bitmap b(36, 52);
    b.rect(16, 34, 4, 14, 4);
    b.poly({{18, 2}, {32, 22}, {4, 22}}, 2);
    b.poly({{18, 14}, {34, 36}, {2, 36}}, 1);
    b.outline(5, false);
    return b;
}

Bitmap weedArt() {
    Bitmap b(22, 40);
    b.line(6, 38, 8, 8, 1, 2.f);
    b.line(12, 38, 11, 4, 2, 2.f);
    b.line(17, 38, 15, 12, 1, 1.6f);
    b.ellipse(8, 7, 3, 2.2f, 1);
    b.ellipse(11, 4, 2.4f, 2, 3);
    return b;
}

Bitmap rockArt() {
    Bitmap b(28, 16);
    b.ellipse(14, 10, 12, 6, 6);
    b.ellipse(11, 9, 6, 3, 7);
    b.outline(5, false);
    return b;
}

Bitmap lilyArt() {
    Bitmap b(26, 12);
    b.ellipse(12, 6, 10, 4, 2);
    b.ellipse(12, 6, 3, 2, 1);
    b.line(12, 6, 22, 4, 3, 1.2f);
    return b;
}

Bitmap bubbleArt() {
    Bitmap b(10, 10);
    b.ellipse(5, 5, 3.4f, 3.4f, 6);
    b.ellipse(4, 4, 1.1f, 1.1f, 7);
    return b;
}

Bitmap splashArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 12, 8, 4);
    b.ellipse(16, 16, 6, 4, 5);
    b.ellipse(16, 15, 2.4f, 1.6f, 7);
    return b;
}

Bitmap gleamArt() {
    Bitmap b(32, 8);
    b.ellipse(16, 4, 14, 3, 1);
    b.ellipse(16, 4, 5, 1.4f, 2);
    return b;
}

Bitmap starArt() {
    Bitmap b(7, 7);
    b.line(3, 0, 3, 6, 1, 1.f);
    b.line(0, 3, 6, 3, 1, 1.f);
    b.set(1, 1, 1);
    b.set(5, 1, 1);
    b.set(1, 5, 1);
    b.set(5, 5, 1);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(48, 14);
    b.ellipse(24, 8, 20, 5, 1);
    return b;
}

Bitmap birdArt(int frame) {
    Bitmap b(22, 12);
    if (frame == 0) {
        b.line(1, 8, 11, 5, 5, 1.6f);
        b.line(21, 8, 11, 5, 5, 1.6f);
    } else {
        b.line(1, 2, 11, 6, 5, 1.6f);
        b.line(21, 2, 11, 6, 5, 1.6f);
    }
    b.ellipse(11, 6, 2.2f, 1.5f, 5);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t sh = gs::rgb4(1, 1, 3);
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 13, 15), 0, 0, gs::rgb4(2, 2, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(10, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 6), gs::rgb4(3, 10, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(7, 8, 10), gs::rgb4(4, 5, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});

    setPal(vdp, PAL_BOAT,
           {0, gs::rgb4(13, 14, 12), gs::rgb4(3, 8, 5), gs::rgb4(14, 10, 7), gs::rgb4(8, 5, 2), gs::rgb4(1, 1, 2),
            gs::rgb4(2, 3, 6), gs::rgb4(3, 2, 2), 0, 0, 0, 0, 0, 0, 0, sh});

    fishPal(vdp, PAL_BASS, gs::rgb4(12, 15, 8), gs::rgb4(3, 10, 4), gs::rgb4(8, 13, 6), gs::rgb4(2, 7, 3), gs::rgb4(1, 3, 2));
    fishPal(vdp, PAL_TROUT, gs::rgb4(15, 12, 7), gs::rgb4(10, 7, 4), gs::rgb4(13, 10, 8), gs::rgb4(7, 4, 3), gs::rgb4(3, 2, 2));
    fishPal(vdp, PAL_PIKE, gs::rgb4(12, 14, 8), gs::rgb4(5, 10, 4), gs::rgb4(9, 12, 7), gs::rgb4(2, 6, 3), gs::rgb4(1, 3, 2));
    fishPal(vdp, PAL_WALLEYE, gs::rgb4(15, 14, 8), gs::rgb4(12, 10, 4), gs::rgb4(14, 12, 7), gs::rgb4(7, 5, 2), gs::rgb4(3, 2, 1));
    fishPal(vdp, PAL_CAT, gs::rgb4(12, 10, 8), gs::rgb4(8, 7, 5), gs::rgb4(11, 9, 7), gs::rgb4(5, 4, 3), gs::rgb4(2, 2, 2));

    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 11), gs::rgb4(8, 9, 10), gs::rgb4(14, 15, 15), gs::rgb4(7, 12, 15),
            gs::rgb4(10, 14, 15), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, sh});

    const uint16_t water[16] = {
        0,
        gs::rgb4(2, 5, 4), gs::rgb4(1, 4, 3), gs::rgb4(3, 6, 5),
        gs::rgb4(4, 6, 3), gs::rgb4(3, 5, 2),
        gs::rgb4(2, 6, 9), gs::rgb4(2, 5, 8),
        gs::rgb4(10, 13, 12), gs::rgb4(3, 7, 10), gs::rgb4(2, 6, 9),
        gs::rgb4(3, 9, 12), gs::rgb4(2, 7, 11), gs::rgb4(10, 14, 15),
        gs::rgb4(12, 14, 13), gs::rgb4(14, 15, 15),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_WATER * 16 + i, water[i]);

    setPal(vdp, PAL_TREE,
           {0, gs::rgb4(8, 13, 5), gs::rgb4(3, 8, 3), gs::rgb4(2, 5, 2), gs::rgb4(7, 5, 3), gs::rgb4(1, 2, 2),
            gs::rgb4(6, 6, 6), gs::rgb4(9, 9, 8), 0, 0, 0, 0, 0, 0, 0, sh});

    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 13, 3), gs::rgb4(15, 9, 2), gs::rgb4(15, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_SUNSET, {0, gs::rgb4(15, 10, 4), gs::rgb4(15, 6, 2), gs::rgb4(12, 3, 2), gs::rgb4(15, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});

    loadFont(vdp, art);
    art.fish[0][0] = gs::uploadMipped(vdp, bass(false));
    art.fish[0][1] = gs::uploadMipped(vdp, bass(true));
    art.fish[1][0] = gs::uploadMipped(vdp, trout(false));
    art.fish[1][1] = gs::uploadMipped(vdp, trout(true));
    art.fish[2][0] = gs::uploadMipped(vdp, pike(false));
    art.fish[2][1] = gs::uploadMipped(vdp, pike(true));
    art.fish[3][0] = gs::uploadMipped(vdp, walleye(false));
    art.fish[3][1] = gs::uploadMipped(vdp, walleye(true));
    art.fish[4][0] = gs::uploadMipped(vdp, catfish(false));
    art.fish[4][1] = gs::uploadMipped(vdp, catfish(true));
    art.angler = gs::uploadMipped(vdp, anglerArt());
    art.boat = gs::uploadMipped(vdp, boatArt());
    art.lure = gs::uploadMipped(vdp, lureArt());
    art.line = gs::uploadMipped(vdp, dotArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.weed = gs::uploadMipped(vdp, weedArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.lily = gs::uploadMipped(vdp, lilyArt());
    art.bubble = gs::uploadMipped(vdp, bubbleArt());
    art.splash = gs::uploadMipped(vdp, splashArt());
    art.gleam = gs::uploadMipped(vdp, gleamArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(0));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(1));
}

}  // namespace fish
