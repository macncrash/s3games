#include "game/art.h"

#include <initializer_list>

namespace shelvetape {
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

void paintAisle(gs::Bitmap& b) {
    constexpr int WALL = 1, TRIM = 2, PLANK = 3, SHADOW = 4, DRAWER = 5, SLOT = 6, FLOOR = 7, LAMP = 8;
    b.rect(0, 0, 320, 224, WALL);
    b.rect(0, 176, 320, 48, FLOOR);
    b.rect(150, 28, 158, 140, TRIM);
    for (int i = 0; i < kRows; i++) {
        float y = 34.f + i * 32.f;
        b.rect(156, y, 146, 6, PLANK);
        b.rect(156, y + 6, 146, 22, SHADOW);
    }
    b.rect(12, 148, 120, 28, DRAWER);
    for (int i = 0; i < kTapeN; i++) b.rect(18.f + i * 38.f, 154, 32, 16, SLOT);
    b.rect(148, 20, 6, 156, PLANK);
    b.rect(304, 20, 6, 156, PLANK);
    b.ellipse(40, 22, 10, 6, LAMP);
    b.rect(36, 28, 8, 10, LAMP);
}

gs::Bitmap paintBook(int id) {
    gs::Bitmap b(16, 28);
    b.rect(1, 0, 14, 28, 1);
    b.rect(2, 1, 11, 26, 2);
    b.rect(3, 3, 3, 22, 3);
    if (id == 0) b.rect(8, 6, 4, 4, 4);
    else if (id == 1) b.ellipse(10, 10, 3, 3, 4);
    else if (id == 2) b.poly({{10, 5}, {13, 14}, {7, 14}}, 4);
    else b.rect(8, 8, 5, 8, 4);
    b.rect(4, 22, 8, 2, 4);
    return b;
}

gs::Bitmap paintCart() {
    gs::Bitmap b(36, 28);
    b.rect(2, 4, 32, 16, 1);
    b.rect(4, 6, 28, 12, 2);
    b.rect(0, 18, 36, 4, 3);
    b.ellipse(8, 24, 4, 4, 4);
    b.ellipse(28, 24, 4, 4, 4);
    return b;
}

gs::Bitmap paintHand() {
    gs::Bitmap b(14, 12);
    b.rect(2, 4, 10, 6, 1);
    b.rect(4, 0, 2, 6, 1);
    b.rect(8, 1, 2, 5, 1);
    b.rect(11, 3, 2, 4, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(4, 14, 7)});
    setPal(vdp, PAL_ROOM,
           {0, gs::rgb4(3, 3, 4), gs::rgb4(6, 5, 4), gs::rgb4(8, 5, 2), gs::rgb4(2, 2, 3), gs::rgb4(5, 3, 2),
            gs::rgb4(3, 2, 1), gs::rgb4(4, 4, 5), gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(14, 13, 10), gs::rgb4(8, 7, 5)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(9, 6, 3), gs::rgb4(5, 3, 2), gs::rgb4(12, 9, 4), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_FOLIO, {0, gs::rgb4(3, 8, 4), gs::rgb4(6, 13, 7), gs::rgb4(2, 4, 2), gs::rgb4(14, 14, 10)});
    setPal(vdp, PAL_ATLAS, {0, gs::rgb4(2, 4, 9), gs::rgb4(5, 8, 14), gs::rgb4(1, 2, 5), gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_PRIMER, {0, gs::rgb4(9, 3, 3), gs::rgb4(14, 6, 5), gs::rgb4(5, 2, 2), gs::rgb4(15, 13, 8)});
    setPal(vdp, PAL_LEDGER, {0, gs::rgb4(8, 6, 2), gs::rgb4(13, 10, 4), gs::rgb4(4, 3, 1), gs::rgb4(6, 5, 8)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 14, 12)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap aisle(gs::SCREEN_W, gs::SCREEN_H);
    paintAisle(aisle);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, aisle, PAL_ROOM);
    vdp.A.enabled = false;
    vdp.A.clear();
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(2, 2, 3);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.setFogColor(gs::rgb4(2, 2, 3));

    for (int i = 0; i < kRows; i++) art.book[i] = gs::uploadMipped(vdp, paintBook(i));
    art.cart = gs::uploadMipped(vdp, paintCart());
    art.hand = gs::uploadMipped(vdp, paintHand());
    gs::Bitmap solid(8, 8);
    solid.rect(0, 0, 8, 8, 1);
    art.solid = gs::uploadMipped(vdp, solid);
}

}  // namespace shelvetape
