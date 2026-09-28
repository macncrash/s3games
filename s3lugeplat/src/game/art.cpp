#include "game/art.h"

namespace luge {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap sledArt() {
    gs::Bitmap b(96, 44);
    // Steel runners, feet to the right. The nose is the right edge.
    b.line(8, 36, 90, 34, 3, 2.0f);
    b.line(10, 39, 88, 37, 4, 1.5f);
    b.poly({{16, 30}, {86, 26}, {88, 33}, {14, 36}}, 2);
    b.poly({{22, 24}, {78, 20}, {82, 28}, {20, 31}}, 1);
    b.ellipse(30, 22, 10, 6, 5);  // helmet sits back
    b.rect(28, 16, 8, 6, 6);
    b.poly({{36, 22}, {70, 18}, {74, 26}, {38, 28}}, 7);  // suit
    b.ellipse(76, 20, 5, 4, 7);                           // boots, the nose
    b.ellipse(74, 18, 2, 2, 8);
    b.line(24, 26, 18, 32, 5, 1.5f);
    b.rect(18, 30, 5, 3, 5);
    b.outline(9, false);
    return b;
}

gs::Bitmap platArt() {
    gs::Bitmap b(120, 56);
    b.rect(8, 8, 104, 40, 2);
    for (int i = 0; i < 6; i++) b.rect(10, 10 + i * 6, 100, 2, 3);
    b.rect(8, 8, 6, 40, 4);  // lip face
    b.rect(4, 6, 4, 46, 5);
    for (int i = 0; i < 4; i++) {
        b.rect(28 + i * 22, 44, 4, 10, 1);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(16, 72);
    b.rect(6, 4, 4, 64, 1);
    for (int y = 6; y < 60; y += 8) b.rect(5, y, 6, 4, 2);
    b.rect(2, 0, 12, 6, 3);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(36, 64);
    b.rect(15, 40, 6, 22, 1);
    b.poly({{18, 6}, {34, 44}, {2, 44}}, 2);
    b.poly({{18, 16}, {30, 40}, {6, 40}}, 3);
    b.ellipse(14, 28, 3, 4, 4);
    return b;
}

gs::Bitmap hillArt() {
    gs::Bitmap b(160, 48);
    b.poly({{0, 48}, {0, 30}, {40, 10}, {78, 28}, {110, 6}, {160, 26}, {160, 48}}, 1);
    b.poly({{8, 48}, {46, 18}, {70, 32}, {100, 16}, {150, 36}, {150, 48}}, 2);
    return b;
}

gs::Bitmap iceArt() {
    gs::Bitmap b(64, 20);
    b.rect(0, 0, 64, 16, 1);
    b.rect(0, 14, 64, 6, 2);
    b.line(0, 6, 64, 4, 3, 1.0f);
    b.line(8, 10, 28, 9, 4, 1.0f);
    b.line(40, 8, 58, 11, 4, 1.0f);
    return b;
}

gs::Bitmap chipArt() {
    gs::Bitmap b(12, 12);
    b.poly({{6, 1}, {11, 8}, {2, 10}}, 1);
    b.poly({{5, 3}, {8, 7}, {3, 8}}, 2);
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
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(4, 6, 8), gs::rgb4(15, 12, 4), gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_SLED, {0, gs::rgb4(12, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(12, 13, 14), gs::rgb4(6, 7, 8),
                           gs::rgb4(14, 14, 15), gs::rgb4(15, 15, 15), gs::rgb4(2, 3, 8), gs::rgb4(10, 4, 2),
                           gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_RIDER, {0, gs::rgb4(2, 3, 8), gs::rgb4(14, 14, 15)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(5, 3, 1), gs::rgb4(10, 6, 2), gs::rgb4(13, 9, 4), gs::rgb4(8, 5, 2),
                           gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_ICE, {0, gs::rgb4(10, 13, 15), gs::rgb4(13, 15, 15), gs::rgb4(15, 15, 15), gs::rgb4(7, 10, 13)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(4, 2, 1), gs::rgb4(1, 5, 2), gs::rgb4(2, 8, 3), gs::rgb4(12, 14, 15)});
    setPal(vdp, PAL_HILL, {0, gs::rgb4(12, 13, 15), gs::rgb4(14, 15, 15), gs::rgb4(8, 10, 13)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(2, 2, 3), gs::rgb4(15, 15, 15), gs::rgb4(14, 3, 2)});
    art.sled = gs::uploadMipped(vdp, sledArt());
    art.plat = gs::uploadMipped(vdp, platArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.ice = gs::uploadMipped(vdp, iceArt());
    art.chip = gs::uploadMipped(vdp, chipArt());
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(12, 14, 15));
}

}  // namespace luge
