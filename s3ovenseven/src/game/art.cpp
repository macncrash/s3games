#include "game/art.h"

#include <initializer_list>

namespace ovenseven {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
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
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Bitmap ovenArt() {
    gs::Bitmap b(48, 64);
    b.rect(4, 0, 40, 8, 3);
    b.rect(8, 2, 6, 3, 1);
    b.rect(18, 2, 6, 3, 1);
    b.rect(28, 2, 6, 3, 1);
    b.rect(2, 8, 44, 50, 2);
    b.rect(6, 12, 36, 36, 5);
    b.rect(10, 16, 28, 28, 4);
    b.rect(12, 18, 8, 6, 6);
    b.rect(14, 46, 20, 4, 3);
    b.rect(6, 58, 8, 5, 1);
    b.rect(34, 58, 8, 5, 1);
    b.rect(4, 10, 4, 46, 1);
    return b;
}

gs::Bitmap loafArt() {
    gs::Bitmap b(28, 16);
    b.ellipse(14, 9, 12.f, 6.2f, 3);
    b.ellipse(14, 8, 9.f, 4.2f, 2);
    b.ellipse(10, 7, 3.f, 1.6f, 1);
    b.rect(6, 10, 16, 2, 4);
    b.rect(8, 12, 3, 2, 5);
    b.rect(14, 12, 3, 2, 5);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(12, 18);
    b.poly({{6, 1}, {11, 16}, {1, 16}}, 3);
    b.poly({{6, 5}, {9, 16}, {3, 16}}, 2);
    b.poly({{6, 9}, {8, 16}, {4, 16}}, 1);
    return b;
}

gs::Bitmap headArt() {
    gs::Bitmap b(18, 26);
    b.rect(3, 0, 12, 5, 4);
    b.ellipse(9, 12, 6.2f, 6.f, 1);
    b.set(11, 11, 5);
    b.set(12, 11, 5);
    b.rect(7, 14, 3, 1, 3);
    b.rect(4, 18, 10, 7, 2);
    b.rect(2, 19, 3, 5, 2);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3.4f, 3.4f, 2);
    b.ellipse(4, 4, 2.f, 2.f, 1);
    return b;
}

gs::Bitmap rackArt() {
    gs::Bitmap b(40, 8);
    b.rect(0, 3, 40, 2, 2);
    for (int i = 0; i < 5; i++) b.rect(2 + i * 8, 1, 2, 6, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(14, 12, 8);
    const uint16_t shadow = gs::rgb4(3, 1, 1);
    setPal(vdp, PAL_INK, {0, ink, shadow});
    setPal(vdp, PAL_OVEN,
           {0, gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 9), gs::rgb4(12, 12, 13), gs::rgb4(2, 1, 1), gs::rgb4(4, 4, 5),
            gs::rgb4(14, 13, 8)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(4, 2, 2), gs::rgb4(9, 4, 3), gs::rgb4(13, 7, 5), gs::rgb4(2, 1, 1), gs::rgb4(5, 2, 2),
            gs::rgb4(12, 8, 4)});
    setPal(vdp, PAL_LOAF,
           {0, gs::rgb4(15, 13, 8), gs::rgb4(12, 8, 3), gs::rgb4(8, 4, 1), gs::rgb4(6, 3, 1), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 14, 6), gs::rgb4(13, 9, 2), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_FLAME, {0, gs::rgb4(15, 15, 8), gs::rgb4(15, 8, 1), gs::rgb4(12, 2, 1)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 13, 9), gs::rgb4(12, 9, 5), gs::rgb4(8, 5, 2)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 4, 3), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 6), gs::rgb4(2, 6, 2)});
    setPal(vdp, PAL_TITLE, {0, gs::rgb4(15, 10, 3), gs::rgb4(6, 2, 1)});
    setPal(vdp, PAL_BAKER, {0, gs::rgb4(14, 10, 7), gs::rgb4(4, 6, 12), gs::rgb4(10, 3, 3), gs::rgb4(6, 3, 2),
                            gs::rgb4(2, 1, 1)});
    loadFont(vdp, art);
    art.oven = gs::uploadMipped(vdp, ovenArt());
    art.loaf = gs::uploadMipped(vdp, loafArt());
    art.flame = gs::uploadMipped(vdp, flameArt());
    art.head = gs::uploadMipped(vdp, headArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.rack = gs::uploadMipped(vdp, rackArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("OVEN", {3, 1, 2, 0, 1}));
    art.seven = gs::uploadImage(vdp, gs::textBitmap("SEVEN", {3, 1, 2, 0, 1}));
    art.leave = gs::uploadImage(vdp, gs::textBitmap("LEAVE", {2, 1, 2, 0, 1}));
    vdp.setFogColor(gs::rgb4(4, 2, 1));
}

}  // namespace ovenseven
