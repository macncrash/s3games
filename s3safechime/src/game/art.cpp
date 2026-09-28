#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace safechime {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

gs::Bitmap bodyArt() {
    gs::Bitmap b(96, 120);
    b.rect(4, 8, 88, 104, 1);
    b.rect(0, 4, 96, 10, 2);
    b.rect(0, 106, 96, 10, 2);
    b.rect(8, 18, 80, 84, 3);
    b.rect(12, 100, 16, 6, 4);
    b.rect(68, 100, 16, 6, 4);
    return b;
}

gs::Bitmap doorArt() {
    gs::Bitmap b(72, 88);
    b.rect(2, 2, 68, 84, 1);
    b.rect(6, 6, 60, 76, 2);
    b.ellipse(36, 42, 24.f, 24.f, 3);
    b.rect(8, 10, 8, 6, 4);
    b.rect(56, 10, 8, 6, 4);
    b.rect(8, 72, 8, 6, 4);
    b.rect(56, 72, 8, 6, 4);
    return b;
}

gs::Bitmap dialArt() {
    gs::Bitmap b(48, 48);
    b.ellipse(24, 24, 22.f, 22.f, 1);
    b.ellipse(24, 24, 16.f, 16.f, 2);
    b.ellipse(24, 24, 4.f, 4.f, 3);
    return b;
}

gs::Bitmap needleArt() {
    gs::Bitmap b(8, 20);
    b.poly({{4, 0}, {7, 18}, {1, 18}}, 1);
    b.rect(3, 16, 2, 4, 2);
    return b;
}

gs::Bitmap tickArt() {
    gs::Bitmap b(4, 8);
    b.rect(1, 0, 2, 8, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5.f, 5.f, 1);
    b.ellipse(6, 6, 2.2f, 2.2f, 2);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(36, 40);
    b.rect(17, 0, 2, 6, 3);
    b.poly({{10, 8}, {26, 8}, {32, 26}, {4, 26}}, 1);
    b.ellipse(18, 26, 14.f, 6.f, 1);
    b.ellipse(18, 26, 5.f, 2.4f, 2);
    b.ellipse(18, 18, 2.f, 3.f, 3);
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(64, 64);
    b.ellipse(32, 32, 30.f, 30.f, 1);
    b.ellipse(32, 32, 24.f, 24.f, 2);
    b.ellipse(32, 32, 3.f, 3.f, 3);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(6, 6);
    b.ellipse(3, 3, 2.2f, 2.2f, 1);
    return b;
}

gs::Bitmap handleArt() {
    gs::Bitmap b(16, 36);
    b.rect(4, 2, 8, 28, 1);
    b.ellipse(8, 8, 6.f, 6.f, 2);
    b.ellipse(8, 28, 5.f, 5.f, 2);
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
    setPal(vdp, PAL_INK, {0, gs::rgb4(14, 14, 13), gs::rgb4(2, 2, 5)});
    setPal(vdp, PAL_STEEL, {0, gs::rgb4(8, 9, 11), gs::rgb4(4, 5, 7), gs::rgb4(3, 3, 4), gs::rgb4(12, 12, 13)});
    setPal(vdp, PAL_DOOR, {0, gs::rgb4(10, 11, 12), gs::rgb4(6, 7, 9), gs::rgb4(2, 2, 3), gs::rgb4(14, 13, 8)});
    setPal(vdp, PAL_DIAL, {0, gs::rgb4(13, 12, 9), gs::rgb4(5, 4, 3), gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 3), gs::rgb4(8, 5, 1)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(15, 12, 4), gs::rgb4(11, 8, 2), gs::rgb4(5, 3, 2)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(12, 11, 9), gs::rgb4(3, 4, 7), gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_DEAD, {0, gs::rgb4(15, 4, 3), gs::rgb4(6, 1, 2)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(4, 8, 4), gs::rgb4(12, 15, 6)});

    loadFont(vdp, art);
    art.body = gs::uploadMipped(vdp, bodyArt());
    art.door = gs::uploadMipped(vdp, doorArt());
    art.dial = gs::uploadMipped(vdp, dialArt());
    art.needle = gs::uploadMipped(vdp, needleArt());
    art.tick = gs::uploadMipped(vdp, tickArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.face = gs::uploadMipped(vdp, faceArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.handle = gs::uploadMipped(vdp, handleArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("SAFE", {4, 1, 2, 0, 1}));
    art.rule = gs::uploadImage(vdp, gs::textBitmap("THE HOUR", {2, 1, 2, 0, 1}));
    vdp.setFogColor(gs::rgb4(1, 1, 2));
}

}  // namespace safechime
