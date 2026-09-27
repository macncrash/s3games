#include "game/art.h"

#include <string>

namespace alley {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; ++i) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
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

gs::Bitmap carArt() {
    gs::Bitmap b(40, 28);
    b.rect(10, 4, 20, 6, 4);
    b.rect(12, 5, 7, 4, 5);
    b.rect(21, 5, 7, 4, 5);
    b.rect(8, 10, 24, 8, 2);
    b.rect(10, 10, 20, 2, 1);
    b.ellipse(12, 17, 3, 3, 8);
    b.ellipse(28, 17, 3, 3, 8);
    b.rect(6, 18, 28, 4, 9);
    b.rect(4, 14, 4, 10, 10);
    b.rect(32, 14, 4, 10, 10);
    b.rect(5, 21, 3, 3, 11);
    b.rect(32, 21, 3, 3, 11);
    b.outline(15, false);
    return b;
}

gs::Bitmap vanArt() {
    gs::Bitmap b(42, 40);
    b.rect(8, 3, 26, 10, 4);
    b.rect(10, 5, 10, 6, 5);
    b.rect(22, 5, 10, 6, 5);
    b.rect(7, 13, 28, 12, 2);
    b.rect(9, 13, 24, 2, 1);
    b.rect(14, 20, 14, 5, 6);
    b.ellipse(12, 24, 4, 4, 8);
    b.ellipse(30, 24, 4, 4, 8);
    b.rect(5, 28, 32, 4, 9);
    b.rect(4, 22, 5, 12, 10);
    b.rect(33, 22, 5, 12, 10);
    b.outline(15, false);
    return b;
}

gs::Bitmap truckArt() {
    gs::Bitmap b(48, 52);
    b.rect(14, 2, 20, 8, 1);
    b.rect(16, 4, 8, 5, 5);
    b.rect(26, 4, 6, 5, 5);
    b.rect(10, 10, 28, 10, 4);
    b.rect(12, 12, 24, 2, 1);
    b.rect(12, 20, 24, 12, 6);
    for (int y = 22; y < 30; y += 3) b.rect(14, y, 20, 1, 7);
    b.ellipse(14, 30, 4, 4, 8);
    b.ellipse(34, 30, 4, 4, 8);
    b.rect(6, 34, 36, 5, 9);
    b.rect(4, 28, 6, 16, 10);
    b.rect(38, 28, 6, 16, 10);
    b.rect(5, 40, 4, 4, 11);
    b.rect(39, 40, 4, 4, 11);
    b.outline(15, false);
    return b;
}

