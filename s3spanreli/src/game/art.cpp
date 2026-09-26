#include "game/art.h"

#include <initializer_list>

namespace spanreli {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap plankArt() {
    Bitmap b(44, 12);
    b.rect(1, 2, 42, 8, 2);
    b.rect(1, 2, 42, 2, 1);
    for (int x = 4; x < 40; x += 8) b.rect(float(x), 3, 1, 6, 3);
    b.rect(8, 7, 3, 2, 6);
    b.rect(30, 6, 3, 2, 6);
    b.outline(7, false);
    return b;
}

Bitmap hangerArt(int kind) {
    Bitmap b(6, 32);
    int body = kind == 0 ? 5 : (kind == 1 ? 4 : 2);
    int edge = kind == 2 ? 1 : 6;
    b.rect(2, 0, 2, 32, body);
    b.rect(1, 0, 1, 32, edge);
    if (kind == 1) b.rect(4, 4, 1, 24, 6);
    return b;
}

Bitmap linkArt() {
    Bitmap b(8, 8);
    b.rect(3, 1, 2, 6, 4);
    b.rect(2, 2, 4, 4, 5);
    b.rect(3, 3, 2, 2, 0);
    b.set(2, 1, 6);
    b.set(5, 6, 6);
    return b;
}

Bitmap yokeArt() {
    Bitmap b(26, 8);
    b.rect(1, 2, 24, 3, 4);
    b.rect(2, 5, 3, 3, 5);
    b.rect(21, 5, 3, 3, 5);
    b.rect(11, 5, 4, 3, 5);
    return b;
}

Bitmap pierArt() {
    Bitmap b(34, 96);
    b.poly({{6, 8}, {28, 8}, {32, 94}, {2, 94}}, 2);
    b.rect(8, 12, 18, 76, 1);
    b.rect(4, 6, 26, 5, 3);
    for (int y = 20; y < 86; y += 14) {
        b.rect(6, float(y), 22, 3, 5);
        b.rect(15, float(y + 3), 4, 8, 3);
    }
    b.rect(10, 88, 14, 6, 3);
    b.outline(6, false);
    return b;
}

Bitmap belfryArt() {
    Bitmap b(40, 108);
    b.poly({{8, 16}, {32, 16}, {36, 106}, {4, 106}}, 2);
    b.rect(10, 20, 20, 78, 1);
    b.rect(6, 10, 28, 6, 3);
    b.rect(12, 4, 16, 8, 5);
    b.rect(14, 18, 12, 16, 0);
    b.rect(12, 18, 2, 16, 3);
    b.rect(26, 18, 2, 16, 3);
    b.rect(12, 32, 16, 3, 3);
    for (int y = 42; y < 96; y += 16) {
        b.rect(8, float(y), 24, 3, 5);
        b.rect(18, float(y + 3), 4, 9, 3);
    }
    b.outline(6, false);
    return b;
}

Bitmap cliffArt() {
    Bitmap b(56, 110);
    b.poly({{0, 28}, {16, 12}, {34, 22}, {56, 8}, {56, 109}, {0, 109}}, 2);
    b.poly({{0, 40}, {20, 26}, {40, 36}, {56, 22}, {56, 70}, {0, 78}}, 1);
    b.rect(0, 24, 56, 4, 4);
    for (int y = 48; y < 100; y += 14) b.line(4, float(y), 48, float(y + 6), 3, 1.2f);
    b.rect(18, 16, 3, 14, 3);
    b.rect(40, 12, 3, 12, 5);
    return b;
}

Bitmap wallArt() {
    Bitmap b(96, 72);
    b.rect(0, 16, 96, 56, 3);
    b.poly({{0, 28}, {24, 10}, {60, 18}, {96, 8}, {96, 40}, {0, 48}}, 2);
    for (int x = 8; x < 90; x += 16) b.line(float(x), 20, float(x + 3), 68, 5, 1.1f);
    return b;
}

Bitmap lipArt() {
    Bitmap b(48, 16);
    b.rect(0, 2, 48, 10, 2);
    b.rect(0, 2, 48, 3, 1);
    b.rect(0, 11, 48, 3, 3);
    for (int x = 6; x < 44; x += 9) b.rect(float(x), 6, 2, 4, 5);
    b.outline(6, false);
    return b;
}

Bitmap coatArt(int frame) {
    Bitmap b(20, 36);
    b.ellipse(10, 6, 3.6f, 3.4f, 1);
    b.rect(7, 3, 6, 3, 5);
    b.rect(6, 10, 8, 12, 2);
    b.rect(6, 10, 3, 12, 3);
    b.rect(6, 20, 8, 2, 7);
    if (frame == 0) {
        b.rect(7, 23, 3, 8, 4);
        b.rect(11, 23, 3, 6, 3);
        b.rect(6, 30, 5, 2, 4);
        b.rect(11, 28, 5, 2, 4);
    } else {
        b.rect(7, 23, 3, 6, 3);
        b.rect(11, 23, 3, 8, 4);
        b.rect(6, 28, 5, 2, 4);
        b.rect(11, 30, 5, 2, 4);
    }
    b.line(15, 8, 16, 24, 5, 1.2f);
    b.outline(8, false);
    return b;
}

Bitmap drumArt(int frame) {
    Bitmap b(26, 38);
    b.ellipse(10, 6, 3.6f, 3.3f, 1);
    b.rect(7, 3, 6, 3, 5);
    b.rect(6, 10, 8, 11, 2);
    b.rect(6, 19, 8, 2, 7);
    b.ellipse(15, 18, 5.2f, 3.4f, 6);
    b.ellipse(15, 18, 2.4f, 1.4f, 3);
    b.line(11, 12, 18, 8, 5, 1.1f);
    float step = frame ? 1.f : 0.f;
    b.rect(7, 23 + step, 3, 8, 4);
    b.rect(11, 23, 3, 7, 3);
    b.rect(6, 31, 5, 2, 4);
    b.outline(8, false);
    return b;
}

Bitmap wagonArt(int frame) {
    Bitmap b(46, 30);
    b.poly({{8, 12}, {14, 6}, {34, 6}, {38, 12}}, 1);
    b.rect(6, 12, 34, 7, 2);
    b.rect(16, 8, 10, 5, 3);
    b.rect(28, 8, 6, 5, 3);
    float ox = frame ? 2.f : 0.f;
    b.ellipse(14 + ox, 22, 5.5f, 5.5f, 4);
    b.ellipse(32, 22, 5.5f, 5.5f, 4);
    b.ellipse(14 + ox, 22, 2.f, 2.f, 5);
    b.ellipse(32, 22, 2.f, 2.f, 5);
    b.outline(6, false);
    return b;
}

Bitmap keeperArt(int frame) {
    Bitmap b(22, 40);
    b.rect(7, 2, 8, 4, 6);
    b.ellipse(11, 8, 4.f, 3.6f, 1);
    b.rect(6, 12, 10, 13, 2);
    b.rect(6, 12, 3, 13, 3);
    b.rect(6, 23, 10, 2, 9);
    if (frame == 0) {
        b.rect(7, 26, 3, 9, 7);
        b.rect(12, 26, 3, 8, 2);
        b.rect(6, 34, 5, 2, 7);
        b.rect(12, 33, 5, 2, 7);
        b.rect(16, 16, 4, 6, 4);
        b.rect(17, 14, 2, 3, 5);
    } else {
        b.rect(7, 26, 3, 8, 7);
        b.rect(12, 26, 3, 9, 2);
        b.rect(5, 33, 6, 2, 7);
        b.rect(12, 34, 5, 2, 7);
        b.line(4, 14, 8, 8, 3, 1.4f);
        b.line(18, 14, 14, 8, 3, 1.4f);
        b.rect(15, 24, 3, 4, 4);
        b.set(16, 23, 5);
    }
    b.outline(8, false);
    return b;
}

Bitmap bellArt() {
    Bitmap b(18, 20);
    b.poly({{9, 1}, {14, 4}, {15, 12}, {12, 16}, {6, 16}, {3, 12}, {4, 4}}, 2);
    b.poly({{9, 3}, {12, 6}, {12, 12}, {9, 14}, {6, 12}, {6, 6}}, 1);
    b.rect(5, 15, 8, 3, 3);
    b.rect(8, 8, 2, 6, 4);
    b.rect(7, 0, 4, 3, 5);
    b.outline(6, false);
    return b;
}

Bitmap ropeArt() {
    Bitmap b(4, 28);
    b.rect(1, 0, 2, 28, 5);
    b.rect(2, 0, 1, 28, 4);
    return b;
}

Bitmap pennantArt() {
    Bitmap b(14, 12);
    b.poly({{1, 1}, {12, 5}, {1, 10}}, 1);
    b.rect(1, 1, 2, 10, 2);
    return b;
}

Bitmap flameArt(int frame) {
    Bitmap b(8, 12);
    float h = frame ? 2.f : 0.f;
    b.poly({{4, h}, {7, 7}, {5, 11}, {3, 11}, {1, 7}}, 2);
    b.poly({{4, h + 3}, {6, 8}, {4, 10}, {2, 8}}, 1);
    return b;
}

Bitmap chevArt() {
    Bitmap b(14, 12);
    b.poly({{7, 1}, {12, 10}, {7, 7}, {2, 10}}, 1);
    return b;
}

Bitmap streakArt() {
    Bitmap b(16, 4);
    b.rect(0, 1, 16, 2, 1);
    b.rect(4, 0, 8, 1, 4);
    return b;
}

Bitmap dustArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3.f, 2.4f, 1);
    b.ellipse(4, 4, 1.4f, 1.f, 3);
    return b;
}

