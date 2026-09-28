#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace keyschime {
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
    gs::Bitmap b(12, 18);
    b.ellipse(5, 13, 4.4f, 3.2f, 1);
    b.ellipse(5, 13, 2.1f, 1.4f, 2);
    b.rect(8, 2, 2, 12, 1);
    b.poly({{8, 2}, {11, 2}, {11, 5}, {8, 6}}, 3);
    return b;
}

gs::Bitmap keyArt() {
    gs::Bitmap b(40, 64);
    b.rect(1, 1, 38, 62, 1);
    b.rect(3, 3, 34, 10, 2);
    b.rect(4, 48, 32, 10, 3);
    b.ellipse(20, 34, 5.f, 4.f, 4);
    return b;
}

gs::Bitmap blackArt() {
    gs::Bitmap b(18, 36);
    b.rect(1, 0, 16, 34, 1);
    b.rect(3, 2, 10, 8, 2);
    b.rect(4, 24, 10, 6, 2);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(36, 40);
    b.rect(17, 0, 2, 5, 3);
    b.poly({{10, 8}, {26, 8}, {32, 26}, {4, 26}}, 1);
    b.ellipse(18, 26, 14.f, 6.f, 1);
    b.ellipse(18, 26, 5.f, 2.4f, 2);
    b.ellipse(18, 20, 2.2f, 3.4f, 3);
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(72, 72);
    b.ellipse(36, 36, 34.f, 34.f, 1);
    b.ellipse(36, 36, 28.f, 28.f, 2);
    b.ellipse(36, 36, 3.f, 3.f, 3);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(6, 6);
    b.ellipse(3, 3, 2.4f, 2.4f, 1);
    return b;
}

gs::Bitmap glowArt() {
    gs::Bitmap b(28, 6);
    b.ellipse(14, 3, 13.f, 2.4f, 1);
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
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 6)});
    setPal(vdp, PAL_IVORY, {0, gs::rgb4(15, 14, 11), gs::rgb4(15, 15, 13), gs::rgb4(9, 8, 6), gs::rgb4(13, 11, 5)});
    setPal(vdp, PAL_EBONY, {0, gs::rgb4(1, 1, 3), gs::rgb4(5, 5, 8)});
    setPal(vdp, PAL_NOTE, {0, gs::rgb4(14, 12, 9), gs::rgb4(3, 3, 6), gs::rgb4(15, 10, 4)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 3), gs::rgb4(8, 5, 1)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(15, 12, 4), gs::rgb4(11, 8, 2), gs::rgb4(5, 3, 2)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(6, 8, 14), gs::rgb4(3, 4, 9)});
    setPal(vdp, PAL_DEAD, {0, gs::rgb4(15, 4, 3), gs::rgb4(6, 1, 2)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(12, 11, 9), gs::rgb4(4, 5, 8), gs::rgb4(15, 13, 6)});

    loadFont(vdp, art);
    art.note = gs::uploadMipped(vdp, noteArt());
    art.key = gs::uploadMipped(vdp, keyArt());
    art.black = gs::uploadMipped(vdp, blackArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.face = gs::uploadMipped(vdp, faceArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("KEYS", {4, 1, 2, 0, 1}));
    art.rule = gs::uploadImage(vdp, gs::textBitmap("THE HOUR", {2, 1, 2, 0, 1}));
    vdp.setFogColor(gs::rgb4(1, 1, 3));
}

}  // namespace keyschime
