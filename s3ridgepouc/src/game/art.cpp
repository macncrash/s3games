#include "game/art.h"

#include <initializer_list>
#include <string>

namespace rpouc {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void legs(Bitmap& b, int x, int y, int step) {
    int liftL = step ? 0 : 4;
    int liftR = step ? 4 : 0;
    b.rect(float(x), float(y + liftL), 6, float(14 - liftL), 3);
    b.rect(float(x + 11), float(y + liftR), 6, float(14 - liftR), 3);
    b.rect(float(x - 1), float(y + 11), 8, 4, 4);
    b.rect(float(x + 10), float(y + 11), 8, 4, 4);
}

// Hooded courier. Hands stay empty so the pouch sprite is the cargo.
Bitmap courier(int step) {
    Bitmap b(48, 84);
    b.poly({{24, 2}, {9, 16}, {14, 28}, {34, 28}, {39, 16}}, 2);
    b.poly({{24, 6}, {14, 16}, {18, 26}, {30, 26}, {34, 16}}, 1);
    b.ellipse(24, 24, 6, 7, 6);
    b.rect(20, 22, 3, 2, 5);
    b.rect(26, 22, 3, 2, 5);
    b.poly({{24, 30}, {8, 42}, {10, 64}, {38, 64}, {40, 42}}, 2);
    b.poly({{24, 34}, {16, 42}, {17, 62}, {26, 62}, {25, 40}}, 3);
    b.rect(14, 46, 20, 4, 7);
    b.rect(22, 45, 5, 6, 1);
    b.poly({{16, 40}, {5, 50}, {9, 56}, {20, 48}}, 2);
    b.poly({{32, 40}, {43, 50}, {39, 56}, {28, 48}}, 2);
    b.rect(18, 50, 12, 5, 8);
    legs(b, 14, 62, step);
    b.outline(5, false);
    return b;
}

Bitmap kneelArt() {
    Bitmap b(52, 72);
    b.poly({{26, 2}, {12, 14}, {16, 26}, {36, 26}, {40, 14}}, 2);
    b.poly({{26, 6}, {16, 14}, {20, 24}, {32, 24}, {36, 14}}, 1);
    b.ellipse(26, 22, 6, 7, 6);
    b.rect(22, 20, 3, 2, 5);
    b.rect(28, 20, 3, 2, 5);
    b.poly({{26, 28}, {10, 40}, {12, 52}, {40, 52}, {42, 38}}, 2);
    b.poly({{26, 32}, {18, 40}, {18, 50}, {28, 50}, {27, 38}}, 3);
    b.rect(16, 44, 18, 3, 7);
    b.poly({{14, 40}, {6, 48}, {10, 52}, {18, 46}}, 2);
    b.rect(12, 54, 16, 6, 3);
    b.rect(11, 58, 18, 4, 4);
    b.rect(30, 50, 8, 14, 3);
    b.rect(29, 62, 12, 4, 4);
    b.outline(5, false);
    return b;
}

Bitmap raider(int step) {
    Bitmap b(54, 80);
    b.line(44, 2, 38, 68, 4, 2.1f);
    b.poly({{40, 0}, {50, 1}, {44, 12}}, 1);
    b.ellipse(13, 40, 11, 15, 4);
    b.ellipse(13, 40, 6, 9, 3);
    b.rect(11, 33, 4, 14, 7);
    b.poly({{32, 12}, {20, 24}, {22, 58}, {44, 58}, {46, 24}}, 2);
    b.ellipse(33, 18, 8, 9, 2);
    b.rect(26, 15, 14, 4, 4);
    b.rect(28, 18, 3, 2, 5);
    b.rect(35, 18, 3, 2, 5);
    b.ellipse(33, 17, 4, 4, 6);
    b.rect(24, 36, 16, 4, 7);
    legs(b, 24, 56, step);
    b.outline(5, false);
    return b;
}

Bitmap snatcher(int step) {
    Bitmap b(48, 78);
    b.poly({{18, 22}, {2, 34}, {6, 60}, {20, 46}}, 3);
    b.poly({{24, 6}, {12, 18}, {15, 58}, {34, 58}, {38, 16}}, 2);
    b.ellipse(25, 16, 7, 8, 2);
    b.ellipse(25, 16, 4, 5, 6);
    b.rect(21, 15, 3, 2, 7);
    b.rect(28, 15, 3, 2, 7);
    b.line(16, 28, 3, 50, 1, 2.4f);
    b.line(33, 28, 45, 48, 1, 2.4f);
    b.rect(0, 48, 7, 4, 4);
    b.rect(42, 46, 6, 4, 4);
    int liftL = step ? 0 : 5;
    int liftR = step ? 5 : 0;
    b.rect(16, float(52 + liftL), 5, float(16 - liftL), 3);
    b.rect(26, float(52 + liftR), 5, float(16 - liftR), 3);
    b.rect(15, 66, 7, 3, 4);
    b.rect(25, 66, 7, 3, 4);
    b.outline(5, false);
    return b;
}

Bitmap pouchArt(int open) {
    Bitmap b(34, 40);
    float y = open ? 1.f : 0.f;
    b.rect(14, 1 + y, 5, 8, 7);
    b.ellipse(17, 24 + y, 13, 12, 2);
    b.ellipse(17, 24 + y, 9, 8, 3);
    b.poly({{5, 16 + y}, {17, 9 + y}, {29, 16 + y}, {26, 22 + y}, {8, 22 + y}}, 6);
    b.poly({{9, 16 + y}, {17, 12 + y}, {25, 16 + y}, {23, 20 + y}, {11, 20 + y}}, 2);
    b.rect(14, 17 + y, 6, 5, 1);
    b.rect(16, 18 + y, 2, 3, 4);
    for (int i = 0; i < 5; ++i) b.set(8 + i * 4, 31, 4);
    b.outline(5, false);
    return b;
}

Bitmap cairnArt() {
    Bitmap b(42, 58);
    b.ellipse(21, 50, 18, 6, 3);
    b.poly({{5, 46}, {12, 32}, {30, 32}, {37, 46}}, 2);
    b.poly({{12, 34}, {16, 20}, {26, 20}, {30, 34}}, 1);
    b.ellipse(21, 18, 7, 6, 6);
    b.rect(15, 24, 12, 3, 4);
    b.outline(5, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(18, 54);
    b.rect(6, 10, 6, 42, 2);
    b.rect(7, 12, 2, 36, 1);
    b.rect(2, 6, 14, 7, 3);
    b.rect(4, 4, 10, 3, 6);
    b.outline(5, false);
    return b;
}

Bitmap rockArt() {
    Bitmap b(36, 28);
    b.ellipse(18, 18, 16, 9, 2);
    b.ellipse(15, 16, 9, 6, 1);
    b.poly({{8, 16}, {14, 7}, {22, 15}}, 6);
    b.poly({{20, 18}, {28, 10}, {32, 18}}, 3);
    b.outline(5, false);
    return b;
}

Bitmap dustArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 13, 10, 3);
    b.ellipse(14, 15, 7, 5, 2);
    b.ellipse(13, 14, 3, 2, 1);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(48, 14);
    b.ellipse(24, 7, 20, 5, 1);
    return b;
}

