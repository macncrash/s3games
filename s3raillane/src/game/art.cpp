#include "game/art.h"

#include <initializer_list>

namespace lane {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 96; c++) {
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
        art.font[c - 32] = t;
    }
}

gs::Bitmap cabArt() {
    gs::Bitmap b(72, 56);
    b.rect(10, 18, 52, 28, 2);
    b.rect(10, 18, 52, 5, 3);
    b.rect(14, 24, 18, 12, 4);
    b.rect(40, 24, 18, 12, 4);
    b.rect(16, 26, 6, 8, 9);
    b.rect(42, 26, 6, 8, 9);
    b.rect(8, 40, 56, 8, 1);
    b.rect(6, 44, 8, 8, 6);
    b.rect(58, 44, 8, 8, 6);
    b.ellipse(10, 48, 5, 5, 7);
    b.ellipse(62, 48, 5, 5, 7);
    b.ellipse(10, 48, 2, 2, 8);
    b.ellipse(62, 48, 2, 2, 8);
    b.rect(18, 46, 6, 4, 5);
    b.rect(48, 46, 6, 4, 10);
    b.rect(32, 42, 8, 4, 8);
    b.rect(34, 14, 4, 6, 1);
    b.rect(22, 8, 28, 8, 3);
    b.rect(26, 4, 20, 5, 11);
    b.rect(30, 2, 12, 3, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(80, 48);
    b.rect(4, 10, 6, 36, 2);
    b.rect(70, 10, 6, 36, 2);
    b.rect(4, 8, 6, 4, 3);
    b.rect(70, 8, 6, 4, 3);
    b.rect(8, 12, 64, 6, 4);
    b.rect(8, 12, 64, 2, 5);
    for (int x = 12; x < 68; x += 8) b.rect(x, 14, 4, 3, 6);
    b.rect(28, 20, 24, 10, 1);
    gs::Bitmap label = gs::textBitmap("END", {1, 3, 0, 0, 1});
    b.blit(label, 40 - label.w / 2, 21);
    b.ellipse(7, 8, 3, 3, 7);
    b.ellipse(73, 8, 3, 3, 7);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(16, 64);
    b.rect(7, 8, 2, 56, 1);
    b.rect(0, 6, 16, 3, 2);
    b.rect(1, 10, 2, 2, 3);
    b.rect(13, 10, 2, 2, 3);
    b.rect(5, 58, 6, 4, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_TEXT, gs::rgb4(15, 15, 15));
    textPal(vdp, PAL_DIM, gs::rgb4(9, 11, 13));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(4, 15, 8));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));

    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 5, 10), gs::rgb4(4, 8, 13), gs::rgb4(8, 12, 15),
            gs::rgb4(15, 4, 3), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 1), gs::rgb4(8, 8, 9), gs::rgb4(12, 14, 15),
            gs::rgb4(15, 13, 4), gs::rgb4(12, 10, 3), 0, 0, 0, 0});

    setPal(vdp, PAL_GATE,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 7), gs::rgb4(15, 15, 15), gs::rgb4(14, 11, 2),
            gs::rgb4(15, 14, 6), gs::rgb4(3, 2, 1), gs::rgb4(15, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0});

    setPal(vdp, PAL_POLE,
           {0, gs::rgb4(3, 3, 4), gs::rgb4(5, 5, 6), gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    // Road bank 12. Indices follow the tarmac road generator.
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(1, 3, 2), gs::rgb4(2, 5, 3), gs::rgb4(3, 6, 3), gs::rgb4(4, 4, 3), gs::rgb4(5, 5, 4),
            gs::rgb4(3, 3, 4), gs::rgb4(4, 4, 5), gs::rgb4(6, 6, 5), gs::rgb4(5, 5, 4), gs::rgb4(7, 7, 6),
            gs::rgb4(2, 4, 6), gs::rgb4(3, 6, 8), gs::rgb4(6, 8, 10), gs::rgb4(15, 12, 2), gs::rgb4(8, 8, 7), 0});

    loadFont(vdp, art);
    art.cab = gs::uploadImage(vdp, cabArt());
    art.gate = gs::uploadImage(vdp, gateArt());
    art.pole = gs::uploadImage(vdp, poleArt());
    art.lamp = gs::uploadImage(vdp, lampArt());
    art.logo = gs::uploadImage(vdp, gs::textBitmap("RAILLANE", {3, 1, 0, 15, 1}));
}

}  // namespace lane
