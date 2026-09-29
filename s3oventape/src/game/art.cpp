#include "game/art.h"

#include <initializer_list>

namespace oventape {
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
    b.rect(6, 0, 36, 7, 3);
    b.rect(10, 2, 5, 3, 1);
    b.rect(21, 2, 5, 3, 1);
    b.rect(32, 2, 5, 3, 1);
    b.rect(2, 8, 44, 48, 2);
    b.rect(7, 14, 34, 32, 5);
    b.rect(11, 18, 26, 22, 4);
    b.rect(14, 20, 8, 5, 6);
    b.rect(16, 42, 16, 3, 3);
    b.rect(8, 56, 8, 6, 1);
    b.rect(32, 56, 8, 6, 1);
    b.rect(3, 10, 3, 44, 1);
    b.rect(42, 10, 3, 44, 1);
    return b;
}

gs::Bitmap loafArt() {
    gs::Bitmap b(28, 16);
    b.ellipse(14, 9, 12.f, 6.f, 3);
    b.ellipse(14, 8, 9.f, 4.f, 2);
    b.ellipse(9, 7, 2.6f, 1.4f, 1);
    b.rect(7, 11, 14, 2, 4);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(12, 18);
    b.poly({{6, 1}, {11, 16}, {1, 16}}, 3);
    b.poly({{6, 5}, {9, 16}, {3, 16}}, 2);
    b.poly({{6, 10}, {7, 16}, {5, 16}}, 1);
    return b;
}

gs::Bitmap headArt() {
    gs::Bitmap b(18, 26);
    b.rect(4, 0, 10, 4, 4);
    b.ellipse(9, 12, 6.f, 6.f, 1);
    b.set(12, 11, 5);
    b.rect(7, 15, 4, 1, 3);
    b.rect(5, 18, 9, 7, 2);
    return b;
}

gs::Bitmap drawerArt() {
    gs::Bitmap b(56, 22);
    b.rect(0, 2, 56, 18, 2);
    b.rect(3, 5, 50, 12, 3);
    b.rect(24, 8, 8, 4, 1);
    b.rect(0, 0, 56, 3, 4);
    return b;
}

gs::Bitmap slipArt() {
    gs::Bitmap b(18, 10);
    b.rect(0, 0, 18, 10, 2);
    b.rect(2, 2, 14, 2, 1);
    b.rect(2, 6, 8, 2, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(14, 12, 8);
    const uint16_t shadow = gs::rgb4(3, 1, 1);
    setPal(vdp, PAL_INK, {0, ink, shadow});
    setPal(vdp, PAL_OVEN,
           {0, gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 9), gs::rgb4(13, 12, 11), gs::rgb4(2, 1, 1), gs::rgb4(5, 4, 4),
            gs::rgb4(15, 12, 6)});
    setPal(vdp, PAL_LOAF, {0, gs::rgb4(15, 13, 8), gs::rgb4(12, 8, 3), gs::rgb4(8, 4, 1), gs::rgb4(5, 3, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 14, 6), gs::rgb4(13, 9, 2), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 11)});
    setPal(vdp, PAL_FLAME, {0, gs::rgb4(15, 15, 8), gs::rgb4(15, 8, 1), gs::rgb4(12, 2, 1)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 14, 10), gs::rgb4(12, 10, 6), gs::rgb4(8, 6, 3)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 4, 3), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 6), gs::rgb4(2, 6, 2)});
    setPal(vdp, PAL_TITLE, {0, gs::rgb4(15, 10, 3), gs::rgb4(6, 2, 1)});
    setPal(vdp, PAL_BAKER,
           {0, gs::rgb4(14, 10, 7), gs::rgb4(4, 6, 12), gs::rgb4(10, 3, 3), gs::rgb4(6, 3, 2), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(10, 6, 2), gs::rgb4(7, 4, 1), gs::rgb4(13, 9, 4), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_TAPE, {0, gs::rgb4(14, 13, 8), gs::rgb4(4, 3, 2), gs::rgb4(9, 3, 2), gs::rgb4(2, 6, 10)});
    loadFont(vdp, art);
    art.oven = gs::uploadMipped(vdp, ovenArt());
    art.loaf = gs::uploadMipped(vdp, loafArt());
    art.flame = gs::uploadMipped(vdp, flameArt());
    art.head = gs::uploadMipped(vdp, headArt());
    art.drawer = gs::uploadMipped(vdp, drawerArt());
    art.slip = gs::uploadMipped(vdp, slipArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("OVEN", {3, 1, 2, 0, 1}));
    art.tapeWord = gs::uploadImage(vdp, gs::textBitmap("TAPE", {3, 1, 2, 0, 1}));
    art.leave = gs::uploadImage(vdp, gs::textBitmap("LEAVE", {2, 1, 2, 0, 1}));
    vdp.setFogColor(gs::rgb4(4, 2, 1));
}

}  // namespace oventape
