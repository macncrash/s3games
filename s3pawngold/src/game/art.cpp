#include "game/art.h"

#include <initializer_list>

namespace pawngold {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t edge) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 2, edge);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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

gs::Bitmap boardArt() {
    gs::Bitmap b(224, 96);
    b.rect(0, 0, 224, 96, 1);
    for (int f = 0; f < 7; f++) {
        int ink = (f & 1) ? 2 : 3;
        b.rect(8 + f * 30, 8, 28, 80, ink);
        b.rect(10 + f * 30, 72, 24, 10, 4);
    }
    return b;
}

gs::Bitmap pawnArt() {
    gs::Bitmap b(20, 36);
    b.rect(2, 28, 16, 6, 1);
    b.rect(5, 24, 10, 5, 2);
    b.rect(8, 12, 4, 14, 2);
    b.ellipse(10.f, 9.f, 6.f, 6.f, 3);
    b.ellipse(8.f, 7.f, 2.f, 1.6f, 4);
    return b;
}

gs::Bitmap crownArt() {
    gs::Bitmap b(16, 10);
    b.poly({{1.f, 9.f}, {2.f, 3.f}, {5.f, 7.f}, {8.f, 1.f}, {11.f, 7.f}, {14.f, 3.f}, {15.f, 9.f}}, 1);
    b.rect(2, 8, 12, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_BOARD,
           {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 8, 4), gs::rgb4(12, 10, 6), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(8, 5, 1), gs::rgb4(14, 10, 2), gs::rgb4(15, 14, 4), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_IVORY, {0, gs::rgb4(7, 6, 5), gs::rgb4(12, 11, 9), gs::rgb4(15, 14, 12), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 12, 3), gs::rgb4(8, 3, 2)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 11), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 12, 3), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 10), gs::rgb4(1, 3, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 6, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 15, 12), gs::rgb4(1, 2, 2));

    loadFont(vdp, art);
    art.board = gs::uploadImage(vdp, boardArt());
    art.pawn = gs::uploadImage(vdp, pawnArt());
    art.crown = gs::uploadImage(vdp, crownArt());
}

}  // namespace pawngold
