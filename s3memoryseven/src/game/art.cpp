#include "game/art.h"

#include <initializer_list>

namespace memoryseven {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& art, gs::TileAlloc& tiles) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

void paintTable(gs::Bitmap& b) {
    constexpr int FELT = 1, EDGE = 2, WELL = 3, RAIL = 4, STITCH = 5;
    b.rect(0, 0, 320, 224, EDGE);
    b.rect(8, 22, 304, 176, RAIL);
    b.rect(14, 28, 292, 164, FELT);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float x = 56 + j * (46 + 8);
            float y = 36 + i * (36 + 6);
            b.rect(x - 2, y - 2, 50, 40, WELL);
        }
    }
    b.rect(14, 28, 292, 2, STITCH);
    b.rect(14, 190, 292, 2, STITCH);
    b.rect(0, 206, 320, 18, EDGE);
}

gs::Bitmap paintBack() {
    gs::Bitmap b(46, 36);
    b.rect(0, 0, 46, 36, 1);
    b.rect(3, 3, 40, 30, 2);
    b.rect(6, 6, 34, 24, 1);
    b.ellipse(23, 18, 7, 9, 3);
    b.rect(21, 8, 4, 20, 3);
    return b;
}

gs::Bitmap paintSym(int id) {
    gs::Bitmap b(24, 24);
    int c = id + 1;
    if (id == 0) {
        b.ellipse(11, 12, 9, 9, c);
        b.ellipse(16, 10, 7, 7, 0);
    } else if (id == 1) {
        b.poly({{12, 1}, {15, 9}, {23, 9}, {16, 14}, {19, 23}, {12, 17}, {5, 23}, {8, 14}, {1, 9}, {9, 9}}, c);
    } else if (id == 2) {
        b.ellipse(8, 10, 6, 6, c);
        b.ellipse(8, 10, 2, 2, 0);
        b.rect(12, 9, 10, 3, c);
        b.rect(18, 9, 3, 7, c);
        b.rect(14, 9, 3, 5, c);
    } else if (id == 3) {
        b.ellipse(11, 12, 9, 5, c);
        b.poly({{18, 12}, {23, 6}, {23, 18}}, c);
        b.ellipse(7, 11, 1.4f, 1.4f, 0);
    } else if (id == 4) {
        b.ellipse(8, 9, 5, 5, c);
        b.ellipse(16, 9, 5, 5, c);
        b.poly({{4, 11}, {12, 22}, {20, 11}}, c);
    } else if (id == 5) {
        b.ellipse(12, 11, 8, 8, c);
        b.rect(4, 12, 16, 6, c);
        b.rect(10, 17, 4, 4, c);
        b.rect(8, 20, 8, 2, c);
    } else if (id == 6) {
        b.ellipse(12, 12, 6, 10, c);
        b.line(12, 4, 12, 21, 0, 1.2f);
    } else {
        b.poly({{12, 2}, {18, 12}, {12, 22}, {6, 12}}, c);
        b.ellipse(12, 11, 3, 3, 0);
    }
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(6, 15, 8)});
    setPal(vdp, PAL_TABLE,
           {0, gs::rgb4(1, 7, 3), gs::rgb4(5, 3, 1), gs::rgb4(0, 5, 2), gs::rgb4(7, 4, 2), gs::rgb4(9, 12, 5)});
    setPal(vdp, PAL_BACK, {0, gs::rgb4(1, 2, 6), gs::rgb4(3, 5, 11), gs::rgb4(14, 11, 4)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(15, 14, 12)});
    setPal(vdp, PAL_SYM,
           {0, gs::rgb4(14, 12, 8), gs::rgb4(15, 12, 3), gs::rgb4(8, 5, 2), gs::rgb4(4, 10, 14), gs::rgb4(14, 4, 5),
            gs::rgb4(15, 8, 2), gs::rgb4(4, 13, 6), gs::rgb4(10, 6, 14)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(0, 2, 1)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap table(gs::SCREEN_W, gs::SCREEN_H);
    paintTable(table);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, table, PAL_TABLE);
    vdp.A.enabled = false;
    vdp.A.clear();
    vdp.B.enabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(3, 2, 1);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.setFogColor(gs::rgb4(0, 2, 1));

    art.back = gs::uploadMipped(vdp, paintBack());
    for (int i = 0; i < KINDS; i++) art.sym[i] = gs::uploadMipped(vdp, paintSym(i));
    gs::Bitmap solid(8, 8);
    solid.rect(0, 0, 8, 8, 1);
    art.solid = gs::uploadMipped(vdp, solid);
}

}  // namespace memoryseven
