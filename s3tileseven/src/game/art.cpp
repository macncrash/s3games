#include "game/art.h"

#include <initializer_list>

namespace tileseven {
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

void paintWall(gs::Bitmap& b) {
    constexpr int PLASTER = 1, SHADE = 2, BEAM = 3, EDGE = 4, MORTAR = 5;
    b.rect(0, 0, 320, 224, PLASTER);
    for (int y = 0; y < 224; y += 16)
        b.rect(0, y, 320, 1, SHADE);
    b.rect(0, 0, 320, 22, EDGE);
    b.rect(0, 200, 320, 24, EDGE);
    b.rect(8, 36, 304, 10, BEAM);
    b.rect(8, 168, 304, 10, BEAM);
    for (int i = 0; i < 7; i++) {
        float x = 28.f + i * 40.f;
        b.rect(x, 52, 28, 22, MORTAR);
        b.rect(x, 140, 28, 22, MORTAR);
    }
}

gs::Bitmap paintTile() {
    gs::Bitmap b(40, 40);
    b.rect(1, 1, 38, 38, 2);
    b.rect(4, 4, 32, 32, 1);
    b.rect(6, 6, 28, 4, 3);
    return b;
}

gs::Bitmap paintPip() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

gs::Bitmap paintCrack() {
    gs::Bitmap b(24, 24);
    b.line(4, 4, 12, 12, 1, 2);
    b.line(12, 12, 20, 6, 1, 2);
    b.line(12, 12, 8, 20, 1, 2);
    return b;
}

gs::Bitmap paintNiche() {
    gs::Bitmap b(26, 18);
    b.rect(0, 0, 26, 18, 1);
    b.rect(2, 2, 22, 14, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(4, 14, 7)});
    setPal(vdp, PAL_WALL,
           {0, gs::rgb4(12, 10, 8), gs::rgb4(8, 7, 6), gs::rgb4(7, 4, 2), gs::rgb4(3, 2, 2), gs::rgb4(5, 4, 4)});
    setPal(vdp, PAL_CLAY, {0, gs::rgb4(13, 8, 4), gs::rgb4(8, 4, 2), gs::rgb4(15, 12, 8)});
    setPal(vdp, PAL_GLAZE, {0, gs::rgb4(4, 10, 14), gs::rgb4(2, 5, 8), gs::rgb4(12, 14, 15)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_IVORY, {0, gs::rgb4(15, 14, 12), gs::rgb4(9, 8, 7)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap wall(gs::SCREEN_W, gs::SCREEN_H);
    paintWall(wall);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, wall, PAL_WALL);
    vdp.A.enabled = false;
    vdp.A.clear();
    vdp.B.enabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(2, 2, 3);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.setFogColor(gs::rgb4(2, 2, 3));

    art.tile = gs::uploadMipped(vdp, paintTile());
    art.pip = gs::uploadMipped(vdp, paintPip());
    art.crack = gs::uploadMipped(vdp, paintCrack());
    art.niche = gs::uploadMipped(vdp, paintNiche());
    gs::Bitmap solid(8, 8);
    solid.rect(0, 0, 8, 8, 1);
    art.solid = gs::uploadMipped(vdp, solid);
}

}  // namespace tileseven
