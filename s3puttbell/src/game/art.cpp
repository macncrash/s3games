#include "game/art.h"

#include <cstdint>

namespace puttbell {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 1));
}

gs::Bitmap ballArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6.3f, 6.3f, 2);
    b.ellipse(8, 8, 5.2f, 5.2f, 1);
    b.set(6, 6, 3);
    b.set(10, 6, 3);
    b.set(8, 8, 3);
    b.set(5, 9, 3);
    b.set(11, 9, 3);
    b.set(7, 11, 3);
    b.set(10, 11, 3);
    b.ellipse(6.2f, 5.6f, 1.5f, 1.1f, 4);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(14, 8);
    b.ellipse(7, 4, 5.6f, 2.4f, 1);
    return b;
}

gs::Bitmap cupArt() {
    gs::Bitmap b(28, 16);
    b.ellipse(14, 8, 13, 7, 2);
    b.ellipse(14, 8, 10.2f, 5.4f, 3);
    b.ellipse(14, 8, 6.4f, 3.5f, 1);
    b.ellipse(11.2f, 6.2f, 2.2f, 1.1f, 4);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(26, 28);
    b.poly({{5, 8}, {21, 8}, {23, 20}, {13, 25}, {3, 20}}, 1);
    b.poly({{8, 10}, {18, 10}, {19, 18}, {13, 22}, {7, 18}}, 2);
    b.ellipse(13, 20, 7.2f, 4.2f, 3);
    b.ellipse(13, 20.5f, 4.4f, 2.4f, 5);
    b.ellipse(9, 12, 2.2f, 2.6f, 4);
    b.rect(12, 4, 2, 5, 6);
    b.rect(8, 3, 10, 2, 6);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 2.6f, 2.6f, 1);
    b.ellipse(3.2f, 3.2f, 1.1f, 1.1f, 2);
    return b;
}

gs::Bitmap yokeArt() {
    gs::Bitmap b(36, 18);
    b.rect(2, 2, 4, 14, 1);
    b.rect(30, 2, 4, 14, 1);
    b.rect(2, 1, 32, 4, 2);
    b.rect(4, 2, 28, 2, 3);
    b.rect(16, 5, 4, 8, 1);
    return b;
}

gs::Bitmap golferArt(int pose) {
    gs::Bitmap b(24, 32);
    b.ellipse(8, 28, 3.2f, 2.1f, 6);
    b.ellipse(16, 28, 3.2f, 2.1f, 6);
    b.rect(7, 19, 4, 9, 4);
    b.rect(13, 19, 4, 9, 4);
    b.rect(6, 12, 12, 8, 2);
    b.rect(3, 13, 3, 7, 1);
    b.rect(18, 13, 3, 7, 1);
    b.ellipse(12, 8, 4.2f, 4.0f, 1);
    b.ellipse(12, 5.2f, 5.4f, 2.6f, 3);
    b.rect(7, 5, 10, 2, 3);
    if (pose == 0) {
        b.line(18, 15, 20, 1, 5, 1);
        b.rect(17, 0, 6, 2, 5);
    } else {
        b.line(16, 14, 4, 3, 5, 1);
        b.rect(1, 1, 6, 2, 5);
    }
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 1);
    b.rect(0, 0, 16, 2, 2);
    b.rect(0, 14, 16, 2, 3);
    b.rect(0, 7, 16, 1, 3);
    b.rect(5, 2, 1, 12, 4);
    b.rect(11, 2, 1, 12, 4);
    return b;
}

gs::Bitmap tuftArt() {
    gs::Bitmap b(10, 12);
    b.line(5, 11, 2, 2, 1, 1);
    b.line(5, 11, 5, 1, 2, 1);
    b.line(5, 11, 8, 3, 1, 1);
    return b;
}

gs::Bitmap chevronArt() {
    gs::Bitmap b(10, 8);
    b.poly({{1, 1}, {8, 4}, {1, 7}}, 1);
    b.poly({{1, 2}, {6, 4}, {1, 6}}, 2);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(26, 34);
    b.rect(11, 20, 4, 12, 3);
    b.rect(8, 30, 10, 3, 3);
    b.ellipse(13, 14, 11, 10, 2);
    b.ellipse(8, 13, 6, 5, 1);
    b.ellipse(17, 11, 6, 5, 1);
    b.ellipse(13, 7, 4, 3, 1);
    return b;
}

gs::Bitmap flowerArt() {
    gs::Bitmap b(12, 14);
    b.line(6, 13, 6, 6, 3, 1);
    b.ellipse(6, 5, 3.2f, 3.2f, 1);
    b.ellipse(6, 5, 1.4f, 1.4f, 2);
    b.ellipse(3, 7, 1.6f, 1.6f, 1);
    b.ellipse(9, 7, 1.6f, 1.6f, 1);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(30, 14);
    b.ellipse(10, 8, 7, 4, 1);
    b.ellipse(18, 6, 8, 5, 1);
    b.ellipse(24, 8, 5, 3, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 5.2f, 5.2f, 1);
    b.ellipse(7.4f, 7.2f, 2.0f, 1.6f, 2);
    b.line(9, 1, 9, 3, 1, 1);
    b.line(9, 15, 9, 17, 1, 1);
    b.line(1, 9, 3, 9, 1, 1);
    b.line(15, 9, 17, 9, 1, 1);
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 9, 10)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(5, 3, 1), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 4), gs::rgb4(15, 12, 4), gs::rgb4(2, 8, 3)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(7, 15, 7), gs::rgb4(1, 5, 2)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 13, 14), gs::rgb4(7, 8, 9), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(15, 12, 3), gs::rgb4(12, 8, 2), gs::rgb4(6, 4, 1), gs::rgb4(15, 15, 8), gs::rgb4(3, 2, 1),
            gs::rgb4(9, 6, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(11, 7, 3), gs::rgb4(14, 10, 5), gs::rgb4(6, 3, 1), gs::rgb4(8, 5, 2)});
    setPal(vdp, PAL_CUP, {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 10, 5), gs::rgb4(7, 5, 2), gs::rgb4(15, 13, 8)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(8, 14, 5), gs::rgb4(3, 9, 3), gs::rgb4(7, 4, 2)});
    setPal(vdp, PAL_AIM, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 9, 2)});
    setPal(vdp, PAL_PLAYER,
           {0, gs::rgb4(14, 11, 8), gs::rgb4(3, 8, 13), gs::rgb4(12, 2, 2), gs::rgb4(2, 3, 8), gs::rgb4(12, 12, 13),
            gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(8, 14, 4), gs::rgb4(4, 10, 3), gs::rgb4(2, 6, 2)});
    setPal(vdp, PAL_CLOUD, {0, gs::rgb4(14, 15, 15)});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 11)});

    loadFont(vdp, art);
    art.ball = gs::uploadMipped(vdp, ballArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cup = gs::uploadMipped(vdp, cupArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.clapper = gs::uploadMipped(vdp, clapperArt());
    art.yoke = gs::uploadMipped(vdp, yokeArt());
    art.golfer[0] = gs::uploadMipped(vdp, golferArt(0));
    art.golfer[1] = gs::uploadMipped(vdp, golferArt(1));
    art.rail = gs::uploadMipped(vdp, railArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.chevron = gs::uploadMipped(vdp, chevronArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.flower = gs::uploadMipped(vdp, flowerArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("PUTTBELL", {2, 1, 2, 0, 1}));
}

}  // namespace puttbell
