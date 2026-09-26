#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace rcler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap worker(int step) {
    gs::Bitmap b(42, 74);
    b.rect(11, 4, 20, 5, 5);
    b.poly({{10, 8}, {8, 14}, {18, 12}}, 5);
    b.ellipse(22, 16, 8, 8, 4);
    b.rect(16, 15, 3, 2, 11);
    b.rect(25, 15, 3, 2, 11);
    b.rect(19, 20, 7, 2, 13);
    b.rect(14, 18, 16, 3, 9);
    b.poly({{8, 22}, {34, 22}, {37, 50}, {5, 50}}, 1);
    b.poly({{8, 22}, {22, 22}, {20, 50}, {6, 50}}, 2);
    b.rect(6, 28, 8, 11, 10);
    b.rect(7, 30, 6, 2, 14);
    b.rect(19, 24, 3, 14, 7);
    b.rect(14, 34, 14, 3, 7);
    b.line(12, 26, 5, 40, 6, 3.f);
    b.ellipse(5, 42, 3, 3, 6);
    b.line(31, 26, 37, 38, 6, 3.f);
    b.ellipse(37, 40, 3, 3, 6);
    if (step == 0) {
        b.rect(12, 48, 6, 18, 3);
        b.rect(24, 48, 6, 14, 8);
        b.rect(10, 64, 10, 4, 8);
        b.rect(22, 60, 10, 4, 3);
    } else {
        b.rect(12, 48, 6, 14, 8);
        b.rect(24, 48, 6, 18, 3);
        b.rect(11, 60, 10, 4, 3);
        b.rect(22, 64, 10, 4, 8);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap barrowArt() {
    gs::Bitmap b(52, 34);
    b.poly({{8, 8}, {42, 6}, {46, 16}, {10, 18}}, 1);
    b.poly({{12, 10}, {40, 8}, {42, 15}, {14, 16}}, 2);
    b.rect(16, 11, 18, 2, 8);
    b.line(12, 16, 6, 28, 3, 2.6f);
    b.line(38, 14, 44, 26, 3, 2.6f);
    b.ellipse(26, 26, 7, 7, 6);
    b.ellipse(26, 26, 2, 2, 4);
    b.line(10, 10, 2, 3, 5, 2.2f);
    b.line(14, 12, 6, 5, 5, 2.f);
    b.outline(15, false);
    return b;
}

gs::Bitmap loadArt() {
    gs::Bitmap b(28, 18);
    b.ellipse(8, 12, 7, 5, 2);
    b.ellipse(16, 9, 8, 6, 1);
    b.ellipse(22, 12, 6, 4, 6);
    b.line(6, 8, 12, 3, 3, 1.6f);
    b.line(14, 6, 18, 2, 3, 1.4f);
    b.line(20, 7, 25, 3, 4, 1.4f);
    b.outline(15, false);
    return b;
}

gs::Bitmap rakeArt() {
    gs::Bitmap b(40, 28);
    b.line(4, 24, 30, 6, 3, 2.4f);
    b.rect(28, 2, 3, 14, 1);
    for (int i = 0; i < 5; i++) b.line(29 + i * 2, 4, 30 + i * 2, 16, 4, 1.3f);
    b.outline(15, false);
    return b;
}

gs::Bitmap brushArt() {
    gs::Bitmap b(36, 28);
    b.ellipse(18, 20, 14, 5, 2);
    b.ellipse(12, 16, 8, 6, 1);
    b.ellipse(22, 14, 9, 7, 4);
    b.line(8, 18, 6, 6, 3, 1.8f);
    b.line(14, 16, 12, 4, 3, 1.8f);
    b.line(20, 15, 22, 3, 3, 1.8f);
    b.line(26, 16, 30, 5, 5, 1.6f);
    b.line(16, 14, 18, 7, 4, 1.5f);
    b.outline(15, false);
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(34, 30);
    b.ellipse(17, 24, 14, 5, 3);
    b.poly({{6, 22}, {16, 16}, {22, 22}, {8, 24}}, 2);
    b.poly({{14, 20}, {28, 14}, {30, 22}, {16, 24}}, 1);
    b.poly({{12, 16}, {20, 8}, {26, 16}, {14, 18}}, 2);
    b.poly({{16, 12}, {22, 4}, {24, 12}}, 6);
    b.rect(18, 10, 3, 2, 7);
    b.outline(15, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(32, 30);
    b.rect(4, 8, 24, 18, 1);
    b.rect(6, 10, 20, 14, 2);
    b.line(6, 10, 24, 22, 3, 1.6f);
    b.line(24, 10, 6, 22, 3, 1.6f);
    b.rect(4, 8, 24, 3, 8);
    b.rect(3, 14, 26, 2, 5);
    b.rect(14, 8, 3, 18, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap cribArt() {
    gs::Bitmap b(44, 40);
    b.rect(6, 10, 4, 26, 3);
    b.rect(34, 10, 4, 26, 3);
    b.rect(4, 30, 36, 5, 1);
    b.rect(8, 18, 28, 4, 2);
    b.rect(8, 12, 28, 3, 1);
    b.line(8, 10, 8, 32, 3, 1.5f);
    b.line(22, 12, 22, 32, 3, 1.5f);
    b.line(34, 10, 34, 32, 3, 1.5f);
    b.rect(10, 8, 24, 3, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap rubbleArt() {
    gs::Bitmap b(16, 12);
    b.poly({{2, 10}, {8, 3}, {14, 10}}, 1);
    b.poly({{3, 10}, {7, 5}, {8, 10}}, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap sweptArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(14, 7, 12, 4, 4);
    b.ellipse(10, 7, 4, 2, 1);
    b.ellipse(18, 8, 3, 1, 3);
    return b;
}

gs::Bitmap pineArt() {
    gs::Bitmap b(34, 58);
    b.rect(15, 34, 5, 20, 3);
    b.rect(16, 36, 2, 14, 4);
    b.poly({{17, 2}, {5, 22}, {29, 22}}, 1);
    b.poly({{17, 14}, {2, 34}, {32, 34}}, 2);
    b.poly({{17, 24}, {6, 44}, {28, 44}}, 1);
    b.ellipse(17, 20, 2, 3, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap cragArt() {
    gs::Bitmap b(26, 32);
    b.poly({{3, 29}, {23, 29}, {20, 14}, {13, 3}, {6, 15}}, 2);
    b.poly({{7, 18}, {13, 6}, {14, 26}, {5, 26}}, 1);
    b.rect(11, 14, 3, 2, 4);
    b.line(8, 20, 16, 16, 7, 1.2f);
    b.outline(15, false);
    return b;
}

gs::Bitmap bushArt() {
    gs::Bitmap b(30, 16);
    b.ellipse(10, 10, 8, 5, 2);
    b.ellipse(18, 8, 8, 6, 1);
    b.ellipse(24, 11, 5, 4, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap grassArt() {
    gs::Bitmap b(14, 16);
    b.line(3, 14, 2, 3, 1, 1.4f);
    b.line(7, 14, 8, 2, 2, 1.4f);
    b.line(11, 14, 12, 5, 1, 1.4f);
    b.line(5, 14, 4, 7, 1, 1.2f);
    return b;
}

gs::Bitmap cairnArt() {
    gs::Bitmap b(24, 36);
    b.ellipse(12, 32, 10, 3, 3);
    b.poly({{4, 30}, {20, 30}, {17, 20}, {7, 20}}, 2);
    b.poly({{7, 20}, {17, 20}, {15, 12}, {9, 12}}, 1);
    b.poly({{9, 12}, {15, 12}, {13, 6}, {11, 6}}, 6);
    b.rect(10, 16, 3, 2, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(16, 64);
    b.rect(6, 4, 4, 56, 3);
    b.rect(7, 6, 1, 48, 1);
    b.rect(2, 58, 12, 4, 2);
    b.rect(4, 2, 8, 4, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap pennantArt(int fr) {
    gs::Bitmap b(28, 16);
    float droop = fr ? 4.f : 1.f;
    b.poly({{2, 2}, {24, 4}, {22, 8 + droop}, {2, 10}}, 1);
    b.poly({{2, 3}, {14, 4}, {13, 8}, {2, 8}}, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(12, 16);
    b.poly({{6, 1}, {10, 10}, {6, 15}, {2, 10}}, 1);
    b.poly({{6, 5}, {8, 11}, {6, 14}, {4, 11}}, 2);
    return b;
}

gs::Bitmap peakArt() {
    gs::Bitmap b(64, 36);
    b.poly({{4, 34}, {24, 8}, {36, 34}}, 3);
    b.poly({{22, 34}, {46, 4}, {62, 34}}, 4);
    b.poly({{42, 12}, {48, 4}, {50, 14}}, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 7, 7, 5);
    b.ellipse(14, 14, 4, 4, 6);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785398f;
        b.line(14 + std::cos(a) * 8, 14 + std::sin(a) * 8, 14 + std::cos(a) * 12, 14 + std::sin(a) * 12, 7, 1.4f);
    }
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(56, 20);
    b.ellipse(16, 12, 12, 6, 1);
    b.ellipse(30, 9, 14, 7, 2);
    b.ellipse(44, 12, 10, 5, 1);
    return b;
}

gs::Bitmap hawkArt(int fr) {
    gs::Bitmap b(28, 12);
    if (fr == 0) {
        b.poly({{14, 6}, {1, 2}, {8, 7}}, 6);
        b.poly({{14, 6}, {27, 2}, {20, 7}}, 6);
    } else {
        b.poly({{14, 7}, {2, 10}, {9, 6}}, 7);
        b.poly({{14, 7}, {26, 10}, {19, 6}}, 7);
    }
    b.ellipse(14, 7, 3, 2, 6);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 5, 4);
    b.ellipse(6, 7, 2, 2, 1);
    b.ellipse(11, 9, 2, 1, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 5);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(16, 14);
    b.poly({{8, 12}, {2, 3}, {6, 3}, {8, 7}, {10, 3}, {14, 3}}, 1);
    b.outline(15, false);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(12, 12);
    b.poly({{6, 1}, {11, 6}, {6, 11}, {1, 6}}, 1);
    b.outline(15, false);
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
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 14, 12), gs::rgb4(11, 10, 8), gs::rgb4(4, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 11, 3), gs::rgb4(15, 14, 8), gs::rgb4(6, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 10, 7), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 6), gs::rgb4(14, 15, 12), gs::rgb4(1, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(12, 11, 9), gs::rgb4(8, 7, 6), gs::rgb4(4, 4, 3), gs::rgb4(6, 7, 4),
                            gs::rgb4(10, 9, 7), gs::rgb4(14, 13, 11), gs::rgb4(3, 3, 2), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FIGURE,
           {0, gs::rgb4(8, 10, 5), gs::rgb4(5, 6, 3), gs::rgb4(3, 3, 2), gs::rgb4(13, 9, 6), gs::rgb4(4, 5, 3),
            gs::rgb4(7, 6, 4), gs::rgb4(12, 9, 3), gs::rgb4(2, 2, 1), gs::rgb4(13, 12, 9), gs::rgb4(6, 4, 2),
            gs::rgb4(1, 1, 1), gs::rgb4(4, 3, 2), gs::rgb4(10, 6, 5), gs::rgb4(9, 7, 3), ink});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(10, 10, 11),
                           gs::rgb4(11, 8, 3), gs::rgb4(3, 3, 3), gs::rgb4(13, 12, 8), gs::rgb4(4, 3, 2),
                           gs::rgb4(13, 11, 6), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_EARTH, {0, gs::rgb4(11, 9, 4), gs::rgb4(7, 5, 2), gs::rgb4(5, 4, 2), gs::rgb4(6, 8, 3),
                            gs::rgb4(3, 2, 1), gs::rgb4(9, 8, 6), gs::rgb4(12, 10, 7), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(4, 8, 3), gs::rgb4(2, 5, 2), gs::rgb4(7, 5, 2), gs::rgb4(9, 7, 4),
                           gs::rgb4(5, 3, 1), gs::rgb4(8, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 13), gs::rgb4(15, 10, 3), gs::rgb4(8, 7, 5), gs::rgb4(12, 11, 8),
                         gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 8), 0, 0, 0, 0, 0, 0,
                         ink});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(14, 14, 15), gs::rgb4(9, 9, 12), gs::rgb4(11, 10, 12), gs::rgb4(5, 5, 8),
                          gs::rgb4(15, 12, 5), gs::rgb4(15, 15, 10), gs::rgb4(15, 13, 6), 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(5, 6, 3),
        gs::rgb4(3, 4, 2),
        gs::rgb4(7, 7, 4),
        gs::rgb4(8, 7, 4),
        gs::rgb4(5, 4, 2),
        gs::rgb4(10, 8, 5),
        gs::rgb4(7, 6, 4),
        gs::rgb4(12, 11, 9),
        gs::rgb4(6, 5, 3),
        gs::rgb4(11, 9, 6),
        gs::rgb4(4, 5, 6),
        gs::rgb4(3, 4, 5),
        gs::rgb4(5, 6, 7),
        gs::rgb4(13, 10, 6),
        gs::rgb4(14, 12, 9),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.worker[0] = gs::uploadMipped(vdp, worker(0));
    art.worker[1] = gs::uploadMipped(vdp, worker(1));
    art.barrow = gs::uploadMipped(vdp, barrowArt());
    art.load = gs::uploadMipped(vdp, loadArt());
    art.rake = gs::uploadMipped(vdp, rakeArt());
    art.brush = gs::uploadMipped(vdp, brushArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.crib = gs::uploadMipped(vdp, cribArt());
    art.rubble = gs::uploadMipped(vdp, rubbleArt());
    art.swept = gs::uploadMipped(vdp, sweptArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.crag = gs::uploadMipped(vdp, cragArt());
    art.bush = gs::uploadMipped(vdp, bushArt());
    art.grass = gs::uploadMipped(vdp, grassArt());
    art.cairn = gs::uploadMipped(vdp, cairnArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.pennant[0] = gs::uploadMipped(vdp, pennantArt(0));
    art.pennant[1] = gs::uploadMipped(vdp, pennantArt(1));
    art.flame = gs::uploadMipped(vdp, flameArt());
    art.peak = gs::uploadMipped(vdp, peakArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.hawk[0] = gs::uploadMipped(vdp, hawkArt(0));
    art.hawk[1] = gs::uploadMipped(vdp, hawkArt(1));
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.mark = gs::uploadMipped(vdp, markArt());
    art.pip = gs::uploadMipped(vdp, pipArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(13, 8, 5));
}

}  // namespace rcler
