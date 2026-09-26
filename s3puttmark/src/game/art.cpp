#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace puttmark {
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
    b.ellipse(8, 8, 6.2f, 6.2f, 4);
    b.ellipse(8, 8, 5.2f, 5.2f, 3);
    b.ellipse(7.4f, 7.2f, 4.2f, 4.2f, 1);
    b.ellipse(5.4f, 5.2f, 1.6f, 1.2f, 2);
    b.set(10, 6, 5);
    b.set(11, 9, 5);
    b.set(7, 11, 5);
    b.set(9, 10, 4);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 8);
    b.ellipse(8, 4, 6.4f, 2.4f, 1);
    return b;
}

gs::Bitmap coinArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6.6f, 6.6f, 4);
    b.ellipse(8, 8, 5.4f, 5.4f, 2);
    b.ellipse(8, 8, 4.2f, 4.2f, 1);
    b.rect(3, 7, 10, 1, 3);
    b.ellipse(8, 8, 1.3f, 1.3f, 5);
    b.set(5, 5, 6);
    return b;
}

gs::Bitmap cupArt() {
    gs::Bitmap b(22, 16);
    b.ellipse(11, 8, 10, 6.6f, 4);
    b.ellipse(11, 8, 7.4f, 4.8f, 2);
    b.ellipse(11, 8, 5.2f, 3.4f, 1);
    b.ellipse(11, 8, 2.4f, 1.6f, 3);
    b.set(8, 6, 2);
    return b;
}

gs::Bitmap flagArt(int frame) {
    gs::Bitmap b(16, 32);
    b.rect(3, 4, 2, 24, 3);
    b.rect(2, 27, 4, 2, 4);
    int tip = frame ? 14 : 15;
    b.rect(5, 4, tip - 5, 8, 1);
    b.rect(5, 8, tip - 6, 3, 2);
    b.rect(5, 4, tip - 5, 1, 5);
    return b;
}

gs::Bitmap flagDownArt() {
    gs::Bitmap b(16, 32);
    b.rect(3, 6, 2, 22, 3);
    b.rect(2, 27, 4, 2, 4);
    b.rect(5, 16, 9, 7, 1);
    b.rect(5, 20, 8, 2, 2);
    return b;
}

gs::Bitmap tuftArt() {
    gs::Bitmap b(10, 12);
    b.line(5, 11, 2, 2, 2, 1);
    b.line(5, 11, 5, 1, 1, 1);
    b.line(5, 11, 8, 3, 3, 1);
    b.set(4, 6, 1);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(26, 22);
    b.rect(11, 14, 4, 6, 4);
    b.ellipse(13, 11, 11, 8, 2);
    b.ellipse(8, 12, 6, 5, 1);
    b.ellipse(17, 10, 6, 5, 3);
    b.ellipse(13, 7, 4, 3, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7.2f, 7.2f, 1);
    b.ellipse(7, 7, 2.6f, 2.6f, 2);
    b.line(9, 1, 9, 3, 2, 1);
    b.line(1, 9, 3, 9, 2, 1);
    b.line(15, 9, 17, 9, 2, 1);
    return b;
}

gs::Bitmap putterArt() {
    gs::Bitmap b(28, 12);
    b.rect(2, 5, 16, 2, 1);
    b.rect(17, 2, 4, 8, 2);
    b.rect(18, 3, 1, 6, 3);
    b.rect(1, 4, 3, 4, 3);
    return b;
}

gs::Bitmap gloveArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 10, 5.6f, 4.2f, 2);
    b.ellipse(8, 9, 4.4f, 3.2f, 1);
    b.ellipse(12.2f, 6.2f, 2.3f, 2.5f, 1);
    b.ellipse(12.2f, 6.2f, 1.2f, 1.3f, 2);
    b.rect(5, 12, 7, 3, 3);
    return b;
}

gs::Bitmap chevronArt() {
    gs::Bitmap b(12, 8);
    b.poly({{1, 1}, {7, 4}, {1, 7}}, 1);
    b.poly({{5, 1}, {11, 4}, {5, 7}}, 2);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(6, 6);
    b.ellipse(3, 3, 2.1f, 2.1f, 1);
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

gs::Image phrase(gs::VDP& vdp, const char* s) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {2, 1, 2, 0, 1}));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 15);
    setPal(vdp, PAL_INK, {0, ink, gs::rgb4(11, 12, 13)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(15, 12, 4), gs::rgb4(8, 5, 1)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 7), gs::rgb4(1, 5, 2)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 5, 4), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 13, 14), gs::rgb4(7, 8, 9), gs::rgb4(3, 3, 4),
                           gs::rgb4(9, 9, 8)});
    setPal(vdp, PAL_COIN, {0, gs::rgb4(15, 13, 5), gs::rgb4(12, 9, 3), gs::rgb4(6, 4, 1), gs::rgb4(3, 2, 1),
                           gs::rgb4(1, 1, 1), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_CUP, {0, gs::rgb4(1, 1, 1), gs::rgb4(8, 6, 3), gs::rgb4(2, 2, 2), gs::rgb4(4, 8, 3)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 2, 2), gs::rgb4(12, 10, 6), gs::rgb4(5, 3, 1),
                           gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(8, 14, 5), gs::rgb4(3, 9, 3), gs::rgb4(2, 6, 2)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(5, 13, 4), gs::rgb4(2, 8, 3), gs::rgb4(3, 11, 4), gs::rgb4(7, 5, 2)});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 11)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(15, 14, 12), gs::rgb4(12, 10, 8), gs::rgb4(4, 6, 8)});
    setPal(vdp, PAL_AIM, {0, gs::rgb4(15, 14, 5), gs::rgb4(10, 7, 1)});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 13, 4), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_PUTTER, {0, gs::rgb4(12, 12, 13), gs::rgb4(4, 4, 5), gs::rgb4(8, 6, 3)});

    loadFont(vdp, art);
    art.ball = gs::uploadMipped(vdp, ballArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.coin = gs::uploadMipped(vdp, coinArt());
    art.cup = gs::uploadMipped(vdp, cupArt());
    art.flag[0] = gs::uploadMipped(vdp, flagArt(0));
    art.flag[1] = gs::uploadMipped(vdp, flagArt(1));
    art.flagDown = gs::uploadMipped(vdp, flagDownArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.putter = gs::uploadMipped(vdp, putterArt());
    art.glove = gs::uploadMipped(vdp, gloveArt());
    art.chevron = gs::uploadMipped(vdp, chevronArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.logo = phrase(vdp, "S3 PUTTMARK");
    art.finished = phrase(vdp, "FINISHED MARK");
    art.open = phrase(vdp, "STILL OPEN");
}

}  // namespace puttmark
