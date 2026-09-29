#include "game/art.h"

#include <initializer_list>

namespace drawertape {
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

void paintDesk(gs::Bitmap& b) {
    constexpr int EDGE = 1, OAK = 2, TAPE = 3, WELL = 4, BRASS = 5, DRAWER = 6, SLOT = 7, LINEN = 8;
    b.rect(0, 0, 320, 224, EDGE);
    b.rect(6, 6, 308, 212, OAK);
    b.rect(16, 14, 288, 40, TAPE);
    b.rect(22, 20, 276, 28, LINEN);
    b.rect(16, 62, 288, 86, WELL);
    b.rect(16, 156, 288, 52, DRAWER);
    b.rect(148, 162, 24, 8, BRASS);
    for (int i = 0; i < kTapeN; i++) {
        float x = 32.f + i * 92.f;
        b.rect(x, 176, 76, 24, SLOT);
    }
    b.rect(16, 154, 288, 4, BRASS);
    b.rect(12, 10, 6, 6, BRASS);
    b.rect(302, 10, 6, 6, BRASS);
    b.rect(12, 206, 6, 6, BRASS);
    b.rect(302, 206, 6, 6, BRASS);
}

gs::Bitmap paintTray() {
    gs::Bitmap b(28, 22);
    b.rect(0, 4, 28, 16, 1);
    b.rect(2, 6, 24, 12, 2);
    b.rect(10, 0, 8, 6, 3);
    return b;
}

gs::Bitmap paintLamp() {
    gs::Bitmap b(20, 16);
    b.ellipse(10, 8, 8, 6, 1);
    b.ellipse(10, 8, 3, 2, 2);
    return b;
}

gs::Bitmap paintHand() {
    gs::Bitmap b(18, 14);
    b.ellipse(9, 8, 8, 5, 1);
    b.rect(2, 2, 3, 6, 2);
    b.rect(7, 1, 3, 6, 2);
    b.rect(12, 2, 3, 6, 2);
    return b;
}

gs::Bitmap paintSlip() {
    gs::Bitmap b(36, 16);
    b.rect(0, 0, 36, 16, 1);
    b.rect(1, 1, 34, 14, 2);
    b.rect(3, 4, 16, 2, 3);
    b.rect(3, 8, 24, 2, 3);
    return b;
}

gs::Bitmap paintMark(int id) {
    gs::Bitmap b(14, 14);
    if (id == 0) {
        b.rect(2, 5, 10, 4, 1);
        b.rect(4, 3, 2, 8, 1);
        b.rect(8, 3, 2, 8, 1);
    } else if (id == 1) {
        b.rect(2, 2, 10, 10, 1);
        b.rect(5, 5, 4, 4, 2);
    } else if (id == 2) {
        b.ellipse(7, 4, 4, 3, 1);
        b.rect(6, 6, 2, 7, 1);
        b.rect(6, 10, 5, 2, 1);
    } else {
        b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
        b.rect(6, 6, 2, 2, 2);
    }
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(4, 14, 7)});
    setPal(vdp, PAL_DESK,
           {0, gs::rgb4(2, 1, 1), gs::rgb4(8, 5, 2), gs::rgb4(4, 3, 2), gs::rgb4(6, 5, 3), gs::rgb4(12, 9, 4),
            gs::rgb4(6, 3, 1), gs::rgb4(3, 2, 1), gs::rgb4(14, 13, 10)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(11, 10, 7), gs::rgb4(15, 14, 11), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(13, 10, 4), gs::rgb4(6, 4, 1), gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 5), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(2, 2, 5), gs::rgb4(10, 3, 2), gs::rgb4(3, 7, 4), gs::rgb4(7, 3, 9)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(9, 6, 3)});
    setPal(vdp, PAL_DECOY, {0, gs::rgb4(9, 8, 11)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap desk(gs::SCREEN_W, gs::SCREEN_H);
    paintDesk(desk);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, desk, PAL_DESK);
    vdp.A.enabled = false;
    vdp.A.clear();
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(1, 1, 1);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.setFogColor(gs::rgb4(1, 1, 1));

    art.tray = gs::uploadMipped(vdp, paintTray());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.hand = gs::uploadMipped(vdp, paintHand());
    art.slip = gs::uploadMipped(vdp, paintSlip());
    for (int i = 0; i < kBins; i++) art.mark[i] = gs::uploadMipped(vdp, paintMark(i));
    gs::Bitmap solid(8, 8);
    solid.rect(0, 0, 8, 8, 1);
    art.solid = gs::uploadMipped(vdp, solid);
}

}  // namespace drawertape
