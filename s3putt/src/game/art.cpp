#include "game/art.h"

#include <cstdint>

namespace putt {
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
    b.ellipse(8, 8, 6.4f, 6.4f, 4);
    b.ellipse(8, 8, 5.4f, 5.4f, 3);
    b.ellipse(7.6f, 7.4f, 4.6f, 4.6f, 1);
    b.ellipse(5.6f, 5.4f, 1.8f, 1.4f, 2);
    b.set(9, 9, 3);
    b.set(10, 7, 3);
    b.set(7, 10, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 8);
    b.ellipse(8, 4, 6.5f, 2.6f, 1);
    return b;
}

gs::Bitmap cupArt() {
    gs::Bitmap b(22, 16);
    b.ellipse(11, 8, 10, 7, 2);
    b.ellipse(11, 8, 7.2f, 5.1f, 3);
    b.ellipse(11, 8, 4.6f, 3.3f, 1);
    b.ellipse(9.4f, 6.6f, 1.3f, 0.8f, 2);
    return b;
}

gs::Bitmap flagArt(int frame) {
    gs::Bitmap b(18, 30);
    b.rect(4, 8, 2, 20, 6);
    float tip = frame ? 15.5f : 16.8f;
    float mid = frame ? 9.5f : 12.0f;
    b.poly({{6, 6}, {tip, mid}, {6, 16}}, 4);
    b.poly({{6, 8}, {tip - 3.0f, mid}, {6, 13}}, 5);
    b.rect(3, 26, 4, 2, 3);
    return b;
}

gs::Bitmap hedgeArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 10, 7.2f, 5.2f, 4);
    b.ellipse(8, 9, 6.4f, 4.6f, 3);
    b.ellipse(5, 8, 3.4f, 3.0f, 2);
    b.ellipse(11, 7, 3.6f, 2.8f, 1);
    b.ellipse(8, 6, 2.4f, 2.0f, 1);
    b.set(6, 7, 1);
    b.set(12, 9, 2);
    return b;
}

gs::Bitmap sandArt() {
    gs::Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 2);
    b.set(2, 3, 1);
    b.set(3, 3, 1);
    b.set(8, 2, 1);
    b.set(13, 5, 1);
    b.set(5, 8, 3);
    b.set(11, 7, 3);
    b.set(4, 12, 1);
    b.set(9, 13, 1);
    b.set(14, 11, 3);
    b.set(7, 5, 3);
    b.set(12, 14, 1);
    return b;
}

gs::Bitmap waterArt(int frame) {
    gs::Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 1);
    int y0 = 2 + frame * 3;
    int y1 = 11 - frame * 2;
    b.rect(0, y0, 16, 2, 2);
    b.rect(0, y1, 16, 1, 3);
    b.set(3, 7 + frame, 3);
    b.set(10, 5 + frame, 2);
    b.set(13, 12 - frame, 3);
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
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 36);
    b.rect(12, 22, 4, 12, 3);
    b.rect(11, 32, 6, 2, 3);
    b.ellipse(14, 16, 12, 10, 2);
    b.ellipse(9, 15, 6, 5, 1);
    b.ellipse(18, 13, 6, 5, 1);
    b.ellipse(14, 10, 4, 3, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 8, 1);
    b.ellipse(8, 8, 3.2f, 3.2f, 2);
    return b;
}

gs::Bitmap tuftArt() {
    gs::Bitmap b(8, 10);
    b.line(4, 9, 2, 2, 1, 1);
    b.line(4, 9, 6, 1, 2, 1);
    b.line(4, 9, 4, 2, 1, 1);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(6, 6);
    b.ellipse(3, 3, 2.2f, 2.2f, 1);
    return b;
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 15);
    setPal(vdp, PAL_WHITE, {0, ink, gs::rgb4(10, 11, 12)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 6, 2)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(6, 15, 6), gs::rgb4(1, 5, 2)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), gs::rgb4(8, 9, 10), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_CUP, {0, gs::rgb4(1, 1, 2), gs::rgb4(12, 9, 5), gs::rgb4(6, 4, 2), gs::rgb4(13, 2, 2),
                          gs::rgb4(8, 1, 1), gs::rgb4(14, 13, 10)});
    setPal(vdp, PAL_HEDGE, {0, gs::rgb4(6, 14, 5), gs::rgb4(3, 11, 3), gs::rgb4(2, 7, 2), gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_SAND, {0, gs::rgb4(14, 12, 7), gs::rgb4(12, 10, 5), gs::rgb4(9, 7, 4)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 6, 12), gs::rgb4(3, 9, 14), gs::rgb4(11, 14, 15)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(10, 6, 3), gs::rgb4(6, 3, 1), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(4, 12, 4), gs::rgb4(2, 8, 3), gs::rgb4(8, 5, 2)});
    setPal(vdp, PAL_AIM, {0, gs::rgb4(15, 14, 4), gs::rgb4(15, 9, 2)});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 13, 4), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 12)});

    loadFont(vdp, art);
    art.ball = gs::uploadMipped(vdp, ballArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cup = gs::uploadMipped(vdp, cupArt());
    art.flag[0] = gs::uploadMipped(vdp, flagArt(0));
    art.flag[1] = gs::uploadMipped(vdp, flagArt(1));
    art.hedge = gs::uploadMipped(vdp, hedgeArt());
    art.sand = gs::uploadMipped(vdp, sandArt());
    art.water[0] = gs::uploadMipped(vdp, waterArt(0));
    art.water[1] = gs::uploadMipped(vdp, waterArt(1));
    art.wood = gs::uploadMipped(vdp, woodArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("S3 PUTT", {2, 1, 2, 0, 1}));
    art.win = gs::uploadMipped(vdp, gs::textBitmap("NINE CUPS", {3, 1, 2, 0, 1}));
}

}  // namespace putt
