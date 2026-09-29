#include "game/art.h"

#include <initializer_list>

namespace causewaypace {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) vdp.setColor(pal * 16 + i++, c);
}

gs::Bitmap coatArt(int stride) {
    gs::Bitmap b(20, 36);
    b.ellipse(10, 5, 4, 4, 4);
    b.rect(8, 2, 4, 2, 5);
    b.set(9, 5, 8);
    b.set(12, 5, 8);
    b.poly({{5, 12}, {10, 9}, {15, 12}, {14, 24}, {6, 24}}, 1);
    b.rect(4, 12, 3, 9, 2);
    b.rect(13, 12, 3, 9, 2);
    b.rect(8, 13, 4, 6, 6);
    int lx = stride ? 5 : 7;
    int rx = stride ? 12 : 10;
    b.rect(lx, 23, 3, 10, 3);
    b.rect(rx, 23, 3, 10, 7);
    b.rect(lx - 1, 32, 5, 2, 9);
    b.rect(rx - 1, 32, 5, 2, 9);
    b.rect(15, 16, 3, 3, 11);
    b.outline(10, false);
    return b;
}

gs::Bitmap fallenArt() {
    gs::Bitmap b(36, 14);
    b.ellipse(8, 6, 4, 3, 4);
    b.poly({{12, 4}, {28, 5}, {26, 10}, {12, 10}}, 1);
    b.rect(24, 6, 8, 3, 3);
    b.rect(14, 10, 5, 2, 9);
    b.rect(22, 10, 5, 2, 9);
    b.outline(10, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 32);
    b.rect(4, 6, 4, 24, 1);
    b.rect(2, 4, 8, 4, 2);
    b.rect(3, 28, 6, 3, 3);
    b.rect(5, 10, 2, 3, 4);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(16, 22);
    b.line(3, 20, 5, 2, 1, 1);
    b.line(8, 21, 7, 4, 2, 1);
    b.line(12, 20, 10, 6, 1, 1);
    b.ellipse(6, 3, 2, 1, 3);
    return b;
}

gs::Bitmap lanternArt() {
    gs::Bitmap b(10, 12);
    b.rect(3, 1, 4, 2, 2);
    b.rect(2, 3, 6, 6, 1);
    b.rect(4, 4, 2, 3, 3);
    b.rect(3, 9, 4, 2, 2);
    return b;
}

gs::Bitmap heronArt(int fr) {
    gs::Bitmap b(22, 16);
    b.line(6, 14, 8, 6, 1, 1);
    b.line(10, 14, 9, 6, 1, 1);
    b.ellipse(9, 5, 3, 2, 2);
    b.line(12, 5, 18, fr ? 2 : 7, 1, 1);
    b.poly({{4, 6}, {1, fr ? 2 : 8}, {6, 7}}, 2);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(16, 16);
    b.rect(7, 0, 2, 4, 1);
    b.rect(7, 12, 2, 4, 1);
    b.rect(0, 7, 4, 2, 1);
    b.rect(12, 7, 4, 2, 1);
    b.ellipse(8, 8, 2, 2, 2);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(16, 16);
    b.poly({{8, 0}, {10, 6}, {16, 8}, {10, 10}, {8, 16}, {6, 10}, {0, 8}, {6, 6}}, 1);
    b.ellipse(8, 8, 3, 3, 2);
    return b;
}

gs::Bitmap sprayArt() {
    gs::Bitmap b(22, 10);
    b.ellipse(6, 6, 5, 2, 1);
    b.ellipse(14, 5, 4, 2, 2);
    b.ellipse(18, 7, 3, 1, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 12, 7, 7, 1);
    b.rect(8, 2, 2, 4, 2);
    b.rect(2, 8, 3, 2, 2);
    b.rect(13, 8, 3, 2, 2);
    return b;
}

gs::Bitmap bankArt() {
    gs::Bitmap b(28, 16);
    b.poly({{0, 15}, {6, 6}, {14, 8}, {22, 3}, {28, 15}}, 1);
    b.rect(4, 12, 8, 3, 2);
    b.rect(16, 10, 6, 4, 3);
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
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 12), gs::rgb4(9, 8, 8), gs::rgb4(3, 3, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 11, 4), gs::rgb4(15, 14, 8), gs::rgb4(6, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 2), gs::rgb4(15, 9, 5), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(7, 14, 9), gs::rgb4(13, 15, 12), gs::rgb4(1, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(10, 9, 8), gs::rgb4(6, 6, 6), gs::rgb4(13, 12, 10), gs::rgb4(4, 4, 4),
                            gs::rgb4(15, 12, 6), gs::rgb4(3, 3, 3), gs::rgb4(8, 7, 6), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0,
                            0, 0, ink});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(3, 4, 6), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2), gs::rgb4(11, 8, 5),
                           gs::rgb4(4, 3, 2), gs::rgb4(7, 6, 4), gs::rgb4(5, 4, 3), gs::rgb4(13, 11, 7),
                           gs::rgb4(1, 1, 1), gs::rgb4(2, 2, 2), gs::rgb4(8, 7, 6), gs::rgb4(15, 12, 3), 0, 0, ink});
    setPal(vdp, PAL_HOLD, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 11), gs::rgb4(5, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_LIVE, {0, gs::rgb4(6, 15, 11), gs::rgb4(3, 11, 8), gs::rgb4(1, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_TIDE, {0, gs::rgb4(4, 8, 7), gs::rgb4(8, 10, 8), gs::rgb4(2, 5, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 9, 3), gs::rgb4(8, 8, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, ink});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 10, 4), gs::rgb4(15, 13, 8), gs::rgb4(10, 6, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(9, 8, 7), gs::rgb4(12, 10, 6), gs::rgb4(5, 4, 3), gs::rgb4(14, 12, 4), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, ink});
    const uint16_t road[16] = {0,
                               gs::rgb4(4, 5, 6),
                               gs::rgb4(7, 8, 8),
                               gs::rgb4(2, 3, 4),
                               gs::rgb4(11, 10, 6),
                               gs::rgb4(1, 4, 6),
                               gs::rgb4(2, 6, 8),
                               gs::rgb4(1, 2, 4),
                               gs::rgb4(3, 4, 5),
                               gs::rgb4(8, 9, 9),
                               gs::rgb4(5, 6, 6),
                               gs::rgb4(2, 2, 3),
                               gs::rgb4(13, 12, 7),
                               gs::rgb4(1, 1, 2),
                               gs::rgb4(6, 7, 7),
                               ink};
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    art.coat[0] = gs::uploadMipped(vdp, coatArt(0));
    art.coat[1] = gs::uploadMipped(vdp, coatArt(1));
    art.fallen = gs::uploadMipped(vdp, fallenArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.lantern = gs::uploadMipped(vdp, lanternArt());
    art.heron[0] = gs::uploadMipped(vdp, heronArt(0));
    art.heron[1] = gs::uploadMipped(vdp, heronArt(1));
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.spray = gs::uploadMipped(vdp, sprayArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.bank = gs::uploadMipped(vdp, bankArt());
    loadFont(vdp, art);
}

}  // namespace causewaypace
