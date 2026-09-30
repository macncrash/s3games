#include "game/art.h"

#include <string>

namespace beacon {
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
    b.rect(18, 12, 2, 4, 12);
    b.outline(8, false);
    return b;
}

gs::Bitmap vanArt() {
    gs::Bitmap b(40, 32);
    b.rect(8, 3, 24, 12, 2);
    b.rect(10, 5, 8, 6, 5);
    b.rect(20, 5, 8, 5, 8);
    b.rect(6, 14, 28, 10, 1);
    b.ellipse(12, 23, 3, 3, 7);
    b.ellipse(28, 23, 3, 3, 7);
    b.rect(16, 16, 3, 6, 12);
    b.outline(7, false);
    return b;
}

gs::Bitmap truckArt() {
    gs::Bitmap b(48, 36);
    b.rect(6, 8, 22, 14, 2);
    b.rect(28, 4, 14, 10, 1);
    b.rect(30, 6, 8, 5, 5);
    b.rect(4, 20, 40, 8, 3);
    b.ellipse(12, 28, 4, 4, 8);
    b.ellipse(36, 28, 4, 4, 8);
    b.rect(8, 10, 16, 3, 11);
    b.rect(18, 22, 4, 4, 13);
    b.outline(8, false);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(28, 64);
    b.rect(8, 18, 12, 42, 2);
    b.rect(10, 20, 8, 38, 3);
    b.rect(6, 10, 16, 10, 1);
    b.rect(4, 8, 20, 4, 4);
    b.rect(12, 2, 4, 8, 5);
    for (int y = 24; y < 56; y += 8) b.rect(11, y, 6, 3, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(24, 24);
    b.ellipse(12, 12, 10, 10, 2);
    b.ellipse(12, 12, 7, 7, 1);
    b.ellipse(12, 12, 3, 3, 8);
    b.outline(3, false);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 2);
    b.ellipse(8, 8, 4, 4, 1);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 40);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(14, 12, 8, 8, 2);
    b.rect(12, 24, 4, 14, 4);
    b.outline(3, false);
    return b;
}

