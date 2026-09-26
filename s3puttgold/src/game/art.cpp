#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace puttgold {
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
    b.ellipse(8, 8, 6.4f, 6.4f, 5);
    b.ellipse(8, 8, 5.5f, 5.5f, 3);
    b.ellipse(8, 8, 4.6f, 4.6f, 1);
    b.ellipse(6.1f, 5.8f, 1.7f, 1.2f, 2);
    b.rect(4, 8, 8, 1, 4);
    b.set(11, 10, 4);
    b.set(5, 11, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 8);
    b.ellipse(8, 4, 6.2f, 2.3f, 1);
    return b;
}

gs::Bitmap cupArt() {
    gs::Bitmap b(24, 18);
    b.ellipse(12, 9, 10.2f, 7.2f, 3);
    b.ellipse(12, 9, 7.4f, 5.1f, 2);
    b.ellipse(12, 9, 4.4f, 3.0f, 5);
    b.ellipse(12, 9, 2.1f, 1.4f, 1);
    b.ellipse(10.2f, 7.2f, 1.3f, 0.7f, 4);
    return b;
}

gs::Bitmap flagArt(int frame) {
    gs::Bitmap b(20, 34);
    b.rect(3, 12, 2, 18, 5);
    b.rect(2, 29, 4, 2, 3);
    float tip = frame ? 18.f : 15.5f;
    float mid = frame ? 9.f : 10.5f;
    b.poly({{5, 5}, {tip, mid}, {5, 16}}, 1);
    b.poly({{5, 7}, {tip - 3.5f, mid}, {5, 14}}, 3);
    return b;
}

gs::Bitmap hedgeArt() {
    gs::Bitmap b(16, 12);
    b.ellipse(8, 7, 7.2f, 4.4f, 4);
    b.ellipse(8, 6, 6.0f, 3.6f, 3);
    b.ellipse(5, 5, 2.8f, 2.2f, 2);
    b.ellipse(11, 5, 2.6f, 2.0f, 1);
    return b;
}

gs::Bitmap woodArt() {
    gs::Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 1);
    b.rect(0, 0, 16, 1, 4);
    b.rect(0, 0, 1, 16, 2);
    b.rect(15, 0, 1, 16, 3);
    b.rect(0, 15, 16, 1, 3);
    b.rect(0, 8, 16, 1, 2);
    b.set(4, 4, 4);
    b.set(11, 12, 3);
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
    gs::Bitmap b(26, 34);
    b.rect(11, 20, 4, 11, 3);
    b.rect(9, 30, 8, 2, 3);
    b.ellipse(13, 14, 11, 9, 2);
    b.ellipse(8, 14, 5, 4, 1);
    b.ellipse(17, 12, 5, 4, 1);
    b.ellipse(13, 8, 3, 2, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 1);
    b.ellipse(7, 7, 3.0f, 3.0f, 2);
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
    const uint16_t ink = gs::rgb4(15, 15, 15);
    setPal(vdp, PAL_INK, {0, ink, gs::rgb4(8, 8, 9)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 3), gs::rgb4(6, 4, 1), gs::rgb4(11, 8, 2), gs::rgb4(15, 15, 12),
                           gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 14, 11), gs::rgb4(6, 5, 4), gs::rgb4(12, 10, 8), gs::rgb4(15, 15, 15),
                            gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 8), gs::rgb4(2, 6, 2)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 5, 4), gs::rgb4(5, 1, 1)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), gs::rgb4(8, 9, 10), gs::rgb4(15, 12, 3),
                           gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(12, 8, 3), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(14, 11, 6)});
    setPal(vdp, PAL_HEDGE, {0, gs::rgb4(8, 14, 5), gs::rgb4(4, 10, 3), gs::rgb4(2, 6, 2), gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(6, 13, 5), gs::rgb4(3, 8, 3), gs::rgb4(9, 6, 3)});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_AIM, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 9, 2)});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 13, 4), gs::rgb4(4, 2, 1)});

    loadFont(vdp, art);
    art.ball = gs::uploadMipped(vdp, ballArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cup = gs::uploadMipped(vdp, cupArt());
    art.flag[0] = gs::uploadMipped(vdp, flagArt(0));
    art.flag[1] = gs::uploadMipped(vdp, flagArt(1));
    art.hedge = gs::uploadMipped(vdp, hedgeArt());
    art.wood = gs::uploadMipped(vdp, woodArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("S3 PUTT GOLD", {2, 1, 2, 0, 1}));
    art.doubled = gs::uploadMipped(vdp, gs::textBitmap("DOUBLE", {3, 1, 2, 0, 1}));
    art.fell = gs::uploadMipped(vdp, gs::textBitmap("SHORT", {3, 1, 2, 0, 1}));
    art.num2 = gs::uploadMipped(vdp, gs::textBitmap("2", {2, 1, 2, 0, 1}));
    art.num1 = gs::uploadMipped(vdp, gs::textBitmap("1", {2, 1, 2, 0, 1}));
}

}  // namespace puttgold
