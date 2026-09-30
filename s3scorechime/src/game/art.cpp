#include "game/art.h"

namespace scorechime {
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(15, 13, 4), gs::rgb4(9, 7, 2), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 7, 6)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(13, 3, 2)});
    setPal(vdp, PAL_CHALK, {0, gs::rgb4(15, 15, 13), gs::rgb4(11, 11, 9), gs::rgb4(4, 5, 4)});
    setPal(vdp, PAL_BOARD, {0, gs::rgb4(3, 6, 3), gs::rgb4(8, 10, 6), gs::rgb4(2, 3, 2), gs::rgb4(12, 13, 8),
                            gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 4, 3), gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_PITCH, {0, gs::rgb4(2, 8, 3), gs::rgb4(14, 14, 12), gs::rgb4(6, 4, 2)});

    {
        gs::Bitmap b(118, 96);
        b.rect(4, 8, 110, 80, 1);
        b.rect(8, 12, 102, 72, 3);
        b.rect(0, 4, 118, 6, 2);
        b.rect(0, 86, 118, 6, 2);
        b.rect(10, 18, 46, 28, 4);
        b.rect(62, 18, 46, 28, 4);
        for (int i = 0; i < 4; i++) b.rect(14, 52 + i * 8, 90, 2, 5);
        art.board = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 28);
        b.line(4, 24, 11, 4, 1, 3);
        b.line(11, 4, 18, 24, 1, 3);
        b.line(7, 16, 15, 16, 2, 2);
        art.tick = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 22);
        b.ellipse(11, 11, 9, 9, 1);
        b.ellipse(8, 8, 3, 2, 3);
        b.rect(10, 4, 2, 14, 2);
        b.rect(4, 10, 14, 2, 2);
        art.ball = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 70);
        b.rect(3, 0, 4, 70, 1);
        b.rect(0, 0, 10, 4, 2);
        b.rect(1, 62, 8, 6, 3);
        art.line = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 40);
        b.ellipse(18, 16, 14, 12, 1);
        b.rect(6, 16, 24, 10, 1);
        b.rect(16, 4, 4, 6, 2);
        b.ellipse(18, 8, 3, 2, 3);
        b.poly({{18, 14}, {14, 26}, {22, 26}}, 2);
        b.ellipse(18, 28, 5, 3, 3);
        b.rect(8, 30, 20, 4, 2);
        art.bell = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace scorechime
