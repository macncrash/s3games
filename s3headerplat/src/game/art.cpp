#include "art.h"

#include <initializer_list>

namespace headerplat {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap carArt() {
    gs::Bitmap b(40, 56);
    b.poly({{10.f, 6.f}, {30.f, 6.f}, {36.f, 18.f}, {36.f, 48.f}, {4.f, 48.f}, {4.f, 18.f}}, 1);
    b.rect(12, 10, 16, 12, 2);
    b.rect(14, 12, 5, 7, 3);
    b.rect(21, 12, 5, 7, 3);
    b.rect(6, 22, 7, 8, 4);
    b.rect(27, 22, 7, 8, 4);
    b.rect(8, 36, 8, 8, 5);
    b.rect(24, 36, 8, 8, 5);
    b.rect(17, 28, 6, 5, 6);
    b.ellipse(12, 46, 5, 6, 7);
    b.ellipse(28, 46, 5, 6, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap dockArt() {
    gs::Bitmap b(96, 28);
    b.rect(0, 6, 96, 16, 1);
    for (int x = 0; x < 96; x += 12) b.rect(x, 6, 6, 16, 2);
    b.rect(0, 0, 96, 6, 3);
    b.rect(0, 22, 96, 6, 4);
    b.rect(40, 8, 16, 4, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(14, 64);
    b.rect(4, 0, 6, 64, 1);
    for (int y = 0; y < 64; y += 8) b.rect(4, y, 6, 4, (y / 8) & 1 ? 2 : 3);
    b.rect(1, 58, 12, 6, 4);
    b.rect(5, 0, 4, 5, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(32, 44);
    b.rect(14, 26, 4, 16, 1);
    b.ellipse(16, 16, 13, 14, 2);
    b.ellipse(12, 14, 6, 7, 3);
    return b;
}

gs::Bitmap signArt() {
    gs::Bitmap b(52, 22);
    b.rect(0, 0, 52, 22, 1);
    b.rect(3, 3, 46, 16, 2);
    b.rect(8, 8, 36, 6, 3);
    b.outline(4, false);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
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
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int fill, int edge) {
    gs::TextStyle st{scale, fill, edge, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(10, 13, 8), gs::rgb4(15, 13, 4), gs::rgb4(14, 4, 3),
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CAR, {0, gs::rgb4(4, 8, 12), gs::rgb4(8, 12, 14), gs::rgb4(14, 15, 15), gs::rgb4(15, 12, 3),
                          gs::rgb4(3, 3, 4), gs::rgb4(14, 8, 2), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 2), ink});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(12, 10, 4), gs::rgb4(4, 4, 3), gs::rgb4(14, 12, 5), gs::rgb4(6, 5, 3),
                           gs::rgb4(14, 3, 2), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(13, 13, 12), gs::rgb4(12, 8, 2), gs::rgb4(3, 3, 3), gs::rgb4(5, 5, 4),
                           gs::rgb4(15, 14, 6), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(6, 4, 2), gs::rgb4(2, 7, 2), gs::rgb4(5, 11, 3), ink});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(2, 4, 7), gs::rgb4(14, 13, 6), gs::rgb4(1, 1, 2), gs::rgb4(15, 15, 14), ink});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(3, 8, 3), gs::rgb4(2, 6, 2), gs::rgb4(5, 9, 4), gs::rgb4(6, 6, 4), gs::rgb4(4, 5, 3),
            gs::rgb4(3, 3, 4), gs::rgb4(5, 5, 6), gs::rgb4(7, 7, 6), gs::rgb4(2, 2, 2), gs::rgb4(8, 7, 5),
            gs::rgb4(2, 5, 8), gs::rgb4(3, 7, 10), gs::rgb4(6, 11, 13), gs::rgb4(15, 15, 14), gs::rgb4(6, 6, 7)});
    vdp.setFogColor(gs::rgb4(11, 12, 10));
    loadFont(vdp, art);
    art.car = gs::uploadMipped(vdp, carArt());
    art.dock = gs::uploadMipped(vdp, dockArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    art.title = words(vdp, "HEADER PLAT", 3, 1, 15);
}

}  // namespace headerplat
