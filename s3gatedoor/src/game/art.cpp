#include "game/art.h"

#include <initializer_list>

namespace gatedoor {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{2, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.shared(px);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

gs::Bitmap pierArt() {
    gs::Bitmap b(22, 96);
    b.rect(4, 8, 14, 86, 2);
    b.rect(14, 8, 4, 86, 1);
    b.rect(5, 8, 3, 86, 3);
    for (int i = 0; i < 8; i++) {
        int y = 12 + i * 10;
        b.rect(5, float(y), 12, 2, 4);
        if (i % 3 == 0) b.set(8, y + 5, 5);
        if (i == 4) b.rect(7, float(y + 4), 5, 2, 6);
    }
    b.rect(1, 1, 20, 7, 3);
    b.rect(1, 1, 20, 2, 7);
    b.rect(0, 7, 22, 3, 1);
    b.rect(2, 86, 18, 8, 2);
    b.rect(2, 86, 18, 2, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(96, 16);
    b.rect(1, 3, 94, 11, 2);
    b.rect(1, 3, 94, 3, 3);
    b.rect(1, 11, 94, 3, 1);
    for (int i = 0; i < 7; i++) {
        b.rect(float(6 + i * 13), 4, 3, 8, 8);
        b.set(7 + i * 13, 7, 9);
    }
    b.rect(44, 1, 8, 4, 7);
    b.outline(15, false);
    return b;
}

gs::Bitmap leafArt() {
    gs::Bitmap b(40, 80);
    b.rect(1, 1, 6, 78, 2);
    b.rect(1, 1, 2, 78, 3);
    b.rect(33, 1, 6, 78, 2);
    b.rect(36, 1, 3, 78, 4);
    for (int i = 0; i < 6; i++) {
        int y = 2 + i * 13;
        b.rect(1, float(y), 38, 7, 2);
        b.rect(1, float(y), 38, 2, 3);
        b.rect(6, float(y + 2), 3, 3, 5);
        b.rect(31, float(y + 2), 3, 3, 5);
        b.set(7, y + 3, 8);
        b.set(32, y + 3, 8);
    }
    b.line(8, 74, 32, 8, 1, 3.4f);
    b.line(9, 72, 31, 8, 3, 1.3f);
    b.rect(29, 34, 10, 12, 6);
    b.rect(31, 36, 6, 8, 7);
    b.rect(33, 38, 2, 3, 8);
    b.outline(15, false);
    return b;
}

gs::Bitmap latchArt() {
    gs::Bitmap b(32, 8);
    b.rect(0, 2, 32, 4, 1);
    b.rect(0, 2, 32, 1, 2);
    b.rect(0, 5, 32, 1, 3);
    for (int x = 3; x < 30; x += 7) b.set(x, 3, 4);
    return b;
}

gs::Bitmap hingeArt() {
    gs::Bitmap b(8, 14);
    b.rect(1, 1, 6, 12, 1);
    b.rect(2, 1, 2, 12, 2);
    b.rect(1, 2, 6, 2, 3);
    b.rect(1, 10, 6, 2, 3);
    b.set(4, 3, 4);
    b.set(4, 11, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap catchArt() {
    gs::Bitmap b(10, 18);
    b.rect(1, 1, 5, 16, 1);
    b.rect(2, 2, 2, 14, 2);
    b.rect(5, 3, 4, 4, 3);
    b.rect(5, 11, 4, 4, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap wedgeArt() {
    gs::Bitmap b(22, 12);
    b.poly({{1, 9}, {20, 9}, {20, 2}}, 2);
    b.line(2, 8, 19, 3, 3, 1.4f);
    b.rect(16, 3, 3, 6, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap keeperArt(int frame) {
    gs::Bitmap b(32, 50);
    b.rect(9, 1, 14, 3, 3);
    b.rect(7, 4, 18, 3, 1);
    b.rect(11, 7, 10, 8, 4);
    b.set(13, 10, 5);
    b.set(17, 10, 5);
    b.rect(14, 12, 4, 1, 9);
    if (frame == 0) {
        b.poly({{8, 16}, {24, 16}, {27, 36}, {5, 36}}, 1);
        b.rect(14, 17, 4, 16, 2);
        b.rect(15, 23, 3, 3, 8);
        b.rect(4, 18, 5, 14, 1);
        b.rect(23, 18, 5, 14, 2);
        b.rect(4, 30, 5, 3, 9);
        b.rect(23, 30, 5, 3, 9);
        b.rect(8, 36, 6, 9, 3);
        b.rect(18, 36, 6, 9, 3);
        b.rect(6, 44, 9, 4, 7);
        b.rect(17, 44, 9, 4, 7);
    } else {
        b.poly({{9, 17}, {25, 15}, {28, 37}, {7, 38}}, 1);
        b.rect(15, 18, 3, 14, 2);
        b.rect(15, 24, 3, 3, 8);
        b.line(12, 20, 1, 11, 1, 3.2f);
        b.line(13, 23, 3, 15, 2, 2.2f);
        b.rect(0, 8, 5, 5, 9);
        b.rect(22, 18, 5, 13, 1);
        b.rect(9, 37, 6, 9, 3);
        b.rect(19, 36, 6, 9, 3);
        b.rect(7, 45, 9, 3, 7);
        b.rect(18, 44, 9, 4, 7);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap raiderArt(int frame) {
    gs::Bitmap b(18, 36);
    b.poly({{9, 1}, {15, 8}, {3, 8}}, 1);
    b.rect(4, 6, 10, 7, 1);
    b.set(7, 9, 5);
    b.set(11, 9, 5);
    b.rect(4, 13, 10, 11, 2);
    b.rect(5, 14, 3, 8, 3);
    float cy = frame ? 9.f : 13.f;
    b.line(12, 16, 17, cy, 6, 2.f);
    b.rect(15, cy - 1.f, 3, 5, 7);
    b.rect(4, 24, 4, 8, 3);
    b.rect(10, frame ? 24 : 25, 4, frame ? 8 : 7, 3);
    b.rect(3, 31, 6, 3, 6);
    b.rect(10, 32, 6, 3, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap handArt() {
    gs::Bitmap b(18, 12);
    b.rect(0, 4, 10, 5, 4);
    b.rect(8, 2, 8, 8, 4);
    b.rect(9, 1, 2, 4, 4);
    b.rect(12, 1, 2, 5, 4);
    b.rect(15, 2, 2, 6, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap ramArt() {
    gs::Bitmap b(56, 16);
    b.rect(2, 4, 42, 8, 2);
    b.rect(2, 4, 42, 2, 3);
    b.rect(2, 10, 42, 2, 1);
    b.ellipse(48, 8, 6, 6, 1);
    b.ellipse(50, 7, 2, 2, 4);
    b.rect(12, 2, 4, 12, 5);
    b.rect(30, 2, 4, 12, 6);
    b.rect(13, 3, 2, 10, 7);
    b.outline(15, false);
    return b;
}

gs::Bitmap lanternArt(int frame) {
    gs::Bitmap b(12, 20);
    b.rect(5, 0, 2, 3, 5);
    b.rect(3, 3, 6, 2, 6);
    b.rect(2, 5, 8, 10, 6);
    b.rect(3, 6, 6, 8, 7);
    if (frame == 0) {
        b.poly({{6, 6}, {9, 12}, {3, 12}}, 2);
        b.poly({{6, 8}, {8, 13}, {4, 13}}, 3);
        b.set(6, 9, 4);
    } else {
        b.poly({{7, 6}, {10, 13}, {3, 11}}, 3);
        b.poly({{6, 9}, {8, 13}, {4, 12}}, 2);
        b.set(6, 10, 4);
    }
    b.rect(3, 15, 6, 2, 5);
    b.rect(4, 17, 4, 2, 6);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(14, 16);
    b.rect(6, 0, 2, 3, 2);
    b.poly({{3, 4}, {11, 4}, {12, 11}, {2, 11}}, 1);
    b.rect(2, 11, 10, 2, 2);
    b.set(7, 8, 4);
    b.ellipse(7, 14, 1.3f, 1.3f, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 1);
    b.ellipse(11, 7, 5, 5, 0);
    b.ellipse(6, 6, 1.3f, 1.3f, 3);
    b.ellipse(7, 10, 1.6f, 1.2f, 3);
    b.set(5, 8, 2);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(5, 5);
    b.set(2, 0, 2);
    b.set(2, 1, 1);
    b.set(0, 2, 2);
    b.set(1, 2, 1);
    b.set(2, 2, 2);
    b.set(3, 2, 1);
    b.set(4, 2, 2);
    b.set(2, 3, 1);
    b.set(2, 4, 2);
    return b;
}

gs::Bitmap tuftArt() {
    gs::Bitmap b(12, 8);
    b.line(2, 7, 1, 1, 2, 1.3f);
    b.line(5, 7, 4, 0, 3, 1.3f);
    b.line(8, 7, 9, 2, 1, 1.3f);
    b.line(6, 7, 7, 3, 2, 1.f);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(20, 30);
    b.rect(8, 16, 4, 13, 4);
    b.rect(9, 16, 1, 13, 5);
    b.ellipse(10, 11, 8, 7, 1);
    b.ellipse(8, 10, 4, 3, 2);
    b.ellipse(12, 8, 2, 2, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(3, 3);
    b.set(1, 0, 4);
    b.set(0, 1, 3);
    b.set(1, 1, 4);
    b.set(2, 1, 2);
    b.set(1, 2, 3);
    return b;
}

gs::Bitmap chipArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    auto C = gs::rgb4;
    const uint16_t ink = C(1, 1, 2);

    setPal(vdp, PAL_HUD, {0, C(15, 14, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOLD, {0, C(15, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(4, 2, 0)});
    setPal(vdp, PAL_ALERT, {0, C(15, 5, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(4, 0, 0)});
    setPal(vdp, PAL_GOOD, {0, C(8, 15, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(1, 3, 1)});

    setPal(vdp, PAL_STONE, {0, C(3, 3, 4), C(5, 5, 6), C(8, 8, 9), C(6, 6, 5), C(3, 5, 3), C(2, 2, 3), C(9, 8, 7),
                            C(5, 5, 6), C(12, 10, 5), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WOOD, {0, C(4, 2, 1), C(6, 3, 1), C(8, 5, 2), C(9, 6, 3), C(3, 3, 4), C(6, 6, 7), C(10, 10, 11),
                           C(12, 9, 4), C(3, 5, 2), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_IRON, {0, C(5, 5, 6), C(9, 9, 10), C(3, 3, 4), C(12, 10, 5), C(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           ink});
    setPal(vdp, PAL_COAT, {0, C(2, 4, 6), C(4, 7, 9), C(1, 2, 4), C(12, 8, 6), C(2, 1, 1), C(4, 3, 2), C(2, 2, 2),
                           C(10, 8, 3), C(11, 7, 5), 0, 0, 0, 0, 0, C(1, 1, 1)});
    setPal(vdp, PAL_RAIDER, {0, C(3, 1, 2), C(5, 2, 2), C(2, 1, 1), C(8, 5, 4), C(14, 13, 8), C(5, 3, 2), C(6, 6, 7), 0,
                             0, 0, 0, 0, 0, 0, C(1, 0, 0)});
    setPal(vdp, PAL_FIRE, {0, C(8, 2, 0), C(14, 6, 1), C(15, 12, 3), C(15, 15, 11), C(3, 3, 4), C(6, 6, 7), C(8, 12, 8),
                           0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MOON, {0, C(13, 13, 11), C(15, 15, 14), C(8, 8, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GRASS, {0, C(1, 3, 1), C(2, 5, 2), C(4, 7, 3), C(4, 3, 2), C(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            C(1, 1, 1)});

    const uint16_t road[16] = {0,          C(2, 4, 2), C(1, 3, 1), C(3, 5, 2), C(3, 3, 2), C(2, 2, 1),
                               C(5, 4, 3), C(3, 3, 2), C(6, 5, 4), C(4, 3, 2), C(2, 2, 1), C(1, 2, 3),
                               C(1, 1, 2), C(2, 3, 2), C(8, 7, 4), C(7, 6, 5)};
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    vdp.setFogColor(C(2, 2, 5));
    vdp.A.enabled = false;
    vdp.B.enabled = false;

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);

    art.pier = gs::uploadMipped(vdp, pierArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.leaf = gs::uploadMipped(vdp, leafArt());
    art.latch = gs::uploadMipped(vdp, latchArt());
    art.hinge = gs::uploadMipped(vdp, hingeArt());
    art.catchPlate = gs::uploadMipped(vdp, catchArt());
    art.wedge = gs::uploadMipped(vdp, wedgeArt());
    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.raider[0] = gs::uploadMipped(vdp, raiderArt(0));
    art.raider[1] = gs::uploadMipped(vdp, raiderArt(1));
    art.hand = gs::uploadMipped(vdp, handArt());
    art.ram = gs::uploadMipped(vdp, ramArt());
    art.lantern[0] = gs::uploadMipped(vdp, lanternArt(0));
    art.lantern[1] = gs::uploadMipped(vdp, lanternArt(1));
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.chip = gs::uploadMipped(vdp, chipArt());
}

}  // namespace gatedoor
