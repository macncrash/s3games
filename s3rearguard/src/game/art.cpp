#include "game/art.h"

#include <initializer_list>
#include <string>

namespace rearguard {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

// Back view. The rifle points up the bitmap, which is up the road.
Bitmap backMan(int step, bool player) {
    Bitmap b(48, 80);
    b.ellipse(24, 12, 9, 8, 4);
    b.ellipse(24, 14, 7, 4, player ? 11 : 2);
    b.rect(16, 22, 16, 12, 8);
    b.rect(18, 24, 12, 8, 3);
    b.poly({{13, 20}, {35, 20}, {39, 48}, {9, 48}}, 2);
    b.poly({{16, 21}, {32, 21}, {34, 40}, {14, 40}}, 1);
    if (player) b.rect(18, 20, 12, 4, 11);
    b.rect(32, 6, 3, 38, 7);
    b.rect(30, 30, 8, 5, 6);
    b.line(14, 26, 8, 40, 5, 3);
    int s = step ? 5 : -4;
    b.rect(15, 48, 7, 18 + s, 3);
    b.rect(26, 48, 7, 18 - s, 3);
    b.rect(14, 64 + s, 8, 5, 9);
    b.rect(26, 64 - s, 8, 5, 9);
    b.outline(15, false);
    return b;
}

Bitmap frontMan(int step, int kind) {
    Bitmap b(44, 76);
    // kind 0 rifle, 1 runner, 3 officer. A cap instead of a helmet on the runner.
    if (kind == 1) b.ellipse(22, 11, 8, 4, 4);
    else b.ellipse(22, 11, 9, 8, kind == 3 ? 10 : 4);
    if (kind == 3) b.rect(20, 1, 4, 10, 11);
    b.ellipse(22, 20, 7, 7, 5);
    b.set(19, 19, 8);
    b.set(25, 19, 8);
    b.poly({{12, 26}, {32, 26}, {36, 50}, {8, 50}}, 2);
    b.poly({{15, 27}, {29, 27}, {31, 44}, {13, 44}}, 1);
    if (kind == 0 || kind == 3) {
        b.rect(8, 32, 28, 3, 7);
        b.rect(8, 30, 6, 6, 6);
    } else {
        b.line(12, 30, 4, 40, 5, 3);
        b.line(32, 30, 40, 40, 5, 3);
    }
    int s = step ? 4 : -3;
    b.rect(13, 50, 6, 16 + s, 3);
    b.rect(24, 50, 6, 16 - s, 3);
    b.rect(12, 64 + s, 8, 4, 9);
    b.rect(23, 64 - s, 8, 4, 9);
    b.outline(15, false);
    return b;
}

Bitmap horseArt() {
    Bitmap b(72, 80);
    b.ellipse(36, 28, 16, 14, 2);
    b.ellipse(36, 24, 10, 8, 1);
    b.ellipse(36, 16, 7, 8, 2);
    b.ellipse(28, 12, 4, 6, 9);
    b.ellipse(44, 12, 4, 6, 9);
    b.ellipse(36, 18, 3, 3, 10);
    b.set(34, 16, 8);
    b.set(39, 16, 8);
    b.rect(22, 40, 6, 28, 3);
    b.rect(32, 42, 6, 26, 3);
    b.rect(42, 42, 6, 26, 2);
    b.rect(50, 40, 6, 28, 3);
    b.rect(20, 66, 8, 4, 8);
    b.rect(48, 66, 8, 4, 8);
    b.ellipse(36, 22, 8, 8, 4);
    b.ellipse(36, 12, 6, 5, 6);
    b.rect(34, 4, 4, 8, 6);
    b.rect(30, 28, 12, 8, 4);
    b.outline(15, false);
    return b;
}

Bitmap wagonArt() {
    Bitmap b(64, 52);
    b.poly({{8, 18}, {32, 6}, {56, 18}, {52, 28}, {12, 28}}, 6);
    b.rect(10, 26, 44, 14, 3);
    b.rect(14, 30, 36, 8, 2);
    b.ellipse(16, 42, 8, 8, 7);
    b.ellipse(48, 42, 8, 8, 7);
    b.ellipse(16, 42, 3, 3, 5);
    b.ellipse(48, 42, 3, 3, 5);
    b.outline(15, false);
    return b;
}

Bitmap gateArt() {
    Bitmap b(96, 112);
    b.rect(4, 28, 22, 80, 5);
    b.rect(8, 10, 14, 22, 5);
    b.rect(6, 32, 18, 14, 6);
    b.rect(70, 28, 22, 80, 5);
    b.rect(74, 10, 14, 22, 5);
    b.rect(72, 32, 18, 14, 6);
    b.rect(22, 40, 52, 68, 6);
    b.rect(34, 62, 28, 46, 4);
    b.rect(40, 74, 16, 30, 7);
    b.rect(46, 2, 3, 16, 9);
    b.poly({{49, 3}, {72, 9}, {49, 15}}, 8);
    b.outline(15, false);
    return b;
}

Bitmap treeArt() {
    Bitmap b(40, 68);
    b.rect(17, 36, 6, 28, 3);
    b.rect(15, 58, 10, 4, 4);
    b.ellipse(20, 28, 16, 18, 2);
    b.ellipse(14, 22, 9, 10, 1);
    b.outline(15, false);
    return b;
}

Bitmap bushArt() {
    Bitmap b(36, 28);
    b.ellipse(18, 16, 15, 10, 2);
    b.ellipse(12, 14, 8, 6, 1);
    b.ellipse(24, 15, 7, 5, 1);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(80, 28);
    b.ellipse(24, 16, 18, 9, 4);
    b.ellipse(46, 14, 22, 10, 4);
    b.ellipse(62, 17, 14, 7, 5);
    b.ellipse(40, 13, 10, 5, 2);
    return b;
}

Bitmap flashArt() {
    Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 8, 3);
    b.ellipse(10, 10, 5, 5, 1);
    b.ellipse(10, 10, 2, 2, 2);
    return b;
}

