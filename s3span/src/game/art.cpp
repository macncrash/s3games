#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace span {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap plankArt() {
    Bitmap b(48, 14);
    b.rect(1, 2, 46, 10, 2);
    b.rect(1, 2, 46, 3, 1);
    for (int x = 4; x < 46; x += 7) b.rect(float(x), 3, 1, 8, 3);
    b.rect(0, 5, 48, 2, 4);
    b.rect(6, 5, 3, 3, 5);
    b.rect(40, 5, 3, 3, 5);
    b.outline(6, false);
    return b;
}

Bitmap ropeArt(bool taut) {
    Bitmap b(6, 28);
    b.rect(2, 0, 2, 28, taut ? 3 : 2);
    b.rect(1, 0, 1, 28, 1);
    if (taut) b.rect(3, 0, 1, 28, 4);
    return b;
}

Bitmap linkArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3.2f, 2.4f, 1);
    b.ellipse(4, 4, 1.6f, 1.0f, 0);
    b.rect(3, 1, 2, 2, 3);
    return b;
}

Bitmap towerArt() {
    Bitmap b(36, 96);
    b.poly({{4, 8}, {32, 8}, {34, 94}, {2, 94}}, 2);
    b.rect(6, 10, 24, 80, 1);
    b.rect(4, 6, 28, 6, 4);
    b.rect(8, 4, 20, 4, 5);
    for (int y = 18; y < 88; y += 12) {
        b.rect(4, float(y), 28, 3, 3);
        b.rect(16, float(y), 4, 10, 4);
    }
    b.poly({{2, 94}, {10, 70}, {14, 94}}, 3);
    b.poly({{34, 94}, {26, 70}, {22, 94}}, 3);
    b.outline(6, false);
    return b;
}

Bitmap cliffArt() {
    Bitmap b(72, 130);
    b.poly({{0, 36}, {14, 18}, {36, 26}, {58, 12}, {72, 28}, {72, 129}, {0, 129}}, 2);
    b.poly({{0, 44}, {22, 30}, {48, 38}, {72, 24}, {72, 70}, {0, 78}}, 1);
    b.rect(0, 28, 72, 5, 4);
    b.rect(8, 22, 4, 12, 3);
    b.poly({{18, 30}, {24, 8}, {28, 30}}, 5);
    b.poly({{46, 26}, {52, 4}, {56, 26}}, 5);
    b.rect(22, 18, 3, 12, 3);
    b.rect(50, 14, 3, 12, 3);
    for (int y = 48; y < 120; y += 16) b.line(6, float(y), 60, float(y + 8), 3, 1.2f);
    b.outline(6, false);
    return b;
}

Bitmap wallArt() {
    Bitmap b(120, 90);
    b.rect(0, 10, 120, 80, 2);
    b.poly({{0, 24}, {30, 8}, {70, 18}, {120, 6}, {120, 40}, {0, 48}}, 1);
    for (int x = 12; x < 110; x += 18) b.line(float(x), 20, float(x + 4), 88, 3, 1.4f);
    b.rect(0, 78, 120, 12, 3);
    return b;
}

Bitmap lipArt() {
    Bitmap b(56, 18);
    b.rect(0, 2, 56, 12, 2);
    b.rect(0, 2, 56, 4, 1);
    b.rect(0, 12, 56, 4, 3);
    for (int x = 6; x < 52; x += 10) b.rect(float(x), 6, 2, 5, 3);
    b.outline(6, false);
    return b;
}

Bitmap marcher(int frame) {
    Bitmap b(26, 40);
    b.ellipse(12, 7, 4.5f, 4.2f, 1);
    b.ellipse(12, 5, 4.2f, 2.4f, 8);
    b.rect(7, 12, 11, 13, 2);
    b.rect(7, 12, 3, 13, 3);
    b.rect(7, 21, 11, 2, 4);
    if (frame == 0) {
        b.rect(8, 26, 4, 9, 3);
        b.rect(14, 26, 4, 7, 6);
        b.rect(7, 34, 6, 3, 6);
        b.rect(14, 32, 6, 3, 6);
    } else {
        b.rect(8, 26, 4, 7, 6);
        b.rect(14, 26, 4, 9, 3);
        b.rect(7, 32, 6, 3, 6);
        b.rect(13, 34, 6, 3, 6);
    }
    b.line(20, 3, 18, 34, 5, 1.4f);
    b.poly({{17, 2}, {24, 5}, {18, 9}}, 5);
    b.outline(14, false);
    return b;
}

