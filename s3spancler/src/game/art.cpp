#include "game/art.h"

#include <initializer_list>
#include <string>

namespace spancler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap handArt(int step) {
    gs::Bitmap b(36, 68);
    b.ellipse(18, 12, 7, 7, 4);
    b.rect(14, 10, 3, 2, 11);
    b.rect(21, 10, 3, 2, 11);
    b.rect(10, 4, 16, 4, 5);
    b.poly({{8, 18}, {28, 18}, {30, 42}, {6, 42}}, 1);
    b.poly({{8, 18}, {18, 18}, {16, 42}, {6, 42}}, 2);
    b.rect(12, 20, 4, 12, 8);
    b.rect(6, 22, 6, 10, 9);
    b.line(8, 24, 2, 36, 6, 2.4f);
    b.ellipse(2, 38, 3, 3, 6);
    if (step == 0) {
        b.rect(10, 42, 6, 18, 3);
        b.rect(20, 42, 6, 14, 7);
        b.rect(8, 58, 10, 4, 8);
        b.rect(18, 54, 10, 4, 3);
    } else {
        b.rect(10, 42, 6, 14, 7);
        b.rect(20, 42, 6, 18, 3);
        b.rect(8, 54, 10, 4, 3);
        b.rect(18, 58, 10, 4, 8);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap broomArt() {
    gs::Bitmap b(44, 36);
    b.line(6, 30, 28, 8, 3, 2.6f);
    b.rect(26, 2, 16, 10, 1);
    for (int i = 0; i < 7; i++) b.line(28 + i * 2, 4, 30 + i * 2, 18, 4, 1.4f);
    b.outline(15, false);
    return b;
}

gs::Bitmap barrelArt() {
    gs::Bitmap b(28, 32);
    b.ellipse(14, 8, 10, 4, 2);
    b.rect(4, 8, 20, 16, 1);
    b.ellipse(14, 24, 10, 4, 3);
    b.rect(4, 12, 20, 2, 5);
    b.rect(4, 18, 20, 2, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(30, 28);
    b.rect(3, 6, 24, 18, 1);
    b.rect(5, 8, 20, 14, 2);
    b.line(5, 8, 23, 20, 3, 1.5f);
    b.line(23, 8, 5, 20, 3, 1.5f);
    b.rect(3, 6, 24, 3, 8);
    b.outline(15, false);
    return b;
}

gs::Bitmap coilArt() {
    gs::Bitmap b(32, 22);
    b.ellipse(16, 12, 13, 8, 2);
    b.ellipse(16, 12, 8, 5, 1);
    b.ellipse(16, 12, 3, 2, 6);
    b.line(6, 8, 12, 14, 4, 1.4f);
    b.line(20, 8, 26, 14, 4, 1.4f);
    b.outline(15, false);
    return b;
}

gs::Bitmap patchArt() {
    gs::Bitmap b(26, 10);
    b.ellipse(13, 5, 12, 3, 3);
    b.ellipse(8, 5, 3, 1, 1);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(28, 72);
    b.poly({{4, 70}, {24, 70}, {20, 18}, {8, 18}}, 1);
    b.rect(6, 8, 16, 14, 2);
    b.rect(10, 2, 8, 8, 3);
    b.rect(8, 28, 12, 3, 4);
    b.rect(9, 44, 10, 3, 4);
    b.rect(12, 12, 4, 6, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 18);
    b.rect(6, 10, 2, 8, 2);
    b.poly({{2, 10}, {7, 2}, {12, 10}}, 1);
    b.ellipse(7, 6, 2, 2, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap cableArt() {
    gs::Bitmap b(48, 10);
    b.line(1, 2, 24, 8, 1, 2.f);
    b.line(24, 8, 47, 2, 1, 2.f);
    return b;
}

gs::Bitmap gullArt(int fr) {
    gs::Bitmap b(26, 10);
    if (fr == 0) {
        b.poly({{13, 6}, {1, 2}, {8, 6}}, 1);
        b.poly({{13, 6}, {25, 2}, {18, 6}}, 1);
    } else {
        b.poly({{13, 5}, {2, 8}, {9, 5}}, 2);
        b.poly({{13, 5}, {24, 8}, {17, 5}}, 2);
    }
    b.ellipse(13, 6, 2, 1, 1);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(48, 16);
    b.ellipse(14, 10, 10, 5, 1);
    b.ellipse(26, 7, 12, 6, 2);
    b.ellipse(38, 10, 8, 4, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 7, 7, 1);
    b.ellipse(11, 11, 4, 4, 2);
    return b;
}

gs::Bitmap splashArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 10, 6, 3, 1);
    b.ellipse(5, 7, 2, 3, 2);
    b.ellipse(11, 6, 2, 4, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 10);
    b.ellipse(14, 5, 12, 3, 5);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(10, 10);
    b.poly({{5, 1}, {9, 5}, {5, 9}, {1, 5}}, 1);
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
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 15, 15), gs::rgb4(9, 11, 12), gs::rgb4(3, 4, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 14, 8), gs::rgb4(6, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 10, 6), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 15, 8), gs::rgb4(13, 15, 12), gs::rgb4(1, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_STEEL, {0, gs::rgb4(10, 11, 12), gs::rgb4(6, 7, 8), gs::rgb4(3, 4, 5), gs::rgb4(13, 12, 8),
                            gs::rgb4(4, 5, 6), gs::rgb4(15, 14, 6), gs::rgb4(8, 9, 10), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FIGURE,
           {0, gs::rgb4(4, 7, 12), gs::rgb4(2, 4, 8), gs::rgb4(2, 2, 3), gs::rgb4(13, 9, 6), gs::rgb4(3, 3, 4),
            gs::rgb4(8, 6, 4), gs::rgb4(5, 4, 3), gs::rgb4(1, 1, 2), gs::rgb4(12, 11, 9), gs::rgb4(7, 5, 3),
            gs::rgb4(1, 1, 1), gs::rgb4(4, 3, 2), gs::rgb4(10, 6, 5), gs::rgb4(9, 8, 4), ink});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(11, 7, 3), gs::rgb4(7, 4, 2), gs::rgb4(5, 3, 1), gs::rgb4(13, 10, 5),
                           gs::rgb4(9, 6, 2), gs::rgb4(3, 2, 1), gs::rgb4(14, 12, 8), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_DECK, {0, gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 4), gs::rgb4(11, 10, 8), gs::rgb4(6, 7, 6),
                           gs::rgb4(3, 3, 3), gs::rgb4(12, 11, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(6, 12, 14), gs::rgb4(3, 7, 12), gs::rgb4(10, 14, 15), gs::rgb4(2, 4, 8), 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 13, 14), gs::rgb4(8, 9, 10), gs::rgb4(4, 5, 6),
                         gs::rgb4(2, 3, 4), gs::rgb4(14, 14, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(14, 14, 15), gs::rgb4(8, 10, 14), gs::rgb4(11, 12, 14), gs::rgb4(4, 6, 10),
                          gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(6, 6, 6),
        gs::rgb4(4, 4, 5),
        gs::rgb4(8, 8, 7),
        gs::rgb4(10, 9, 6),
        gs::rgb4(3, 3, 4),
        gs::rgb4(12, 11, 8),
        gs::rgb4(7, 7, 6),
        gs::rgb4(5, 6, 7),
        gs::rgb4(9, 9, 8),
        gs::rgb4(2, 3, 4),
        gs::rgb4(11, 10, 8),
        gs::rgb4(3, 6, 9),
        gs::rgb4(2, 4, 7),
        gs::rgb4(5, 9, 12),
        gs::rgb4(8, 12, 14),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.hand[0] = gs::uploadMipped(vdp, handArt(0));
    art.hand[1] = gs::uploadMipped(vdp, handArt(1));
    art.broom = gs::uploadMipped(vdp, broomArt());
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.coil = gs::uploadMipped(vdp, coilArt());
    art.patch = gs::uploadMipped(vdp, patchArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.cable = gs::uploadMipped(vdp, cableArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(0));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(1));
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.splash = gs::uploadMipped(vdp, splashArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.pip = gs::uploadMipped(vdp, pipArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(8, 10, 13));
}

}  // namespace spancler
