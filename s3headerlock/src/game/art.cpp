#include "art.h"

#include <initializer_list>

namespace headerlock {
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
    b.poly({{8.f, 8.f}, {32.f, 8.f}, {36.f, 22.f}, {36.f, 50.f}, {4.f, 50.f}, {4.f, 22.f}}, 1);
    b.rect(10, 12, 20, 14, 2);
    b.rect(12, 14, 7, 8, 3);
    b.rect(21, 14, 7, 8, 3);
    b.rect(6, 24, 6, 10, 4);
    b.rect(28, 24, 6, 10, 4);
    b.rect(8, 40, 8, 10, 5);
    b.rect(24, 40, 8, 10, 5);
    b.rect(16, 30, 8, 6, 6);
    b.rect(14, 48, 12, 4, 7);
    b.ellipse(12, 46, 5, 6, 8);
    b.ellipse(28, 46, 5, 6, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(16, 72);
    b.rect(4, 0, 8, 72, 1);
    for (int y = 0; y < 72; y += 10) b.rect(4, y, 8, 5, (y / 10) & 1 ? 2 : 3);
    b.rect(2, 66, 12, 6, 4);
    b.rect(6, 0, 4, 6, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(36, 48);
    b.rect(16, 28, 5, 18, 1);
    b.ellipse(18, 18, 14, 16, 2);
    b.ellipse(14, 16, 7, 8, 3);
    b.ellipse(22, 20, 5, 6, 4);
    return b;
}

gs::Bitmap bannerArt() {
    gs::Bitmap b(72, 28);
    for (int y = 0; y < 28; y += 7) {
        for (int x = 0; x < 72; x += 9) b.rect(x, y, 9, 7, ((x / 9 + y / 7) & 1) ? 1 : 2);
    }
    b.rect(0, 12, 72, 8, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap signArt() {
    gs::Bitmap b(48, 20);
    b.rect(0, 0, 48, 20, 1);
    b.rect(3, 3, 42, 14, 2);
    b.rect(8, 7, 6, 6, 3);
    b.rect(18, 7, 14, 6, 3);
    b.rect(36, 7, 6, 6, 3);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 12, 14), gs::rgb4(15, 12, 4), gs::rgb4(14, 4, 3),
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CAR, {0, gs::rgb4(12, 2, 2), gs::rgb4(6, 10, 14), gs::rgb4(14, 15, 15), gs::rgb4(15, 14, 6),
                          gs::rgb4(3, 3, 4), gs::rgb4(14, 8, 3), gs::rgb4(8, 8, 9), gs::rgb4(2, 2, 2),
                          gs::rgb4(1, 1, 1), ink});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(14, 14, 13), gs::rgb4(12, 3, 2), gs::rgb4(14, 12, 3), gs::rgb4(5, 5, 5),
                           gs::rgb4(15, 15, 8), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(6, 4, 2), gs::rgb4(2, 7, 2), gs::rgb4(4, 10, 3), gs::rgb4(8, 12, 4), ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 1), gs::rgb4(12, 2, 2), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(2, 3, 6), gs::rgb4(14, 12, 4), gs::rgb4(1, 1, 2), gs::rgb4(15, 15, 14), ink});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(3, 8, 3), gs::rgb4(2, 6, 2), gs::rgb4(5, 9, 4), gs::rgb4(6, 6, 4), gs::rgb4(4, 5, 3),
            gs::rgb4(3, 3, 4), gs::rgb4(5, 5, 6), gs::rgb4(7, 7, 6), gs::rgb4(2, 2, 2), gs::rgb4(8, 7, 5),
            gs::rgb4(2, 5, 8), gs::rgb4(3, 7, 10), gs::rgb4(6, 11, 13), gs::rgb4(15, 15, 14), gs::rgb4(6, 6, 7)});
    vdp.setFogColor(gs::rgb4(10, 12, 14));
    loadFont(vdp, art);
    art.car = gs::uploadMipped(vdp, carArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    art.title = words(vdp, "HEADER LOCK", 3, 1, 15);
}

}  // namespace headerlock
