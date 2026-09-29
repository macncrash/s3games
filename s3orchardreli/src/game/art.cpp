#include "game/art.h"

#include <initializer_list>
#include <string>

namespace orchard {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap treeArt() {
    Bitmap b(48, 72);
    b.ellipse(24, 22, 18, 16, 2);
    b.ellipse(16, 26, 10, 9, 3);
    b.ellipse(30, 28, 9, 8, 1);
    b.rect(21, 36, 6, 28, 4);
    b.rect(18, 60, 12, 4, 5);
    b.ellipse(14, 16, 3, 3, 6);
    b.ellipse(28, 14, 3, 3, 6);
    b.ellipse(22, 24, 2, 2, 6);
    b.ellipse(34, 24, 2, 2, 7);
    b.outline(8, false);
    return b;
}

Bitmap keepArt(bool swing) {
    Bitmap b(36, 64);
    b.ellipse(16, 10, 7, 8, 3);
    b.rect(12, 4, 8, 3, 1);
    b.rect(11, 18, 12, 16, 2);
    b.rect(13, 20, 8, 6, 4);
    b.rect(12, 34, 4, 18, 2);
    b.rect(18, 34, 4, 18, 2);
    b.rect(11, 50, 6, 4, 5);
    b.rect(17, 50, 6, 4, 5);
    if (!swing) {
        b.rect(22, 22, 3, 16, 6);
        b.ellipse(23, 40, 4, 3, 7);
    } else {
        b.rect(8, 16, 22, 3, 6);
        b.ellipse(30, 16, 4, 3, 7);
    }
    b.outline(8, false);
    return b;
}

Bitmap crowArt(int frame) {
    Bitmap b(36, 24);
    b.ellipse(18, 14, 8, 5, 1);
    b.ellipse(26, 12, 4, 3, 2);
    b.set(27, 11, 4);
    b.rect(14, 16, 3, 5, 3);
    int lift = frame ? -6 : 2;
    b.poly({{10, 14}, {2, 8 + lift}, {12, 12}}, 1);
    b.poly({{22, 12}, {34, 6 + lift}, {24, 14}}, 2);
    b.outline(8, false);
    return b;
}

Bitmap pickerArt() {
    Bitmap b(28, 56);
    b.ellipse(14, 8, 6, 6, 3);
    b.rect(10, 6, 8, 3, 1);
    b.rect(8, 16, 12, 16, 2);
    b.rect(20, 18, 6, 8, 4);
    b.rect(9, 32, 4, 16, 2);
    b.rect(15, 32, 4, 16, 5);
    b.rect(8, 46, 6, 3, 6);
    b.rect(14, 46, 6, 3, 6);
    b.outline(8, false);
    return b;
}

Bitmap basketArt() {
    Bitmap b(28, 22);
    b.poly({{4, 6}, {24, 6}, {22, 18}, {6, 18}}, 2);
    b.rect(6, 8, 16, 2, 3);
    b.rect(8, 12, 12, 2, 1);
    b.ellipse(8, 6, 2, 2, 4);
    b.ellipse(14, 5, 2, 2, 4);
    b.ellipse(20, 6, 2, 2, 5);
    b.outline(8, false);
    return b;
}

Bitmap bellArt() {
    Bitmap b(28, 32);
    b.rect(13, 0, 2, 6, 1);
    b.poly({{6, 10}, {22, 10}, {24, 22}, {4, 22}}, 2);
    b.rect(4, 20, 20, 3, 3);
    b.ellipse(14, 26, 3, 3, 4);
    b.outline(8, false);
    return b;
}

Bitmap ropeArt() {
    Bitmap b(6, 40);
    b.rect(2, 0, 2, 40, 1);
    b.outline(8, false);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(32, 10);
    b.ellipse(16, 5, 14, 4, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t shadow = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(6, 8, 4), gs::rgb4(14, 12, 4), gs::rgb4(14, 6, 3), gs::rgb4(8, 13, 6),
                          gs::rgb4(12, 10, 6), gs::rgb4(3, 4, 3), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(3, 8, 2), gs::rgb4(2, 6, 2), gs::rgb4(4, 10, 3), gs::rgb4(6, 4, 2),
                           gs::rgb4(4, 3, 2), gs::rgb4(13, 3, 2), gs::rgb4(14, 10, 3), shadow, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_KEEP, {0, gs::rgb4(8, 5, 2), gs::rgb4(4, 7, 9), gs::rgb4(12, 9, 6), gs::rgb4(10, 8, 4),
                           gs::rgb4(3, 3, 3), gs::rgb4(7, 5, 2), gs::rgb4(12, 4, 3), shadow, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CROW, {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5), gs::rgb4(8, 6, 3), gs::rgb4(14, 12, 4), shadow,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PICK, {0, gs::rgb4(5, 3, 2), gs::rgb4(7, 4, 6), gs::rgb4(12, 9, 7), gs::rgb4(9, 6, 3),
                           gs::rgb4(4, 5, 7), gs::rgb4(3, 3, 3), shadow, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(6, 5, 3), gs::rgb4(13, 11, 4), gs::rgb4(15, 14, 8), gs::rgb4(10, 8, 3), shadow,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BASK, {0, gs::rgb4(10, 7, 3), gs::rgb4(8, 5, 2), gs::rgb4(6, 4, 2), gs::rgb4(13, 3, 2),
                           gs::rgb4(12, 10, 3), gs::rgb4(4, 3, 2), shadow, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t floor[16] = {
        0,
        gs::rgb4(3, 8, 2), gs::rgb4(2, 6, 2), gs::rgb4(4, 9, 3),
        gs::rgb4(5, 8, 3), gs::rgb4(3, 7, 2),
        gs::rgb4(8, 6, 3), gs::rgb4(6, 5, 2),
        gs::rgb4(5, 4, 2), gs::rgb4(7, 5, 3), gs::rgb4(9, 7, 3),
        gs::rgb4(2, 5, 6), gs::rgb4(3, 6, 7), gs::rgb4(5, 8, 9),
        gs::rgb4(12, 11, 6), gs::rgb4(7, 6, 3),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FLOOR * 16 + i, floor[i]);

    loadFont(vdp, art);
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.keep[0] = gs::uploadMipped(vdp, keepArt(false));
    art.keep[1] = gs::uploadMipped(vdp, keepArt(true));
    art.crow[0] = gs::uploadMipped(vdp, crowArt(0));
    art.crow[1] = gs::uploadMipped(vdp, crowArt(1));
    art.picker = gs::uploadMipped(vdp, pickerArt());
    art.basket = gs::uploadMipped(vdp, basketArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace orchard
