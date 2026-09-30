#include "game/art.h"

#include <initializer_list>
#include <string>

namespace foundrydoor {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap founderArt(int step) {
    gs::Bitmap b(32, 64);
    b.ellipse(16, 10, 6, 6, 4);
    b.poly({{8, 3}, {24, 3}, {22, 10}, {10, 10}}, 8);
    b.rect(12, 9, 2, 2, 9);
    b.rect(18, 9, 2, 2, 9);
    b.poly({{8, 16}, {24, 16}, {26, 38}, {6, 38}}, 2);
    b.rect(13, 18, 6, 14, 11);
    b.line(10, 20, 4, 34, 6, 3.f);
    b.ellipse(4, 36, 3, 3, 7);
    b.line(22, 20, 28, 32, 6, 3.f);
    b.ellipse(28, 34, 3, 3, 6);
    if (step == 0) {
        b.rect(9, 38, 6, 20, 3);
        b.rect(17, 38, 6, 16, 5);
        b.rect(8, 56, 8, 4, 14);
        b.rect(16, 52, 8, 4, 8);
    } else {
        b.rect(9, 38, 6, 16, 5);
        b.rect(17, 38, 6, 20, 3);
        b.rect(8, 52, 8, 4, 8);
        b.rect(16, 56, 8, 4, 14);
    }
    b.outline(12, false);
    return b;
}

gs::Bitmap doorArt() {
    gs::Bitmap b(56, 96);
    b.rect(4, 4, 48, 88, 2);
    b.rect(8, 8, 40, 80, 1);
    b.rect(12, 16, 32, 8, 3);
    b.rect(12, 36, 32, 8, 3);
    b.rect(12, 56, 32, 8, 3);
    b.ellipse(40, 48, 5, 5, 5);
    b.rect(38, 46, 4, 4, 6);
    b.rect(2, 8, 4, 16, 4);
    b.rect(2, 72, 4, 16, 4);
    b.outline(8, false);
    return b;
}

gs::Bitmap flameArt(int fr) {
    gs::Bitmap b(18, 28);
    b.poly({{9, 2}, {16, 14}, {11, 26}, {7, 26}, {2, 14}}, fr ? 2 : 1);
    b.poly({{9, 10}, {12, 16}, {9, 22}, {6, 16}}, 3);
    return b;
}

gs::Bitmap ladleArt() {
    gs::Bitmap b(28, 18);
    b.ellipse(10, 10, 8, 6, 2);
    b.ellipse(10, 9, 5, 3, 4);
    b.rect(16, 8, 10, 3, 1);
    b.outline(3, false);
    return b;
}

gs::Bitmap hookArt() {
    gs::Bitmap b(14, 28);
    b.rect(6, 0, 2, 16, 1);
    b.poly({{6, 14}, {12, 18}, {10, 26}, {4, 22}}, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(6, 6);
    b.line(1, 5, 5, 1, 1, 1.2f);
    b.set(3, 2, 2);
    return b;
}

gs::Bitmap notchArt() {
    gs::Bitmap b(10, 14);
    b.rect(2, 1, 6, 12, 1);
    b.rect(3, 3, 4, 8, 2);
    return b;
}

gs::Bitmap slabArt() {
    gs::Bitmap b(20, 10);
    b.rect(1, 2, 18, 6, 1);
    b.rect(2, 3, 16, 4, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 2);
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

void brickTile(gs::VDP& vdp) {
    uint8_t px[64];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int mortar = (y == 0) || (y < 4 ? x == 0 : x == 4);
            px[y * 8 + x] = mortar ? 2 : ((x + y) & 1 ? 1 : 3);
        }
    }
    vdp.loadTile(1, px);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 0, 0);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 12, 9), gs::rgb4(8, 7, 6), gs::rgb4(3, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_EMBER, {0, gs::rgb4(15, 8, 1), gs::rgb4(15, 13, 5), gs::rgb4(6, 2, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 3, 2), gs::rgb4(15, 9, 5), gs::rgb4(5, 1, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 5), gs::rgb4(13, 15, 9), gs::rgb4(1, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 5), gs::rgb4(12, 11, 10), gs::rgb4(2, 2, 3),
                           gs::rgb4(14, 9, 3), gs::rgb4(15, 12, 4), gs::rgb4(6, 5, 4), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_FIGURE,
           {0, gs::rgb4(11, 6, 2), gs::rgb4(6, 3, 1), gs::rgb4(3, 2, 2), gs::rgb4(13, 10, 7), gs::rgb4(4, 3, 2),
            gs::rgb4(9, 7, 5), gs::rgb4(15, 9, 2), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), gs::rgb4(8, 5, 2),
            gs::rgb4(14, 12, 6), gs::rgb4(1, 0, 0), gs::rgb4(6, 4, 3), gs::rgb4(4, 3, 2), ink});
    setPal(vdp, PAL_HEAT, {0, gs::rgb4(15, 6, 1), gs::rgb4(15, 12, 3), gs::rgb4(8, 2, 0), gs::rgb4(15, 15, 8), 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SOOT, {0, gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 2), gs::rgb4(6, 5, 4), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 8, 1), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, ink});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(9, 4, 2), gs::rgb4(4, 2, 1), gs::rgb4(12, 6, 3), gs::rgb4(2, 1, 1), 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SLAG, {0, gs::rgb4(5, 4, 3), gs::rgb4(10, 5, 1), gs::rgb4(3, 2, 2), gs::rgb4(14, 7, 1), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, ink});

    loadFont(vdp, art);
    brickTile(vdp);
    art.founder[0] = gs::uploadMipped(vdp, founderArt(0));
    art.founder[1] = gs::uploadMipped(vdp, founderArt(1));
    art.door = gs::uploadMipped(vdp, doorArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.ladle = gs::uploadMipped(vdp, ladleArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.notch = gs::uploadMipped(vdp, notchArt());
    art.slab = gs::uploadMipped(vdp, slabArt());
    vdp.setFogColor(gs::rgb4(6, 2, 1));
}

}  // namespace foundrydoor
