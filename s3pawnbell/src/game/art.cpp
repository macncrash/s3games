#include "game/art.h"

#include <initializer_list>

namespace pawnbell {
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
    vdp.setColor(pal * 16 + 15, edge);
}

void tileSolid(gs::VDP& vdp, int index, uint8_t c, uint8_t edge) {
    uint8_t px[64];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            bool rim = x == 0 || y == 0;
            px[y * 8 + x] = rim ? edge : c;
        }
    }
    vdp.loadTile(index, px);
}

gs::Bitmap pawnArt() {
    gs::Bitmap b(16, 22);
    b.ellipse(8.f, 5.f, 3.2f, 3.2f, 1);
    b.rect(7, 7, 2, 3, 2);
    b.poly({{4, 11}, {12, 11}, {14, 16}, {2, 16}}, 1);
    b.ellipse(8.f, 17.5f, 6.2f, 3.1f, 1);
    b.rect(3, 19, 10, 2, 3);
    b.rect(6, 4, 1, 1, 4);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(28, 36);
    b.rect(13, 0, 2, 6, 3);
    b.poly({{6, 12}, {22, 12}, {25, 26}, {3, 26}}, 1);
    b.poly({{9, 14}, {19, 14}, {20, 22}, {8, 22}}, 2);
    b.rect(3, 26, 22, 3, 4);
    b.ellipse(14.f, 30.f, 2.2f, 2.4f, 5);
    b.rect(12, 6, 4, 4, 3);
    return b;
}

gs::Bitmap bobArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8.f, 8.f, 6.4f, 6.4f, 1);
    b.ellipse(6.2f, 6.f, 2.1f, 1.6f, 2);
    b.rect(7, 1, 2, 3, 3);
    return b;
}

gs::Bitmap crownArt() {
    gs::Bitmap b(10, 6);
    b.poly({{0, 5}, {2, 0}, {4, 4}, {5, 0}, {8, 0}, {9, 5}}, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 8);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 12), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 6, 4), gs::rgb4(2, 0, 0));
    setPal(vdp, PAL_PAWN,
           {0, gs::rgb4(14, 12, 9), gs::rgb4(8, 6, 4), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 8), gs::rgb4(6, 4, 2), gs::rgb4(10, 8, 2), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_BOB, {0, gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 9), gs::rgb4(6, 5, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(10, 7, 4), gs::rgb4(6, 4, 2), gs::rgb4(12, 9, 5), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_IVORY, {0, gs::rgb4(13, 11, 8), gs::rgb4(7, 5, 3)});

    tileSolid(vdp, art.tileLight, 1, 3);
    tileSolid(vdp, art.tileDark, 2, 4);
    tileSolid(vdp, art.tileWall, 2, 2);
    tileSolid(vdp, art.tileTrim, 3, 1);

    art.pawn = gs::uploadImage(vdp, pawnArt());
    art.bell = gs::uploadImage(vdp, bellArt());
    art.bob = gs::uploadImage(vdp, bobArt());
    art.crown = gs::uploadImage(vdp, crownArt());
    loadFont(vdp, art);
}

}  // namespace pawnbell
