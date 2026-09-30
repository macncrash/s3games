#include "game/art.h"

#include <initializer_list>

namespace cuetape {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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
    }
}

gs::Bitmap feltTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.set(1, 2, 3);
    b.set(5, 6, 1);
    return b;
}

gs::Bitmap railTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.rect(0, 0, 8, 2, 3);
    b.rect(0, 6, 8, 2, 1);
    return b;
}

gs::Bitmap tableArt() {
    gs::Bitmap b(248, 150);
    b.rect(0, 0, 248, 150, 1);
    b.rect(8, 8, 232, 134, 2);
    b.rect(16, 16, 216, 118, 3);
    b.ellipse(28, 28, 10, 10, 4);
    b.ellipse(124, 22, 10, 10, 4);
    b.ellipse(220, 28, 10, 10, 4);
    b.ellipse(28, 122, 10, 10, 4);
    b.ellipse(124, 128, 10, 10, 4);
    b.ellipse(220, 122, 10, 10, 4);
    b.rect(118, 40, 2, 70, 5);
    return b;
}

gs::Bitmap cueArt() {
    gs::Bitmap b(96, 8);
    b.rect(0, 2, 72, 4, 2);
    b.rect(0, 3, 60, 2, 3);
    b.rect(68, 1, 14, 6, 4);
    b.rect(82, 2, 8, 4, 1);
    b.rect(90, 3, 6, 2, 5);
    return b;
}

gs::Bitmap ballArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 2);
    b.ellipse(6, 6, 2, 2, 3);
    return b;
}

gs::Bitmap objectArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 2);
    b.rect(3, 6, 10, 4, 3);
    b.ellipse(6, 6, 2, 2, 4);
    return b;
}

gs::Bitmap chalkArt() {
    gs::Bitmap b(14, 12);
    b.rect(1, 1, 12, 10, 2);
    b.rect(3, 3, 8, 4, 3);
    b.rect(2, 9, 10, 2, 1);
    return b;
}

gs::Bitmap pocketArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 1);
    b.ellipse(11, 11, 6, 6, 2);
    return b;
}

gs::Bitmap slipArt() {
    gs::Bitmap b(18, 12);
    b.rect(0, 0, 18, 12, 1);
    b.rect(2, 2, 14, 2, 2);
    b.rect(2, 6, 8, 2, 2);
    return b;
}

gs::Bitmap slotArt() {
    gs::Bitmap b(20, 8);
    b.rect(0, 2, 20, 4, 1);
    b.rect(2, 3, 16, 2, 2);
    return b;
}

gs::Bitmap drawerArt() {
    gs::Bitmap b(40, 28);
    b.rect(0, 0, 40, 28, 2);
    b.rect(3, 4, 34, 18, 3);
    b.rect(16, 10, 8, 4, 1);
    b.rect(2, 24, 36, 3, 4);
    return b;
}

gs::Bitmap playerArt() {
    gs::Bitmap b(36, 56);
    b.ellipse(18, 8, 6, 6, 2);
    b.rect(12, 15, 12, 16, 3);
    b.rect(4, 16, 8, 4, 4);
    b.rect(24, 17, 8, 4, 3);
    b.rect(12, 31, 4, 16, 1);
    b.rect(18, 31, 4, 16, 1);
    b.rect(10, 46, 7, 5, 4);
    b.rect(18, 46, 7, 5, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 15, 12), gs::rgb4(3, 5, 3), gs::rgb4(1, 2, 1)});
    vdp.setColor(PAL_HUD * 16 + 15, gs::rgb4(1, 1, 1));
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(14, 13, 9), gs::rgb4(8, 7, 4), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_CLOTH, {0, gs::rgb4(0, 3, 1), gs::rgb4(1, 6, 2), gs::rgb4(2, 10, 4), gs::rgb4(0, 0, 0),
                            gs::rgb4(6, 5, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(4, 2, 1), gs::rgb4(7, 4, 1), gs::rgb4(10, 6, 2), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_CUE, {0, gs::rgb4(6, 3, 1), gs::rgb4(11, 7, 3), gs::rgb4(14, 10, 5), gs::rgb4(8, 4, 1),
                          gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(8, 8, 8), gs::rgb4(15, 15, 14), gs::rgb4(11, 11, 10)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(8, 5, 1), gs::rgb4(14, 11, 2), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_PLAYER, {0, gs::rgb4(2, 2, 2), gs::rgb4(11, 8, 6), gs::rgb4(2, 3, 7), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_CHALK, {0, gs::rgb4(1, 2, 6), gs::rgb4(2, 5, 12), gs::rgb4(5, 9, 15)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(6, 1, 1), gs::rgb4(12, 3, 2), gs::rgb4(15, 8, 4)});
    setPal(vdp, PAL_SLOT, {0, gs::rgb4(5, 4, 2), gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(0, 0, 0)});

    gs::TileAlloc tiles(vdp, 1);
    art.felt = tiles.shared(feltTile().px.data());
    art.rail = tiles.shared(railTile().px.data());

    art.table = gs::uploadMipped(vdp, tableArt());
    art.cue = gs::uploadMipped(vdp, cueArt());
    art.ball = gs::uploadMipped(vdp, ballArt());
    art.object = gs::uploadMipped(vdp, objectArt());
    art.chalk = gs::uploadMipped(vdp, chalkArt());
    art.pocket = gs::uploadMipped(vdp, pocketArt());
    art.slip = gs::uploadMipped(vdp, slipArt());
    art.slot = gs::uploadMipped(vdp, slotArt());
    art.drawer = gs::uploadMipped(vdp, drawerArt());
    art.player = gs::uploadMipped(vdp, playerArt());
    loadFont(vdp, tiles, art);
}

}  // namespace cuetape
