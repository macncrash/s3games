#include "game/art.h"

#include <initializer_list>

namespace boardseven {
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

void paintBoard(gs::Bitmap& b) {
    constexpr int WOOD = 1, EDGE = 2, FELT = 3, WELL = 4, BRASS = 5, SCREW = 6;
    b.rect(0, 0, 320, 224, EDGE);
    b.rect(10, 28, 300, 168, WOOD);
    b.rect(22, 48, 276, 132, FELT);
    b.rect(36, 62, 248, 36, WELL);
    for (int i = 0; i < JACKS; i++) {
        float x = 48.f + i * 60.f;
        b.rect(x - 16, 118, 40, 46, WELL);
        b.ellipse(x + 4, 140, 10, 10, BRASS);
        b.ellipse(x + 4, 140, 4, 4, WOOD);
    }
    b.rect(22, 48, 276, 3, BRASS);
    b.rect(22, 177, 276, 3, BRASS);
    b.rect(16, 34, 6, 6, SCREW);
    b.rect(298, 34, 6, 6, SCREW);
    b.rect(16, 186, 6, 6, SCREW);
    b.rect(298, 186, 6, 6, SCREW);
    b.rect(0, 206, 320, 18, EDGE);
}

gs::Bitmap paintJack() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 13, 13, 1);
    b.ellipse(14, 14, 8, 8, 2);
    b.ellipse(14, 14, 3, 3, 3);
    return b;
}

gs::Bitmap paintLamp() {
    gs::Bitmap b(40, 28);
    b.rect(4, 8, 32, 16, 2);
    b.ellipse(20, 14, 14, 10, 1);
    b.ellipse(20, 14, 6, 4, 3);
    b.rect(16, 22, 8, 6, 2);
    return b;
}

gs::Bitmap paintMark(int id) {
    gs::Bitmap b(16, 16);
    if (id == 0) {
        b.ellipse(8, 8, 6, 6, 1);
        b.ellipse(8, 8, 2, 2, 0);
    } else if (id == 1) {
        b.poly({{8, 1}, {10, 6}, {15, 6}, {11, 10}, {13, 15}, {8, 12}, {3, 15}, {5, 10}, {1, 6}, {6, 6}}, 1);
    } else if (id == 2) {
        b.rect(3, 3, 10, 10, 1);
        b.rect(6, 6, 4, 4, 0);
    } else {
        b.poly({{8, 1}, {15, 8}, {8, 15}, {1, 8}}, 1);
        b.ellipse(8, 8, 2, 2, 0);
    }
    return b;
}

gs::Bitmap paintPlug() {
    gs::Bitmap b(12, 18);
    b.rect(3, 0, 6, 8, 1);
    b.rect(1, 7, 10, 8, 2);
    b.rect(4, 14, 2, 4, 3);
    b.rect(7, 14, 2, 4, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(5, 15, 8)});
    setPal(vdp, PAL_BOARD,
           {0, gs::rgb4(6, 3, 1), gs::rgb4(2, 1, 1), gs::rgb4(1, 4, 3), gs::rgb4(0, 2, 1), gs::rgb4(12, 9, 3),
            gs::rgb4(9, 8, 6)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(13, 10, 4), gs::rgb4(6, 4, 1), gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 6), gs::rgb4(8, 6, 2), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_MARK,
           {0, gs::rgb4(15, 12, 3), gs::rgb4(14, 4, 3), gs::rgb4(4, 12, 14), gs::rgb4(12, 6, 14)});
    setPal(vdp, PAL_CORD, {0, gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_IVORY, {0, gs::rgb4(15, 14, 12)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap board(gs::SCREEN_W, gs::SCREEN_H);
    paintBoard(board);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, board, PAL_BOARD);
    vdp.A.enabled = false;
    vdp.A.clear();
    vdp.B.enabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(1, 1, 2);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.setFogColor(gs::rgb4(1, 1, 2));

    art.jack = gs::uploadMipped(vdp, paintJack());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    for (int i = 0; i < JACKS; i++) art.mark[i] = gs::uploadMipped(vdp, paintMark(i));
    art.plug = gs::uploadMipped(vdp, paintPlug());
    gs::Bitmap solid(8, 8);
    solid.rect(0, 0, 8, 8, 1);
    art.solid = gs::uploadMipped(vdp, solid);
}

}  // namespace boardseven
