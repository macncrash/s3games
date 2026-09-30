#include "game/art.h"

#include <initializer_list>

namespace pawntape {
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

void paintFelt(gs::Bitmap& b) {
    constexpr int EDGE = 1, FELT = 2, LIGHT = 3, DARK = 4, POCKET = 5, WOOD = 6, INK = 7;
    b.rect(0, 0, 320, 224, EDGE);
    b.rect(0, 48, 320, 112, FELT);
    for (int f = 0; f < 10; f++) {
        int c = (f & 1) ? DARK : LIGHT;
        b.rect(8 + f * 30, 64, 28, 72, c);
    }
    b.rect(140, 60, 48, 80, POCKET);
    b.rect(140, 60, 48, 3, INK);
    b.rect(140, 137, 48, 3, INK);
    b.rect(0, 160, 320, 64, WOOD);
    b.rect(24, 172, 80, 36, EDGE);
    b.rect(120, 172, 80, 36, EDGE);
    b.rect(216, 172, 80, 36, EDGE);
    b.rect(28, 176, 72, 28, WOOD);
    b.rect(124, 176, 72, 28, WOOD);
    b.rect(220, 176, 72, 28, WOOD);
}

gs::Bitmap paintPawn(int body, int trim, bool crown) {
    gs::Bitmap b(24, 40);
    b.ellipse(12, 34, 9, 4, body);
    b.rect(8, 30, 8, 4, body);
    b.ellipse(12, 26, 7, 5, body);
    b.rect(10, 16, 4, 12, body);
    b.ellipse(12, 14, 6, 4, trim);
    b.ellipse(12, 8, 5, 5, body);
    b.ellipse(10, 6, 2, 1, trim);
    if (crown) {
        b.poly({{6, 8}, {9, 2}, {12, 7}, {15, 2}, {18, 8}}, trim);
        b.rect(8, 8, 8, 2, trim);
    }
    return b;
}

gs::Bitmap paintGhost() {
    gs::Bitmap b(24, 40);
    b.ellipse(12, 34, 9, 4, 1);
    b.ellipse(12, 26, 7, 5, 1);
    b.rect(10, 16, 4, 12, 1);
    b.ellipse(12, 14, 6, 4, 2);
    b.ellipse(12, 8, 5, 5, 1);
    b.rect(10, 7, 2, 2, 3);
    b.rect(14, 7, 2, 2, 3);
    return b;
}

gs::Bitmap paintSlip() {
    gs::Bitmap b(28, 16);
    b.rect(1, 1, 26, 14, 1);
    b.rect(3, 4, 16, 2, 2);
    b.rect(3, 8, 10, 2, 2);
    b.rect(22, 3, 3, 10, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(5, 15, 8)});
    setPal(vdp, PAL_FELT,
           {0, gs::rgb4(2, 3, 2), gs::rgb4(3, 8, 4), gs::rgb4(10, 12, 8), gs::rgb4(5, 7, 4), gs::rgb4(13, 11, 4),
            gs::rgb4(7, 4, 2), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_IVORY, {0, gs::rgb4(15, 14, 12), gs::rgb4(8, 7, 5)});
    setPal(vdp, PAL_EBONY, {0, gs::rgb4(2, 2, 3), gs::rgb4(10, 9, 8)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_GHOST, {0, gs::rgb4(9, 10, 11), gs::rgb4(5, 6, 7), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(14, 13, 9), gs::rgb4(6, 5, 3), gs::rgb4(12, 3, 3)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap felt(gs::SCREEN_W, gs::SCREEN_H);
    paintFelt(felt);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, felt, PAL_FELT);
    vdp.A.enabled = false;
    vdp.A.clear();
    vdp.B.enabled = true;
    vdp.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(1, 2, 1);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.setFogColor(gs::rgb4(1, 2, 1));

    art.ivory = gs::uploadMipped(vdp, paintPawn(1, 2, false));
    art.ebony = gs::uploadMipped(vdp, paintPawn(1, 2, false));
    art.crown = gs::uploadMipped(vdp, paintPawn(1, 2, true));
    art.ghost = gs::uploadMipped(vdp, paintGhost());
    art.slip = gs::uploadMipped(vdp, paintSlip());
    gs::Bitmap dot(4, 4);
    dot.rect(0, 0, 4, 4, 1);
    art.solid = gs::uploadMipped(vdp, dot);
}

}  // namespace pawntape
