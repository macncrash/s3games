#include "game/art.h"

#include <cstdint>

namespace ovengold {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* c) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
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

gs::Bitmap loafArt() {
    gs::Bitmap b(36, 28);
    b.ellipse(18, 16, 14, 8, 2);
    b.ellipse(18, 14, 13, 7, 3);
    b.ellipse(14, 12, 4, 2, 4);
    b.line(10, 15, 16, 18, 5, 1.2f);
    b.line(18, 12, 24, 17, 5, 1.2f);
    b.outline(1, false);
    return b;
}

gs::Bitmap archArt() {
    gs::Bitmap b(52, 48);
    b.rect(2, 14, 48, 32, 2);
    for (int y = 18; y < 44; y += 6) b.rect(2, y, 48, 1, 1);
    b.ellipse(26, 30, 16, 14, 3);
    b.ellipse(26, 32, 12, 10, 4);
    b.rect(22, 6, 8, 10, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(16, 20);
    b.poly({{8, 2}, {2, 12}, {5, 18}, {8, 15}, {11, 18}, {14, 12}}, 1);
    b.poly({{8, 7}, {5, 13}, {8, 16}, {11, 13}}, 2);
    b.ellipse(8, 14, 2, 2.4f, 3);
    return b;
}

gs::Bitmap peelArt() {
    gs::Bitmap b(28, 10);
    b.rect(0, 3, 20, 4, 2);
    b.rect(16, 1, 12, 8, 3);
    b.outline(1, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink[16] = {0, gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1)};
    const uint16_t gold[16] = {0, gs::rgb4(15, 14, 6), gs::rgb4(8, 5, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 10),
                               gs::rgb4(12, 9, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 0)};
    const uint16_t cream[16] = {0, gs::rgb4(15, 15, 13), gs::rgb4(9, 8, 6), gs::rgb4(13, 12, 9), gs::rgb4(15, 14, 12),
                                gs::rgb4(10, 9, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 3, 2)};
    const uint16_t brick[16] = {0, gs::rgb4(4, 2, 2), gs::rgb4(10, 4, 3), gs::rgb4(6, 3, 2), gs::rgb4(2, 1, 1),
                                gs::rgb4(12, 8, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t fire[16] = {0, gs::rgb4(15, 6, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 8), 0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0, 0};
    const uint16_t loaf[16] = {0, gs::rgb4(3, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(12, 7, 3), gs::rgb4(14, 11, 6),
                               gs::rgb4(15, 13, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t alert[16] = {0, gs::rgb4(15, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 0, 0)};
    const uint16_t ok[16] = {0, gs::rgb4(8, 15, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 3, 1)};
    const uint16_t wood[16] = {0, gs::rgb4(4, 2, 1), gs::rgb4(10, 6, 2), gs::rgb4(13, 9, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0};
    const uint16_t ash[16] = {0, gs::rgb4(6, 6, 6), gs::rgb4(3, 3, 3), gs::rgb4(9, 9, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0};
    setPal(vdp, PAL_INK, ink);
    setPal(vdp, PAL_GOLD, gold);
    setPal(vdp, PAL_CREAM, cream);
    setPal(vdp, PAL_BRICK, brick);
    setPal(vdp, PAL_FIRE, fire);
    setPal(vdp, PAL_LOAF, loaf);
    setPal(vdp, PAL_ALERT, alert);
    setPal(vdp, PAL_OK, ok);
    setPal(vdp, PAL_WOOD, wood);
    setPal(vdp, PAL_ASH, ash);
    loadFont(vdp, art);
    art.loaf = gs::uploadMipped(vdp, loafArt());
    art.arch = gs::uploadMipped(vdp, archArt());
    art.flame = gs::uploadMipped(vdp, flameArt());
    art.peel = gs::uploadMipped(vdp, peelArt());
    gs::Bitmap dot(4, 4);
    dot.rect(0, 0, 4, 4, 1);
    art.solid = gs::uploadImage(vdp, dot);
    art.word = gs::uploadImage(vdp, gs::textBitmap("OVEN", {3, 1, 2, 0, 1}));
    art.doubled = gs::uploadImage(vdp, gs::textBitmap("DOUBLE", {3, 1, 2, 0, 1}));
    art.noDouble = gs::uploadImage(vdp, gs::textBitmap("NO DOUBLE", {2, 1, 2, 0, 1}));
}

}  // namespace ovengold
