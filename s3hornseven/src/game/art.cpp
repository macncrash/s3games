#include "game/art.h"

#include <initializer_list>

namespace hornseven {
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
    setPal(vdp, pal, {0, ink, edge});
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 1));
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8 && x + 2 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

// Index 1 skin, 2 coat, 3 brass, 4 dark, 5 hair, 6 bell highlight.
gs::Bitmap playerArt(bool step) {
    gs::Bitmap b(36, 48);
    b.ellipse(14, 8, 5.2f, 5.4f, 1);
    b.rect(9, 2, 10, 4, 5);
    b.rect(10, 16, 9, 14, 2);
    b.rect(8, 17, 4, 8, 2);
    b.rect(18, 18, 3, 7, 1);
    b.rect(20, 20, 12, 3, 3);
    b.ellipse(32, 21, 3.4f, 3.6f, 3);
    b.ellipse(31, 20, 1.2f, 1.2f, 6);
    b.set(12, 8, 4);
    b.set(16, 8, 4);
    if (step) {
        b.rect(10, 30, 4, 12, 4);
        b.rect(16, 31, 4, 11, 4);
        b.rect(9, 41, 5, 3, 4);
        b.rect(16, 41, 5, 3, 4);
    } else {
        b.rect(11, 30, 4, 13, 4);
        b.rect(16, 30, 4, 12, 4);
        b.rect(10, 42, 5, 3, 4);
        b.rect(15, 41, 5, 3, 4);
    }
    b.outline(4, false);
    return b;
}

gs::Bitmap rivalArt(bool step) {
    gs::Bitmap b(28, 40);
    b.ellipse(12, 7, 4.4f, 4.6f, 1);
    b.rect(8, 2, 8, 3, 5);
    b.rect(8, 14, 8, 12, 2);
    b.rect(6, 15, 3, 7, 2);
    b.rect(4, 18, 8, 2, 3);
    b.ellipse(4, 19, 2.4f, 2.6f, 3);
    b.set(10, 7, 4);
    b.set(14, 7, 4);
    if (step) {
        b.rect(8, 26, 3, 10, 4);
        b.rect(13, 27, 3, 9, 4);
    } else {
        b.rect(9, 26, 3, 10, 4);
        b.rect(13, 26, 3, 10, 4);
    }
    b.outline(4, false);
    return b;
}

gs::Bitmap curtainArt() {
    gs::Bitmap b(28, 80);
    for (int x = 0; x < 28; x++) {
        int c = ((x / 4) % 2) ? 1 : 2;
        for (int y = 0; y < 80; y++) b.set(x, y, c);
    }
    b.rect(0, 0, 28, 4, 3);
    for (int y = 8; y < 80; y += 10) b.set(1, y, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 12);
    b.rect(4, 0, 2, 3, 2);
    b.ellipse(5, 7, 4.2f, 4.2f, 1);
    b.ellipse(4, 6, 1.6f, 1.4f, 3);
    return b;
}

gs::Bitmap noteArt() {
    gs::Bitmap b(12, 16);
    b.ellipse(4, 12, 3.2f, 2.4f, 1);
    b.rect(6, 2, 2, 10, 1);
    b.rect(6, 2, 5, 2, 1);
    b.rect(9, 2, 2, 4, 1);
    return b;
}

gs::Bitmap standArt() {
    gs::Bitmap b(96, 16);
    b.rect(0, 0, 96, 5, 1);
    b.rect(0, 5, 96, 3, 2);
    b.rect(0, 8, 96, 8, 3);
    for (int x = 6; x < 96; x += 12) b.rect(float(x), 8, 2, 8, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 10), gs::rgb4(4, 2, 1));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(6, 3, 0));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 6, 4), gs::rgb4(4, 1, 1));
    setPal(vdp, PAL_YOU, {0, gs::rgb4(14, 10, 7), gs::rgb4(12, 3, 3), gs::rgb4(14, 11, 3), gs::rgb4(2, 1, 1),
                          gs::rgb4(4, 2, 1), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_THEM, {0, gs::rgb4(10, 8, 7), gs::rgb4(3, 4, 8), gs::rgb4(8, 8, 5), gs::rgb4(1, 1, 2),
                           gs::rgb4(2, 2, 3), gs::rgb4(12, 12, 8)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(10, 6, 3), gs::rgb4(6, 3, 1), gs::rgb4(4, 2, 1), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_NOTE, {0, gs::rgb4(15, 14, 6), gs::rgb4(8, 6, 2)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 2), gs::rgb4(15, 13, 5)});
    loadFont(vdp, art);
    art.player[0] = gs::uploadMipped(vdp, playerArt(false));
    art.player[1] = gs::uploadMipped(vdp, playerArt(true));
    art.rival[0] = gs::uploadMipped(vdp, rivalArt(false));
    art.rival[1] = gs::uploadMipped(vdp, rivalArt(true));
    art.curtain = gs::uploadMipped(vdp, curtainArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.note = gs::uploadMipped(vdp, noteArt());
    art.stand = gs::uploadMipped(vdp, standArt());
}

}  // namespace hornseven
