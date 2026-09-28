#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace lanterntape {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
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

gs::Bitmap walkerBmp() {
    gs::Bitmap b(28, 44);
    b.rect(10, 2, 8, 8, 3);
    b.ellipse(14, 6, 5, 5, 2);
    b.rect(8, 11, 12, 14, 1);
    b.rect(6, 13, 4, 10, 1);
    b.rect(18, 14, 8, 5, 4);
    b.ellipse(26, 16, 3, 3, 5);
    b.rect(10, 25, 4, 14, 1);
    b.rect(15, 25, 4, 14, 1);
    b.rect(8, 37, 6, 3, 6);
    b.rect(15, 37, 6, 3, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap lampBmp() {
    gs::Bitmap b(22, 30);
    b.rect(10, 1, 2, 5, 2);
    b.ellipse(11, 14, 8, 9, 4);
    b.ellipse(11, 14, 5, 6, 3);
    b.rect(9, 8, 4, 4, 5);
    b.rect(6, 22, 10, 3, 2);
    b.rect(8, 25, 6, 3, 1);
    b.outline(6, false);
    return b;
}

gs::Bitmap flameBmp() {
    gs::Bitmap b(12, 16);
    b.ellipse(6, 9, 4, 6, 2);
    b.ellipse(6, 8, 2, 4, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap postBmp() {
    gs::Bitmap b(20, 56);
    b.rect(8, 4, 4, 46, 1);
    b.rect(3, 6, 14, 5, 2);
    b.rect(5, 48, 10, 4, 1);
    b.ellipse(10, 10, 3, 3, 4);
    b.outline(6, false);
    return b;
}

gs::Bitmap slipBmp() {
    gs::Bitmap b(36, 18);
    b.rect(1, 1, 34, 16, 1);
    b.rect(4, 5, 16, 2, 2);
    b.rect(4, 10, 10, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap moonBmp() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(18, 12, 9, 9, 0);
    b.outline(2, false);
    return b;
}

gs::Bitmap starBmp() {
    gs::Bitmap b(7, 7);
    b.set(3, 0, 1);
    b.set(3, 1, 1);
    b.set(3, 2, 1);
    b.set(3, 3, 1);
    b.set(3, 4, 1);
    b.set(3, 5, 1);
    b.set(3, 6, 1);
    b.set(0, 3, 1);
    b.set(1, 3, 1);
    b.set(2, 3, 1);
    b.set(4, 3, 1);
    b.set(5, 3, 1);
    b.set(6, 3, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 10), gs::rgb4(12, 8, 3), gs::rgb4(4, 12, 6), gs::rgb4(14, 4, 3),
                          gs::rgb4(8, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(3, 2, 2), gs::rgb4(10, 7, 2), gs::rgb4(15, 12, 3), gs::rgb4(12, 13, 8),
                           gs::rgb4(6, 4, 2), gs::rgb4(2, 1, 1), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WALK, {0, gs::rgb4(2, 3, 6), gs::rgb4(13, 9, 6), gs::rgb4(4, 3, 2), gs::rgb4(10, 7, 2),
                           gs::rgb4(15, 11, 3), gs::rgb4(1, 1, 2), gs::rgb4(0, 0, 1)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(5, 4, 3), gs::rgb4(8, 8, 9), gs::rgb4(3, 3, 4), gs::rgb4(4, 4, 3), 0,
                           gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_LIT, {0, gs::rgb4(6, 4, 2), gs::rgb4(12, 9, 3), gs::rgb4(15, 12, 4), gs::rgb4(14, 10, 3), 0,
                          gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_SLIP, {0, gs::rgb4(14, 12, 8), gs::rgb4(3, 3, 5), gs::rgb4(12, 3, 2), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_MOON, {0, gs::rgb4(13, 13, 11), gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 14, 11), gs::rgb4(2, 2, 3)});

    art.walker = gs::uploadMipped(vdp, walkerBmp());
    art.lamp = gs::uploadMipped(vdp, lampBmp());
    art.flame = gs::uploadMipped(vdp, flameBmp());
    art.post = gs::uploadMipped(vdp, postBmp());
    art.slip = gs::uploadMipped(vdp, slipBmp());
    art.moon = gs::uploadMipped(vdp, moonBmp());
    art.star = gs::uploadMipped(vdp, starBmp());

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);
    vdp.hudEnabled = true;
    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace lanterntape
