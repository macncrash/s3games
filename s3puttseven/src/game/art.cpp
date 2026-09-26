#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace puttseven {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap ballArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6.1f, 6.1f, 4);
    b.ellipse(7, 7, 5.2f, 5.2f, 1);
    b.ellipse(5.1f, 4.8f, 1.6f, 1.1f, 2);
    b.rect(3, 7, 8, 1, 3);
    b.set(10, 9, 3);
    b.set(4, 9, 5);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(14, 6);
    b.ellipse(7, 3, 5.6f, 2.0f, 1);
    return b;
}

gs::Bitmap cupArt() {
    gs::Bitmap b(26, 18);
    b.ellipse(13, 9, 11.4f, 7.4f, 3);
    b.ellipse(13, 9, 8.2f, 5.2f, 2);
    b.ellipse(13, 9, 4.6f, 2.8f, 1);
    b.ellipse(10.4f, 7.2f, 1.5f, 0.8f, 4);
    return b;
}

gs::Bitmap flagArt(int frame) {
    gs::Bitmap b(24, 40);
    b.rect(4, 14, 2, 22, 3);
    b.rect(3, 34, 4, 3, 5);
    float tip = frame ? 22.f : 19.f;
    b.poly({{6, 4}, {tip, 11}, {6, 19}}, 1);
    b.poly({{6, 7}, {tip - 3.f, 11}, {6, 16}}, 2);
    b.rect(8, 7, 8, 2, 4);
    b.rect(14, 8, 2, 8, 4);
    return b;
}

gs::Bitmap hedgeArt() {
    gs::Bitmap b(16, 12);
    b.ellipse(8, 7, 7.2f, 4.2f, 4);
    b.ellipse(8, 6, 5.8f, 3.4f, 3);
    b.ellipse(5, 5, 2.6f, 2.0f, 2);
    b.ellipse(11, 5, 2.4f, 1.8f, 1);
    return b;
}

gs::Bitmap woodArt() {
    gs::Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 1);
    b.rect(0, 0, 16, 1, 2);
    b.rect(0, 0, 1, 16, 2);
    b.rect(15, 0, 1, 16, 3);
    b.rect(0, 15, 16, 1, 3);
    b.rect(0, 8, 16, 1, 3);
    b.set(4, 4, 4);
    b.set(12, 12, 4);
    return b;
}

gs::Bitmap sandArt() {
    gs::Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 1);
    b.line(0, 4, 15, 5, 2, 1);
    b.line(0, 9, 15, 8, 2, 1);
    b.line(0, 13, 15, 13, 3, 1);
    b.set(3, 2, 3);
    b.set(11, 11, 4);
    return b;
}

gs::Bitmap tuftArt() {
    gs::Bitmap b(8, 10);
    b.line(4, 9, 1, 2, 2, 1);
    b.line(4, 9, 4, 1, 1, 1);
    b.line(4, 9, 7, 3, 3, 1);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 36);
    b.rect(12, 22, 4, 11, 3);
    b.rect(10, 32, 8, 2, 4);
    b.ellipse(14, 14, 12, 10, 2);
    b.ellipse(8, 15, 5, 4, 1);
    b.ellipse(19, 13, 5, 4, 1);
    b.ellipse(14, 8, 3, 2, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7.6f, 7.6f, 1);
    b.ellipse(7, 7, 2.6f, 2.4f, 2);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(6, 6);
    b.ellipse(3, 3, 2.2f, 2.2f, 1);
    return b;
}

gs::Bitmap pegArt() {
    gs::Bitmap b(6, 8);
    b.rect(2, 0, 2, 6, 1);
    b.rect(1, 6, 4, 2, 2);
    return b;
}

gs::Bitmap golferArt() {
    gs::Bitmap b(22, 36);
    b.rect(5, 3, 8, 2, 1);
    b.ellipse(9, 7, 4.2f, 3.6f, 6);
    b.ellipse(9, 8, 3.4f, 2.8f, 2);
    b.rect(5, 12, 9, 8, 1);
    b.rect(6, 13, 7, 3, 7);
    b.rect(2, 13, 3, 6, 2);
    b.rect(14, 13, 3, 6, 2);
    b.line(16, 16, 20, 30, 3, 1);
    b.rect(17, 29, 4, 2, 3);
    b.rect(6, 20, 3, 9, 4);
    b.rect(10, 20, 3, 9, 4);
    b.rect(5, 29, 5, 2, 5);
    b.rect(10, 29, 5, 2, 5);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_INK, {0, ink, gs::rgb4(8, 8, 9)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(5, 13, 7), gs::rgb4(14, 11, 8), gs::rgb4(12, 12, 13), gs::rgb4(3, 4, 8),
                          gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2), gs::rgb4(11, 15, 11)});
    setPal(vdp, PAL_THEM, {0, gs::rgb4(13, 5, 3), gs::rgb4(14, 11, 8), gs::rgb4(12, 12, 13), gs::rgb4(4, 4, 5),
                           gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2), gs::rgb4(15, 10, 7)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), gs::rgb4(15, 12, 4), gs::rgb4(8, 9, 10),
                           gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_CUP, {0, gs::rgb4(1, 2, 2), gs::rgb4(4, 6, 5), gs::rgb4(10, 12, 8), gs::rgb4(14, 15, 12)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(15, 14, 10), gs::rgb4(12, 9, 4), gs::rgb4(10, 8, 5), gs::rgb4(2, 3, 8),
                           gs::rgb4(5, 4, 2)});
    setPal(vdp, PAL_HEDGE, {0, gs::rgb4(9, 14, 6), gs::rgb4(5, 11, 4), gs::rgb4(3, 7, 3), gs::rgb4(1, 4, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(11, 7, 3), gs::rgb4(14, 11, 6), gs::rgb4(6, 4, 2), gs::rgb4(8, 6, 3)});
    setPal(vdp, PAL_SAND, {0, gs::rgb4(13, 11, 6), gs::rgb4(15, 13, 8), gs::rgb4(9, 7, 4), gs::rgb4(7, 5, 3)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(7, 13, 6), gs::rgb4(3, 8, 4), gs::rgb4(8, 6, 3), gs::rgb4(5, 4, 2)});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 12, 5), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_AIM, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 9, 2)});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 13, 5), gs::rgb4(4, 3, 1)});
    setPal(vdp, PAL_ALARM, {0, gs::rgb4(15, 6, 4), gs::rgb4(5, 1, 1)});

    loadFont(vdp, art);
    art.ball = gs::uploadMipped(vdp, ballArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cup = gs::uploadMipped(vdp, cupArt());
    art.flag[0] = gs::uploadMipped(vdp, flagArt(0));
    art.flag[1] = gs::uploadMipped(vdp, flagArt(1));
    art.hedge = gs::uploadMipped(vdp, hedgeArt());
    art.wood = gs::uploadMipped(vdp, woodArt());
    art.sand = gs::uploadMipped(vdp, sandArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.peg = gs::uploadMipped(vdp, pegArt());
    art.golfer = gs::uploadMipped(vdp, golferArt());
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("S3 PUTT SEVEN", {2, 1, 2, 0, 1}));
    art.win = gs::uploadMipped(vdp, gs::textBitmap("FIRST TO SEVEN", {2, 1, 2, 0, 1}));
    art.lose = gs::uploadMipped(vdp, gs::textBitmap("SECOND", {2, 1, 2, 0, 1}));
}

}  // namespace puttseven
