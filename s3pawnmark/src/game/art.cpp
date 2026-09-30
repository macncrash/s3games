#include "game/art.h"

#include <string>

namespace pawnmark {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

void ink(gs::VDP& vdp, int pal, uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
    uint16_t p[16] = {};
    p[1] = gs::rgb4(15, 15, 14);
    p[2] = a;
    p[3] = b;
    p[4] = c;
    p[5] = d;
    p[15] = gs::rgb4(1, 1, 2);
    setPal(vdp, pal, p);
}

gs::Bitmap pawnArt() {
    gs::Bitmap b(16, 28);
    b.ellipse(8, 5, 3.2f, 3.2f, 2);
    b.ellipse(8, 4.2f, 1.2f, 1.0f, 1);
    b.rect(6, 8, 4, 3, 3);
    b.ellipse(8, 14, 4.2f, 5.2f, 3);
    b.ellipse(8, 13, 2.2f, 3.0f, 2);
    b.rect(4, 19, 8, 3, 4);
    b.rect(3, 22, 10, 3, 5);
    b.rect(2, 25, 12, 2, 4);
    return b;
}

gs::Bitmap foeArt() {
    gs::Bitmap b(16, 28);
    b.ellipse(8, 6, 3.4f, 3.2f, 4);
    b.rect(5, 9, 6, 3, 5);
    b.ellipse(8, 15, 4.4f, 5.0f, 4);
    b.rect(3, 20, 10, 3, 5);
    b.rect(2, 23, 12, 3, 3);
    return b;
}

gs::Bitmap squareArt() {
    gs::Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 3);
    b.rect(1, 1, 14, 14, 2);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7.5f, 7.5f, 4);
    b.ellipse(9, 9, 5.2f, 5.2f, 3);
    b.rect(8, 4, 2, 10, 1);
    b.rect(5, 7, 8, 2, 1);
    return b;
}

gs::Bitmap crownArt() {
    gs::Bitmap b(20, 12);
    b.rect(1, 7, 18, 4, 3);
    b.rect(2, 6, 16, 2, 2);
    b.rect(2, 2, 3, 6, 4);
    b.rect(8, 1, 4, 7, 4);
    b.rect(15, 2, 3, 6, 4);
    b.ellipse(3, 2, 1.4f, 1.4f, 1);
    b.ellipse(10, 1, 1.4f, 1.4f, 1);
    b.ellipse(16, 2, 1.4f, 1.4f, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art, gs::TileAlloc& tiles) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        if (g) {
            for (int y = 0; y < 7; y++) {
                for (int x = 0; x < 5; x++) {
                    if (!g[y * 5 + x]) continue;
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
        art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    ink(vdp, PAL_INK, gs::rgb4(14, 14, 12), gs::rgb4(8, 8, 7), gs::rgb4(3, 3, 3), gs::rgb4(1, 1, 1));
    ink(vdp, PAL_WOOD, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(4, 2, 1), gs::rgb4(2, 1, 0));
    ink(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(12, 8, 2), gs::rgb4(7, 4, 1), gs::rgb4(3, 2, 1));
    ink(vdp, PAL_IVORY, gs::rgb4(15, 14, 11), gs::rgb4(12, 11, 8), gs::rgb4(7, 6, 4), gs::rgb4(3, 2, 1));
    ink(vdp, PAL_NIGHT, gs::rgb4(6, 6, 8), gs::rgb4(3, 3, 5), gs::rgb4(1, 1, 2), gs::rgb4(0, 0, 1));
    ink(vdp, PAL_OK, gs::rgb4(8, 15, 8), gs::rgb4(3, 10, 4), gs::rgb4(1, 5, 2), gs::rgb4(1, 2, 1));
    ink(vdp, PAL_BAD, gs::rgb4(15, 5, 4), gs::rgb4(10, 2, 2), gs::rgb4(5, 1, 1), gs::rgb4(2, 0, 0));
    ink(vdp, PAL_FELT, gs::rgb4(4, 12, 6), gs::rgb4(2, 7, 3), gs::rgb4(1, 4, 2), gs::rgb4(0, 2, 1));
    vdp.setFogColor(gs::rgb4(1, 2, 1));

    art.pawn = gs::uploadMipped(vdp, pawnArt());
    art.foe = gs::uploadMipped(vdp, foeArt());
    art.square = gs::uploadMipped(vdp, squareArt());
    art.mark = gs::uploadMipped(vdp, markArt());
    art.crown = gs::uploadMipped(vdp, crownArt());

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);
}

}  // namespace pawnmark
