#include "game/art.h"

#include <initializer_list>
#include <string>

namespace qcol {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap dumperArt() {
    Bitmap b(68, 78);
    b.poly({{18, 6}, {50, 6}, {58, 22}, {10, 22}}, 2);
    b.rect(12, 20, 44, 22, 3);
    b.rect(16, 24, 16, 8, 6);
    b.rect(36, 24, 16, 8, 1);
    b.rect(10, 42, 48, 10, 4);
    b.rect(14, 44, 18, 4, 8);
    b.ellipse(18, 62, 8, 8, 5);
    b.ellipse(50, 62, 8, 8, 5);
    b.ellipse(18, 62, 3, 3, 1);
    b.ellipse(50, 62, 3, 3, 7);
    b.rect(6, 46, 6, 4, 7);
    b.rect(56, 46, 6, 4, 7);
    b.outline(15, false);
    return b;
}

Bitmap flagArt() {
    Bitmap b(20, 30);
    b.rect(8, 2, 3, 26, 3);
    b.poly({{11, 4}, {18, 9}, {11, 15}}, 2);
    b.outline(15, false);
    return b;
}

Bitmap boardArt() {
    Bitmap b(36, 28);
    b.rect(2, 2, 32, 18, 1);
    b.rect(2, 2, 32, 4, 2);
    b.rect(16, 20, 4, 6, 3);
    b.outline(15, false);
    return b;
}

Bitmap marshalArt() {
    Bitmap b(36, 58);
    b.ellipse(18, 9, 7, 7, 2);
    b.rect(12, 4, 12, 4, 3);
    b.poly({{18, 16}, {8, 24}, {7, 40}, {29, 40}, {28, 24}}, 1);
    b.rect(14, 22, 8, 8, 4);
    b.rect(26, 24, 8, 4, 6);
    b.rect(10, 40, 6, 14, 5);
    b.rect(20, 40, 6, 14, 5);
    b.outline(15, false);
    return b;
}

Bitmap cliffArt() {
    Bitmap b(48, 96);
    b.poly({{4, 90}, {8, 20}, {18, 8}, {30, 16}, {40, 6}, {46, 90}}, 2);
    b.poly({{12, 70}, {16, 34}, {26, 28}, {34, 70}}, 3);
    b.rect(18, 48, 8, 6, 1);
    b.rect(28, 62, 6, 5, 4);
    b.outline(15, false);
    return b;
}

Bitmap crusherArt() {
    Bitmap b(72, 64);
    b.rect(8, 28, 56, 28, 2);
    b.poly({{12, 28}, {36, 8}, {60, 28}}, 3);
    b.rect(30, 16, 12, 14, 1);
    b.rect(16, 36, 16, 10, 5);
    b.rect(40, 36, 16, 10, 6);
    b.rect(4, 54, 64, 6, 4);
    b.outline(15, false);
    return b;
}

Bitmap hopperArt() {
    Bitmap b(40, 48);
    b.poly({{4, 8}, {36, 8}, {30, 40}, {10, 40}}, 3);
    b.rect(14, 40, 12, 6, 5);
    b.rect(8, 4, 24, 6, 1);
    b.outline(15, false);
    return b;
}

Bitmap boulderArt() {
    Bitmap b(32, 24);
    b.ellipse(16, 14, 13, 8, 2);
    b.ellipse(12, 12, 5, 4, 3);
    b.outline(15, false);
    return b;
}

Bitmap dustArt() {
    Bitmap b(20, 16);
    b.ellipse(10, 8, 8, 5, 1);
    b.ellipse(7, 7, 3, 2, 2);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(28, 10);
    b.ellipse(14, 5, 12, 3, 1);
    return b;
}

Bitmap blockArt() {
    Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 1);
    gs::TextStyle big{2, 1, 0, 0, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        if (g) {
            for (int row = 0; row < 7; ++row)
                for (int col = 0; col < 5; ++col)
                    if (g[row] & (1 << (4 - col))) px[row * 8 + col] = 1;
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(2, 1, 1);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11), gs::rgb4(8, 7, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3), gs::rgb4(10, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 2), gs::rgb4(15, 12, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(5, 14, 6), gs::rgb4(13, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_DUMP, {0, gs::rgb4(14, 12, 3), gs::rgb4(11, 8, 2), gs::rgb4(6, 5, 3), gs::rgb4(3, 3, 2),
                           gs::rgb4(1, 1, 1), gs::rgb4(8, 11, 13), gs::rgb4(15, 15, 12), gs::rgb4(12, 4, 2), 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(9, 8, 7), gs::rgb4(6, 5, 4), gs::rgb4(4, 4, 3), gs::rgb4(11, 10, 8),
                           gs::rgb4(3, 3, 2), gs::rgb4(8, 7, 5), gs::rgb4(12, 11, 9), gs::rgb4(5, 4, 3), 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_CRUSH, {0, gs::rgb4(10, 10, 11), gs::rgb4(6, 6, 7), gs::rgb4(4, 4, 5), gs::rgb4(8, 5, 3),
                            gs::rgb4(3, 3, 3), gs::rgb4(12, 8, 3), gs::rgb4(14, 12, 6), gs::rgb4(2, 2, 2), 0, 0, 0, 0,
                            0, 0, ink});
    setPal(vdp, PAL_BOARD, {0, gs::rgb4(14, 3, 2), gs::rgb4(15, 14, 12), gs::rgb4(5, 4, 3), gs::rgb4(1, 1, 1), 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(10, 7, 4), gs::rgb4(13, 10, 6), gs::rgb4(4, 3, 2), gs::rgb4(8, 12, 14),
                           gs::rgb4(2, 2, 2), gs::rgb4(14, 11, 3), gs::rgb4(15, 8, 3), gs::rgb4(6, 5, 4), 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(12, 10, 7), gs::rgb4(8, 7, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(14, 13, 8), gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(12, 3, 2), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(6, 5, 3),
        gs::rgb4(4, 3, 2),
        gs::rgb4(7, 6, 4),
        gs::rgb4(5, 4, 3),
        gs::rgb4(2, 2, 1),
        gs::rgb4(3, 3, 2),
        gs::rgb4(3, 2, 2),
        gs::rgb4(6, 5, 4),
        gs::rgb4(5, 4, 3),
        gs::rgb4(8, 6, 4),
        gs::rgb4(3, 4, 3),
        gs::rgb4(4, 5, 3),
        gs::rgb4(5, 6, 4),
        gs::rgb4(14, 10, 3),
        gs::rgb4(7, 6, 4),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    art.dumper = gs::uploadMipped(vdp, dumperArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.board = gs::uploadMipped(vdp, boardArt());
    art.marshal = gs::uploadMipped(vdp, marshalArt());
    art.cliff = gs::uploadMipped(vdp, cliffArt());
    art.crusher = gs::uploadMipped(vdp, crusherArt());
    art.hopper = gs::uploadMipped(vdp, hopperArt());
    art.boulder = gs::uploadMipped(vdp, boulderArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.block = gs::uploadMipped(vdp, blockArt());
    loadFont(vdp, art);
}

}  // namespace qcol