Bitmap bannerMan(int frame) {
    Bitmap b(40, 44);
    float fy = frame ? 5.f : 8.f;
    b.rect(14, 6, 2, 32, 4);
    b.poly({{16, fy}, {32, fy + 5}, {16, fy + 13}}, 7);
    b.poly({{16, fy + 1}, {26, fy + 5}, {16, fy + 8}}, 2);
    b.ellipse(12, 8, 4.2f, 4.f, 1);
    b.ellipse(12, 6, 4.f, 2.2f, 8);
    b.rect(7, 13, 10, 12, 2);
    b.rect(7, 22, 10, 2, 4);
    b.rect(8, 26, 3, 10, 3);
    b.rect(13, 26, 3, 9, 6);
    b.rect(7, 35, 5, 3, 6);
    b.rect(12, 34, 5, 3, 6);
    b.outline(14, false);
    return b;
}

Bitmap cartArt(int frame) {
    Bitmap b(46, 34);
    b.poly({{6, 16}, {12, 8}, {34, 8}, {40, 16}}, 1);
    b.rect(6, 16, 34, 8, 2);
    b.rect(14, 11, 12, 7, 5);
    b.rect(4, 18, 6, 3, 3);
    float ox = frame ? 1.f : 0.f;
    b.ellipse(14 + ox, 26, 6, 6, 3);
    b.ellipse(32, 26, 6, 6, 3);
    b.ellipse(14 + ox, 26, 2.2f, 2.2f, 4);
    b.ellipse(32, 26, 2.2f, 2.2f, 4);
    b.line(14 + ox, 26, 14 + ox + (frame ? 3.f : -3.f), 22, 4, 1.2f);
    b.outline(13, false);
    return b;
}

Bitmap keeperArt(int pose) {
    Bitmap b(30, 42);
    b.ellipse(15, 8, 4.6f, 4.4f, 1);
    b.ellipse(15, 6, 4.2f, 2.4f, 9);
    b.rect(10, 13, 10, 12, 2);
    b.rect(10, 13, 3, 12, 3);
    b.rect(13, 21, 4, 3, 4);
    if (pose == 0) {
        b.rect(6, 15, 4, 10, 2);
        b.rect(20, 15, 4, 10, 2);
        b.rect(11, 25, 4, 10, 3);
        b.rect(16, 25, 4, 10, 6);
        b.rect(10, 34, 6, 3, 6);
        b.rect(16, 34, 6, 3, 6);
    } else {
        b.rect(5, 3, 4, 12, 2);
        b.rect(21, 3, 4, 12, 2);
        b.ellipse(7, 3, 2.2f, 2.f, 1);
        b.ellipse(23, 3, 2.2f, 2.f, 1);
        b.rect(9, 24, 5, 8, 3);
        b.rect(16, 24, 5, 8, 3);
        b.rect(8, 31, 7, 4, 6);
        b.rect(16, 31, 7, 4, 6);
        b.rect(12, 16, 6, 3, 5);
    }
    b.outline(14, false);
    return b;
}

Bitmap chevArt() {
    Bitmap b(16, 16);
    b.poly({{8, 1}, {15, 10}, {11, 10}, {11, 15}, {5, 15}, {5, 10}, {1, 10}}, 2);
    b.poly({{8, 3}, {12, 9}, {9, 9}, {9, 13}, {7, 13}, {7, 9}, {4, 9}}, 1);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(64, 24);
    b.ellipse(18, 14, 14, 8, 1);
    b.ellipse(34, 12, 16, 9, 1);
    b.ellipse(50, 14, 12, 7, 1);
    b.ellipse(32, 11, 10, 6, 2);
    return b;
}

Bitmap flameArt(int frame) {
    Bitmap b(12, 16);
    b.poly({{6, 1}, {11, 10}, {8, 9}, {10, 15}, {2, 15}, {5, 8}, {1, 9}}, frame ? 5 : 6);
    b.poly({{6, 5}, {8, 12}, {4, 12}}, 1);
    return b;
}

Bitmap dustArt() {
    Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4, 3);
    b.ellipse(5, 5, 2.4f, 2, 1);
    return b;
}

Bitmap glintArt() {
    Bitmap b(14, 6);
    b.ellipse(7, 3, 6, 2.2f, 4);
    b.ellipse(5, 3, 2.2f, 1.1f, 1);
    return b;
}

Bitmap streakArt() {
    Bitmap b(22, 4);
    b.rect(0, 1, 22, 2, 1);
    b.rect(4, 0, 10, 1, 2);
    return b;
}

Bitmap pennantArt() {
    Bitmap b(18, 22);
    b.rect(2, 1, 2, 20, 4);
    b.poly({{4, 3}, {16, 8}, {4, 13}}, 7);
    b.outline(14, false);
    return b;
}

