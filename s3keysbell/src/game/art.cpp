#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace keysbell {
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
    gs::Bitmap b(14, 16);
    b.ellipse(6, 11, 5.2f, 3.6f, 1);
    b.ellipse(6, 11, 2.8f, 1.8f, 2);
    b.rect(10, 1, 2, 11, 1);
    b.rect(6, 1, 6, 2, 3);
    return b;
}

gs::Bitmap keyArt() {
    gs::Bitmap b(36, 56);
    b.rect(1, 1, 34, 54, 1);
    b.rect(3, 3, 30, 8, 2);
    b.rect(4, 44, 28, 8, 3);
    b.rect(14, 24, 8, 8, 4);
    return b;
}

gs::Bitmap blackArt() {
    gs::Bitmap b(16, 32);
    b.rect(1, 0, 14, 30, 1);
    b.rect(3, 2, 8, 6, 2);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(28, 32);
    b.rect(13, 0, 2, 4, 3);
    b.poly({{8, 6}, {20, 6}, {24, 22}, {4, 22}}, 1);
    b.ellipse(14, 22, 10.f, 5.f, 1);
    b.ellipse(14, 22, 4.f, 2.2f, 2);
    b.rect(13, 24, 2, 5, 3);
    return b;
}

gs::Bitmap glowArt() {
    gs::Bitmap b(22, 8);
    b.ellipse(11, 4, 10, 3.2f, 1);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4.2f, 4.2f, 1);
    b.ellipse(5, 5, 2.f, 2.f, 2);
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
    setPal(vdp, PAL_BELL, {0, gs::rgb4(15, 12, 3), gs::rgb4(10, 7, 2), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(7, 4, 2), gs::rgb4(11, 7, 3)});
    setPal(vdp, PAL_DEAD, {0, gs::rgb4(15, 5, 4), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_CLAP, {0, gs::rgb4(12, 14, 15), gs::rgb4(4, 6, 8)});

    loadFont(vdp, art);
    art.note = gs::uploadMipped(vdp, noteArt());
    art.key = gs::uploadMipped(vdp, keyArt());
    art.black = gs::uploadMipped(vdp, blackArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.mark = gs::uploadMipped(vdp, markArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("KEYS BELL", {3, 1, 2, 0, 1}));
    art.rule = gs::uploadImage(vdp, gs::textBitmap("BEFORE THE THIRD", {2, 1, 2, 0, 1}));
    vdp.setFogColor(gs::rgb4(1, 1, 2));
}

}  // namespace keysbell
