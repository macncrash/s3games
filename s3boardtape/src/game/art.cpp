#include "game/art.h"

#include <initializer_list>

namespace boardtape {
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
    constexpr int EDGE = 1, WOOD = 2, BAY = 3, FELT = 4, BRASS = 5, DRAWER = 6, SLOT = 7;
    b.rect(0, 0, 320, 224, EDGE);
    b.rect(8, 8, 304, 208, WOOD);
    b.rect(18, 16, 284, 36, BAY);
    b.rect(22, 20, 276, 28, 8);
    b.rect(18, 60, 284, 100, FELT);
    b.rect(18, 168, 284, 40, DRAWER);
    for (int i = 0; i < kTapeN; i++) {
        float x = 36.f + i * 90.f;
        b.rect(x, 178, 72, 22, SLOT);
    }
    b.rect(18, 60, 284, 3, BRASS);
    b.rect(18, 157, 284, 3, BRASS);
    b.rect(14, 12, 5, 5, BRASS);
    b.rect(301, 12, 5, 5, BRASS);
    b.rect(14, 204, 5, 5, BRASS);
    b.rect(301, 204, 5, 5, BRASS);
}

gs::Bitmap paintJack() {
    gs::Bitmap b(26, 26);
    b.ellipse(13, 13, 12, 12, 1);
    b.ellipse(13, 13, 7, 7, 2);
    b.ellipse(13, 13, 3, 3, 3);
    return b;
}

gs::Bitmap paintLamp() {
    gs::Bitmap b(36, 24);
    b.rect(6, 16, 24, 6, 2);
    b.ellipse(18, 11, 14, 9, 1);
    b.ellipse(18, 10, 5, 3, 3);
    return b;
}

gs::Bitmap paintPlug() {
    gs::Bitmap b(12, 16);
    b.rect(3, 0, 6, 6, 1);
    b.rect(1, 5, 10, 6, 2);
    b.rect(3, 11, 2, 5, 3);
    b.rect(7, 11, 2, 5, 3);
    return b;
}

gs::Bitmap paintSlip() {
    gs::Bitmap b(40, 18);
    b.rect(0, 0, 40, 18, 1);
    b.rect(1, 1, 38, 16, 2);
    b.rect(3, 4, 18, 2, 3);
    b.rect(3, 8, 28, 2, 3);
    b.rect(3, 12, 12, 2, 3);
    return b;
}

gs::Bitmap paintStamp(int id) {
    gs::Bitmap b(14, 14);
    if (id == 0) {
        b.ellipse(7, 7, 5, 5, 1);
    } else if (id == 1) {
        b.rect(2, 2, 10, 10, 1);
    } else if (id == 2) {
        b.poly({{7, 1}, {13, 12}, {1, 12}}, 1);
    } else {
        b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    }
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(4, 14, 7)});
    setPal(vdp, PAL_BOARD,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(7, 4, 2), gs::rgb4(4, 3, 2), gs::rgb4(1, 5, 4), gs::rgb4(12, 9, 4),
            gs::rgb4(5, 3, 2), gs::rgb4(3, 2, 1), gs::rgb4(13, 12, 9)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(12, 11, 8), gs::rgb4(15, 14, 11), gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(13, 10, 4), gs::rgb4(6, 4, 1), gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 5), gs::rgb4(8, 6, 2), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(2, 2, 6), gs::rgb4(8, 2, 2), gs::rgb4(2, 6, 3), gs::rgb4(6, 2, 8)});
    setPal(vdp, PAL_CORD, {0, gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(8, 8, 12)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap board(gs::SCREEN_W, gs::SCREEN_H);
    paintBoard(board);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, board, PAL_BOARD);
    vdp.A.enabled = false;
    vdp.A.clear();
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(1, 1, 2);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.setFogColor(gs::rgb4(1, 1, 2));

    art.jack = gs::uploadMipped(vdp, paintJack());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.plug = gs::uploadMipped(vdp, paintPlug());
    art.slip = gs::uploadMipped(vdp, paintSlip());
    for (int i = 0; i < kJacks; i++) art.stamp[i] = gs::uploadMipped(vdp, paintStamp(i));
    gs::Bitmap solid(8, 8);
    solid.rect(0, 0, 8, 8, 1);
    art.solid = gs::uploadMipped(vdp, solid);
}

}  // namespace boardtape
