#include "game/art.h"

#include <initializer_list>
#include <string>

namespace dcler {
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

void courses(gs::Bitmap& b, int x, int y, int w, int h, int brick, int shade, int mortar) {
    b.rect(float(x), float(y), float(w), float(h), brick);
    b.rect(float(x), float(y), 6.f, float(h), shade);
    for (int row = y; row < y + h; row += 5) {
        b.rect(float(x), float(row), float(w), 1.f, mortar);
        int shift = ((row - y) / 5) & 1 ? 7 : 1;
        for (int col = x + shift; col < x + w; col += 14) b.rect(float(col), float(row), 1.f, 5.f, mortar);
    }
}

gs::Bitmap clerkArt(int step) {
    gs::Bitmap b(40, 64);
    b.ellipse(20, 8, 9, 4, 5);
    b.rect(12, 8, 16, 3, 5);
    b.ellipse(20, 15, 7, 7, 4);
    b.rect(16, 12, 8, 3, 8);
    b.set(17, 15, 9);
    b.set(23, 15, 9);
    b.rect(17, 19, 6, 2, 10);
    b.poly({{11, 23}, {29, 23}, {32, 42}, {8, 42}}, 1);
    b.poly({{11, 23}, {20, 23}, {18, 42}, {8, 42}}, 2);
    b.rect(16, 24, 8, 10, 7);
    b.rect(18, 28, 4, 6, 10);
    b.line(28, 28, 36, 40, 6, 2.4f);
    b.rect(33, 38, 5, 3, 6);
    if (step == 0) {
        b.rect(13, 42, 6, 14, 3);
        b.rect(22, 42, 6, 12, 3);
        b.rect(11, 54, 9, 4, 6);
        b.rect(21, 52, 9, 4, 6);
    } else {
        b.rect(13, 42, 6, 12, 3);
        b.rect(22, 42, 6, 14, 3);
        b.rect(12, 52, 9, 4, 6);
        b.rect(20, 54, 9, 4, 6);
    }
    b.outline(9, false);
    return b;
}

gs::Bitmap dollyArt() {
    gs::Bitmap b(34, 40);
    b.line(8, 6, 8, 30, 5, 2.4f);
    b.line(16, 4, 16, 28, 5, 2.4f);
    b.line(8, 8, 16, 6, 3, 2.f);
    b.rect(6, 28, 16, 4, 5);
    b.rect(4, 31, 10, 3, 7);
    b.ellipse(10, 34, 5, 5, 6);
    b.ellipse(22, 33, 5, 5, 6);
    b.ellipse(10, 34, 2, 2, 3);
    b.ellipse(22, 33, 2, 2, 3);
    b.outline(8, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(36, 32);
    b.poly({{4, 10}, {30, 7}, {33, 26}, {6, 29}}, 1);
    b.poly({{4, 10}, {16, 8}, {16, 27}, {6, 29}}, 2);
    b.line(5, 16, 31, 13, 4, 1.6f);
    b.line(6, 22, 32, 19, 4, 1.6f);
    b.line(16, 8, 17, 27, 3, 1.6f);
    b.rect(22, 4, 8, 4, 1);
    b.outline(5, false);
    return b;
}

gs::Bitmap drumArt() {
    gs::Bitmap b(30, 36);
    b.ellipse(15, 8, 11, 5, 3);
    b.rect(4, 8, 22, 20, 1);
    b.ellipse(15, 28, 11, 5, 2);
    b.rect(4, 12, 22, 3, 4);
    b.rect(4, 22, 22, 3, 4);
    b.rect(4, 8, 3, 20, 2);
    b.ellipse(15, 8, 6, 2, 3);
    b.outline(8, false);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(32, 34);
    b.ellipse(16, 20, 12, 11, 1);
    b.ellipse(13, 16, 7, 6, 2);
    b.poly({{12, 8}, {20, 6}, {18, 14}, {11, 14}}, 3);
    b.rect(14, 4, 4, 6, 5);
    b.rect(10, 18, 8, 6, 4);
    b.outline(3, false);
    return b;
}

gs::Bitmap boardsArt() {
    gs::Bitmap b(44, 22);
    b.rect(2, 6, 40, 4, 1);
    b.rect(2, 11, 40, 4, 2);
    b.rect(4, 16, 36, 3, 1);
    b.rect(6, 4, 3, 16, 3);
    b.rect(34, 4, 3, 16, 3);
    b.rect(18, 5, 3, 14, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap carArt() {
    gs::Bitmap b(120, 78);
    b.rect(6, 16, 108, 44, 1);
    b.rect(6, 16, 108, 8, 2);
    b.rect(8, 28, 104, 12, 3);
    b.rect(46, 24, 28, 34, 4);
    b.rect(48, 26, 24, 10, 7);
    b.rect(4, 56, 112, 6, 6);
    b.rect(10, 60, 16, 8, 5);
    b.rect(90, 60, 16, 8, 5);
    b.ellipse(18, 66, 7, 7, 5);
    b.ellipse(98, 66, 7, 7, 5);
    b.ellipse(18, 66, 3, 3, 6);
    b.ellipse(98, 66, 3, 3, 6);
    b.rect(2, 48, 6, 8, 6);
    b.rect(112, 48, 6, 8, 6);
    gs::Bitmap label = gs::textBitmap("S3", {2, 2, 0, 0, 1});
    b.blit(label, 18, 30);
    b.outline(9, false);
    return b;
}

gs::Bitmap cargoArt() {
    gs::Bitmap b(22, 16);
    b.rect(2, 4, 14, 10, 1);
    b.rect(3, 5, 12, 3, 2);
    b.ellipse(16, 10, 5, 5, 3);
    b.rect(6, 8, 8, 2, 4);
    return b;
}

gs::Bitmap wallArt() {
    gs::Bitmap b(300, 96);
    b.rect(0, 10, 300, 8, 6);
    b.rect(0, 8, 300, 4, 7);
    courses(b, 0, 18, 300, 62, 1, 2, 3);
    b.rect(0, 78, 300, 16, 7);
    b.rect(0, 78, 300, 3, 6);
    auto window = [&](int x) {
        b.rect(float(x), 28, 22, 26, 6);
        b.rect(float(x + 2), 30, 18, 22, 4);
        b.rect(float(x + 3), 31, 7, 8, 5);
        b.rect(float(x + 2), 40, 18, 1, 6);
        b.rect(float(x + 10), 30, 1, 22, 6);
    };
    window(18);
    window(58);
    window(214);
    window(256);
    b.rect(118, 36, 64, 56, 8);
    b.rect(124, 42, 52, 16, 4);
    b.rect(126, 44, 16, 6, 5);
    b.rect(128, 64, 44, 3, 6);
    b.rect(146, 70, 6, 8, 9);
    gs::Bitmap sign = gs::textBitmap("DEPOT", {2, 5, 0, 0, 1});
    b.rect(112, 20, float(sign.w + 8), float(sign.h + 4), 8);
    b.blit(sign, 116, 22);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(36, 36);
    b.ellipse(18, 18, 16, 16, 3);
    b.ellipse(18, 18, 13, 13, 2);
    b.ellipse(18, 18, 11, 11, 1);
    b.rect(17, 8, 2, 10, 3);
    b.rect(17, 16, 8, 2, 3);
    b.rect(16, 2, 4, 4, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(18, 36);
    b.rect(8, 0, 2, 10, 3);
    b.poly({{3, 10}, {15, 10}, {16, 20}, {2, 20}}, 1);
    b.ellipse(9, 15, 4, 3, 2);
    b.rect(6, 20, 6, 12, 3);
    b.rect(4, 30, 10, 3, 4);
    b.outline(4, false);
    return b;
}

gs::Bitmap stackArt() {
    gs::Bitmap b(48, 36);
    b.rect(4, 6, 40, 6, 1);
    b.rect(6, 12, 36, 6, 2);
    b.rect(4, 18, 40, 6, 1);
    b.rect(8, 24, 32, 6, 2);
    b.rect(10, 4, 4, 26, 3);
    b.rect(34, 4, 4, 26, 3);
    b.outline(5, false);
    return b;
}

gs::Bitmap chimneyArt() {
    gs::Bitmap b(28, 70);
    b.rect(6, 10, 16, 56, 1);
    b.rect(6, 10, 5, 56, 2);
    b.rect(4, 6, 20, 8, 3);
    b.rect(8, 0, 12, 8, 4);
    for (int y = 18; y < 60; y += 8) b.rect(6, y, 16, 2, 3);
    b.outline(5, false);
    return b;
}

gs::Bitmap railsArt() {
    gs::Bitmap b(320, 16);
    for (int x = 4; x < 316; x += 16) b.rect(float(x), 4, 8, 8, 2);
    b.rect(0, 3, 320, 2, 1);
    b.rect(0, 11, 320, 2, 1);
    return b;
}

gs::Bitmap truckArt() {
    gs::Bitmap b(52, 36);
    b.rect(8, 10, 28, 14, 1);
    b.rect(8, 10, 10, 14, 2);
    b.rect(30, 14, 16, 4, 5);
    b.rect(36, 16, 12, 2, 3);
    b.rect(14, 6, 6, 6, 4);
    b.ellipse(16, 26, 5, 5, 6);
    b.ellipse(32, 26, 5, 5, 6);
    b.ellipse(16, 26, 2, 2, 3);
    b.ellipse(32, 26, 2, 2, 3);
    b.outline(8, false);
    return b;
}

gs::Bitmap steamArt() {
    gs::Bitmap b(20, 14);
    b.ellipse(8, 8, 6, 4, 1);
    b.ellipse(13, 6, 5, 4, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 12);
    b.ellipse(8, 7, 6, 3, 1);
    b.ellipse(6, 6, 3, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(36, 10);
    b.ellipse(18, 5, 15, 3, 1);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(16, 12);
    b.poly({{2, 2}, {8, 10}, {14, 2}, {11, 2}, {8, 6}, {5, 2}}, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 15, 14), gs::rgb4(10, 10, 12), gs::rgb4(4, 4, 6)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 14, 10), gs::rgb4(6, 5, 3), gs::rgb4(3, 3, 3),
                           gs::rgb4(8, 7, 5)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 12, 9), gs::rgb4(6, 2, 2), gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 6), gs::rgb4(14, 15, 12), gs::rgb4(2, 5, 2), gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_BRICK,
           {0, gs::rgb4(11, 5, 3), gs::rgb4(8, 3, 2), gs::rgb4(12, 10, 8), gs::rgb4(2, 3, 6), gs::rgb4(13, 9, 4),
            gs::rgb4(9, 8, 7), gs::rgb4(4, 3, 3), gs::rgb4(2, 2, 3), gs::rgb4(6, 3, 2), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_CLERK,
           {0, gs::rgb4(14, 7, 2), gs::rgb4(10, 4, 1), gs::rgb4(3, 3, 6), gs::rgb4(13, 9, 6), gs::rgb4(2, 3, 5),
            gs::rgb4(2, 2, 2), gs::rgb4(14, 13, 11), gs::rgb4(4, 3, 2), gs::rgb4(1, 1, 1), gs::rgb4(8, 2, 2)});
    setPal(vdp, PAL_SACK,
           {0, gs::rgb4(11, 9, 5), gs::rgb4(8, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(12, 3, 2), gs::rgb4(13, 12, 8)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 2), gs::rgb4(10, 10, 11), gs::rgb4(3, 2, 1),
            gs::rgb4(14, 11, 6)});
    setPal(vdp, PAL_STEEL,
           {0, gs::rgb4(5, 8, 6), gs::rgb4(3, 5, 4), gs::rgb4(12, 12, 13), gs::rgb4(14, 11, 2), gs::rgb4(6, 6, 7),
            gs::rgb4(2, 2, 2), gs::rgb4(8, 4, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(10, 9, 8), gs::rgb4(13, 12, 10), gs::rgb4(8, 8, 9)});
    setPal(vdp, PAL_NIGHT,
           {0, gs::rgb4(1, 2, 4), gs::rgb4(3, 4, 7), gs::rgb4(8, 8, 11), gs::rgb4(12, 12, 14), gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_CAR,
           {0, gs::rgb4(12, 3, 2), gs::rgb4(7, 2, 2), gs::rgb4(13, 11, 8), gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 3),
            gs::rgb4(5, 5, 6), gs::rgb4(4, 5, 7), gs::rgb4(14, 12, 4), gs::rgb4(2, 1, 1)});

    const uint16_t dock[16] = {
        0,
        gs::rgb4(6, 6, 6), gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 7),
        gs::rgb4(7, 7, 6), gs::rgb4(5, 5, 5),
        gs::rgb4(9, 9, 8), gs::rgb4(7, 7, 6),
        gs::rgb4(10, 10, 9), gs::rgb4(5, 5, 5), gs::rgb4(12, 10, 3),
        gs::rgb4(4, 5, 6), gs::rgb4(3, 4, 5), gs::rgb4(6, 7, 8),
        gs::rgb4(13, 11, 2), gs::rgb4(6, 6, 6),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_DOCK * 16 + i, dock[i]);

    loadFont(vdp, art);
    art.clerk[0] = gs::uploadMipped(vdp, clerkArt(0));
    art.clerk[1] = gs::uploadMipped(vdp, clerkArt(1));
    art.dolly = gs::uploadMipped(vdp, dollyArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.boards = gs::uploadMipped(vdp, boardsArt());
    art.car = gs::uploadMipped(vdp, carArt());
    art.cargo = gs::uploadMipped(vdp, cargoArt());
    art.wall = gs::uploadMipped(vdp, wallArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.stack = gs::uploadMipped(vdp, stackArt());
    art.chimney = gs::uploadMipped(vdp, chimneyArt());
    art.rails = gs::uploadMipped(vdp, railsArt());
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.steam = gs::uploadMipped(vdp, steamArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.mark = gs::uploadMipped(vdp, markArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(5, 5, 7));
}

}  // namespace dcler