Bitmap moonArt() {
    Bitmap b(20, 20);
    b.ellipse(10, 10, 8.f, 8.f, 2);
    b.ellipse(13, 8, 5.f, 5.f, 0);
    b.set(6, 6, 1);
    b.set(8, 12, 1);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(36, 14);
    b.ellipse(12, 8, 10.f, 5.f, 3);
    b.ellipse(22, 7, 9.f, 4.5f, 3);
    b.ellipse(18, 6, 6.f, 3.f, 1);
    return b;
}

Bitmap birdArt() {
    Bitmap b(14, 8);
    b.line(1, 6, 7, 2, 7, 1.2f);
    b.line(7, 2, 13, 6, 7, 1.2f);
    return b;
}

Bitmap glintArt() {
    Bitmap b(6, 4);
    b.rect(1, 1, 4, 2, 4);
    b.set(2, 1, 1);
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
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 11, 12), gs::rgb4(15, 14, 12), gs::rgb4(8, 3, 3),
                          gs::rgb4(4, 12, 6), gs::rgb4(14, 12, 6), gs::rgb4(4, 7, 12), gs::rgb4(6, 6, 7), 0, 0, 0, 0,
                          0, 0, shadow});
    setPal(vdp, PAL_DECK, {0, gs::rgb4(13, 9, 5), gs::rgb4(9, 6, 3), gs::rgb4(5, 3, 2), gs::rgb4(11, 12, 13),
                           gs::rgb4(5, 6, 7), gs::rgb4(14, 12, 6), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(14, 11, 8), gs::rgb4(7, 9, 11), gs::rgb4(4, 5, 7), gs::rgb4(2, 2, 3),
                           gs::rgb4(12, 13, 14), gs::rgb4(12, 8, 4), gs::rgb4(6, 5, 3), gs::rgb4(1, 1, 2),
                           gs::rgb4(8, 6, 3), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_KEEPER, {0, gs::rgb4(14, 11, 8), gs::rgb4(3, 4, 8), gs::rgb4(5, 7, 12), gs::rgb4(14, 11, 4),
                             gs::rgb4(15, 13, 6), gs::rgb4(1, 1, 2), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2),
                             gs::rgb4(8, 6, 3), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(11, 12, 13), gs::rgb4(7, 8, 9), gs::rgb4(4, 4, 5), gs::rgb4(4, 7, 4),
                            gs::rgb4(5, 5, 6), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WAGON, {0, gs::rgb4(11, 12, 10), gs::rgb4(8, 5, 3), gs::rgb4(11, 8, 4), gs::rgb4(3, 3, 3),
                            gs::rgb4(8, 8, 8), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(15, 14, 8), gs::rgb4(13, 10, 3), gs::rgb4(8, 6, 2), gs::rgb4(4, 3, 3),
                           gs::rgb4(15, 15, 12), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(15, 15, 15), gs::rgb4(14, 12, 6), gs::rgb4(8, 9, 12), gs::rgb4(6, 12, 13),
                            gs::rgb4(12, 13, 15), gs::rgb4(1, 1, 3), gs::rgb4(3, 3, 5), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_HEAT, {0, gs::rgb4(15, 8, 2), gs::rgb4(12, 3, 1), gs::rgb4(8, 2, 1), gs::rgb4(15, 13, 4),
                           gs::rgb4(6, 2, 1), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    auto textPal = [&](int pal, uint16_t c) { setPal(vdp, pal, {0, c, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow}); };
    textPal(PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(PAL_ALERT, gs::rgb4(15, 4, 3));
    textPal(PAL_GOOD, gs::rgb4(8, 15, 7));
    textPal(PAL_TITLE, gs::rgb4(15, 13, 6));
    textPal(PAL_DIM, gs::rgb4(9, 10, 12));

    art.plank = gs::uploadMipped(vdp, plankArt());
    art.hanger = gs::uploadMipped(vdp, hangerArt(0));
    art.taut = gs::uploadMipped(vdp, hangerArt(1));
    art.hot = gs::uploadMipped(vdp, hangerArt(2));
    art.link = gs::uploadMipped(vdp, linkArt());
    art.yoke = gs::uploadMipped(vdp, yokeArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.belfry = gs::uploadMipped(vdp, belfryArt());
    art.cliff = gs::uploadMipped(vdp, cliffArt());
    art.wall = gs::uploadMipped(vdp, wallArt());
    art.lip = gs::uploadMipped(vdp, lipArt());
    art.coat[0] = gs::uploadMipped(vdp, coatArt(0));
    art.coat[1] = gs::uploadMipped(vdp, coatArt(1));
    art.drum[0] = gs::uploadMipped(vdp, drumArt(0));
    art.drum[1] = gs::uploadMipped(vdp, drumArt(1));
    art.wagon[0] = gs::uploadMipped(vdp, wagonArt(0));
    art.wagon[1] = gs::uploadMipped(vdp, wagonArt(1));
    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.chev = gs::uploadMipped(vdp, chevArt());
    art.streak = gs::uploadMipped(vdp, streakArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.bird = gs::uploadMipped(vdp, birdArt());
    art.glint = gs::uploadMipped(vdp, glintArt());

    gs::TextStyle big{3, 1, 0, 15, 1};
    art.title = gs::uploadMipped(vdp, gs::textBitmap("S3 SPAN", big));
    art.held = gs::uploadMipped(vdp, gs::textBitmap("HELD", big));
    art.broke = gs::uploadMipped(vdp, gs::textBitmap("BROKE", big));
    art.late = gs::uploadMipped(vdp, gs::textBitmap("LATE", big));
    art.paused = gs::uploadMipped(vdp, gs::textBitmap("PAUSED", big));
    loadFont(vdp, art);
}

}  // namespace spanreli
