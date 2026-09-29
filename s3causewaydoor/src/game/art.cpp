#include "game/art.h"

#include <string>

namespace causewaydoor {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    gs::TextStyle big{2, 1, 0, 15, 1};
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

gs::Bitmap keeper(int pose) {
    gs::Bitmap b(32, 48);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = (pose == 1 || pose == 3) ? 1 : 0;
    R(10, 2 + bob, 12, 5, 6);
    R(10, 6 + bob, 12, 7, 4);
    R(18, 8 + bob, 2, 2, 1);
    R(8, 13 + bob, 16, 14, 2);
    R(8, 13 + bob, 4, 14, 3);
    R(12, 22 + bob, 8, 2, 5);
    if (pose == 0) {
        R(4, 16, 6, 5, 3);
        R(22, 18, 6, 4, 3);
        R(10, 27, 5, 14, 2);
        R(17, 27, 5, 14, 2);
        R(9, 40, 7, 4, 7);
        R(16, 40, 7, 4, 7);
    } else if (pose == 1) {
        R(2, 18, 8, 5, 3);
        R(22, 16, 7, 5, 3);
        R(11, 27, 5, 13, 2);
        R(18, 28, 5, 12, 2);
        R(10, 39, 7, 4, 7);
        R(17, 39, 7, 4, 7);
    } else if (pose == 2) {
        R(20, 14, 8, 6, 3);
        R(22, 20, 6, 4, 5);
        R(10, 26, 5, 15, 2);
        R(16, 26, 5, 15, 2);
        R(9, 40, 7, 4, 7);
        R(15, 40, 7, 4, 7);
    } else {
        R(6, 12, 8, 6, 3);
        R(20, 12, 8, 6, 3);
        R(11, 26, 5, 12, 2);
        R(16, 26, 5, 12, 2);
        R(9, 37, 8, 4, 7);
        R(15, 37, 8, 4, 7);
    }
    return b;
}

gs::Bitmap doorBmp() {
    gs::Bitmap b(28, 96);
    b.rect(2, 2, 24, 92, 2);
    b.rect(2, 2, 24, 6, 4);
    b.rect(2, 86, 24, 6, 1);
    b.rect(4, 14, 20, 3, 3);
    b.rect(4, 40, 20, 3, 3);
    b.rect(4, 66, 20, 3, 3);
    for (int y = 10; y < 84; y += 18)
        for (int x = 6; x < 22; x += 12) b.rect(float(x), float(y), 3, 3, 5);
    b.rect(18, 46, 5, 8, 6);
    b.rect(20, 48, 2, 4, 7);
    return b;
}

gs::Bitmap chainBmp() {
    gs::Bitmap b(10, 28);
    for (int i = 0; i < 4; i++) {
        b.rect(2, float(2 + i * 7), 6, 5, 2);
        b.rect(3, float(3 + i * 7), 4, 3, 0);
        b.rect(3, float(3 + i * 7), 4, 1, 4);
    }
    return b;
}

gs::Bitmap lampBmp() {
    gs::Bitmap b(12, 32);
    b.rect(5, 10, 2, 22, 2);
    b.rect(2, 1, 8, 10, 3);
    b.rect(3, 2, 6, 7, 5);
    b.rect(4, 4, 4, 3, 6);
    return b;
}

gs::Bitmap slabBmp() {
    gs::Bitmap b(36, 18);
    b.rect(0, 2, 36, 14, 2);
    b.rect(0, 2, 36, 3, 4);
    b.rect(0, 13, 36, 3, 1);
    b.rect(17, 4, 2, 10, 3);
    return b;
}

gs::Bitmap pileBmp() {
    gs::Bitmap b(12, 56);
    b.rect(3, 0, 6, 56, 2);
    b.rect(2, 0, 2, 56, 4);
    b.rect(1, 46, 10, 6, 1);
    b.rect(4, 8, 4, 3, 3);
    return b;
}

gs::Bitmap waveBmp() {
    gs::Bitmap b(40, 12);
    b.ellipse(10, 7, 9, 4, 3);
    b.ellipse(26, 8, 10, 3, 2);
    b.rect(6, 9, 28, 2, 4);
    return b;
}

gs::Bitmap gullBmp() {
    gs::Bitmap b(18, 8);
    b.line(1, 5, 8, 2, 2, 1.2f);
    b.line(8, 2, 16, 5, 2, 1.2f);
    b.set(8, 3, 3);
    return b;
}

gs::Bitmap boltBmp() {
    gs::Bitmap b(8, 16);
    b.rect(3, 0, 2, 16, 4);
    b.rect(2, 6, 4, 4, 5);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 15, 13), gs::rgb4(2, 4, 6), gs::rgb4(15, 12, 4),
                          gs::rgb4(12, 3, 3), gs::rgb4(6, 8, 9), gs::rgb4(1, 2, 3)});
    setPal(vdp, PAL_IRON, {gs::rgb4(0, 0, 0), gs::rgb4(2, 2, 3), gs::rgb4(5, 6, 7), gs::rgb4(3, 4, 5),
                           gs::rgb4(9, 10, 11), gs::rgb4(12, 8, 3), gs::rgb4(8, 5, 2), gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_COAT, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(7, 3, 2), gs::rgb4(4, 2, 2),
                           gs::rgb4(13, 9, 7), gs::rgb4(10, 6, 3), gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_SEA, {gs::rgb4(0, 0, 0), gs::rgb4(1, 3, 5), gs::rgb4(2, 6, 8), gs::rgb4(4, 9, 10),
                          gs::rgb4(8, 12, 12), gs::rgb4(1, 2, 3)});
    setPal(vdp, PAL_STONE, {gs::rgb4(0, 0, 0), gs::rgb4(3, 3, 3), gs::rgb4(6, 6, 6), gs::rgb4(4, 4, 5),
                            gs::rgb4(9, 9, 8), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_LAMP, {gs::rgb4(0, 0, 0), gs::rgb4(3, 3, 2), gs::rgb4(5, 5, 4), gs::rgb4(8, 6, 2),
                           gs::rgb4(12, 9, 3), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_CHAIN, {gs::rgb4(0, 0, 0), gs::rgb4(2, 2, 2), gs::rgb4(6, 6, 6), gs::rgb4(4, 4, 4),
                            gs::rgb4(10, 10, 9), gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_SKY, {gs::rgb4(0, 0, 0), gs::rgb4(8, 8, 9), gs::rgb4(12, 12, 13), gs::rgb4(6, 6, 7)});

    art.leanL = gs::uploadMipped(vdp, keeper(0));
    art.leanR = gs::uploadMipped(vdp, keeper(1));
    art.brace = gs::uploadMipped(vdp, keeper(2));
    art.shoulder = gs::uploadMipped(vdp, keeper(3));
    art.door = gs::uploadMipped(vdp, doorBmp());
    art.chain = gs::uploadMipped(vdp, chainBmp());
    art.lamp = gs::uploadMipped(vdp, lampBmp());
    art.slab = gs::uploadMipped(vdp, slabBmp());
    art.pile = gs::uploadMipped(vdp, pileBmp());
    art.wave = gs::uploadMipped(vdp, waveBmp());
    art.gull = gs::uploadMipped(vdp, gullBmp());
    art.bolt = gs::uploadMipped(vdp, boltBmp());
    loadFont(vdp, art, tiles);
    vdp.setFogColor(gs::rgb4(3, 5, 7));
}

}  // namespace causewaydoor
