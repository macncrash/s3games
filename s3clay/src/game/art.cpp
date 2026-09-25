#include "game/art.h"

#include <cmath>
#include <string>

namespace clay {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap clayArt(int frame) {
    Bitmap b(36, 36);
    float ang = frame * 3.14159265f / 6.f;
    float ry = 3.2f + 12.2f * std::fabs(std::sin(ang));
    b.ellipse(18, 18, 15.5f, ry + 1.6f, 5);
    b.ellipse(18, 18, 14.2f, ry, 1);
    b.ellipse(18, 18 - ry * 0.18f, 11.4f, std::max(1.4f, ry * 0.62f), 2);
    if (ry > 7.f) b.ellipse(18, 18 + ry * 0.22f, 9.5f, ry * 0.22f, 4);
    b.ellipse(13, 18 - ry * 0.32f, 3.2f, std::max(1.2f, ry * 0.22f), 6);
    b.ellipse(18, 18, 5.5f, std::max(1.1f, ry * 0.28f), 7);
    b.outline(5, false);
    return b;
}

Bitmap shardArt(int kind) {
    Bitmap b(16, 16);
    if (kind == 0) b.poly({{2, 13}, {13, 4}, {14, 8}, {5, 14}}, 1);
    else if (kind == 1) b.poly({{3, 3}, {14, 2}, {10, 12}, {2, 9}}, 2);
    else if (kind == 2) b.poly({{1, 8}, {8, 1}, {14, 7}, {7, 14}}, 1);
    else b.poly({{4, 2}, {13, 6}, {9, 14}, {2, 10}}, 4);
    b.ellipse(6, 6, 2, 1.4f, 3);
    return b;
}

Bitmap gunArt() {
    Bitmap b(56, 68);
    b.rect(20, 2, 7, 46, 1);
    b.rect(30, 2, 7, 46, 1);
    b.rect(21, 2, 3, 44, 2);
    b.rect(31, 2, 3, 44, 2);
    b.rect(27, 4, 3, 40, 3);
    b.ellipse(23, 4, 2.4f, 2.4f, 7);
    b.ellipse(33, 4, 2.4f, 2.4f, 7);
    b.ellipse(28, 8, 1.3f, 1.3f, 8);
    b.poly({{16, 42}, {40, 42}, {48, 66}, {8, 66}}, 4);
    b.poly({{18, 46}, {38, 46}, {44, 62}, {12, 62}}, 5);
    b.rect(24, 50, 8, 10, 6);
    b.outline(7, false);
    return b;
}

Bitmap houseArt() {
    Bitmap b(96, 48);
    b.poly({{8, 44}, {10, 20}, {86, 20}, {88, 44}}, 1);
    b.poly({{10, 20}, {22, 10}, {74, 10}, {86, 20}}, 5);
    b.rect(14, 20, 68, 5, 3);
    b.rect(34, 26, 28, 8, 4);
    b.rect(42, 28, 12, 3, 2);
    b.rect(16, 32, 10, 6, 2);
    b.rect(70, 34, 12, 5, 6);
    b.rect(18, 14, 8, 4, 2);
    b.rect(70, 13, 10, 3, 6);
    b.outline(8, false);
    return b;
}

Bitmap treeArt(int kind) {
    Bitmap b(40, 58);
    b.rect(17, 34, 6, 22, 1);
    b.rect(18, 36, 2, 18, 5);
    if (kind == 0) {
        b.ellipse(20, 26, 16, 14, 2);
        b.ellipse(12, 30, 9, 9, 3);
        b.ellipse(28, 22, 9, 8, 4);
    } else {
        b.ellipse(20, 24, 14, 16, 3);
        b.ellipse(14, 18, 8, 8, 4);
        b.ellipse(27, 30, 8, 7, 2);
    }
    b.outline(1, false);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(72, 30);
    b.ellipse(24, 18, 16, 8, 1);
    b.ellipse(40, 14, 18, 10, 1);
    b.ellipse(54, 18, 12, 7, 2);
    b.ellipse(30, 16, 8, 5, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 10, 10, 3);
    b.ellipse(11, 11, 4, 4, 4);
    return b;
}

Bitmap hillArt() {
    Bitmap b(240, 48);
    b.poly({{0, 47}, {0, 30}, {28, 18}, {52, 28}, {86, 12}, {120, 24}, {156, 10}, {190, 22}, {220, 16}, {239, 28}, {239, 47}}, 5);
    b.poly({{0, 47}, {0, 36}, {40, 26}, {78, 34}, {110, 24}, {150, 36}, {188, 26}, {239, 34}, {239, 47}}, 6);
    b.ellipse(46, 28, 12, 8, 2);
    b.ellipse(100, 20, 16, 10, 3);
    b.ellipse(168, 22, 14, 9, 4);
    b.ellipse(206, 28, 10, 7, 2);
    return b;
}

Bitmap beadArt() {
    Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(7, 7, 4, 4, 0);
    b.ellipse(7, 7, 1.6f, 1.6f, 2);
    return b;
}

Bitmap flashArt() {
    Bitmap b(28, 28);
    b.poly({{14, 1}, {17, 10}, {27, 14}, {17, 18}, {14, 27}, {11, 18}, {1, 14}, {11, 10}}, 2);
    b.ellipse(14, 14, 5, 5, 1);
    b.ellipse(14, 14, 2.4f, 2.4f, 3);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(28, 12);
    b.ellipse(14, 6, 12, 4, 1);
    return b;
}

Bitmap tuftArt() {
    Bitmap b(16, 20);
    b.line(8, 18, 2, 4, 3, 1.6f);
    b.line(8, 18, 8, 2, 4, 1.6f);
    b.line(8, 18, 14, 5, 3, 1.6f);
    b.line(8, 18, 5, 7, 2, 1.2f);
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
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(11, 11, 12), gs::rgb4(15, 13, 6), gs::rgb4(15, 4, 3), gs::rgb4(5, 13, 5),
                          gs::rgb4(6, 6, 8), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CLAY, {0, gs::rgb4(12, 4, 1), gs::rgb4(15, 8, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 14, 5),
                           gs::rgb4(4, 2, 1), gs::rgb4(15, 15, 13), gs::rgb4(3, 1, 0), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GUN, {0, gs::rgb4(3, 3, 5), gs::rgb4(8, 9, 10), gs::rgb4(12, 12, 13), gs::rgb4(8, 4, 1),
                          gs::rgb4(12, 7, 2), gs::rgb4(4, 2, 1), gs::rgb4(1, 1, 1), gs::rgb4(15, 13, 4), 0, 0, 0, 0, 0,
                          0, shadow});
    setPal(vdp, PAL_HOUSE, {0, gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 4), gs::rgb4(6, 7, 6), gs::rgb4(1, 1, 1),
                            gs::rgb4(10, 10, 8), gs::rgb4(6, 6, 5), gs::rgb4(14, 7, 1), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0,
                            0, shadow});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(5, 3, 1), gs::rgb4(2, 5, 1), gs::rgb4(3, 8, 2), gs::rgb4(6, 11, 3),
                           gs::rgb4(7, 5, 2), gs::rgb4(3, 6, 3), gs::rgb4(4, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(14, 14, 15), gs::rgb4(11, 12, 14), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 12), 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SHARD, {0, gs::rgb4(15, 7, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 12), gs::rgb4(8, 3, 1), 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BEAD, {0, gs::rgb4(15, 15, 13), gs::rgb4(15, 3, 2), gs::rgb4(8, 8, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 3), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FLASH, {0, gs::rgb4(15, 15, 15), gs::rgb4(15, 14, 4), gs::rgb4(15, 8, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, shadow});
    setPal(vdp, PAL_MATCH, {0, gs::rgb4(11, 7, 1), gs::rgb4(15, 11, 2), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 11),
                            gs::rgb4(6, 3, 1), gs::rgb4(15, 15, 14), gs::rgb4(5, 2, 0), 0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t field[16] = {
        0,
        gs::rgb4(2, 6, 2), gs::rgb4(3, 8, 2), gs::rgb4(6, 10, 3),
        gs::rgb4(6, 5, 2), gs::rgb4(8, 6, 3),
        gs::rgb4(7, 5, 2), gs::rgb4(9, 7, 3),
        gs::rgb4(5, 4, 3), gs::rgb4(6, 4, 2), gs::rgb4(10, 8, 4),
        gs::rgb4(4, 6, 3), gs::rgb4(3, 5, 2), gs::rgb4(8, 9, 4),
        gs::rgb4(12, 11, 7), gs::rgb4(12, 10, 6),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FIELD * 16 + i, field[i]);
    vdp.setFogColor(gs::rgb4(13, 10, 7));

    loadFont(vdp, art);
    for (int i = 0; i < 6; i++) art.clay[i] = gs::uploadMipped(vdp, clayArt(i));
    for (int i = 0; i < 4; i++) art.shard[i] = gs::uploadMipped(vdp, shardArt(i));
    art.gun = gs::uploadMipped(vdp, gunArt());
    art.house = gs::uploadMipped(vdp, houseArt());
    art.tree[0] = gs::uploadMipped(vdp, treeArt(0));
    art.tree[1] = gs::uploadMipped(vdp, treeArt(1));
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
}

}  // namespace clay
