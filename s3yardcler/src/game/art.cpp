#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace yardcler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap keeperArt(int step) {
    gs::Bitmap b(48, 64);
    b.ellipse(22, 9, 8, 5, 5);
    b.ellipse(22, 11, 13, 3, 5);
    b.rect(14, 10, 16, 3, 6);
    b.ellipse(22, 18, 7, 7, 3);
    b.rect(16, 14, 12, 4, 5);
    b.set(19, 18, 12);
    b.set(25, 18, 12);
    b.rect(20, 22, 4, 1, 13);
    b.poly({{12, 26}, {34, 26}, {36, 44}, {10, 44}}, 1);
    b.poly({{12, 26}, {22, 26}, {20, 44}, {10, 44}}, 2);
    b.rect(20, 26, 3, 12, 2);
    b.rect(15, 28, 6, 4, 14);
    if (step == 2) {
        b.line(30, 32, 44, 40, 3, 2.2f);
        b.line(34, 36, 46, 44, 9, 2.2f);
        b.line(44, 40, 46, 48, 10, 1.6f);
        b.line(42, 42, 44, 50, 10, 1.4f);
        b.line(46, 42, 48, 49, 10, 1.4f);
    } else {
        b.line(30, 30, 42, 26, 3, 2.2f);
        b.line(36, 28, 46, 18, 9, 2.2f);
        b.line(44, 16, 46, 22, 10, 1.5f);
        b.line(42, 15, 44, 21, 10, 1.3f);
        b.line(46, 15, 47, 22, 10, 1.3f);
    }
    if (step == 1) {
        b.rect(14, 44, 6, 12, 7);
        b.rect(24, 44, 6, 14, 7);
        b.rect(12, 54, 9, 4, 8);
        b.rect(22, 56, 9, 4, 8);
    } else {
        b.rect(14, 44, 6, 14, 7);
        b.rect(24, 44, 6, 12, 7);
        b.rect(12, 56, 9, 4, 8);
        b.rect(22, 54, 9, 4, 8);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap barrowArt() {
    gs::Bitmap b(46, 32);
    b.poly({{10, 8}, {34, 6}, {38, 18}, {8, 20}}, 1);
    b.poly({{10, 8}, {20, 7}, {20, 19}, {8, 20}}, 2);
    b.line(6, 10, 2, 4, 3, 2.f);
    b.line(14, 8, 10, 2, 3, 2.f);
    b.rect(8, 18, 28, 3, 1);
    b.line(30, 18, 40, 26, 3, 2.f);
    b.ellipse(40, 26, 5, 5, 4);
    b.ellipse(40, 26, 2, 2, 5);
    b.rect(16, 20, 3, 7, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap leavesArt() {
    gs::Bitmap b(36, 24);
    b.ellipse(12, 14, 8, 5, 1);
    b.ellipse(22, 13, 9, 6, 2);
    b.ellipse(18, 16, 10, 5, 4);
    b.ellipse(26, 15, 6, 4, 3);
    b.ellipse(15, 12, 4, 3, 6);
    b.ellipse(8, 15, 4, 3, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap sticksArt() {
    gs::Bitmap b(40, 20);
    b.line(4, 14, 34, 6, 1, 2.2f);
    b.line(6, 16, 36, 8, 2, 2.f);
    b.line(8, 12, 32, 4, 3, 1.6f);
    b.line(10, 15, 30, 10, 1, 1.4f);
    b.rect(16, 8, 4, 8, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap branchArt() {
    gs::Bitmap b(56, 22);
    b.line(4, 14, 50, 8, 1, 3.2f);
    b.line(6, 16, 48, 12, 2, 2.f);
    b.line(18, 12, 14, 4, 3, 1.8f);
    b.line(34, 10, 40, 3, 3, 1.6f);
    b.ellipse(14, 4, 3, 2, 4);
    b.ellipse(42, 3, 3, 2, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap toysArt() {
    gs::Bitmap b(34, 28);
    b.ellipse(12, 16, 8, 8, 1);
    b.ellipse(10, 14, 3, 2, 2);
    b.rect(20, 8, 10, 10, 3);
    b.rect(22, 10, 6, 6, 5);
    b.rect(18, 18, 12, 8, 4);
    b.rect(22, 20, 4, 4, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap clippingsArt() {
    gs::Bitmap b(34, 24);
    b.ellipse(16, 14, 12, 7, 1);
    b.ellipse(10, 13, 6, 4, 2);
    b.ellipse(22, 12, 7, 4, 3);
    b.ellipse(16, 15, 5, 3, 4);
    b.rect(8, 14, 16, 1, 5);
    b.rect(12, 16, 10, 1, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap stonesArt() {
    gs::Bitmap b(34, 18);
    b.ellipse(10, 11, 7, 5, 1);
    b.ellipse(20, 12, 8, 5, 2);
    b.ellipse(28, 11, 5, 4, 3);
    b.ellipse(16, 10, 3, 2, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap rakedArt() {
    gs::Bitmap b(40, 16);
    b.ellipse(20, 8, 16, 5, 1);
    b.ellipse(14, 8, 6, 2, 2);
    b.line(8, 8, 32, 7, 3, 1.f);
    b.line(10, 10, 30, 10, 3, 1.f);
    return b;
}

gs::Bitmap binArt() {
    gs::Bitmap b(36, 48);
    b.poly({{6, 14}, {30, 14}, {33, 42}, {3, 42}}, 1);
    b.poly({{6, 14}, {16, 14}, {14, 42}, {3, 42}}, 2);
    b.ellipse(18, 14, 12, 4, 4);
    b.ellipse(18, 14, 8, 2, 5);
    for (int y = 18; y < 40; y += 5) b.rect(5, float(y), 26, 2, 3);
    b.rect(4, 40, 28, 4, 2);
    b.rect(8, 6, 3, 12, 3);
    b.rect(25, 6, 3, 12, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap houseArt() {
    gs::Bitmap b(164, 78);
    b.poly({{8, 34}, {82, 8}, {156, 34}}, 3);
    b.poly({{8, 34}, {82, 8}, {82, 34}}, 4);
    b.rect(118, 4, 16, 26, 11);
    b.rect(118, 4, 16, 3, 12);
    b.rect(118, 12, 16, 2, 12);
    b.rect(118, 20, 16, 2, 12);
    b.rect(16, 32, 132, 32, 1);
    b.rect(16, 32, 10, 32, 2);
    b.rect(24, 38, 28, 18, 5);
    b.rect(26, 40, 24, 14, 6);
    b.rect(37, 40, 2, 14, 5);
    b.rect(26, 46, 24, 2, 5);
    b.rect(112, 38, 28, 18, 5);
    b.rect(114, 40, 24, 14, 7);
    b.rect(125, 40, 2, 14, 5);
    b.rect(114, 46, 24, 2, 5);
    b.rect(68, 40, 24, 24, 8);
    b.rect(70, 42, 20, 20, 9);
    b.rect(78, 42, 2, 20, 8);
    b.set(86, 52, 14);
    b.rect(8, 62, 148, 8, 10);
    b.rect(8, 62, 148, 2, 2);
    b.rect(20, 62, 4, 12, 10);
    b.rect(140, 62, 4, 12, 10);
    b.rect(62, 70, 40, 4, 13);
    b.rect(40, 36, 6, 8, 2);
    b.ellipse(148, 58, 5, 3, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(72, 104);
    b.rect(32, 62, 10, 36, 5);
    b.rect(32, 62, 3, 36, 6);
    b.ellipse(36, 40, 26, 22, 1);
    b.ellipse(24, 46, 16, 14, 3);
    b.ellipse(48, 44, 16, 14, 2);
    b.ellipse(36, 28, 14, 12, 2);
    b.ellipse(22, 36, 3, 3, 4);
    b.ellipse(44, 32, 3, 3, 4);
    b.ellipse(52, 50, 3, 3, 4);
    b.ellipse(30, 52, 2, 2, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap fenceArt() {
    gs::Bitmap b(24, 40);
    for (int i = 0; i < 3; i++) {
        float x = 3.f + float(i) * 7.f;
        b.rect(x, 4, 4, 30, 1);
        b.poly({{x, 4}, {x + 2, 1}, {x + 4, 4}}, 1);
    }
    b.rect(2, 12, 20, 3, 2);
    b.rect(2, 24, 20, 3, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap flowerArt() {
    gs::Bitmap b(14, 20);
    b.line(7, 10, 7, 18, 4, 1.6f);
    b.line(7, 14, 3, 16, 5, 1.4f);
    b.line(7, 13, 11, 16, 5, 1.4f);
    b.ellipse(7, 7, 4, 4, 1);
    b.ellipse(4, 6, 2, 2, 2);
    b.ellipse(10, 6, 2, 2, 2);
    b.ellipse(7, 4, 2, 2, 2);
    b.ellipse(7, 7, 1, 1, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap hoseArt() {
    gs::Bitmap b(28, 22);
    b.ellipse(14, 12, 9, 6, 1);
    b.ellipse(14, 12, 5, 3, 2);
    b.ellipse(14, 12, 2, 2, 3);
    b.rect(20, 8, 6, 3, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap canArt() {
    gs::Bitmap b(26, 24);
    b.poly({{6, 8}, {16, 8}, {18, 18}, {4, 18}}, 1);
    b.ellipse(11, 8, 6, 3, 2);
    b.rect(16, 6, 8, 3, 3);
    b.line(8, 8, 4, 2, 4, 1.6f);
    b.line(4, 2, 8, 4, 4, 1.4f);
    b.outline(15, false);
    return b;
}

gs::Bitmap lineArt() {
    gs::Bitmap b(88, 26);
    b.line(2, 6, 84, 6, 9, 1.4f);
    b.rect(16, 6, 12, 10, 1);
    b.rect(18, 8, 8, 6, 2);
    b.rect(52, 6, 14, 12, 7);
    b.rect(54, 8, 10, 8, 5);
    b.rect(14, 4, 2, 4, 6);
    b.rect(66, 4, 2, 4, 6);
    return b;
}

gs::Bitmap birdArt(int flap) {
    gs::Bitmap b(20, 14);
    b.ellipse(10, 8, 6, 3, 1);
    b.ellipse(8, 8, 3, 2, 4);
    if (flap) {
        b.line(4, 8, 1, 3, 2, 1.8f);
        b.line(15, 8, 18, 3, 2, 1.8f);
    } else {
        b.line(4, 8, 1, 11, 2, 1.8f);
        b.line(15, 8, 18, 11, 2, 1.8f);
    }
    b.rect(15, 7, 4, 2, 3);
    b.set(12, 7, 15);
    b.outline(15, false);
    b.set(12, 7, 15);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 8, 8, 1);
    b.ellipse(14, 14, 5, 5, 2);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785f;
        float c = std::cos(a), s = std::sin(a);
        b.line(14 + c * 8, 14 + s * 8, 14 + c * 13, 14 + s * 13, 1, 1.4f);
    }
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(44, 18);
    b.ellipse(16, 10, 10, 5, 1);
    b.ellipse(26, 8, 12, 6, 1);
    b.ellipse(32, 11, 8, 4, 2);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(24, 24);
    b.ellipse(12, 12, 10, 10, 1);
    b.ellipse(12, 12, 8, 8, 2);
    b.line(12, 12, 12, 6, 3, 1.5f);
    b.line(12, 12, 16, 13, 3, 1.4f);
    b.ellipse(12, 12, 1, 1, 4);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(14, 10);
    b.ellipse(7, 6, 5, 3, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 10);
    b.ellipse(16, 5, 13, 3, 1);
    return b;
}

gs::Bitmap aimArt() {
    gs::Bitmap b(16, 12);
    b.poly({{2, 2}, {8, 10}, {14, 2}, {11, 2}, {8, 6}, {5, 2}}, 1);
    return b;
}

int loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 1);
    gs::TextStyle big{3, 1, 0, 0, 1};
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
    return tiles.used();
}

void paintLawn(gs::VDP& vdp, int tileStart) {
    gs::Bitmap b(gs::SCREEN_W, gs::SCREEN_H);
    for (int y = 96; y < gs::SCREEN_H; y++) {
        for (int x = 0; x < gs::SCREEN_W; x++) {
            int h = (x * 17 + y * 13) & 31;
            int base = ((x >> 3) + (y >> 3)) & 1 ? 12 : 2;
            if (h == 0) base = 3;
            else if (h == 1) base = 1;
            if (y > 216) base = 14;
            b.set(x, y, base);
        }
    }
    for (int y = 126; y < 142; y++) {
        for (int x = 86; x < 230; x++) {
            int k = (x * 3 + y * 5) & 7;
            int c = 9;
            if (k == 0) c = 7;
            else if (k == 1) c = 8;
            else if (k == 2) c = 11;
            b.set(x, y, c);
        }
    }
    for (int y = 132; y < 156; y++) {
        for (int x = 8; x < 70; x++) {
            int k = (x * 5 + y * 3) & 11;
            if (k == 0) b.set(x, y, 7);
            else if (k == 1) b.set(x, y, 8);
            else if (k < 4) b.set(x, y, 9);
        }
    }
    b.ellipse(54, 170, 24, 14, 6);
    b.ellipse(50, 168, 10, 5, 13);
    auto stone = [&](float cx, float cy) {
        b.ellipse(cx, cy, 8, 4, 4);
        b.ellipse(cx - 2, cy - 1, 4, 2, 5);
    };
    stone(158, 140);
    stone(148, 156);
    stone(112, 162);
    stone(74, 168);
    stone(176, 186);
    stone(154, 198);
    b.rect(146, 124, 28, 7, 6);
    b.rect(148, 126, 24, 3, 13);

    gs::TileAlloc tiles(vdp, tileStart);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, b, PAL_LAWN);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 15, 14), gs::rgb4(11, 12, 10), gs::rgb4(3, 4, 3)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 14, 10), gs::rgb4(8, 5, 2), gs::rgb4(3, 2, 1),
                           gs::rgb4(13, 9, 4)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 12, 9), gs::rgb4(6, 2, 2), gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 6), gs::rgb4(14, 15, 12), gs::rgb4(2, 6, 2), gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_KEEPER,
           {0, gs::rgb4(3, 11, 4), gs::rgb4(2, 7, 2), gs::rgb4(13, 9, 6), gs::rgb4(9, 6, 4), gs::rgb4(13, 10, 3),
            gs::rgb4(8, 5, 2), gs::rgb4(4, 4, 7), gs::rgb4(4, 2, 1), gs::rgb4(9, 6, 3), gs::rgb4(9, 10, 9),
            gs::rgb4(1, 1, 1), gs::rgb4(2, 2, 2), gs::rgb4(10, 4, 3), gs::rgb4(15, 13, 8)});
    setPal(vdp, PAL_LEAF,
           {0, gs::rgb4(13, 7, 2), gs::rgb4(14, 11, 3), gs::rgb4(12, 3, 2), gs::rgb4(7, 4, 2), gs::rgb4(4, 2, 1),
            gs::rgb4(15, 12, 6)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(11, 7, 3), gs::rgb4(7, 4, 2), gs::rgb4(13, 9, 5), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 3),
            gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_TOY,
           {0, gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(3, 5, 12), gs::rgb4(14, 12, 2), gs::rgb4(14, 14, 13),
            gs::rgb4(2, 3, 8)});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(11, 11, 10), gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 4), gs::rgb4(5, 7, 3), gs::rgb4(8, 5, 2),
            gs::rgb4(13, 8, 3)});
    setPal(vdp, PAL_HOUSE,
           {0, gs::rgb4(14, 12, 9), gs::rgb4(9, 7, 5), gs::rgb4(12, 3, 2), gs::rgb4(7, 2, 2), gs::rgb4(15, 14, 12),
            gs::rgb4(6, 9, 12), gs::rgb4(12, 14, 15), gs::rgb4(6, 3, 2), gs::rgb4(4, 2, 1), gs::rgb4(9, 6, 3),
            gs::rgb4(9, 4, 3), gs::rgb4(5, 4, 4), gs::rgb4(8, 6, 4), gs::rgb4(14, 11, 3)});
    setPal(vdp, PAL_TREE,
           {0, gs::rgb4(3, 9, 3), gs::rgb4(6, 12, 4), gs::rgb4(2, 6, 2), gs::rgb4(12, 2, 2), gs::rgb4(6, 4, 2),
            gs::rgb4(4, 2, 1), gs::rgb4(8, 10, 4)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(10, 9, 6), gs::rgb4(13, 11, 7), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_BIRD,
           {0, gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 9), gs::rgb4(13, 8, 2), gs::rgb4(13, 13, 11)});
    setPal(vdp, PAL_BLOOM,
           {0, gs::rgb4(13, 3, 4), gs::rgb4(15, 9, 8), gs::rgb4(14, 11, 2), gs::rgb4(3, 8, 2), gs::rgb4(4, 10, 3)});

    const uint16_t lawn[16] = {
        0,
        gs::rgb4(11, 14, 5), gs::rgb4(7, 12, 3), gs::rgb4(4, 8, 2),
        gs::rgb4(12, 10, 6), gs::rgb4(8, 7, 5), gs::rgb4(6, 4, 2),
        gs::rgb4(13, 3, 3), gs::rgb4(14, 12, 3), gs::rgb4(2, 6, 2),
        gs::rgb4(9, 11, 6), gs::rgb4(14, 8, 9), gs::rgb4(5, 9, 3),
        gs::rgb4(8, 6, 3), gs::rgb4(3, 5, 2), gs::rgb4(2, 3, 1),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_LAWN * 16 + i, lawn[i]);

    int next = loadFont(vdp, art);
    paintLawn(vdp, next);

    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.keeper[2] = gs::uploadMipped(vdp, keeperArt(2));
    art.barrow = gs::uploadMipped(vdp, barrowArt());
    art.leaves = gs::uploadMipped(vdp, leavesArt());
    art.sticks = gs::uploadMipped(vdp, sticksArt());
    art.branch = gs::uploadMipped(vdp, branchArt());
    art.toys = gs::uploadMipped(vdp, toysArt());
    art.clippings = gs::uploadMipped(vdp, clippingsArt());
    art.stones = gs::uploadMipped(vdp, stonesArt());
    art.raked = gs::uploadMipped(vdp, rakedArt());
    art.bin = gs::uploadMipped(vdp, binArt());
    art.house = gs::uploadMipped(vdp, houseArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.fence = gs::uploadMipped(vdp, fenceArt());
    art.flower = gs::uploadMipped(vdp, flowerArt());
    art.hose = gs::uploadMipped(vdp, hoseArt());
    art.can = gs::uploadMipped(vdp, canArt());
    art.line = gs::uploadMipped(vdp, lineArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(0));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(1));
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.aim = gs::uploadMipped(vdp, aimArt());

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    vdp.B.scroll(0, 0);
    vdp.setFogColor(gs::rgb4(8, 10, 12));
}

}  // namespace yardcler
