#include "game/art.h"

namespace cranelock {
namespace {

void textPal(gs::VDP& v, int pal, uint16_t ink) {
    v.setColor(pal * 16 + 1, ink);
    v.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void paintPals(gs::VDP& vdp) {
    textPal(vdp, PAL_HUD, gs::rgb4(14, 15, 15));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 8));

    vdp.setColor(PAL_HULL * 16 + 1, gs::rgb4(12, 13, 14));
    vdp.setColor(PAL_HULL * 16 + 2, gs::rgb4(6, 7, 8));
    vdp.setColor(PAL_HULL * 16 + 3, gs::rgb4(15, 12, 3));
    vdp.setColor(PAL_HULL * 16 + 4, gs::rgb4(8, 12, 15));
    vdp.setColor(PAL_HULL * 16 + 5, gs::rgb4(3, 3, 4));
    vdp.setColor(PAL_HULL * 16 + 6, gs::rgb4(14, 6, 3));

    vdp.setColor(PAL_HOOK * 16 + 1, gs::rgb4(15, 13, 4));
    vdp.setColor(PAL_HOOK * 16 + 2, gs::rgb4(8, 6, 2));
    vdp.setColor(PAL_HOOK * 16 + 3, gs::rgb4(4, 4, 5));

    vdp.setColor(PAL_GATE * 16 + 1, gs::rgb4(12, 8, 4));
    vdp.setColor(PAL_GATE * 16 + 2, gs::rgb4(6, 4, 2));
    vdp.setColor(PAL_GATE * 16 + 3, gs::rgb4(15, 14, 8));
    vdp.setColor(PAL_GATE * 16 + 4, gs::rgb4(2, 2, 2));

    vdp.setColor(PAL_STONE * 16 + 1, gs::rgb4(8, 8, 7));
    vdp.setColor(PAL_STONE * 16 + 2, gs::rgb4(5, 5, 4));
    vdp.setColor(PAL_STONE * 16 + 3, gs::rgb4(11, 11, 9));
    vdp.setColor(PAL_STONE * 16 + 4, gs::rgb4(3, 5, 3));

    vdp.setColor(PAL_CREW * 16 + 1, gs::rgb4(15, 6, 4));
    vdp.setColor(PAL_CREW * 16 + 2, gs::rgb4(8, 2, 2));
    vdp.setColor(PAL_CREW * 16 + 3, gs::rgb4(15, 14, 10));

    vdp.setColor(PAL_MARK * 16 + 1, gs::rgb4(15, 14, 6));
    vdp.setColor(PAL_MARK * 16 + 2, gs::rgb4(4, 10, 6));
    vdp.setColor(PAL_MARK * 16 + 3, gs::rgb4(10, 14, 8));
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
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
        art.font[c - 32] = t;
        gs::Bitmap b(8, 8);
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++)
                if (px[y * 8 + x] == 1) b.set(x, y, 1);
        art.glyph[c - 32] = gs::uploadMipped(vdp, b);
    }
}

gs::Bitmap hullArt() {
    gs::Bitmap b(40, 28);
    b.rect(6, 2, 28, 24, 2);
    b.rect(8, 3, 24, 22, 1);
    b.rect(10, 5, 20, 8, 4);
    b.rect(16, 1, 8, 6, 5);
    b.rect(18, 2, 4, 3, 3);
    b.rect(12, 16, 6, 6, 6);
    b.rect(22, 16, 6, 6, 6);
    b.rect(18, 20, 4, 4, 3);
    b.rect(4, 8, 4, 12, 2);
    b.rect(32, 8, 4, 12, 2);
    return b;
}

gs::Bitmap hookArt() {
    gs::Bitmap b(14, 14);
    b.rect(2, 1, 10, 10, 2);
    b.rect(4, 2, 6, 7, 1);
    b.rect(6, 8, 2, 5, 3);
    b.rect(4, 12, 6, 2, 3);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(6, 6);
    b.rect(1, 1, 4, 4, 1);
    b.set(2, 2, 2);
    b.set(3, 2, 2);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(12, 16);
    b.rect(0, 0, 12, 16, 2);
    b.rect(1, 1, 10, 14, 1);
    b.rect(2, 2, 3, 12, 3);
    for (int y = 3; y < 14; y += 4) b.rect(1, y, 10, 1, 4);
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 2);
    b.rect(1, 1, 7, 6, 1);
    b.rect(9, 2, 6, 5, 3);
    b.rect(2, 9, 6, 6, 3);
    b.rect(9, 9, 6, 6, 1);
    b.rect(0, 15, 16, 1, 4);
    return b;
}

gs::Bitmap berthArt() {
    gs::Bitmap b(48, 18);
    b.rect(0, 0, 48, 18, 2);
    b.rect(2, 2, 44, 14, 3);
    b.rect(6, 6, 36, 3, 1);
    b.rect(20, 10, 8, 4, 1);
    return b;
}

gs::Bitmap crewArt() {
    gs::Bitmap b(16, 12);
    b.rect(1, 2, 14, 8, 2);
    b.rect(2, 3, 12, 6, 1);
    b.rect(6, 0, 4, 4, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    paintPals(vdp);
    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.hull = gs::uploadMipped(vdp, hullArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.berth = gs::uploadMipped(vdp, berthArt());
    art.crew = gs::uploadMipped(vdp, crewArt());
}

}  // namespace cranelock