gs::Bitmap brickArt() {
    gs::Bitmap b(36, 64);
    b.rect(2, 2, 32, 60, 2);
    for (int y = 4; y < 58; y += 8) {
        int off = ((y / 8) & 1) ? 8 : 0;
        b.rect(4, y, 28, 6, 1);
        for (int x = 4 + off; x < 32; x += 12) b.rect(x, y, 2, 6, 3);
        b.rect(4, y + 6, 28, 2, 4);
    }
    b.rect(2, 2, 32, 3, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(28, 26);
    b.rect(3, 3, 22, 20, 2);
    b.rect(5, 5, 18, 16, 1);
    b.line(5, 5, 23, 21, 4, 1);
    b.line(23, 5, 5, 21, 4, 1);
    b.rect(12, 4, 4, 18, 3);
    b.rect(5, 11, 18, 3, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap barrowArt() {
    gs::Bitmap b(40, 22);
    b.rect(4, 4, 28, 10, 2);
    b.rect(6, 6, 24, 3, 1);
    b.rect(6, 10, 24, 2, 6);
    b.ellipse(10, 16, 4, 4, 8);
    b.ellipse(28, 16, 4, 4, 8);
    b.rect(32, 6, 6, 3, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 28);
    b.rect(6, 10, 4, 14, 3);
    b.rect(4, 2, 8, 10, 1);
    b.rect(5, 4, 6, 6, 2);
    b.rect(3, 22, 10, 4, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(40, 56);
    b.ellipse(20, 16, 14, 12, 1);
    b.ellipse(14, 22, 10, 10, 2);
    b.ellipse(26, 22, 10, 10, 3);
    b.rect(17, 30, 6, 20, 4);
    b.rect(18, 30, 2, 20, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(20, 14);
    b.ellipse(10, 7, 8, 5, 1);
    b.ellipse(6, 6, 3, 2, 2);
    b.ellipse(13, 8, 4, 2, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 8);
    b.ellipse(14, 4, 12, 3, 1);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(36, 16);
    b.ellipse(12, 9, 8, 5, 1);
    b.ellipse(22, 8, 10, 6, 2);
    b.ellipse(18, 6, 6, 4, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_TEXT, gs::rgb4(14, 14, 13));
    textPal(vdp, PAL_GOLD, gs::rgb4(14, 11, 3));
    textPal(vdp, PAL_ALERT, gs::rgb4(14, 3, 2));
    textPal(vdp, PAL_GOOD, gs::rgb4(4, 13, 5));

    setPal(vdp, PAL_CAR,
           {0, gs::rgb4(13, 12, 10), gs::rgb4(9, 8, 6), gs::rgb4(4, 4, 4), gs::rgb4(3, 5, 8), gs::rgb4(7, 9, 12),
            gs::rgb4(12, 6, 2), gs::rgb4(6, 6, 5), gs::rgb4(1, 1, 1), gs::rgb4(5, 5, 5), gs::rgb4(2, 2, 2),
            gs::rgb4(14, 8, 3), gs::rgb4(8, 8, 7), gs::rgb4(15, 15, 12), gs::rgb4(6, 7, 8), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_VAN,
           {0, gs::rgb4(7, 9, 13), gs::rgb4(4, 5, 10), gs::rgb4(2, 2, 5), gs::rgb4(3, 4, 7), gs::rgb4(8, 10, 13),
            gs::rgb4(12, 12, 13), gs::rgb4(1, 1, 2), gs::rgb4(14, 14, 10), gs::rgb4(6, 6, 7), gs::rgb4(1, 1, 1),
            gs::rgb4(9, 9, 8), gs::rgb4(13, 7, 2), gs::rgb4(10, 12, 15), gs::rgb4(5, 6, 8), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_TRUCK,
           {0, gs::rgb4(10, 11, 6), gs::rgb4(6, 7, 3), gs::rgb4(3, 4, 2), gs::rgb4(2, 3, 4), gs::rgb4(6, 8, 9),
            gs::rgb4(3, 3, 2), gs::rgb4(5, 5, 3), gs::rgb4(1, 1, 1), gs::rgb4(4, 4, 3), gs::rgb4(2, 2, 1),
            gs::rgb4(12, 8, 2), gs::rgb4(8, 8, 4), gs::rgb4(14, 13, 7), gs::rgb4(7, 8, 4), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BRICK,
           {0, gs::rgb4(12, 6, 4), gs::rgb4(9, 4, 3), gs::rgb4(6, 3, 2), gs::rgb4(4, 2, 2), gs::rgb4(14, 8, 5),
            gs::rgb4(8, 5, 3), gs::rgb4(3, 2, 1), gs::rgb4(11, 7, 4), gs::rgb4(7, 4, 3), gs::rgb4(5, 3, 2),
            gs::rgb4(13, 9, 6), gs::rgb4(2, 1, 1), gs::rgb4(10, 6, 4), gs::rgb4(15, 10, 7), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CRATE,
           {0, gs::rgb4(12, 8, 3), gs::rgb4(9, 6, 2), gs::rgb4(6, 4, 1), gs::rgb4(4, 3, 1), gs::rgb4(14, 10, 4),
            gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1), gs::rgb4(13, 9, 4),
            gs::rgb4(7, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(15, 12, 6), gs::rgb4(10, 7, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_TREE,
           {0, gs::rgb4(6, 10, 3), gs::rgb4(4, 7, 2), gs::rgb4(3, 5, 2), gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 1),
            gs::rgb4(8, 6, 3), gs::rgb4(2, 3, 1), gs::rgb4(5, 4, 2), gs::rgb4(9, 8, 3), gs::rgb4(1, 2, 1),
            gs::rgb4(7, 9, 3), gs::rgb4(3, 4, 2), gs::rgb4(10, 8, 4), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(14, 14, 12), gs::rgb4(11, 11, 10), gs::rgb4(8, 8, 7), gs::rgb4(15, 13, 8), gs::rgb4(6, 6, 5),
            gs::rgb4(12, 11, 8), gs::rgb4(9, 8, 6), gs::rgb4(4, 4, 3), gs::rgb4(13, 12, 9), gs::rgb4(7, 6, 4),
            gs::rgb4(5, 5, 4), gs::rgb4(15, 15, 13), gs::rgb4(10, 9, 7), gs::rgb4(3, 3, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_RED,
           {0, gs::rgb4(13, 2, 1), gs::rgb4(8, 1, 1), gs::rgb4(15, 7, 4), gs::rgb4(6, 2, 1), gs::rgb4(14, 4, 2),
            0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(15, 13, 4), gs::rgb4(14, 10, 2), gs::rgb4(5, 5, 5), gs::rgb4(3, 3, 3), gs::rgb4(8, 8, 7),
            gs::rgb4(12, 11, 6), gs::rgb4(2, 2, 2), gs::rgb4(15, 15, 10), gs::rgb4(7, 6, 3), gs::rgb4(1, 1, 1),
            gs::rgb4(9, 8, 4), gs::rgb4(4, 4, 3), gs::rgb4(13, 12, 8), gs::rgb4(6, 5, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FLAG,
           {0, gs::rgb4(14, 12, 2), gs::rgb4(12, 2, 2), gs::rgb4(15, 15, 13), gs::rgb4(5, 4, 2), gs::rgb4(2, 2, 1),
            gs::rgb4(10, 8, 2), gs::rgb4(8, 1, 1), gs::rgb4(13, 12, 8), gs::rgb4(6, 5, 2), gs::rgb4(1, 1, 1),
            gs::rgb4(11, 9, 3), gs::rgb4(4, 3, 2), gs::rgb4(14, 10, 4), gs::rgb4(7, 6, 3), gs::rgb4(1, 1, 1)});

    int r = PAL_ROAD * 16;
    vdp.setColor(r + 0, 0);
    vdp.setColor(r + 1, gs::rgb4(7, 8, 4));
    vdp.setColor(r + 2, gs::rgb4(4, 5, 3));
    vdp.setColor(r + 3, gs::rgb4(9, 8, 5));
    vdp.setColor(r + 4, gs::rgb4(8, 7, 4));
    vdp.setColor(r + 5, gs::rgb4(5, 4, 3));
    vdp.setColor(r + 6, gs::rgb4(3, 3, 3));
    vdp.setColor(r + 7, gs::rgb4(5, 5, 5));
    vdp.setColor(r + 8, gs::rgb4(6, 5, 4));
    vdp.setColor(r + 9, gs::rgb4(2, 2, 2));
    vdp.setColor(r + 10, gs::rgb4(4, 4, 4));
    vdp.setColor(r + 11, gs::rgb4(3, 4, 6));
    vdp.setColor(r + 12, gs::rgb4(4, 5, 7));
    vdp.setColor(r + 13, gs::rgb4(8, 9, 10));
    vdp.setColor(r + 14, gs::rgb4(12, 11, 6));
    vdp.setColor(r + 15, gs::rgb4(6, 6, 5));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.car = gs::uploadMipped(vdp, carArt());
    art.van = gs::uploadMipped(vdp, vanArt());
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.brick = gs::uploadMipped(vdp, brickArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.barrow = gs::uploadMipped(vdp, barrowArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
}

}  // namespace alley
