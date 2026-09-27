#include "game/art.h"

namespace golfgold {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    }
}

gs::Bitmap ballArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5.2f, 5.2f, 1);
    b.ellipse(4.4f, 4.2f, 2.1f, 1.6f, 2);
    b.set(8, 8, 3);
    return b;
}

gs::Bitmap golferArt(int swing) {
    gs::Bitmap b(24, 32);
    b.ellipse(13, 6, 4.2f, 4.0f, 1);
    b.ellipse(13, 4.4f, 4.4f, 2.0f, 2);
    b.rect(11, 10, 6, 8, 3);
    b.rect(12, 18, 3, 8, 4);
    b.rect(16, 18, 3, 8, 4);
    b.rect(11, 26, 4, 2, 5);
    b.rect(16, 26, 4, 2, 5);
    if (!swing) {
        b.line(16, 13, 22, 26, 6, 1);
        b.set(22, 26, 7);
    } else {
        b.line(14, 12, 4, 8, 6, 1);
        b.line(4, 8, 21, 7, 6, 1);
        b.set(21, 7, 7);
    }
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(16, 28);
    b.line(3, 2, 3, 27, 1, 1);
    b.poly({{3, 3}, {15, 7}, {3, 12}}, 2);
    b.poly({{4, 5}, {12, 7}, {4, 10}}, 3);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(22, 28);
    b.rect(10, 16, 3, 12, 3);
    b.ellipse(11, 11, 9, 8, 1);
    b.ellipse(8, 9, 4, 3, 2);
    return b;
}

gs::Bitmap cupArt() {
    gs::Bitmap b(18, 10);
    b.ellipse(9, 5, 8, 3.2f, 1);
    b.ellipse(9, 4.2f, 5.5f, 1.8f, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 4), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 15, 11), gs::rgb4(15, 12, 3), gs::rgb4(11, 8, 1), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 15, 13), gs::rgb4(13, 12, 8), gs::rgb4(9, 8, 5), gs::rgb4(15, 14, 12)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(4, 13, 5), gs::rgb4(2, 8, 3), gs::rgb4(8, 15, 7), gs::rgb4(1, 5, 2)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15), gs::rgb4(6, 7, 8)});
    setPal(vdp, PAL_MAN, {0, gs::rgb4(14, 10, 7), gs::rgb4(4, 3, 2), gs::rgb4(15, 15, 14), gs::rgb4(2, 3, 8),
                          gs::rgb4(2, 2, 2), gs::rgb4(10, 11, 12), gs::rgb4(12, 2, 2)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(8, 12, 15), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 9, 3), gs::rgb4(5, 13, 5), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_CUP, {0, gs::rgb4(2, 6, 2), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_WORD, {0, gs::rgb4(15, 14, 6), gs::rgb4(4, 2, 1), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 8), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 5, 4), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_METER, {0, gs::rgb4(5, 5, 7), gs::rgb4(8, 15, 7), gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 3)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(12, 12, 11), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_SAND, {0, gs::rgb4(13, 11, 6), gs::rgb4(9, 7, 3)});
    setPal(vdp, PAL_SHADOW, {0, gs::rgb4(1, 1, 2)});

    loadFont(vdp, art);
    art.ball = gs::uploadMipped(vdp, ballArt());
    art.golfer[0] = gs::uploadMipped(vdp, golferArt(0));
    art.golfer[1] = gs::uploadMipped(vdp, golferArt(1));
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.cup = gs::uploadMipped(vdp, cupArt());
    gs::Bitmap one(1, 1);
    one.set(0, 0, 1);
    art.blot = gs::uploadImage(vdp, one);
    gs::Bitmap sh(8, 4);
    sh.ellipse(4, 2, 3.5f, 1.6f, 1);
    art.shadow = gs::uploadImage(vdp, sh);
}

}  // namespace golfgold
