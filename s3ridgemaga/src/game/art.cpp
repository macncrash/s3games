#include "game/art.h"

#include <initializer_list>
#include <string>

namespace rmaga {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void legs(Bitmap& b, int x, int y, int step, int c) {
    int a = step ? 0 : 4;
    int d = step ? 4 : 0;
    b.rect(float(x), float(y + a), 6, float(14 - a), c);
    b.rect(float(x + 10), float(y + d), 6, float(14 - d), c);
    b.rect(float(x - 1), float(y + 12), 8, 3, 4);
    b.rect(float(x + 9), float(y + 12), 8, 3, 4);
}

// The firer, seen from behind on the lip: hat, coat, rifle angled up the path.
Bitmap rifleman(int step) {
    Bitmap b(44, 76);
    b.rect(14, 2, 16, 4, 8);
    b.rect(12, 5, 20, 3, 3);
    b.ellipse(22, 14, 7, 7, 8);
    b.ellipse(22, 15, 5, 5, 3);
    b.poly({{22, 18}, {10, 28}, {12, 56}, {32, 56}, {34, 26}}, 2);
    b.poly({{22, 22}, {16, 30}, {18, 54}, {28, 54}}, 3);
    b.rect(14, 38, 16, 3, 7);
    b.rect(20, 36, 4, 7, 7);
    b.line(30, 22, 38, 4, 4, 2.2f);
    b.rect(35, 2, 5, 4, 1);
    b.rect(12, 28, 5, 12, 3);
    legs(b, 13, 54, step, 3);
    b.outline(5, false);
    return b;
}

// Path raider, facing the lip, rifle across the chest.
Bitmap raider(int step) {
    Bitmap b(42, 74);
    b.poly({{14, 2}, {28, 2}, {26, 8}, {16, 8}}, 8);
    b.ellipse(21, 14, 7, 8, 6);
    b.set(18, 13, 5);
    b.set(24, 13, 5);
    b.rect(17, 16, 8, 2, 3);
    b.poly({{21, 18}, {11, 26}, {12, 52}, {31, 52}, {32, 26}}, 2);
    b.poly({{21, 22}, {16, 28}, {17, 50}, {26, 50}}, 3);
    b.rect(13, 34, 16, 3, 7);
    b.line(10, 32, 34, 40, 4, 2.0f);
    b.rect(32, 37, 5, 3, 1);
    legs(b, 12, 50, step, 3);
    b.outline(5, false);
    return b;
}

// Shoulder runner: hood, no rifle, a bundle. Not a target worth a round.
Bitmap peeler(int step) {
    Bitmap b(40, 68);
    b.ellipse(20, 12, 9, 10, 8);
    b.ellipse(20, 16, 6, 6, 6);
    b.poly({{20, 18}, {10, 28}, {12, 48}, {28, 48}, {30, 26}}, 2);
    b.poly({{20, 22}, {15, 28}, {16, 46}, {24, 46}}, 3);
    b.ellipse(30, 32, 6, 7, 4);
    b.rect(12, 34, 12, 3, 7);
    legs(b, 11, 46, step, 3);
    b.outline(5, false);
    return b;
}

Bitmap fallenArt() {
    Bitmap b(68, 26);
    b.ellipse(14, 14, 7, 6, 6);
    b.rect(20, 10, 32, 8, 2);
    b.rect(22, 12, 26, 4, 3);
    b.rect(50, 14, 12, 4, 3);
    b.line(24, 8, 48, 6, 4, 1.6f);
    b.outline(5, false);
    return b;
}

Bitmap cairnArt() {
    Bitmap b(36, 44);
    b.ellipse(18, 34, 14, 7, 3);
    b.ellipse(18, 26, 10, 7, 2);
    b.ellipse(18, 18, 7, 6, 1);
    b.ellipse(18, 11, 4, 4, 6);
    b.outline(5, false);
    return b;
}

Bitmap stakeArt() {
    Bitmap b(16, 48);
    b.rect(6, 8, 4, 38, 2);
    b.rect(7, 10, 2, 34, 1);
    b.poly({{4, 8}, {12, 8}, {8, 2}}, 7);
    b.rect(5, 14, 8, 6, 6);
    b.outline(5, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(22, 64);
    b.rect(8, 8, 6, 54, 2);
    b.rect(9, 10, 2, 48, 1);
    b.rect(4, 6, 14, 6, 3);
    b.outline(5, false);
    return b;
}

Bitmap bannerArt() {
    Bitmap b(36, 60);
    b.rect(6, 2, 3, 56, 4);
    b.rect(9, 8, 22, 16, 2);
    b.rect(11, 10, 16, 12, 3);
    b.poly({{14, 12}, {24, 16}, {14, 20}}, 6);
    b.outline(5, false);
    return b;
}

Bitmap mountSharp() {
    Bitmap b(80, 48);
    b.poly({{0, 47}, {0, 30}, {18, 22}, {32, 34}, {50, 6}, {66, 26}, {80, 16}, {80, 47}}, 2);
    b.poly({{0, 47}, {12, 36}, {30, 30}, {46, 16}, {64, 32}, {80, 24}, {80, 47}}, 3);
    b.poly({{44, 14}, {50, 6}, {58, 16}}, 1);
    return b;
}

Bitmap mountLong() {
    Bitmap b(96, 40);
    b.poly({{0, 39}, {0, 22}, {20, 14}, {38, 26}, {56, 8}, {74, 20}, {96, 12}, {96, 39}}, 2);
    b.poly({{0, 39}, {16, 26}, {40, 20}, {62, 16}, {82, 24}, {96, 18}, {96, 39}}, 3);
    b.poly({{50, 14}, {56, 8}, {64, 16}}, 1);
    return b;
}

Bitmap sunArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 11, 11, 2);
    b.ellipse(14, 14, 7, 7, 5);
    b.ellipse(11, 11, 3, 3, 1);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(64, 24);
    b.ellipse(18, 14, 14, 7, 2);
    b.ellipse(34, 12, 16, 8, 1);
    b.ellipse(50, 14, 11, 6, 2);
    return b;
}

