#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace yardpace {
namespace {

constexpr float TAU = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap hand(int step) {
    gs::Bitmap b(40, 68);
    b.ellipse(20, 10, 9, 5, 5);
    b.rect(11, 13, 18, 3, 6);
    b.ellipse(20, 22, 6, 7, 4);
    b.set(17, 21, 8);
    b.set(23, 21, 8);
    b.rect(17, 25, 6, 2, 9);
    b.rect(17, 28, 6, 3, 4);
    b.poly({{10, 32}, {30, 32}, {33, 50}, {7, 50}}, 1);
    b.poly({{15, 33}, {25, 33}, {24, 47}, {16, 47}}, 2);
    b.rect(14, 47, 12, 3, 9);
    if (step == 0) {
        b.line(11, 34, 8, 46, 1, 3.2f);
        b.line(29, 34, 32, 46, 1, 3.2f);
        b.ellipse(8, 48, 3, 3, 4);
        b.ellipse(32, 48, 3, 3, 4);
        b.rect(12, 50, 6, 11, 1);
        b.rect(22, 50, 6, 11, 1);
        b.rect(11, 59, 8, 5, 3);
        b.rect(21, 59, 8, 5, 3);
    } else {
        b.line(11, 34, 5, 44, 1, 3.2f);
        b.line(29, 34, 35, 50, 1, 3.2f);
        b.ellipse(5, 46, 3, 3, 4);
        b.ellipse(35, 52, 3, 3, 4);
        b.rect(13, 50, 6, 8, 10);
        b.rect(22, 50, 6, 12, 1);
        b.rect(12, 56, 8, 5, 3);
        b.rect(21, 60, 8, 5, 3);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap downed() {
    gs::Bitmap b(80, 30);
    b.ellipse(14, 16, 7, 6, 4);
    b.ellipse(14, 9, 8, 4, 5);
    b.rect(8, 11, 12, 3, 6);
    b.poly({{22, 12}, {64, 14}, {68, 24}, {20, 22}}, 1);
    b.poly({{30, 13}, {50, 14}, {48, 22}, {28, 21}}, 2);
    b.rect(60, 20, 12, 5, 3);
    b.rect(22, 21, 8, 4, 3);
    b.outline(8, false);
    return b;
}

gs::Bitmap cribArt() {
    gs::Bitmap b(44, 120);
    for (int i = 0; i < 10; i++) {
        int y = 6 + i * 11;
        b.rect(3, y, 38, 9, (i & 1) ? 2 : 1);
        b.rect(5, y + 2, 6, 4, 4);
        b.rect(33, y + 2, 5, 4, 4);
        if (i % 3 == 1) b.rect(18, y + 3, 4, 3, 9);
    }
    b.rect(0, 2, 4, 114, 5);
    b.rect(40, 2, 4, 114, 5);
    b.rect(1, 2, 2, 114, 6);
    b.outline(8, false);
    return b;
}

gs::Bitmap balkArt() {
    gs::Bitmap b(40, 36);
    for (int i = 0; i < 4; i++) {
        int y = 2 + i * 8;
        b.rect(2, y, 36, 7, (i & 1) ? 1 : 2);
        b.rect(4, y + 2, 5, 3, 4);
        b.rect(30, y + 2, 5, 3, 4);
    }
    b.rect(0, 2, 3, 32, 5);
    b.rect(37, 2, 3, 32, 5);
    b.outline(8, false);
    return b;
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(16, 44);
    b.rect(6, 16, 4, 26, 3);
    b.rect(1, 2, 14, 16, 1);
    b.rect(1, 2, 14, 3, 2);
    b.rect(2, 9, 12, 3, 5);
    b.outline(8, false);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(72, 16);
    b.rect(0, 3, 72, 10, 2);
    b.rect(0, 3, 72, 3, 1);
    for (int x = 8; x < 70; x += 16) b.ellipse(float(x), 8, 2, 2, 5);
    b.outline(8, false);
    return b;
}

gs::Bitmap hoistArt() {
    gs::Bitmap b(16, 42);
    b.line(8, 0, 8, 12, 2, 1.6f);
    b.rect(3, 12, 10, 12, 1);
    b.rect(5, 14, 6, 8, 5);
    b.line(8, 24, 8, 34, 2, 1.6f);
    b.poly({{4, 34}, {12, 34}, {8, 40}}, 6);
    b.outline(8, false);
    return b;
}

gs::Bitmap coilArt() {
    gs::Bitmap b(30, 16);
    b.ellipse(15, 9, 12, 6, 3);
    b.ellipse(15, 9, 7, 3, 2);
    b.ellipse(15, 8, 3, 2, 1);
    b.outline(8, false);
    return b;
}

gs::Bitmap kegArt() {
    gs::Bitmap b(22, 28);
    b.ellipse(11, 6, 8, 4, 1);
    b.rect(3, 6, 16, 16, 2);
    b.rect(3, 9, 16, 3, 5);
    b.rect(3, 16, 16, 3, 5);
    b.ellipse(11, 22, 8, 4, 2);
    b.outline(8, false);
    return b;
}

gs::Bitmap horseArt() {
    gs::Bitmap b(40, 28);
    b.line(8, 4, 16, 24, 2, 3.f);
    b.line(32, 4, 24, 24, 2, 3.f);
    b.line(6, 5, 34, 5, 1, 3.f);
    b.rect(12, 12, 16, 3, 3);
    b.outline(8, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 52);
    b.rect(4, 4, 4, 42, 3);
    b.rect(5, 6, 2, 36, 2);
    b.rect(2, 0, 8, 5, 5);
    b.rect(2, 46, 8, 5, 3);
    b.outline(8, false);
    return b;
}

gs::Bitmap lanternArt() {
    gs::Bitmap b(14, 22);
    b.rect(6, 0, 2, 4, 3);
    b.rect(3, 4, 8, 10, 1);
    b.ellipse(7, 9, 3, 3, 2);
    b.rect(4, 14, 6, 5, 3);
    b.outline(3, false);
    return b;
}

gs::Bitmap barrowArt() {
    gs::Bitmap b(40, 28);
    b.poly({{8, 6}, {30, 5}, {32, 15}, {8, 16}}, 2);
    b.line(10, 15, 4, 24, 3, 2.2f);
    b.line(28, 15, 36, 24, 3, 2.2f);
    b.ellipse(12, 22, 4, 4, 5);
    b.ellipse(30, 22, 4, 4, 5);
    b.outline(8, false);
    return b;
}

gs::Bitmap pennantArt(int frame) {
    gs::Bitmap b(30, 18);
    b.rect(0, 0, 3, 18, 3);
    if (frame == 0) b.poly({{3, 2}, {26, 5}, {22, 10}, {3, 13}}, 1);
    else b.poly({{3, 3}, {24, 1}, {28, 9}, {3, 14}}, 1);
    b.poly({{3, 7}, {18, 8}, {16, 11}, {3, 12}}, 2);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(20, 20);
    for (int i = 0; i < 18; i++) {
        float a = i * TAU / 18.f;
        int x = int(std::lround(10 + std::cos(a) * 7));
        int y = int(std::lround(10 + std::sin(a) * 7));
        b.set(x, y, 1);
    }
    b.rect(9, 1, 2, 4, 1);
    b.rect(9, 15, 2, 4, 1);
    b.rect(1, 9, 4, 2, 1);
    b.rect(15, 9, 4, 2, 1);
    b.rect(9, 9, 2, 2, 2);
    return b;
}

gs::Bitmap rifleArt() {
    gs::Bitmap b(26, 62);
    b.rect(11, 0, 4, 32, 1);
    b.rect(10, 2, 2, 26, 2);
    b.rect(8, 30, 10, 6, 5);
    b.rect(7, 36, 12, 16, 3);
    b.rect(8, 38, 5, 12, 4);
    b.rect(5, 48, 16, 8, 6);
    b.outline(8, false);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(14, 14);
    b.poly({{7, 1}, {12, 7}, {7, 12}, {2, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 5, 2);
    b.ellipse(10, 10, 3, 3, 1);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(28, 14);
    b.ellipse(14, 8, 12, 5, 3);
    b.ellipse(9, 7, 5, 3, 4);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 10);
    b.ellipse(16, 5, 14, 4, 1);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(32, 8);
    b.rect(0, 2, 32, 4, 1);
    b.rect(0, 3, 32, 2, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(24, 24);
    b.ellipse(12, 12, 7, 7, 1);
    b.ellipse(12, 12, 3, 3, 2);
    for (int i = 0; i < 8; i++) {
        float a = i * TAU / 8.f;
        b.line(12 + std::cos(a) * 8, 12 + std::sin(a) * 8, 12 + std::cos(a) * 11, 12 + std::sin(a) * 11, 1, 1.4f);
    }
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(48, 18);
    b.ellipse(16, 11, 12, 6, 3);
    b.ellipse(30, 9, 12, 7, 4);
    b.ellipse(38, 12, 8, 5, 3);
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
    const uint16_t ink = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 15, 14), gs::rgb4(11, 10, 9), gs::rgb4(4, 4, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 11, 3), gs::rgb4(15, 15, 11), gs::rgb4(4, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(14, 3, 2), gs::rgb4(15, 10, 7), gs::rgb4(4, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 6), gs::rgb4(14, 15, 12), gs::rgb4(1, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(12, 8, 4), gs::rgb4(9, 6, 3), gs::rgb4(5, 3, 2), gs::rgb4(7, 5, 3), gs::rgb4(6, 6, 7),
            gs::rgb4(11, 11, 12), gs::rgb4(4, 6, 3), gs::rgb4(2, 1, 1), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FIGURE,
           {0, gs::rgb4(3, 5, 9), gs::rgb4(13, 8, 3), gs::rgb4(4, 2, 1), gs::rgb4(13, 9, 6), gs::rgb4(2, 3, 5),
            gs::rgb4(1, 1, 2), gs::rgb4(14, 13, 10), gs::rgb4(1, 1, 1), gs::rgb4(7, 2, 1), gs::rgb4(6, 7, 9), 0, 0, 0, 0,
            ink});
    setPal(vdp, PAL_HOLD, {0, gs::rgb4(15, 10, 2), gs::rgb4(15, 15, 12), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LIVE, {0, gs::rgb4(12, 15, 9), gs::rgb4(15, 15, 13), gs::rgb4(1, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_IRON,
           {0, gs::rgb4(11, 11, 12), gs::rgb4(5, 5, 6), gs::rgb4(8, 5, 3), gs::rgb4(12, 8, 4), gs::rgb4(13, 10, 4),
            gs::rgb4(8, 4, 2), gs::rgb4(14, 14, 15), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 9, 3), gs::rgb4(9, 7, 5), gs::rgb4(13, 11, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 11), gs::rgb4(13, 9, 7), gs::rgb4(9, 7, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(8, 7, 3), gs::rgb4(5, 4, 2), gs::rgb4(9, 8, 4),
        gs::rgb4(7, 6, 3), gs::rgb4(4, 3, 2),
        gs::rgb4(10, 8, 5), gs::rgb4(7, 6, 4),
        gs::rgb4(12, 10, 7), gs::rgb4(8, 7, 5), gs::rgb4(6, 5, 3),
        gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5),
        gs::rgb4(13, 12, 8), gs::rgb4(11, 9, 6),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.hand[0] = gs::uploadMipped(vdp, hand(0));
    art.hand[1] = gs::uploadMipped(vdp, hand(1));
    art.fallen = gs::uploadMipped(vdp, downed());
    art.crib = gs::uploadMipped(vdp, cribArt());
    art.balk = gs::uploadMipped(vdp, balkArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.hoist = gs::uploadMipped(vdp, hoistArt());
    art.coil = gs::uploadMipped(vdp, coilArt());
    art.keg = gs::uploadMipped(vdp, kegArt());
    art.horse = gs::uploadMipped(vdp, horseArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.lantern = gs::uploadMipped(vdp, lanternArt());
    art.barrow = gs::uploadMipped(vdp, barrowArt());
    art.pennant[0] = gs::uploadMipped(vdp, pennantArt(0));
    art.pennant[1] = gs::uploadMipped(vdp, pennantArt(1));
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(10, 7, 4));
}

}  // namespace yardpace
