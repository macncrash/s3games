#include "game/art.h"

#include <initializer_list>

namespace pawnchime {
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
    gs::Bitmap b(248, 52);
    b.rect(0, 0, 248, 52, 1);
    for (int f = 0; f < 8; f++) {
        int ink = (f & 1) ? 3 : 2;
        b.rect(4 + f * 30, 6, 28, 40, ink);
        b.rect(6 + f * 30, 38, 24, 4, 4);
    }
    return b;
}

gs::Bitmap pawnArt() {
    gs::Bitmap b(18, 32);
    b.rect(2, 26, 14, 5, 1);
    b.rect(5, 21, 8, 6, 2);
    b.rect(7, 11, 4, 12, 2);
    b.ellipse(9.f, 8.f, 5.5f, 5.5f, 3);
    b.ellipse(7.f, 6.f, 2.f, 1.4f, 4);
    return b;
}

gs::Bitmap foeArt() {
    gs::Bitmap b(18, 32);
    b.rect(2, 26, 14, 5, 1);
    b.rect(5, 21, 8, 6, 2);
    b.rect(7, 11, 4, 12, 2);
    b.ellipse(9.f, 8.f, 5.5f, 5.5f, 3);
    b.rect(6, 7, 6, 2, 4);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14.f, 16.f, 11.f, 9.f, 1);
    b.ellipse(14.f, 14.f, 8.f, 6.f, 2);
    b.rect(12, 3, 4, 5, 3);
    b.rect(6, 22, 16, 3, 3);
    b.ellipse(14.f, 14.f, 2.f, 2.f, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_BOARD,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 8, 5), gs::rgb4(10, 12, 8), gs::rgb4(3, 4, 3)});
    setPal(vdp, PAL_PAWN, {0, gs::rgb4(6, 4, 2), gs::rgb4(12, 9, 4), gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_FOE, {0, gs::rgb4(3, 2, 2), gs::rgb4(7, 3, 3), gs::rgb4(12, 5, 4), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(8, 7, 2), gs::rgb4(14, 12, 4), gs::rgb4(15, 15, 10), gs::rgb4(6, 5, 2)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 11), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 4), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 10), gs::rgb4(1, 3, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 6, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 15, 13), gs::rgb4(1, 2, 2));

    loadFont(vdp, art);
    art.board = gs::uploadImage(vdp, boardArt());
    art.pawn = gs::uploadImage(vdp, pawnArt());
    art.foe = gs::uploadImage(vdp, foeArt());
    art.bell = gs::uploadImage(vdp, bellArt());
}

}  // namespace pawnchime
