#include "game/art.h"

namespace tww {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
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

gs::Bitmap towerArt() {
    gs::Bitmap b(72, 160);
    b.rect(18, 28, 36, 128, 3);
    b.rect(22, 32, 28, 120, 2);
    b.poly({{8, 36}, {36, 4}, {64, 36}}, 4);
    b.poly({{16, 32}, {36, 10}, {56, 32}}, 1);
    b.rect(10, 36, 52, 8, 5);
    b.rect(14, 44, 44, 6, 6);
    for (int i = 0; i < 5; i++) b.rect(16 + i * 9, 36, 3, 8, 7);
    b.rect(30, 118, 12, 38, 8);
    b.rect(33, 124, 6, 10, 9);
    b.rect(26, 70, 8, 14, 9);
    b.rect(40, 70, 8, 14, 9);
    b.rect(33, 92, 8, 12, 9);
    b.line(18, 60, 54, 60, 4, 2);
    b.line(18, 100, 54, 100, 4, 2);
    b.outline(10, false);
    return b;
}

gs::Bitmap wellArt(bool hurt) {
    gs::Bitmap b(80, 64);
    b.ellipse(40, 28, 30, 14, 3);
    b.ellipse(40, 28, 22, 9, 5);
    b.ellipse(40, 30, 16, 6, 6);
    b.ellipse(34, 28, 5, 2, 7);
    b.rect(12, 28, 56, 22, 2);
    b.rect(14, 30, 52, 18, 3);
    b.rect(16, 36, 8, 8, 1);
    b.rect(56, 38, 8, 6, 4);
    b.line(18, 28, 18, 48, 4, 2);
    b.line(62, 28, 62, 48, 4, 2);
    b.rect(36, 6, 8, 24, 8);
    b.line(40, 8, 40, 26, 9, 1.5f);
    if (hurt) {
        b.line(20, 34, 34, 46, 10, 2);
        b.line(48, 32, 60, 44, 10, 2);
        b.rect(22, 40, 10, 6, 10);
    }
    b.outline(4, false);
    return b;
}

gs::Bitmap bucketArt() {
    gs::Bitmap b(16, 18);
    b.rect(3, 4, 10, 12, 2);
    b.rect(4, 6, 8, 8, 1);
    b.line(3, 4, 8, 0, 3, 1);
    b.line(13, 4, 8, 0, 3, 1);
    return b;
}

gs::Bitmap raiderArt(bool brute) {
    gs::Bitmap b(36, 48);
    b.ellipse(18, 10, 7, 7, brute ? 2 : 3);
    b.rect(12, 16, 12, 16, brute ? 1 : 2);
    b.rect(10, 18, 16, 10, brute ? 4 : 5);
    b.line(14, 30, 10, 46, brute ? 1 : 6, 3);
    b.line(22, 30, 26, 46, brute ? 1 : 6, 3);
    b.line(24, 20, 34, 8, 7, 2);
    b.poly({{32, 4}, {35, 10}, {30, 12}}, 8);
    b.rect(8, 22, 6, 4, 7);
    b.outline(9, false);
    return b;
}

gs::Bitmap boltArt() {
    gs::Bitmap b(22, 8);
    b.poly({{0, 4}, {8, 1}, {8, 7}}, 1);
    b.rect(8, 2, 12, 4, 2);
    b.rect(16, 3, 5, 2, 3);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 16, 10, 8, 2);
    b.ellipse(12, 12, 6, 5, 1);
    b.ellipse(18, 14, 4, 3, 3);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(40, 64);
    b.rect(17, 36, 6, 26, 4);
    b.ellipse(20, 24, 16, 16, 2);
    b.ellipse(18, 18, 8, 8, 1);
    b.outline(5, false);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(28, 36);
    b.line(6, 34, 8, 6, 1, 2);
    b.line(14, 34, 12, 2, 2, 2);
    b.line(20, 34, 22, 10, 1, 2);
    b.ellipse(8, 6, 3, 2, 3);
    b.ellipse(12, 3, 3, 2, 3);
    return b;
}

gs::Bitmap bannerArt() {
    gs::Bitmap b(20, 28);
    b.rect(2, 0, 3, 28, 4);
    b.poly({{5, 2}, {18, 8}, {5, 16}}, 1);
    b.poly({{6, 4}, {14, 8}, {6, 13}}, 2);
    return b;
}

gs::Bitmap slitArt() {
    gs::Bitmap b(14, 10);
    b.rect(1, 1, 12, 8, 2);
    b.rect(4, 2, 6, 6, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(15, 12, 4), gs::rgb4(15, 5, 3), gs::rgb4(6, 14, 6), gs::rgb4(8, 10, 14),
                          gs::rgb4(10, 8, 5), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(12, 12, 11), gs::rgb4(8, 8, 8), gs::rgb4(6, 6, 7), gs::rgb4(4, 4, 5),
                            gs::rgb4(9, 7, 4), gs::rgb4(6, 5, 3), gs::rgb4(14, 13, 10), gs::rgb4(3, 3, 4),
                            gs::rgb4(2, 3, 6), gs::rgb4(1, 1, 2), 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WELL, {0, gs::rgb4(11, 11, 10), gs::rgb4(7, 7, 7), gs::rgb4(9, 8, 6), gs::rgb4(4, 4, 4),
                           gs::rgb4(2, 5, 8), gs::rgb4(1, 3, 6), gs::rgb4(6, 10, 12), gs::rgb4(6, 4, 2),
                           gs::rgb4(4, 3, 2), gs::rgb4(8, 3, 2), 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(12, 8, 3), gs::rgb4(8, 5, 2), gs::rgb4(14, 10, 4), gs::rgb4(5, 3, 1),
                           gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(15, 13, 8), gs::rgb4(3, 2, 1),
                           gs::rgb4(2, 2, 2), gs::rgb4(7, 8, 9), gs::rgb4(1, 1, 1), 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RAIDER, {0, gs::rgb4(10, 4, 3), gs::rgb4(6, 2, 2), gs::rgb4(4, 3, 2), gs::rgb4(12, 6, 4),
                             gs::rgb4(8, 5, 3), gs::rgb4(3, 2, 2), gs::rgb4(5, 4, 3), gs::rgb4(12, 11, 8),
                             gs::rgb4(14, 12, 6), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BRUTE, {0, gs::rgb4(8, 2, 3), gs::rgb4(4, 1, 2), gs::rgb4(3, 2, 2), gs::rgb4(12, 3, 3),
                            gs::rgb4(6, 3, 3), gs::rgb4(2, 1, 1), gs::rgb4(9, 8, 6), gs::rgb4(13, 12, 9),
                            gs::rgb4(15, 14, 8), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 6), gs::rgb4(14, 9, 2), gs::rgb4(15, 15, 12), gs::rgb4(8, 4, 1),
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FIELD, {0, gs::rgb4(5, 9, 3), gs::rgb4(3, 7, 2), gs::rgb4(10, 12, 5), gs::rgb4(6, 4, 2),
                            gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(12, 3, 3), gs::rgb4(15, 12, 4), gs::rgb4(8, 2, 2), gs::rgb4(5, 3, 2),
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.well = gs::uploadMipped(vdp, wellArt(false));
    art.wellHurt = gs::uploadMipped(vdp, wellArt(true));
    art.bucket = gs::uploadMipped(vdp, bucketArt());
    art.raider = gs::uploadMipped(vdp, raiderArt(false));
    art.brute = gs::uploadMipped(vdp, raiderArt(true));
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.slit = gs::uploadMipped(vdp, slitArt());
    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace tww
