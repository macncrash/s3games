#include "game/art.h"

#include <initializer_list>

namespace drumseven {
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

void paintStage(gs::Bitmap& b) {
    constexpr int SKY = 1, CURT = 2, WOOD = 3, PLANK = 4, SHADOW = 5, RIM = 6;
    b.rect(0, 0, 320, 224, SKY);
    b.rect(0, 0, 320, 78, CURT);
    for (int x = 8; x < 320; x += 28) b.rect(x, 0, 4, 78, SHADOW);
    b.rect(0, 70, 320, 10, RIM);
    for (int y = 88; y < 224; y += 12) {
        int c = ((y / 12) & 1) ? WOOD : PLANK;
        b.rect(0, y, 320, 12, c);
    }
    b.rect(24, 168, 272, 6, SHADOW);
}

gs::Bitmap paintDrum() {
    gs::Bitmap b(48, 64);
    constexpr int BODY = 1, MID = 2, HI = 3, HEAD = 4, RIM = 5, HOOP = 6;
    b.ellipse(24, 50, 18, 7, BODY);
    b.rect(6, 22, 36, 30, BODY);
    b.rect(10, 22, 8, 28, MID);
    b.rect(30, 24, 4, 24, HI);
    b.ellipse(24, 22, 18, 8, HEAD);
    b.ellipse(24, 20, 16, 5, HOOP);
    b.ellipse(24, 18, 18, 7, RIM);
    b.rect(8, 28, 3, 3, RIM);
    b.rect(37, 28, 3, 3, RIM);
    b.rect(8, 40, 3, 3, RIM);
    b.rect(37, 40, 3, 3, RIM);
    return b;
}

gs::Bitmap paintMallet() {
    gs::Bitmap b(14, 40);
    b.rect(6, 10, 3, 26, 1);
    b.ellipse(7, 8, 6, 6, 2);
    b.ellipse(5, 6, 2, 2, 3);
    return b;
}

gs::Bitmap paintGlow() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 1);
    b.ellipse(8, 8, 3, 3, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(4, 15, 7)});
    setPal(vdp, PAL_STAGE,
           {0, gs::rgb4(3, 2, 6), gs::rgb4(8, 1, 3), gs::rgb4(7, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(2, 1, 1),
            gs::rgb4(12, 9, 4)});
    setPal(vdp, PAL_DRUM,
           {0, gs::rgb4(10, 2, 2), gs::rgb4(13, 3, 3), gs::rgb4(15, 6, 5), gs::rgb4(14, 12, 8), gs::rgb4(12, 9, 3),
            gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_GOLD,
           {0, gs::rgb4(10, 7, 1), gs::rgb4(14, 10, 2), gs::rgb4(15, 14, 6), gs::rgb4(14, 12, 8), gs::rgb4(13, 10, 3),
            gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_BLUE,
           {0, gs::rgb4(1, 3, 10), gs::rgb4(2, 5, 13), gs::rgb4(6, 9, 15), gs::rgb4(14, 12, 8), gs::rgb4(8, 8, 10),
            gs::rgb4(3, 3, 5)});
    setPal(vdp, PAL_MALLET, {0, gs::rgb4(8, 5, 2), gs::rgb4(12, 3, 3), gs::rgb4(15, 10, 8)});
    setPal(vdp, PAL_GLOW, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap stage(gs::SCREEN_W, gs::SCREEN_H);
    paintStage(stage);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, stage, PAL_STAGE);
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

    art.drum = gs::uploadMipped(vdp, paintDrum());
    art.mallet = gs::uploadMipped(vdp, paintMallet());
    art.glow = gs::uploadMipped(vdp, paintGlow());
}

}  // namespace drumseven
