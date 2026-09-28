#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace keysseven {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

gs::Bitmap noteArt() {
    gs::Bitmap b(16, 18);
    b.ellipse(7, 12, 6.2f, 4.2f, 1);
    b.ellipse(7, 12, 3.6f, 2.2f, 2);
    b.rect(12, 2, 2, 11, 1);
    b.rect(8, 2, 6, 2, 3);
    return b;
}

gs::Bitmap keyArt() {
    gs::Bitmap b(40, 52);
    b.rect(1, 1, 38, 50, 1);
    b.rect(3, 3, 34, 8, 2);
    b.rect(4, 40, 32, 8, 3);
    b.rect(16, 22, 8, 8, 4);
    return b;
}

gs::Bitmap blackArt() {
    gs::Bitmap b(18, 30);
    b.rect(1, 0, 16, 28, 1);
    b.rect(3, 2, 10, 6, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5.2f, 5.2f, 2);
    b.ellipse(6, 6, 2.6f, 2.6f, 1);
    return b;
}

gs::Bitmap glowArt() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 9, 3.2f, 1);
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
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 15, 15), gs::rgb4(3, 3, 5)});
    setPal(vdp, PAL_IVORY, {0, gs::rgb4(15, 14, 12), gs::rgb4(15, 15, 14), gs::rgb4(8, 7, 6), gs::rgb4(12, 10, 6)});
    setPal(vdp, PAL_EBONY, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_NOTE, {0, gs::rgb4(14, 13, 11), gs::rgb4(4, 4, 6), gs::rgb4(15, 12, 5)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 5, 1)});
    setPal(vdp, PAL_RIVAL, {0, gs::rgb4(12, 6, 8), gs::rgb4(5, 2, 3)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(7, 4, 2), gs::rgb4(11, 7, 3)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 5), gs::rgb4(5, 4, 2)});
    setPal(vdp, PAL_MISS, {0, gs::rgb4(15, 5, 4), gs::rgb4(6, 1, 1)});

    loadFont(vdp, art);
    art.note = gs::uploadMipped(vdp, noteArt());
    art.key = gs::uploadMipped(vdp, keyArt());
    art.black = gs::uploadMipped(vdp, blackArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("KEYS SEVEN", {3, 1, 2, 0, 1}));
    art.leave = gs::uploadImage(vdp, gs::textBitmap("FIRST TO SEVEN", {2, 1, 2, 0, 1}));
    vdp.setFogColor(gs::rgb4(1, 1, 2));
}

}  // namespace keysseven
