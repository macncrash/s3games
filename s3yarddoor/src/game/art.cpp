#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace yarddoor {
namespace {

constexpr float TAU = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
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

gs::Bitmap leafArt() {
    gs::Bitmap b(40, 96);
    for (int i = 0; i < 5; i++) {
        int x = 2 + i * 7;
        b.rect(float(x), 2, 7, 92, (i & 1) ? 2 : 1);
        b.rect(float(x), 2, 2, 92, 7);
        b.rect(float(x + 5), 2, 2, 92, 3);
        b.set(x + 3, 18, 8);
        b.set(x + 3, 70, 8);
    }
    b.line(6, 24, 34, 74, 3, 2.4f);
    b.line(6, 74, 34, 24, 3, 2.4f);
    b.line(8, 26, 32, 72, 9, 1.1f);
    b.rect(2, 42, 36, 7, 5);
    b.rect(2, 42, 36, 2, 6);
    b.rect(26, 34, 10, 18, 5);
    b.rect(28, 36, 6, 14, 6);
    b.rect(30, 40, 2, 6, 4);
    b.rect(8, 86, 4, 8, 5);
    b.rect(28, 86, 4, 8, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(18, 100);
    b.rect(4, 10, 10, 82, 2);
    b.rect(4, 10, 3, 82, 1);
    b.rect(12, 10, 2, 82, 3);
    for (int y = 22; y < 86; y += 16) b.rect(5, float(y), 8, 2, 8);
    b.rect(2, 2, 14, 10, 5);
    b.rect(3, 3, 12, 3, 6);
    b.rect(7, 5, 3, 3, 4);
    b.rect(5, 88, 8, 10, 3);
    b.rect(3, 94, 12, 4, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(96, 16);
    b.rect(1, 3, 94, 11, 2);
    b.rect(1, 3, 94, 3, 1);
    b.rect(1, 11, 94, 3, 3);
    for (int i = 0; i < 8; i++) b.rect(float(6 + i * 11), 5, 3, 7, 5);
    b.rect(44, 1, 8, 4, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap cribArt() {
    gs::Bitmap b(36, 100);
    for (int i = 0; i < 8; i++) {
        int y = 4 + i * 12;
        int tone = (i & 1) ? 2 : 1;
        b.rect(3, float(y), 30, 10, tone);
        b.rect(3, float(y), 30, 2, 7);
        b.rect(3, float(y + 8), 30, 2, 3);
        b.rect(6, float(y + 3), 4, 4, 4);
        b.rect(26, float(y + 3), 4, 4, 4);
        if (i % 3 == 1) b.rect(15, float(y + 4), 5, 3, 8);
    }
    b.rect(0, 2, 4, 96, 5);
    b.rect(32, 2, 4, 96, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap handArt(int frame) {
    gs::Bitmap b(36, 58);
    b.ellipse(18, 8, 7, 4, 4);
    b.rect(12, 10, 12, 3, 4);
    b.ellipse(18, 16, 6, 6, 3);
    b.set(16, 16, 8);
    b.set(21, 16, 8);
    b.rect(16, 19, 5, 2, 2);
    b.poly({{10, 23}, {26, 23}, {28, 40}, {8, 40}}, 1);
    b.poly({{15, 24}, {22, 24}, {21, 38}, {15, 38}}, 9);
    b.rect(14, 36, 8, 3, 10);
    if (frame == 0) {
        b.line(12, 26, 8, 40, 1, 3.0f);
        b.ellipse(7, 42, 3, 3, 7);
        b.line(8, 44, 6, 54, 6, 2.2f);
        b.line(6, 52, 3, 50, 6, 2.0f);
        b.rect(10, 40, 5, 12, 2);
        b.rect(20, 40, 5, 12, 1);
        b.rect(9, 50, 7, 4, 5);
        b.rect(19, 50, 7, 4, 5);
    } else {
        b.line(12, 26, 2, 30, 1, 3.2f);
        b.line(2, 30, 1, 26, 6, 2.4f);
        b.line(1, 24, 6, 22, 6, 2.0f);
        b.ellipse(2, 32, 3, 3, 7);
        b.line(24, 26, 32, 34, 1, 3.0f);
        b.rect(9, 40, 6, 11, 2);
        b.rect(20, 40, 6, 12, 1);
        b.rect(7, 49, 8, 4, 5);
        b.rect(20, 51, 8, 4, 5);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap breakerArt(int frame) {
    gs::Bitmap b(24, 46);
    b.ellipse(12, 6, 5, 3, 4);
    b.ellipse(12, 12, 5, 5, 3);
    b.set(10, 12, 8);
    b.set(14, 12, 8);
    b.rect(8, 16, 8, 12, 1);
    b.rect(9, 17, 3, 9, 2);
    b.rect(8, 27, 8, 3, 10);
    if (frame == 0) {
        b.line(8, 18, 3, 28, 1, 2.6f);
        b.line(16, 18, 21, 28, 1, 2.6f);
        b.rect(7, 30, 4, 10, 5);
        b.rect(13, 30, 4, 10, 5);
    } else {
        b.line(8, 18, 1, 20, 1, 2.8f);
        b.line(16, 18, 22, 22, 1, 2.6f);
        b.ellipse(2, 20, 2, 2, 3);
        b.rect(7, 30, 4, 9, 5);
        b.rect(14, 30, 4, 11, 5);
    }
    b.rect(6, 39, 6, 3, 6);
    b.rect(13, 40, 6, 3, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap logArt() {
    gs::Bitmap b(52, 18);
    b.rect(6, 4, 40, 10, 2);
    b.rect(6, 4, 40, 3, 1);
    b.rect(6, 11, 40, 3, 3);
    for (int x = 12; x < 44; x += 8) b.set(x, 8, 8);
    b.ellipse(6, 9, 5, 7, 4);
    b.ellipse(6, 9, 2, 3, 8);
    b.ellipse(46, 9, 5, 7, 4);
    b.ellipse(46, 9, 2, 3, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(28, 8);
    b.rect(0, 2, 28, 4, 1);
    b.rect(0, 2, 28, 1, 4);
    b.rect(0, 5, 28, 1, 3);
    b.rect(22, 1, 5, 6, 2);
    b.set(4, 3, 5);
    b.set(12, 4, 5);
    b.set(18, 3, 5);
    return b;
}

gs::Bitmap chockArt() {
    gs::Bitmap b(20, 12);
    b.poly({{1, 10}, {18, 10}, {18, 3}}, 2);
    b.line(2, 9, 16, 4, 1, 1.4f);
    b.rect(14, 4, 3, 6, 5);
    b.set(8, 8, 8);
    b.outline(15, false);
    return b;
}

gs::Bitmap lanternArt() {
    gs::Bitmap b(12, 16);
    b.rect(5, 0, 2, 3, 2);
    b.rect(2, 3, 8, 9, 1);
    b.rect(3, 4, 6, 7, 3);
    b.rect(4, 12, 4, 3, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(8, 10);
    b.poly({{4, 0}, {7, 6}, {4, 9}, {1, 6}}, 1);
    b.poly({{4, 3}, {6, 7}, {4, 9}, {2, 7}}, 2);
    b.set(4, 7, 3);
    return b;
}

gs::Bitmap hoistArt() {
    gs::Bitmap b(22, 32);
    b.ellipse(11, 7, 6, 6, 1);
    b.ellipse(11, 7, 2, 2, 3);
    b.rect(10, 1, 2, 4, 2);
    b.line(11, 13, 11, 26, 2, 1.6f);
    b.poly({{7, 26}, {15, 26}, {13, 30}, {9, 30}}, 5);
    b.rect(8, 28, 6, 2, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap pennantArt(int frame) {
    gs::Bitmap b(18, 12);
    b.rect(1, 1, 2, 10, 3);
    if (frame == 0) b.poly({{3, 2}, {16, 4}, {3, 8}}, 1);
    else b.poly({{3, 3}, {15, 2}, {12, 7}, {3, 9}}, 1);
    b.line(4, 4, 12, 5, 2, 1.0f);
    return b;
}

gs::Bitmap kegArt() {
    gs::Bitmap b(16, 20);
    b.ellipse(8, 4, 6, 3, 2);
    b.rect(2, 4, 12, 12, 1);
    b.rect(2, 4, 3, 12, 7);
    b.rect(2, 8, 12, 2, 5);
    b.rect(2, 13, 12, 2, 5);
    b.ellipse(8, 16, 6, 3, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap coilArt() {
    gs::Bitmap b(18, 16);
    b.ellipse(9, 8, 7, 6, 1);
    b.ellipse(9, 8, 4, 3, 2);
    b.ellipse(9, 8, 2, 1, 4);
    b.line(2, 8, 0, 13, 1, 1.5f);
    b.outline(15, false);
    return b;
}

gs::Bitmap horseArt() {
    gs::Bitmap b(32, 20);
    b.line(4, 16, 10, 4, 5, 2.2f);
    b.line(16, 16, 10, 4, 5, 2.2f);
    b.line(16, 16, 22, 4, 5, 2.2f);
    b.line(28, 16, 22, 4, 5, 2.2f);
    b.rect(6, 3, 20, 4, 1);
    b.rect(6, 3, 20, 1, 7);
    b.outline(15, false);
    return b;
}

gs::Bitmap barrowArt() {
    gs::Bitmap b(28, 18);
    b.poly({{4, 6}, {22, 6}, {20, 12}, {6, 12}}, 1);
    b.rect(4, 6, 18, 2, 7);
    b.line(6, 12, 2, 16, 5, 1.8f);
    b.line(18, 12, 24, 16, 5, 1.8f);
    b.ellipse(22, 14, 4, 4, 4);
    b.ellipse(22, 14, 1, 1, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.ellipse(4, 4, 1, 1, 2);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(6, 40);
    b.rect(1, 0, 4, 40, 1);
    b.rect(2, 0, 2, 40, 2);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(8, 8);
    b.line(1, 4, 7, 4, 1, 1.4f);
    b.line(4, 1, 4, 7, 2, 1.4f);
    b.set(4, 4, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 10);
    b.ellipse(16, 5, 14, 4, 1);
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(32, 6);
    b.rect(0, 2, 32, 3, 2);
    b.rect(0, 2, 32, 1, 1);
    for (int x = 3; x < 32; x += 8) b.set(x, 3, 4);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(44, 16);
    b.ellipse(14, 9, 10, 5, 3);
    b.ellipse(26, 8, 11, 6, 4);
    b.ellipse(34, 10, 7, 4, 3);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 6, 6, 1);
    b.ellipse(11, 11, 3, 3, 2);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * TAU / 8.f;
        b.line(11 + std::cos(a) * 7, 11 + std::sin(a) * 7, 11 + std::cos(a) * 10, 11 + std::sin(a) * 10, 1, 1.3f);
    }
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(2, 1, 1);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 15, 13), gs::rgb4(12, 11, 9), gs::rgb4(4, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 11), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(14, 3, 2), gs::rgb4(15, 12, 8), gs::rgb4(4, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 14, 5), gs::rgb4(14, 15, 12), gs::rgb4(1, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(13, 9, 4), gs::rgb4(9, 6, 3), gs::rgb4(5, 3, 2), gs::rgb4(7, 5, 3), gs::rgb4(8, 8, 9),
            gs::rgb4(14, 13, 11), gs::rgb4(15, 11, 6), gs::rgb4(4, 2, 1), gs::rgb4(11, 8, 4), gs::rgb4(6, 4, 2), 0, 0, 0,
            0, ink});
    setPal(vdp, PAL_COAT,
           {0, gs::rgb4(4, 6, 9), gs::rgb4(2, 3, 6), gs::rgb4(13, 9, 6), gs::rgb4(4, 3, 2), gs::rgb4(3, 2, 2),
            gs::rgb4(10, 10, 11), gs::rgb4(8, 6, 3), gs::rgb4(1, 1, 1), gs::rgb4(8, 9, 11), gs::rgb4(6, 4, 2), 0, 0, 0, 0,
            ink});
    setPal(vdp, PAL_IRON,
           {0, gs::rgb4(12, 12, 13), gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5), gs::rgb4(15, 14, 12), gs::rgb4(9, 6, 3),
            gs::rgb4(14, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BREAK,
           {0, gs::rgb4(12, 3, 2), gs::rgb4(7, 2, 2), gs::rgb4(12, 8, 5), gs::rgb4(3, 2, 2), gs::rgb4(3, 3, 5),
            gs::rgb4(2, 2, 2), gs::rgb4(9, 9, 8), gs::rgb4(1, 1, 1), 0, gs::rgb4(6, 4, 2), 0, 0, 0, 0, ink});
    setPal(vdp, PAL_PROP,
           {0, gs::rgb4(10, 8, 4), gs::rgb4(6, 4, 2), 0, gs::rgb4(8, 8, 9), gs::rgb4(9, 6, 3), 0, gs::rgb4(13, 10, 6), 0,
            0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 2), gs::rgb4(12, 4, 1), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 12), gs::rgb4(13, 10, 8), gs::rgb4(15, 13, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(14, 12, 8), gs::rgb4(10, 8, 5), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(10, 8, 4), gs::rgb4(7, 5, 3), gs::rgb4(12, 9, 5),
        gs::rgb4(8, 6, 3), gs::rgb4(5, 4, 2),
        gs::rgb4(9, 7, 4), gs::rgb4(6, 5, 3),
        gs::rgb4(4, 3, 2), gs::rgb4(8, 6, 3), gs::rgb4(12, 10, 6),
        gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6),
        gs::rgb4(13, 11, 6), gs::rgb4(11, 9, 5),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.leaf = gs::uploadMipped(vdp, leafArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.crib = gs::uploadMipped(vdp, cribArt());
    art.hand[0] = gs::uploadMipped(vdp, handArt(0));
    art.hand[1] = gs::uploadMipped(vdp, handArt(1));
    art.breaker[0] = gs::uploadMipped(vdp, breakerArt(0));
    art.breaker[1] = gs::uploadMipped(vdp, breakerArt(1));
    art.log = gs::uploadMipped(vdp, logArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.chock = gs::uploadMipped(vdp, chockArt());
    art.lantern = gs::uploadMipped(vdp, lanternArt());
    art.flame = gs::uploadMipped(vdp, flameArt());
    art.hoist = gs::uploadMipped(vdp, hoistArt());
    art.pennant[0] = gs::uploadMipped(vdp, pennantArt(0));
    art.pennant[1] = gs::uploadMipped(vdp, pennantArt(1));
    art.keg = gs::uploadMipped(vdp, kegArt());
    art.coil = gs::uploadMipped(vdp, coilArt());
    art.horse = gs::uploadMipped(vdp, horseArt());
    art.barrow = gs::uploadMipped(vdp, barrowArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.rail = gs::uploadMipped(vdp, railArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(8, 6, 4));
}

}  // namespace yarddoor