Bitmap puffArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 18, 12, 10, 5);
    b.ellipse(12, 14, 8, 7, 4);
    b.ellipse(20, 15, 6, 5, 2);
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
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 10, 12), gs::rgb4(15, 15, 15), gs::rgb4(15, 5, 4),
                          gs::rgb4(4, 14, 6), gs::rgb4(15, 12, 4), gs::rgb4(6, 8, 14), gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0,
                          shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 8, 2), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(10, 2, 2), gs::rgb4(15, 12, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(6, 15, 7), gs::rgb4(3, 10, 4), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            shadow});

    setPal(vdp, PAL_PLAYER, {0, gs::rgb4(9, 11, 13), gs::rgb4(5, 7, 9), gs::rgb4(3, 4, 6), gs::rgb4(6, 7, 6), gs::rgb4(13, 9, 6),
                             gs::rgb4(8, 5, 2), gs::rgb4(11, 12, 13), gs::rgb4(4, 5, 3), gs::rgb4(2, 2, 2), gs::rgb4(12, 10, 5),
                             gs::rgb4(14, 13, 8), 0, 0, 0, shadow});
    setPal(vdp, PAL_FRIEND, {0, gs::rgb4(12, 10, 6), gs::rgb4(8, 7, 4), gs::rgb4(4, 3, 2), gs::rgb4(6, 7, 5), gs::rgb4(13, 9, 6),
                             gs::rgb4(10, 9, 6), gs::rgb4(2, 2, 2), gs::rgb4(9, 6, 3), gs::rgb4(1, 1, 1), gs::rgb4(7, 7, 8),
                             gs::rgb4(6, 4, 2), 0, 0, 0, shadow});
    setPal(vdp, PAL_ENEMY, {0, gs::rgb4(13, 5, 4), gs::rgb4(10, 3, 3), gs::rgb4(6, 2, 2), gs::rgb4(4, 4, 3), gs::rgb4(13, 9, 6),
                            gs::rgb4(7, 5, 2), gs::rgb4(9, 10, 11), gs::rgb4(2, 2, 2), gs::rgb4(2, 1, 1), gs::rgb4(12, 10, 4),
                            gs::rgb4(15, 2, 2), 0, 0, 0, shadow});
    setPal(vdp, PAL_HORSE, {0, gs::rgb4(12, 9, 6), gs::rgb4(8, 6, 4), gs::rgb4(4, 3, 2), gs::rgb4(14, 3, 2), gs::rgb4(13, 9, 6),
                            gs::rgb4(13, 11, 4), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), gs::rgb4(6, 4, 3), gs::rgb4(15, 14, 12), 0,
                            0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 5), gs::rgb4(15, 15, 14), gs::rgb4(15, 8, 2), gs::rgb4(12, 11, 10), gs::rgb4(7, 6, 6),
                         gs::rgb4(10, 8, 5), gs::rgb4(15, 15, 8), gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PROP, {0, gs::rgb4(6, 10, 3), gs::rgb4(3, 7, 2), gs::rgb4(7, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(9, 9, 8),
                           gs::rgb4(5, 5, 5), gs::rgb4(8, 6, 3), gs::rgb4(13, 2, 2), gs::rgb4(14, 12, 5), gs::rgb4(11, 11, 10), 0,
                           0, 0, 0, shadow});

    const uint16_t field[16] = {
        0,
        gs::rgb4(6, 8, 3), gs::rgb4(4, 6, 2), gs::rgb4(8, 8, 3),
        gs::rgb4(7, 7, 3), gs::rgb4(5, 5, 2),
        gs::rgb4(7, 6, 4), gs::rgb4(5, 4, 3),
        gs::rgb4(8, 7, 5),
        gs::rgb4(3, 3, 2),
        gs::rgb4(6, 5, 3),
        gs::rgb4(3, 4, 6), gs::rgb4(2, 3, 5), gs::rgb4(6, 7, 8),
        gs::rgb4(12, 11, 8),
        gs::rgb4(8, 7, 5),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, field[i]);

    loadFont(vdp, art);
    art.player[0] = gs::uploadMipped(vdp, backMan(0, true));
    art.player[1] = gs::uploadMipped(vdp, backMan(1, true));
    art.file[0] = gs::uploadMipped(vdp, backMan(0, false));
    art.file[1] = gs::uploadMipped(vdp, backMan(1, false));
    art.rifle[0] = gs::uploadMipped(vdp, frontMan(0, 0));
    art.rifle[1] = gs::uploadMipped(vdp, frontMan(1, 0));
    art.runner = gs::uploadMipped(vdp, frontMan(1, 1));
    art.officer = gs::uploadMipped(vdp, frontMan(0, 3));
    art.horse = gs::uploadMipped(vdp, horseArt());
    art.wagon = gs::uploadMipped(vdp, wagonArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.bush = gs::uploadMipped(vdp, bushArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    Bitmap shot(4, 12);
    shot.rect(1, 0, 2, 12, 7);
    shot.rect(1, 0, 2, 4, 2);
    art.shot = gs::uploadMipped(vdp, shot);
    Bitmap shade(28, 10);
    shade.ellipse(14, 5, 12, 4, 1);
    art.shadow = gs::uploadMipped(vdp, shade);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace rearguard
