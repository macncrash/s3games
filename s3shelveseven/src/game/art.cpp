#include "game/art.h"

namespace shelveseven {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

void ink(gs::VDP& vdp, int pal, uint16_t face, uint16_t shade) {
    uint16_t c[16] = {};
    c[1] = face;
    c[15] = shade;
    setPal(vdp, pal, c);
}

gs::Bitmap bookArt(int stripe) {
    gs::Bitmap b(14, 28);
    b.rect(1, 1, 12, 26, 2);
    b.rect(2, 2, 9, 24, 1);
    b.rect(3, 3, 2, 22, 3);
    for (int y = 6; y < 22; y += 4) b.rect(7, y + stripe, 3, 1, 4);
    b.rect(1, 1, 12, 1, 3);
    b.rect(1, 26, 12, 1, 3);
    return b;
}

gs::Bitmap cartArt() {
    gs::Bitmap b(36, 30);
    b.poly({{2, 6}, {30, 6}, {34, 18}, {0, 18}}, 2);
    b.poly({{6, 8}, {26, 8}, {28, 16}, {4, 16}}, 1);
    b.rect(2, 18, 32, 4, 3);
    b.ellipse(8, 25, 4, 4, 4);
    b.ellipse(26, 25, 4, 4, 4);
    b.ellipse(8, 25, 1.5f, 1.5f, 5);
    b.ellipse(26, 25, 1.5f, 1.5f, 5);
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(140, 6);
    b.rect(0, 0, 140, 6, 2);
    b.rect(0, 0, 140, 1, 1);
    b.rect(0, 5, 140, 1, 3);
    for (int x = 18; x < 140; x += 22) b.rect(x, 1, 1, 4, 3);
    return b;
}

gs::Bitmap plateArt() {
    gs::Bitmap b(16, 22);
    b.rect(1, 1, 14, 20, 2);
    b.rect(3, 3, 10, 16, 1);
    b.rect(5, 6, 2, 10, 3);
    return b;
}

gs::Bitmap sevenArt() {
    gs::Bitmap b(22, 28);
    b.rect(2, 2, 18, 4, 1);
    b.poly({{16, 6}, {20, 6}, {12, 26}, {8, 26}}, 1);
    b.rect(3, 3, 16, 1, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    ink(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(3, 2, 2));
    ink(vdp, PAL_GOLD, gs::rgb4(15, 13, 5), gs::rgb4(4, 3, 1));
    ink(vdp, PAL_ALERT, gs::rgb4(15, 5, 3), gs::rgb4(4, 1, 1));
    ink(vdp, PAL_OK, gs::rgb4(6, 15, 7), gs::rgb4(1, 4, 2));
    ink(vdp, PAL_DIM, gs::rgb4(10, 8, 6), gs::rgb4(2, 1, 1));

    const uint16_t spines[4][16] = {
        {0, gs::rgb4(12, 3, 3), gs::rgb4(8, 2, 2), gs::rgb4(15, 10, 8), gs::rgb4(15, 14, 6), 0},
        {0, gs::rgb4(3, 6, 13), gs::rgb4(2, 3, 8), gs::rgb4(10, 14, 15), gs::rgb4(15, 14, 6), 0},
        {0, gs::rgb4(3, 11, 5), gs::rgb4(1, 6, 3), gs::rgb4(12, 15, 10), gs::rgb4(15, 14, 6), 0},
        {0, gs::rgb4(12, 8, 2), gs::rgb4(7, 4, 1), gs::rgb4(15, 13, 8), gs::rgb4(15, 14, 6), 0},
    };
    for (int i = 0; i < 4; i++) setPal(vdp, PAL_A + i, spines[i]);

    const uint16_t wood[16] = {
        0,
        gs::rgb4(14, 10, 5),
        gs::rgb4(10, 6, 3),
        gs::rgb4(5, 3, 1),
        gs::rgb4(2, 2, 2),
        gs::rgb4(12, 12, 11),
    };
    const uint16_t mark[16] = {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12), gs::rgb4(8, 6, 2)};
    setPal(vdp, PAL_WOOD, wood);
    setPal(vdp, PAL_MARK, mark);

    loadFont(vdp, art);
    for (int i = 0; i < 4; i++) art.book[i] = gs::uploadMipped(vdp, bookArt(i));
    art.cart = gs::uploadMipped(vdp, cartArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.plate = gs::uploadMipped(vdp, plateArt());
    art.seven = gs::uploadMipped(vdp, sevenArt());
}

}  // namespace shelveseven
