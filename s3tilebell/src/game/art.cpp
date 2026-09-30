#include "game/art.h"

namespace tilebell {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, int* font) {
    gs::TileAlloc tiles(vdp, 1);
    for (int c = 0; c < 96; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c + 32));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + (x + 1)] = 1;
        font[c] = tiles.shared(px);
    }
}

void body(gs::Bitmap& b) {
    b.rect(1, 1, 30, 30, 1);
    b.rect(2, 2, 28, 28, 4);
    b.rect(3, 3, 26, 26, 1);
    b.line(2, 2, 28, 4, 5, 1.f);
}

void faceCross(gs::Bitmap& b) {
    body(b);
    b.rect(14, 6, 4, 20, 3);
    b.rect(6, 14, 20, 4, 3);
}

void faceRing(gs::Bitmap& b) {
    body(b);
    b.ellipse(16, 16, 9, 9, 3);
    b.ellipse(16, 16, 5, 5, 1);
    b.ellipse(16, 16, 2, 2, 5);
}

void faceBars(gs::Bitmap& b) {
    body(b);
    b.rect(6, 7, 20, 4, 3);
    b.rect(6, 14, 20, 4, 3);
    b.rect(6, 21, 20, 4, 5);
}

void faceDiamond(gs::Bitmap& b) {
    body(b);
    b.poly({{16, 6}, {26, 16}, {16, 26}, {6, 16}}, 3);
    b.poly({{16, 10}, {22, 16}, {16, 22}, {10, 16}}, 5);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_TILE, {0, gs::rgb4(11, 5, 2), gs::rgb4(4, 3, 2), gs::rgb4(2, 3, 8), gs::rgb4(13, 10, 7),
                           gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_GHOST, {0, gs::rgb4(5, 3, 2), gs::rgb4(3, 2, 2), gs::rgb4(3, 3, 5), gs::rgb4(6, 5, 4),
                            gs::rgb4(7, 6, 3)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(8, 5, 1), gs::rgb4(14, 10, 2), gs::rgb4(15, 14, 6), gs::rgb4(5, 3, 1)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(5, 3, 1), gs::rgb4(9, 5, 2), gs::rgb4(12, 8, 3), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(14, 12, 4), gs::rgb4(6, 6, 7), gs::rgb4(14, 3, 3)});

    {
        gs::Bitmap b(32, 32);
        faceRing(b);
        art.tile[0] = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(32, 32);
        faceDiamond(b);
        art.tile[1] = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(32, 32);
        faceCross(b);
        art.tile[2] = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(32, 32);
        faceBars(b);
        art.tile[3] = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(40, 40);
        b.rect(0, 0, 40, 40, 2);
        b.rect(3, 3, 34, 34, 1);
        b.rect(6, 6, 28, 28, 0);
        b.rect(6, 34, 28, 3, 3);
        art.socket = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(44, 44);
        b.rect(0, 0, 44, 3, 1);
        b.rect(0, 41, 44, 3, 1);
        b.rect(0, 0, 3, 44, 1);
        b.rect(41, 0, 3, 44, 1);
        art.cursor = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 28);
        b.poly({{18, 1}, {6, 8}, {30, 8}}, 3);
        b.poly({{6, 8}, {3, 20}, {33, 20}, {30, 8}}, 2);
        b.rect(8, 10, 20, 5, 3);
        b.ellipse(18, 22, 4, 3, 1);
        b.rect(16, 0, 4, 4, 1);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(70, 10);
        b.rect(0, 3, 70, 4, 2);
        b.rect(30, 0, 10, 10, 3);
        art.yoke = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 10);
        b.ellipse(5, 5, 4, 4, 1);
        b.ellipse(5, 5, 2, 2, 2);
        art.lamp = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace tilebell