gs::Bitmap rockArt() {
    gs::Bitmap b(24, 16);
    b.poly({{2, 14}, {6, 4}, {16, 3}, {22, 12}, {14, 15}}, 2);
    b.poly({{6, 8}, {12, 5}, {16, 9}, {10, 12}}, 1);
    b.outline(3, false);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(20, 16);
    b.ellipse(8, 9, 6, 4, 2);
    b.ellipse(13, 7, 5, 4, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 8);
    b.ellipse(14, 4, 12, 3, 1);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(8, 8);
    b.rect(3, 1, 2, 6, 1);
    b.rect(1, 3, 6, 2, 1);
    b.set(4, 4, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_TEXT, gs::rgb4(13, 14, 15));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 14, 7));

    setPal(vdp, PAL_CAR,
           {0, gs::rgb4(12, 12, 11), gs::rgb4(8, 8, 9), gs::rgb4(3, 3, 4), gs::rgb4(4, 6, 9), gs::rgb4(8, 10, 13),
            gs::rgb4(11, 6, 3), gs::rgb4(5, 5, 6), gs::rgb4(1, 1, 2), gs::rgb4(6, 6, 6), gs::rgb4(2, 2, 2),
            gs::rgb4(14, 9, 4), gs::rgb4(10, 8, 6), gs::rgb4(15, 15, 13), gs::rgb4(7, 8, 9), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_VAN,
           {0, gs::rgb4(6, 8, 12), gs::rgb4(3, 4, 8), gs::rgb4(2, 2, 4), gs::rgb4(4, 5, 8), gs::rgb4(9, 11, 14),
            gs::rgb4(13, 13, 14), gs::rgb4(1, 1, 2), gs::rgb4(14, 14, 10), gs::rgb4(6, 6, 7), gs::rgb4(1, 1, 1),
            gs::rgb4(8, 8, 8), gs::rgb4(13, 7, 2), gs::rgb4(11, 13, 15), gs::rgb4(5, 6, 8), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_TRUCK,
           {0, gs::rgb4(9, 10, 6), gs::rgb4(5, 6, 3), gs::rgb4(3, 3, 2), gs::rgb4(2, 3, 4), gs::rgb4(6, 8, 9),
            gs::rgb4(3, 3, 2), gs::rgb4(5, 5, 3), gs::rgb4(1, 1, 1), gs::rgb4(4, 4, 3), gs::rgb4(2, 2, 1),
            gs::rgb4(13, 9, 2), gs::rgb4(8, 8, 4), gs::rgb4(14, 13, 7), gs::rgb4(7, 8, 4), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(8, 8, 9), gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(10, 10, 11), gs::rgb4(6, 6, 7),
            gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3), gs::rgb4(12, 12, 12), gs::rgb4(7, 7, 8), gs::rgb4(1, 1, 2),
            gs::rgb4(9, 9, 10), gs::rgb4(4, 5, 6), gs::rgb4(11, 11, 12), gs::rgb4(6, 7, 8), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BEAM,
           {0, gs::rgb4(15, 14, 6), gs::rgb4(14, 10, 2), gs::rgb4(10, 7, 1), gs::rgb4(15, 15, 10), gs::rgb4(8, 6, 2),
            gs::rgb4(12, 11, 4), gs::rgb4(6, 4, 1), gs::rgb4(15, 13, 8), gs::rgb4(9, 8, 3), gs::rgb4(4, 3, 1),
            gs::rgb4(13, 12, 5), gs::rgb4(7, 5, 2), gs::rgb4(15, 12, 4), gs::rgb4(11, 9, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_TREE,
           {0, gs::rgb4(3, 7, 3), gs::rgb4(2, 5, 2), gs::rgb4(1, 3, 2), gs::rgb4(5, 4, 2), gs::rgb4(3, 3, 1),
            gs::rgb4(6, 5, 2), gs::rgb4(1, 2, 1), gs::rgb4(4, 4, 2), gs::rgb4(2, 4, 2), gs::rgb4(1, 1, 1),
            gs::rgb4(4, 8, 3), gs::rgb4(2, 3, 2), gs::rgb4(6, 7, 3), gs::rgb4(1, 2, 1), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(14, 14, 12), gs::rgb4(10, 10, 9), gs::rgb4(7, 7, 6), gs::rgb4(15, 13, 7), gs::rgb4(5, 5, 4),
            gs::rgb4(12, 11, 8), gs::rgb4(8, 7, 5), gs::rgb4(3, 3, 2), gs::rgb4(13, 12, 8), gs::rgb4(6, 5, 3),
            gs::rgb4(4, 4, 3), gs::rgb4(15, 15, 12), gs::rgb4(9, 8, 6), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_NIGHT,
           {0, gs::rgb4(14, 14, 15), gs::rgb4(10, 12, 15), gs::rgb4(2, 3, 6), gs::rgb4(4, 5, 9), gs::rgb4(1, 1, 3),
            0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(15, 14, 5), gs::rgb4(14, 9, 2), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3), gs::rgb4(8, 8, 7),
            gs::rgb4(12, 11, 5), gs::rgb4(1, 1, 2), gs::rgb4(15, 15, 11), gs::rgb4(7, 6, 2), gs::rgb4(1, 1, 1),
            gs::rgb4(10, 8, 3), gs::rgb4(3, 3, 3), gs::rgb4(13, 12, 6), gs::rgb4(6, 5, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_TOWER,
           {0, gs::rgb4(11, 11, 12), gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5), gs::rgb4(13, 12, 8), gs::rgb4(15, 13, 4),
            gs::rgb4(6, 6, 7), gs::rgb4(2, 2, 3), gs::rgb4(9, 9, 10), gs::rgb4(5, 5, 6), gs::rgb4(1, 1, 2),
            gs::rgb4(12, 10, 6), gs::rgb4(8, 7, 5), gs::rgb4(14, 14, 12), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 1)});

    int r = PAL_ROAD * 16;
    vdp.setColor(r + 0, 0);
    vdp.setColor(r + 1, gs::rgb4(2, 3, 2));
    vdp.setColor(r + 2, gs::rgb4(1, 2, 2));
    vdp.setColor(r + 3, gs::rgb4(3, 4, 3));
    vdp.setColor(r + 4, gs::rgb4(3, 3, 2));
    vdp.setColor(r + 5, gs::rgb4(2, 2, 2));
    vdp.setColor(r + 6, gs::rgb4(3, 3, 4));
    vdp.setColor(r + 7, gs::rgb4(2, 2, 3));
    vdp.setColor(r + 8, gs::rgb4(4, 4, 4));
    vdp.setColor(r + 9, gs::rgb4(1, 1, 2));
    vdp.setColor(r + 10, gs::rgb4(5, 5, 5));
    vdp.setColor(r + 11, gs::rgb4(1, 2, 4));
    vdp.setColor(r + 12, gs::rgb4(2, 3, 5));
    vdp.setColor(r + 13, gs::rgb4(6, 7, 8));
    vdp.setColor(r + 14, gs::rgb4(13, 12, 5));
    vdp.setColor(r + 15, gs::rgb4(5, 5, 6));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.car = gs::uploadMipped(vdp, carArt());
    art.van = gs::uploadMipped(vdp, vanArt());
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.star = gs::uploadMipped(vdp, starArt());
}

}  // namespace beacon