Bitmap sunArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 10, 10, 1);
    b.ellipse(14, 14, 6, 6, 2);
    b.ellipse(11, 11, 2, 2, 4);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(64, 26);
    b.ellipse(18, 15, 14, 7, 2);
    b.ellipse(34, 13, 16, 9, 1);
    b.ellipse(50, 16, 11, 6, 2);
    return b;
}

Bitmap peakNear() {
    Bitmap b(86, 48);
    b.poly({{0, 47}, {0, 30}, {18, 22}, {34, 34}, {52, 6}, {68, 26}, {86, 16}, {86, 47}}, 2);
    b.poly({{0, 47}, {12, 36}, {32, 30}, {48, 16}, {66, 32}, {86, 24}, {86, 47}}, 3);
    b.poly({{46, 14}, {52, 6}, {60, 16}}, 1);
    return b;
}

Bitmap peakFar() {
    Bitmap b(96, 40);
    b.poly({{0, 39}, {0, 22}, {20, 14}, {38, 26}, {58, 8}, {76, 20}, {96, 12}, {96, 39}}, 2);
    b.poly({{0, 39}, {16, 28}, {40, 20}, {62, 16}, {80, 24}, {96, 18}, {96, 39}}, 3);
    b.poly({{52, 14}, {58, 8}, {66, 15}}, 1);
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
    const uint16_t ink = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 9, 11), gs::rgb4(5, 6, 8), 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 5), gs::rgb4(10, 8, 3), 0, 0, gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 4), gs::rgb4(8, 2, 2), 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 8, 4), 0, 0, gs::rgb4(1, 3, 1)});
    vdp.setColor(PAL_HUD * 16 + 15, gs::rgb4(2, 2, 4));
    vdp.setColor(PAL_AMBER * 16 + 15, gs::rgb4(3, 2, 1));
    vdp.setColor(PAL_ALERT * 16 + 15, gs::rgb4(3, 0, 0));
    vdp.setColor(PAL_GOOD * 16 + 15, gs::rgb4(1, 3, 1));

    setPal(vdp, PAL_YOU, {0, gs::rgb4(12, 9, 6), gs::rgb4(7, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 2), ink,
                          gs::rgb4(13, 9, 6), gs::rgb4(12, 10, 4), gs::rgb4(8, 7, 6)});
    setPal(vdp, PAL_FOE, {0, gs::rgb4(11, 12, 13), gs::rgb4(8, 3, 3), gs::rgb4(4, 2, 2), gs::rgb4(6, 4, 2), ink,
                          gs::rgb4(13, 9, 7), gs::rgb4(9, 9, 8), gs::rgb4(5, 3, 2)});
    setPal(vdp, PAL_POUCH, {0, gs::rgb4(14, 12, 5), gs::rgb4(10, 4, 2), gs::rgb4(5, 2, 1), gs::rgb4(12, 9, 6), ink,
                            gs::rgb4(13, 7, 4), gs::rgb4(6, 3, 2)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(12, 11, 9), gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 4), gs::rgb4(5, 6, 3), ink,
                            gs::rgb4(10, 10, 8), gs::rgb4(8, 8, 9)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 13, 8), gs::rgb4(12, 9, 5), gs::rgb4(7, 6, 4), gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_MOUNT, {0, gs::rgb4(12, 11, 12), gs::rgb4(5, 6, 8), gs::rgb4(3, 3, 5), 0});
    setPal(vdp, PAL_SNEAK, {0, gs::rgb4(8, 9, 8), gs::rgb4(4, 5, 4), gs::rgb4(2, 3, 3), gs::rgb4(10, 10, 9), ink,
                            gs::rgb4(11, 9, 8), gs::rgb4(12, 3, 2)});

    const uint16_t field[16] = {
        0,
        gs::rgb4(4, 5, 4),
        gs::rgb4(3, 3, 3),
        gs::rgb4(5, 5, 4),
        gs::rgb4(6, 6, 4),
        gs::rgb4(4, 4, 3),
        gs::rgb4(6, 5, 4),
        gs::rgb4(8, 7, 6),
        gs::rgb4(10, 9, 8),
        gs::rgb4(5, 4, 3),
        gs::rgb4(7, 6, 5),
        gs::rgb4(2, 3, 5),
        gs::rgb4(1, 2, 4),
        gs::rgb4(3, 4, 6),
        gs::rgb4(11, 10, 8),
        gs::rgb4(9, 8, 7),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_FIELD * 16 + i, field[i]);

    art.you[0] = gs::uploadMipped(vdp, courier(0));
    art.you[1] = gs::uploadMipped(vdp, courier(1));
    art.kneel = gs::uploadMipped(vdp, kneelArt());
    art.foe[0] = gs::uploadMipped(vdp, raider(0));
    art.foe[1] = gs::uploadMipped(vdp, raider(1));
    art.sneak[0] = gs::uploadMipped(vdp, snatcher(0));
    art.sneak[1] = gs::uploadMipped(vdp, snatcher(1));
    art.pouch[0] = gs::uploadMipped(vdp, pouchArt(0));
    art.pouch[1] = gs::uploadMipped(vdp, pouchArt(1));
    art.cairn = gs::uploadMipped(vdp, cairnArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.peak[0] = gs::uploadMipped(vdp, peakNear());
    art.peak[1] = gs::uploadMipped(vdp, peakFar());
    loadFont(vdp, art);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace rpouc