Bitmap dustArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 11, 9, 4);
    b.ellipse(13, 13, 6, 5, 3);
    b.ellipse(12, 12, 2, 2, 1);
    return b;
}

Bitmap flashArt() {
    Bitmap b(16, 16);
    b.poly({{8, 1}, {10, 6}, {15, 8}, {10, 10}, {8, 15}, {6, 10}, {1, 8}, {6, 6}}, 1);
    b.ellipse(8, 8, 3, 3, 2);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(48, 12);
    b.ellipse(24, 6, 20, 4, 1);
    return b;
}

Bitmap roundArt() {
    Bitmap b(8, 18);
    b.rect(2, 4, 4, 12, 6);
    b.rect(2, 2, 4, 4, 1);
    b.rect(3, 6, 1, 8, 7);
    return b;
}

Bitmap spentArt() {
    Bitmap b(8, 18);
    b.rect(2, 4, 4, 12, 8);
    b.rect(2, 2, 4, 4, 9);
    return b;
}

Bitmap beadArt() {
    Bitmap b(12, 12);
    b.poly({{6, 0}, {12, 6}, {6, 12}, {0, 6}}, 5);
    b.poly({{6, 3}, {9, 6}, {6, 9}, {3, 6}}, 1);
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
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 14, 12), gs::rgb4(9, 8, 8), gs::rgb4(6, 5, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 5), gs::rgb4(10, 7, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 8, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    setPal(vdp, PAL_YOU, {0, gs::rgb4(12, 12, 13), gs::rgb4(5, 6, 8), gs::rgb4(3, 3, 5), gs::rgb4(9, 9, 10),
                          gs::rgb4(1, 1, 2), gs::rgb4(8, 6, 5), gs::rgb4(12, 9, 4), gs::rgb4(4, 4, 6), 0, 0, 0, 0, 0, 0,
                          shadow});
    setPal(vdp, PAL_RAID, {0, gs::rgb4(14, 10, 8), gs::rgb4(11, 4, 3), gs::rgb4(6, 2, 2), gs::rgb4(8, 8, 9),
                           gs::rgb4(1, 0, 0), gs::rgb4(13, 9, 7), gs::rgb4(8, 7, 4), gs::rgb4(3, 3, 4), 0, 0, 0, 0, 0, 0,
                           shadow});
    setPal(vdp, PAL_PEEL, {0, gs::rgb4(12, 11, 8), gs::rgb4(8, 7, 4), gs::rgb4(5, 4, 2), gs::rgb4(6, 6, 5),
                           gs::rgb4(1, 1, 1), gs::rgb4(4, 3, 2), gs::rgb4(7, 3, 2), gs::rgb4(6, 5, 3), 0, 0, 0, 0, 0, 0,
                           shadow});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(12, 11, 9), gs::rgb4(7, 6, 5), gs::rgb4(4, 4, 3), gs::rgb4(8, 8, 7),
                            gs::rgb4(1, 1, 1), gs::rgb4(14, 13, 11), gs::rgb4(10, 3, 2), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 13), gs::rgb4(15, 12, 4), gs::rgb4(12, 10, 8), gs::rgb4(7, 6, 5),
                         gs::rgb4(15, 8, 3), gs::rgb4(13, 9, 3), gs::rgb4(8, 6, 2), gs::rgb4(6, 6, 6), gs::rgb4(3, 3, 3),
                         0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MOUNT, {0, gs::rgb4(12, 10, 12), gs::rgb4(6, 5, 8), gs::rgb4(3, 2, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, shadow});
    setPal(vdp, PAL_CLOTH, {0, gs::rgb4(14, 13, 10), gs::rgb4(9, 2, 2), gs::rgb4(5, 1, 1), gs::rgb4(8, 7, 6),
                            gs::rgb4(1, 1, 1), gs::rgb4(12, 10, 4), gs::rgb4(6, 5, 4), 0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t field[16] = {
        0,
        gs::rgb4(3, 3, 4),
        gs::rgb4(2, 2, 3),
        gs::rgb4(5, 5, 6),
        gs::rgb4(7, 6, 3),
        gs::rgb4(4, 4, 2),
        gs::rgb4(8, 7, 6),
        gs::rgb4(5, 4, 4),
        gs::rgb4(12, 11, 9),
        gs::rgb4(4, 3, 3),
        gs::rgb4(9, 8, 6),
        gs::rgb4(3, 4, 6),
        gs::rgb4(2, 3, 5),
        gs::rgb4(4, 5, 7),
        gs::rgb4(13, 10, 6),
        gs::rgb4(14, 12, 9),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FIELD * 16 + i, field[i]);

    art.you[0] = gs::uploadMipped(vdp, rifleman(0));
    art.you[1] = gs::uploadMipped(vdp, rifleman(1));
    art.raider[0] = gs::uploadMipped(vdp, raider(0));
    art.raider[1] = gs::uploadMipped(vdp, raider(1));
    art.peel[0] = gs::uploadMipped(vdp, peeler(0));
    art.peel[1] = gs::uploadMipped(vdp, peeler(1));
    art.fallen = gs::uploadMipped(vdp, fallenArt());
    art.cairn = gs::uploadMipped(vdp, cairnArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.mount[0] = gs::uploadMipped(vdp, mountSharp());
    art.mount[1] = gs::uploadMipped(vdp, mountLong());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.round = gs::uploadMipped(vdp, roundArt());
    art.spent = gs::uploadMipped(vdp, spentArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    loadFont(vdp, art);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.setFogColor(gs::rgb4(13, 7, 4));
}

}  // namespace rmaga
