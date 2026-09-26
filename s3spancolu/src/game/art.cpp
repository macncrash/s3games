#include "game/art.h"

#include <initializer_list>
#include <string>

namespace scol {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap handArt(int step) {
    Bitmap b(40, 72);
    b.ellipse(20, 12, 8, 9, 2);
    b.ellipse(20, 11, 5, 6, 3);
    b.poly({{20, 18}, {10, 28}, {8, 48}, {32, 48}, {30, 28}}, 1);
    b.rect(12, 30, 16, 4, 4);
    b.rect(17, 28, 6, 8, 6);
    int a = step ? 0 : 4;
    b.rect(11, float(50 + a), 6, float(14 - a), 5);
    b.rect(23, float(50 + (step ? 4 : 0)), 6, float(14 - (step ? 4 : 0)), 5);
    b.rect(9, 62, 10, 4, 7);
    b.rect(21, 62, 10, 4, 7);
    b.rect(28, 34, 8, 3, 4);
    b.outline(15, false);
    return b;
}

Bitmap truckArt() {
    Bitmap b(58, 78);
    b.poly({{10, 10}, {48, 10}, {44, 2}, {14, 2}}, 2);
    b.rect(8, 10, 42, 18, 1);
    b.rect(12, 13, 34, 8, 6);
    b.rect(14, 14, 12, 5, 3);
    b.rect(30, 14, 12, 5, 3);
    b.rect(6, 28, 46, 16, 1);
    b.rect(10, 30, 38, 6, 7);
    b.ellipse(16, 40, 4, 3, 5);
    b.ellipse(42, 40, 4, 3, 5);
    b.rect(4, 44, 50, 8, 7);
    b.ellipse(14, 62, 9, 9, 4);
    b.ellipse(44, 62, 9, 9, 4);
    b.ellipse(14, 62, 3, 3, 5);
    b.ellipse(44, 62, 3, 3, 5);
    b.rect(8, 48, 6, 4, 2);
    b.rect(44, 48, 6, 4, 2);
    b.outline(15, false);
    return b;
}

Bitmap beamArt() {
    Bitmap b(128, 22);
    b.rect(2, 4, 124, 14, 1);
    for (int i = 0; i < 8; ++i) b.rect(4 + i * 16, 6, 8, 10, i & 1 ? 2 : 3);
    b.rect(0, 8, 6, 6, 4);
    b.rect(122, 8, 6, 6, 4);
    b.outline(15, false);
    return b;
}

Bitmap pierArt() {
    Bitmap b(36, 96);
    b.poly({{8, 8}, {28, 8}, {32, 90}, {4, 90}}, 1);
    b.poly({{12, 12}, {24, 12}, {26, 86}, {10, 86}}, 2);
    b.rect(6, 4, 24, 8, 3);
    b.rect(10, 0, 16, 6, 4);
    for (int i = 0; i < 5; ++i) b.rect(8, 18 + i * 14, 20, 2, 5);
    b.outline(15, false);
    return b;
}

Bitmap towerArt() {
    Bitmap b(44, 80);
    b.poly({{4, 76}, {20, 8}, {24, 8}, {8, 76}}, 1);
    b.poly({{40, 76}, {24, 8}, {20, 8}, {36, 76}}, 2);
    b.rect(6, 28, 32, 4, 3);
    b.rect(8, 48, 28, 3, 3);
    b.rect(18, 4, 8, 8, 4);
    b.outline(15, false);
    return b;
}

Bitmap cabinArt() {
    Bitmap b(48, 40);
    b.poly({{2, 16}, {24, 2}, {46, 16}}, 2);
    b.rect(6, 16, 36, 20, 1);
    b.rect(10, 20, 10, 8, 3);
    b.rect(28, 20, 8, 12, 4);
    b.rect(20, 28, 8, 8, 5);
    b.outline(15, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 2);
    b.ellipse(7, 7, 3, 3, 1);
    return b;
}

Bitmap buoyArt() {
    Bitmap b(18, 28);
    b.rect(8, 2, 2, 8, 3);
    b.poly({{9, 8}, {16, 16}, {2, 16}}, 1);
    b.ellipse(9, 20, 6, 5, 2);
    b.ellipse(9, 22, 4, 2, 4);
    b.outline(15, false);
    return b;
}

Bitmap dustArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 16, 11, 7, 2);
    b.ellipse(13, 15, 6, 4, 1);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(56, 14);
    b.ellipse(28, 7, 22, 4, 1);
    return b;
}

