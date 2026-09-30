#include "game/art.h"

#include <initializer_list>

namespace pawnseven {
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
    constexpr int EDGE = 1, LIGHT = 2, DARK = 3, LANE = 4, GOAL = 5, INK = 6;
    b.rect(0, 0, 320, 224, EDGE);
    b.rect(48, 28, 224, 176, EDGE);
    for (int r = 0; r < 8; r++) {
        for (int f = 0; f < 8; f++) {
            int c = ((r + f) & 1) ? DARK : LIGHT;
            if (f == 1 || f == 6) c = LANE;
            if (r == 0) c = GOAL;
            b.rect(56 + f * 26, 36 + r * 20, 24, 18, c);
        }
    }
    b.rect(56, 36, 208, 2, INK);
    b.rect(56, 194, 208, 2, INK);
}

gs::Bitmap paintPawn(bool white) {
    gs::Bitmap b(24, 40);
    int body = white ? 1 : 2;
    int trim = white ? 3 : 4;
    b.ellipse(12, 34, 9, 4, body);
    b.rect(8, 30, 8, 4, body);
    b.ellipse(12, 26, 7, 5, body);
    b.rect(10, 16, 4, 12, body);
    b.ellipse(12, 14, 6, 4, trim);
    b.ellipse(12, 8, 5, 5, body);
    b.ellipse(10, 6, 2, 1, trim);
    return b;
}

gs::Bitmap paintFlag() {
    gs::Bitmap b(16, 20);
    b.rect(2, 2, 2, 16, 1);
    b.poly({{4, 2}, {14, 6}, {4, 10}}, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(5, 15, 8)});
    setPal(vdp, PAL_BOARD,
           {0, gs::rgb4(4, 2, 1), gs::rgb4(12, 10, 7), gs::rgb4(6, 4, 2), gs::rgb4(8, 10, 6), gs::rgb4(13, 11, 3),
            gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_WHITE, {0, gs::rgb4(15, 14, 12), gs::rgb4(2, 2, 2), gs::rgb4(11, 10, 8), gs::rgb4(6, 6, 6)});
    setPal(vdp, PAL_BLACK, {0, gs::rgb4(15, 14, 12), gs::rgb4(1, 1, 1), gs::rgb4(14, 13, 10), gs::rgb4(5, 5, 5)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(10, 7, 2), gs::rgb4(15, 12, 3)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap board(gs::SCREEN_W, gs::SCREEN_H);
    paintBoard(board);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, board, PAL_BOARD);
    vdp.A.enabled = false;
    vdp.A.clear();
    vdp.B.enabled = true;
    vdp.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(1, 1, 2);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.setFogColor(gs::rgb4(1, 1, 2));

    art.white = gs::uploadMipped(vdp, paintPawn(true));
    art.black = gs::uploadMipped(vdp, paintPawn(false));
    art.flag = gs::uploadMipped(vdp, paintFlag());
}

}  // namespace pawnseven
