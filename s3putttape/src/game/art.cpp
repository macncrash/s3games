#include "game/art.h"

#include <initializer_list>

namespace putttape {
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
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6.4f, 6.4f, 2);
    b.ellipse(8, 8, 5.2f, 5.2f, 1);
    b.ellipse(6.2f, 5.8f, 1.6f, 1.2f, 4);
    b.set(6, 7, 3);
    b.set(9, 6, 3);
    b.set(8, 9, 3);
    b.set(11, 8, 3);
    b.set(7, 11, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(14, 8);
    b.ellipse(7, 4, 5.5f, 2.3f, 1);
    return b;
}

gs::Bitmap cupArt() {
    gs::Bitmap b(24, 14);
    b.ellipse(12, 7, 11, 6, 1);
    b.ellipse(12, 7, 8.2f, 4.4f, 2);
    b.ellipse(12, 7.2f, 5.2f, 2.8f, 3);
    b.ellipse(9.2f, 5.4f, 2.2f, 1.1f, 4);
    return b;
}

gs::Bitmap coinArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6.2f, 6.2f, 3);
    b.ellipse(7, 7, 5.1f, 5.1f, 1);
    b.ellipse(7, 7, 3.2f, 3.2f, 2);
    b.ellipse(5.2f, 5.0f, 1.4f, 1.0f, 4);
    return b;
}

gs::Bitmap tokenArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6.2f, 6.2f, 3);
    b.ellipse(7, 7, 5.1f, 5.1f, 1);
    b.rect(6, 3, 2, 8, 0);
    b.rect(4, 6, 6, 2, 0);
    b.rect(3, 6, 8, 1, 4);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(12, 20);
    b.rect(2, 3, 1, 16, 3);
    b.poly({{3, 3}, {11, 6}, {3, 9}}, 1);
    b.poly({{4, 4}, {9, 6}, {4, 8}}, 2);
    return b;
}

gs::Bitmap golferArt(int pose) {
    gs::Bitmap b(26, 32);
    b.ellipse(9, 29, 3.1f, 1.8f, 5);
    b.ellipse(16, 29, 3.1f, 1.8f, 5);
    b.rect(8, 18, 3, 10, 6);
    b.rect(14, 18, 3, 10, 6);
    b.rect(7, 12, 12, 8, 2);
    b.rect(8, 13, 10, 3, 4);
    b.ellipse(12, 8, 4.0f, 3.8f, 1);
    b.ellipse(12, 5.2f, 5.2f, 2.2f, 3);
    b.rect(8, 4, 8, 2, 3);
    b.rect(14, 5, 4, 1, 4);
    if (pose == 0) {
        b.rect(4, 14, 4, 3, 1);
        b.line(6, 16, 22, 26, 5, 1);
        b.rect(20, 24, 5, 2, 5);
    } else {
        b.rect(16, 13, 5, 3, 1);
        b.line(18, 14, 24, 6, 5, 1);
        b.rect(22, 4, 4, 2, 5);
    }
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(24, 32);
    b.rect(10, 18, 4, 11, 3);
    b.rect(7, 27, 10, 3, 3);
    b.ellipse(12, 13, 10, 9, 1);
    b.ellipse(8, 12, 5, 4, 2);
    b.ellipse(16, 11, 5, 4, 2);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(9, 7, 6, 3.4f, 1);
    b.ellipse(16, 5, 7, 4, 1);
    b.ellipse(22, 7, 5, 3, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 4.4f, 4.4f, 1);
    b.ellipse(6.6f, 6.6f, 1.5f, 1.2f, 2);
    b.line(8, 1, 8, 3, 1, 1);
    b.line(8, 13, 8, 15, 1, 1);
    b.line(1, 8, 3, 8, 1, 1);
    b.line(13, 8, 15, 8, 1, 1);
    return b;
}

gs::Bitmap tuftArt() {
    gs::Bitmap b(10, 12);
    b.line(5, 11, 2, 2, 1, 1);
    b.line(5, 11, 5, 1, 2, 1);
    b.line(5, 11, 8, 3, 1, 1);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(6, 6);
    b.ellipse(3, 3, 2.0f, 2.0f, 1);
    return b;
}

gs::Bitmap paperArt() {
    gs::Bitmap b(24, 40);
    b.rect(0, 0, 24, 40, 1);
    b.rect(0, 0, 3, 40, 3);
    for (int y = 4; y < 38; y += 5) b.rect(5, y, 16, 1, 2);
    for (int y = 2; y < 40; y += 6) b.set(1, y, 0);
    return b;
}

gs::Bitmap woodArt() {
    gs::Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 1);
    b.rect(0, 0, 16, 2, 3);
    b.rect(0, 14, 16, 2, 2);
    b.rect(3, 4, 1, 8, 2);
    b.rect(10, 5, 1, 7, 4);
    return b;
}

gs::Bitmap slotArt() {
    gs::Bitmap b(18, 8);
    b.ellipse(9, 4, 8, 3, 1);
    b.ellipse(9, 4, 5, 1.6f, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 15, 13), gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(10, 7, 1), gs::rgb4(4, 2, 1), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_SILVER, {0, gs::rgb4(13, 14, 15), gs::rgb4(7, 8, 10), gs::rgb4(3, 3, 5), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_COPPER, {0, gs::rgb4(14, 7, 3), gs::rgb4(8, 4, 2), gs::rgb4(3, 1, 1), gs::rgb4(15, 12, 8)});
    setPal(vdp, PAL_TOKEN, {0, gs::rgb4(11, 13, 14), gs::rgb4(5, 6, 8), gs::rgb4(2, 2, 4), gs::rgb4(14, 4, 4)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 6), gs::rgb4(2, 8, 3), gs::rgb4(13, 15, 10)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 5, 4), gs::rgb4(8, 1, 1), gs::rgb4(15, 12, 8)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(15, 15, 15), gs::rgb4(9, 12, 10), gs::rgb4(4, 5, 5), gs::rgb4(13, 15, 13)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(12, 7, 3), gs::rgb4(7, 4, 1), gs::rgb4(15, 11, 6), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(14, 13, 10), gs::rgb4(9, 8, 6), gs::rgb4(5, 4, 3), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_PLAYER,
           {0, gs::rgb4(14, 11, 8), gs::rgb4(3, 7, 13), gs::rgb4(13, 3, 3), gs::rgb4(15, 14, 12), gs::rgb4(2, 2, 3),
            gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(3, 9, 3), gs::rgb4(8, 14, 5), gs::rgb4(7, 4, 2)});
    setPal(vdp, PAL_AIM, {0, gs::rgb4(15, 15, 8), gs::rgb4(15, 9, 3)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 2, 2), gs::rgb4(5, 5, 6)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(0, 0, 0)});

    loadFont(vdp, art);
    art.ball = gs::uploadMipped(vdp, ballArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cup = gs::uploadMipped(vdp, cupArt());
    art.coin = gs::uploadMipped(vdp, coinArt());
    art.token = gs::uploadMipped(vdp, tokenArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.golfer[0] = gs::uploadMipped(vdp, golferArt(0));
    art.golfer[1] = gs::uploadMipped(vdp, golferArt(1));
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.paper = gs::uploadMipped(vdp, paperArt());
    art.wood = gs::uploadMipped(vdp, woodArt());
    art.slot = gs::uploadMipped(vdp, slotArt());
}

}  // namespace putttape
