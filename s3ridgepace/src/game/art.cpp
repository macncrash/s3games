#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace rpace {
namespace {

constexpr float TAU = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap scout(int step) {
    gs::Bitmap b(44, 80);
    b.poly({{22, 3}, {10, 18}, {34, 18}}, 5);
    b.ellipse(22, 16, 8, 7, 4);
    b.rect(16, 15, 2, 2, 9);
    b.rect(26, 15, 2, 2, 9);
    b.rect(19, 20, 6, 2, 13);
    b.poly({{9, 22}, {35, 22}, {38, 52}, {6, 52}}, 2);
    b.poly({{11, 22}, {24, 22}, {22, 52}, {7, 52}}, 1);
    b.rect(20, 24, 3, 18, 11);
    b.rect(18, 32, 8, 3, 7);
    b.ellipse(33, 40, 5, 7, 10);
    b.ellipse(33, 41, 3, 4, 13);
    b.line(12, 26, 6, 40, 6, 3.f);
    b.ellipse(6, 42, 3, 3, 6);
    b.line(32, 26, 37, 38, 6, 3.f);
    b.ellipse(37, 40, 3, 3, 6);
    if (step == 0) {
        b.rect(13, 50, 6, 20, 3);
        b.rect(25, 50, 6, 16, 8);
        b.rect(11, 68, 10, 5, 8);
        b.rect(24, 64, 9, 5, 14);
    } else {
        b.rect(13, 50, 6, 16, 8);
        b.rect(25, 50, 6, 20, 3);
        b.rect(12, 64, 9, 5, 14);
        b.rect(23, 68, 10, 5, 8);
    }
    b.outline(12, false);
    return b;
}

gs::Bitmap fallenBody() {
    gs::Bitmap b(92, 36);
    b.ellipse(16, 20, 8, 7, 4);
    b.poly({{12, 8}, {22, 6}, {24, 16}, {10, 18}}, 5);
    b.poly({{22, 12}, {78, 16}, {82, 28}, {20, 26}}, 2);
    b.poly({{22, 12}, {48, 13}, {50, 26}, {20, 24}}, 1);
    b.ellipse(58, 10, 6, 5, 10);
    b.rect(70, 22, 12, 5, 8);
    b.rect(18, 24, 8, 3, 7);
    b.outline(12, false);
    return b;
}

gs::Bitmap cairnArt(int tall) {
    gs::Bitmap b(28, tall ? 46 : 32);
    int base = b.h - 4;
    b.ellipse(14, base, 12, 3, 3);
    b.poly({{5, float(base)}, {23, float(base)}, {20, float(base - 10)}, {8, float(base - 10)}}, 2);
    b.poly({{8, float(base - 10)}, {20, float(base - 10)}, {17, float(base - 18)}, {11, float(base - 18)}}, 1);
    b.poly({{11, float(base - 18)}, {17, float(base - 18)}, {15, float(base - 24)}, {13, float(base - 24)}}, 4);
    if (tall) {
        b.rect(12, base - 32, 4, 8, 1);
        b.rect(11, base - 34, 6, 3, 4);
        b.rect(10, base - 16, 3, 2, 6);
    } else {
        b.rect(8, base - 12, 3, 2, 6);
    }
    b.outline(5, false);
    return b;
}

gs::Bitmap ragArt() {
    gs::Bitmap b(14, 12);
    b.poly({{2, 1}, {12, 3}, {9, 10}, {1, 8}}, 1);
    b.poly({{2, 2}, {7, 3}, {6, 9}, {1, 7}}, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap pineArt() {
    gs::Bitmap b(36, 60);
    b.rect(16, 34, 5, 22, 3);
    b.rect(17, 36, 2, 16, 4);
    b.poly({{18, 3}, {6, 24}, {30, 24}}, 1);
    b.poly({{18, 14}, {3, 36}, {33, 36}}, 2);
    b.poly({{18, 26}, {7, 46}, {29, 46}}, 1);
    b.outline(5, false);
    return b;
}

gs::Bitmap cragArt() {
    gs::Bitmap b(24, 30);
    b.poly({{3, 27}, {21, 27}, {18, 12}, {12, 4}, {6, 14}}, 2);
    b.poly({{7, 16}, {13, 6}, {14, 24}, {5, 24}}, 1);
    b.rect(9, 14, 3, 2, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap bushArt() {
    gs::Bitmap b(30, 16);
    b.ellipse(10, 10, 8, 5, 6);
    b.ellipse(18, 8, 8, 6, 7);
    b.ellipse(24, 11, 5, 4, 6);
    b.outline(5, false);
    return b;
}

gs::Bitmap grassArt() {
    gs::Bitmap b(14, 16);
    b.line(3, 14, 2, 3, 1, 1.4f);
    b.line(7, 14, 8, 2, 8, 1.4f);
    b.line(11, 14, 12, 5, 1, 1.4f);
    b.line(5, 14, 4, 7, 2, 1.2f);
    return b;
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(10, 34);
    b.poly({{5, 2}, {8, 30}, {2, 30}}, 1);
    b.rect(3, 26, 4, 5, 2);
    b.rect(4, 8, 2, 6, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(24, 24);
    for (int i = 0; i < 28; i++) {
        if ((i % 7) < 2) continue;
        float a = i * TAU / 28.f;
        int x = int(std::lround(12 + std::cos(a) * 8));
        int y = int(std::lround(12 + std::sin(a) * 8));
        b.set(x, y, 1);
        b.set(x, y + 1, 2);
    }
    b.rect(11, 2, 2, 4, 1);
    b.rect(11, 18, 2, 4, 1);
    b.rect(2, 11, 4, 2, 1);
    b.rect(18, 11, 4, 2, 1);
    b.set(12, 12, 1);
    return b;
}

gs::Bitmap rifleArt() {
    gs::Bitmap b(26, 72);
    b.rect(11, 0, 4, 38, 1);
    b.rect(12, 1, 2, 34, 2);
    b.rect(10, 2, 6, 3, 5);
    b.rect(8, 34, 10, 10, 2);
    b.rect(9, 36, 6, 6, 1);
    b.poly({{7, 44}, {19, 44}, {21, 68}, {5, 66}}, 3);
    b.poly({{9, 46}, {15, 46}, {15, 64}, {7, 62}}, 4);
    b.rect(4, 60, 16, 5, 2);
    b.outline(6, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 64);
    b.rect(4, 4, 4, 56, 3);
    b.rect(5, 6, 2, 48, 4);
    b.rect(3, 2, 6, 4, 1);
    b.rect(2, 56, 8, 5, 2);
    b.outline(6, false);
    return b;
}

gs::Bitmap pennantArt(int fr) {
    gs::Bitmap b(26, 16);
    b.rect(1, 1, 2, 14, 3);
    if (fr == 0) b.poly({{3, 2}, {23, 5}, {3, 11}}, 1);
    else b.poly({{3, 3}, {21, 8}, {3, 13}}, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap lipArt() {
    gs::Bitmap b(80, 42);
    b.poly({{2, 40}, {76, 38}, {70, 16}, {46, 8}, {18, 18}, {4, 28}}, 2);
    b.poly({{10, 30}, {40, 12}, {52, 28}, {22, 36}}, 1);
    b.rect(24, 20, 8, 3, 4);
    b.rect(48, 22, 6, 2, 6);
    b.outline(5, false);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 7, 5, 4, 1);
    b.ellipse(6, 6, 3, 2, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7, 5, 2);
    b.ellipse(9, 9, 3, 3, 1);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(26, 12);
    b.ellipse(13, 7, 11, 4, 3);
    b.ellipse(8, 6, 4, 2, 4);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(30, 10);
    b.ellipse(15, 5, 13, 3, 1);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(16, 6);
    b.rect(0, 1, 16, 4, 1);
    b.rect(0, 2, 16, 2, 2);
    return b;
}

gs::Bitmap peakArt() {
    gs::Bitmap b(72, 40);
    b.poly({{4, 38}, {30, 6}, {54, 38}}, 2);
    b.poly({{24, 16}, {30, 6}, {36, 16}}, 1);
    b.poly({{40, 38}, {56, 14}, {70, 38}}, 3);
    b.poly({{52, 22}, {56, 14}, {60, 22}}, 1);
    b.outline(4, false);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(32, 32);
    b.ellipse(16, 16, 7, 7, 5);
    b.ellipse(16, 16, 4, 4, 6);
    for (int i = 0; i < 8; i++) {
        float a = i * TAU / 8.f + 0.2f;
        b.line(16 + std::cos(a) * 9.f, 16 + std::sin(a) * 9.f, 16 + std::cos(a) * 13.f, 16 + std::sin(a) * 13.f, 5,
               1.6f);
    }
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(56, 20);
    b.ellipse(16, 12, 12, 6, 8);
    b.ellipse(30, 10, 14, 7, 7);
    b.ellipse(44, 12, 10, 5, 8);
    return b;
}

gs::Bitmap hawkArt(int fr) {
    gs::Bitmap b(28, 12);
    if (fr == 0) {
        b.poly({{14, 6}, {1, 2}, {8, 7}}, 5);
        b.poly({{14, 6}, {27, 2}, {20, 7}}, 5);
    } else {
        b.poly({{14, 7}, {2, 9}, {9, 8}}, 6);
        b.poly({{14, 7}, {26, 9}, {19, 8}}, 6);
    }
    b.ellipse(14, 7, 3, 2, 5);
    b.outline(7, false);
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
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 14, 12), gs::rgb4(11, 10, 9), gs::rgb4(3, 3, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 11, 3), gs::rgb4(15, 14, 9), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 10, 8), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 6), gs::rgb4(14, 15, 12), gs::rgb4(1, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(12, 11, 9), gs::rgb4(8, 7, 6), gs::rgb4(4, 4, 3), gs::rgb4(14, 13, 11),
                            gs::rgb4(2, 2, 2), gs::rgb4(6, 7, 4), gs::rgb4(9, 8, 6), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FIGURE,
           {0, gs::rgb4(9, 11, 5), gs::rgb4(5, 7, 3), gs::rgb4(3, 4, 2), gs::rgb4(13, 9, 6), gs::rgb4(3, 3, 2),
            gs::rgb4(7, 6, 4), gs::rgb4(12, 9, 3), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1), gs::rgb4(8, 6, 3),
            gs::rgb4(12, 13, 9), gs::rgb4(1, 1, 1), gs::rgb4(8, 5, 3), gs::rgb4(4, 3, 2), ink});
    setPal(vdp, PAL_HOLD, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 12), gs::rgb4(6, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_LIVE, {0, gs::rgb4(12, 15, 8), gs::rgb4(7, 13, 6), gs::rgb4(1, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(4, 8, 3), gs::rgb4(2, 5, 2), gs::rgb4(7, 5, 2), gs::rgb4(9, 7, 4),
                           gs::rgb4(1, 2, 1), gs::rgb4(7, 5, 6), gs::rgb4(10, 8, 9), gs::rgb4(8, 10, 4), 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 13), gs::rgb4(15, 9, 2), gs::rgb4(8, 7, 5), gs::rgb4(12, 11, 8),
                         gs::rgb4(3, 3, 4), gs::rgb4(7, 7, 8), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(14, 14, 15), gs::rgb4(6, 6, 8), gs::rgb4(4, 5, 7), gs::rgb4(2, 2, 4),
                          gs::rgb4(15, 11, 4), gs::rgb4(15, 15, 10), gs::rgb4(12, 11, 12), gs::rgb4(8, 8, 11), 0, 0, 0,
                          0, 0, 0, ink});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(12, 12, 14), gs::rgb4(6, 6, 8), gs::rgb4(10, 6, 3), gs::rgb4(6, 3, 1),
                           gs::rgb4(15, 14, 9), gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(6, 7, 4),
        gs::rgb4(3, 4, 2),
        gs::rgb4(8, 7, 4),
        gs::rgb4(9, 8, 5),
        gs::rgb4(5, 5, 3),
        gs::rgb4(11, 10, 8),
        gs::rgb4(7, 7, 6),
        gs::rgb4(13, 12, 10),
        gs::rgb4(5, 5, 4),
        gs::rgb4(12, 11, 9),
        gs::rgb4(4, 5, 6),
        gs::rgb4(3, 4, 5),
        gs::rgb4(5, 6, 7),
        gs::rgb4(14, 12, 7),
        gs::rgb4(8, 7, 6),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.scout[0] = gs::uploadMipped(vdp, scout(0));
    art.scout[1] = gs::uploadMipped(vdp, scout(1));
    art.fallen = gs::uploadMipped(vdp, fallenBody());
    art.cairn[0] = gs::uploadMipped(vdp, cairnArt(0));
    art.cairn[1] = gs::uploadMipped(vdp, cairnArt(1));
    art.rag = gs::uploadMipped(vdp, ragArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.crag = gs::uploadMipped(vdp, cragArt());
    art.bush = gs::uploadMipped(vdp, bushArt());
    art.grass = gs::uploadMipped(vdp, grassArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.pennant[0] = gs::uploadMipped(vdp, pennantArt(0));
    art.pennant[1] = gs::uploadMipped(vdp, pennantArt(1));
    art.lip = gs::uploadMipped(vdp, lipArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.peak = gs::uploadMipped(vdp, peakArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.hawk[0] = gs::uploadMipped(vdp, hawkArt(0));
    art.hawk[1] = gs::uploadMipped(vdp, hawkArt(1));

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(14, 8, 5));
}

}  // namespace rpace