Bitmap sunArt() {
    Bitmap b(26, 26);
    b.ellipse(13, 13, 10, 10, 2);
    b.ellipse(13, 13, 6, 6, 1);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(78, 24);
    b.ellipse(22, 14, 16, 7, 2);
    b.ellipse(42, 12, 18, 8, 1);
    b.ellipse(60, 15, 12, 6, 2);
    return b;
}

Bitmap treeArt() {
    Bitmap b(120, 36);
    b.poly({{0, 35}, {8, 18}, {16, 28}, {28, 8}, {40, 26}, {52, 12}, {64, 30}, {78, 10}, {90, 24}, {104, 14},
            {120, 35}},
           2);
    b.poly({{0, 35}, {18, 26}, {36, 30}, {58, 22}, {80, 28}, {120, 35}}, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 10, 12), gs::rgb4(4, 6, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 6), gs::rgb4(10, 7, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 8), gs::rgb4(2, 8, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(7, 8, 10), gs::rgb4(12, 9, 6), gs::rgb4(14, 12, 9), gs::rgb4(9, 6, 3),
                           gs::rgb4(3, 3, 4), gs::rgb4(15, 13, 8), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TRUCK, {0, gs::rgb4(12, 3, 2), gs::rgb4(7, 2, 1), gs::rgb4(14, 8, 5), gs::rgb4(2, 2, 2),
                            gs::rgb4(15, 14, 9), gs::rgb4(5, 8, 10), gs::rgb4(8, 7, 6), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BEAM, {0, gs::rgb4(13, 3, 2), gs::rgb4(15, 14, 12), gs::rgb4(8, 2, 2), gs::rgb4(5, 5, 6),
                           gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(10, 11, 12), gs::rgb4(6, 7, 8), gs::rgb4(13, 13, 14), gs::rgb4(4, 5, 6),
                            gs::rgb4(8, 8, 7), gs::rgb4(15, 13, 7), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 13, 8), gs::rgb4(12, 10, 7), gs::rgb4(6, 8, 10), gs::rgb4(15, 15, 13), 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(14, 4, 2), gs::rgb4(15, 14, 12), gs::rgb4(4, 4, 5), gs::rgb4(3, 6, 8), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CABIN, {0, gs::rgb4(8, 6, 4), gs::rgb4(10, 3, 2), gs::rgb4(6, 10, 12), gs::rgb4(4, 3, 2),
                            gs::rgb4(12, 10, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t field[16] = {
        0,
        gs::rgb4(6, 8, 4),
        gs::rgb4(3, 5, 2),
        gs::rgb4(8, 9, 5),
        gs::rgb4(8, 6, 3),
        gs::rgb4(5, 4, 2),
        gs::rgb4(9, 8, 6),
        gs::rgb4(6, 5, 4),
        gs::rgb4(11, 10, 8),
        gs::rgb4(5, 5, 4),
        gs::rgb4(12, 11, 8),
        gs::rgb4(4, 8, 11),
        gs::rgb4(2, 5, 9),
        gs::rgb4(8, 12, 14),
        gs::rgb4(14, 12, 6),
        gs::rgb4(10, 9, 7),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_FIELD * 16 + i, field[i]);

    art.hand[0] = gs::uploadMipped(vdp, handArt(0));
    art.hand[1] = gs::uploadMipped(vdp, handArt(1));
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.cabin = gs::uploadMipped(vdp, cabinArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.trees = gs::uploadMipped(vdp, treeArt());
    loadFont(vdp, art);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace scol