Bitmap sunArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 5);
    b.ellipse(14, 14, 7, 7, 6);
    return b;
}

Bitmap birdArt() {
    Bitmap b(14, 8);
    b.line(1, 6, 7, 2, 1, 1.3f);
    b.line(7, 2, 13, 6, 1, 1.3f);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 15);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t out = gs::rgb4(1, 1, 2);

    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(10, 11, 12), gs::rgb4(15, 14, 12), gs::rgb4(8, 3, 3),
                          gs::rgb4(4, 12, 6), gs::rgb4(14, 12, 6), gs::rgb4(4, 7, 12), gs::rgb4(6, 6, 7),
                          0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(13, 9, 4), gs::rgb4(9, 6, 3), gs::rgb4(5, 3, 2), gs::rgb4(4, 4, 5),
                           gs::rgb4(12, 11, 8), gs::rgb4(2, 2, 2), gs::rgb4(6, 8, 4), 0, 0, 0, 0, 0, 0, 0, out});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(14, 10, 7), gs::rgb4(12, 3, 3), gs::rgb4(7, 2, 2), gs::rgb4(4, 3, 2),
                           gs::rgb4(12, 13, 14), gs::rgb4(2, 2, 2), gs::rgb4(15, 12, 3), gs::rgb4(4, 3, 2),
                           0, 0, 0, 0, 0, out, shadow});
    setPal(vdp, PAL_JACK, {0, gs::rgb4(14, 10, 7), gs::rgb4(4, 7, 12), gs::rgb4(2, 4, 7), gs::rgb4(14, 11, 4),
                           gs::rgb4(15, 14, 8), gs::rgb4(2, 2, 2), gs::rgb4(3, 3, 4), 0, gs::rgb4(3, 2, 2),
                           0, 0, 0, 0, out, shadow});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(10, 11, 12), gs::rgb4(6, 7, 8), gs::rgb4(3, 4, 5), gs::rgb4(4, 8, 4),
                           gs::rgb4(3, 6, 3), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, out});
    setPal(vdp, PAL_CART, {0, gs::rgb4(13, 12, 8), gs::rgb4(8, 7, 4), gs::rgb4(3, 3, 3), gs::rgb4(12, 9, 4),
                           gs::rgb4(9, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, out, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 15), gs::rgb4(14, 12, 6), gs::rgb4(8, 7, 5), gs::rgb4(10, 14, 14),
                         gs::rgb4(14, 10, 4), gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROPE, {0, gs::rgb4(11, 9, 5), gs::rgb4(6, 4, 2), gs::rgb4(14, 12, 6), gs::rgb4(15, 14, 10),
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    auto textPal = [&](int pal, uint16_t c) { setPal(vdp, pal, {0, c, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow}); };
    textPal(PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(PAL_ALERT, gs::rgb4(15, 4, 3));
    textPal(PAL_GOOD, gs::rgb4(8, 15, 7));
    textPal(PAL_TITLE, gs::rgb4(15, 12, 5));

    art.plank = gs::uploadMipped(vdp, plankArt());
    art.rope = gs::uploadMipped(vdp, ropeArt(false));
    art.taut = gs::uploadMipped(vdp, ropeArt(true));
    art.link = gs::uploadMipped(vdp, linkArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.cliff = gs::uploadMipped(vdp, cliffArt());
    art.wall = gs::uploadMipped(vdp, wallArt());
    art.lip = gs::uploadMipped(vdp, lipArt());
    art.march[0] = gs::uploadMipped(vdp, marcher(0));
    art.march[1] = gs::uploadMipped(vdp, marcher(1));
    art.banner[0] = gs::uploadMipped(vdp, bannerMan(0));
    art.banner[1] = gs::uploadMipped(vdp, bannerMan(1));
    art.cart[0] = gs::uploadMipped(vdp, cartArt(0));
    art.cart[1] = gs::uploadMipped(vdp, cartArt(1));
    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.chev = gs::uploadMipped(vdp, chevArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.glint = gs::uploadMipped(vdp, glintArt());
    art.streak = gs::uploadMipped(vdp, streakArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.bird = gs::uploadMipped(vdp, birdArt());

    gs::TextStyle big{3, 1, 0, 15, 1};
    art.title = gs::uploadMipped(vdp, gs::textBitmap("S3 SPAN", big));
    art.across = gs::uploadMipped(vdp, gs::textBitmap("ACROSS", big));
    art.broke = gs::uploadMipped(vdp, gs::textBitmap("BROKE", big));
    art.paused = gs::uploadMipped(vdp, gs::textBitmap("PAUSED", big));
    loadFont(vdp, art);
}

}  // namespace span
