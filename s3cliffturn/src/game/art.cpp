#include "game/art.h"

#include <cstring>

namespace cliff {

static void pal(gs::VDP& vdp, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(p * 16 + i++, c);
    while (i < 16) vdp.setColor(p * 16 + i++, 0);
}

static gs::Bitmap carBmp(int bank) {
    gs::Bitmap b(48, 36);
    b.rect(14, 4, 20, 28, 1);
    b.rect(10, 8, 28, 18, 1);
    b.ellipse(24, 14, 8, 5, 3);
    b.rect(18, 26, 12, 6, 2);
    b.rect(12, 30, 6, 3, 5);
    b.rect(30, 30, 6, 3, 5);
    b.ellipse(12, 12, 4, 6, 4);
    b.ellipse(36, 12, 4, 6, 4);
    b.ellipse(12, 26, 4, 6, 4);
    b.ellipse(36, 26, 4, 6, 4);
    if (bank > 0) {
        int s = bank == 2 ? 8 : 4;
        b.rect(6, 6, s, 22, 2);
        b.rect(8, 10, 3, 8, 6);
    }
    b.rect(20, 6, 8, 3, 7);
    return b;
}

static gs::Bitmap postBmp() {
    gs::Bitmap b(8, 28);
    b.rect(3, 2, 2, 24, 1);
    b.rect(1, 0, 6, 4, 2);
    b.rect(2, 24, 4, 3, 3);
    return b;
}

static gs::Bitmap boardBmp() {
    gs::Bitmap b(28, 16);
    b.rect(1, 1, 26, 14, 1);
    b.rect(3, 3, 22, 10, 2);
    b.rect(6, 6, 4, 4, 3);
    b.rect(12, 6, 4, 4, 3);
    b.rect(18, 6, 4, 4, 3);
    return b;
}

static void roadPal(gs::VDP& vdp) {
    using gs::rgb4;
    uint16_t c[16];
    std::memset(c, 0, sizeof c);
    c[1] = rgb4(6, 6, 5);
    c[2] = rgb4(4, 4, 3);
    c[3] = rgb4(8, 7, 5);
    c[4] = rgb4(9, 8, 5);
    c[5] = rgb4(6, 5, 3);
    c[6] = rgb4(3, 3, 4);
    c[7] = rgb4(2, 2, 3);
    c[8] = rgb4(7, 7, 6);
    c[9] = rgb4(2, 2, 2);
    c[10] = rgb4(5, 5, 5);
    c[11] = rgb4(2, 5, 8);
    c[12] = rgb4(1, 3, 6);
    c[13] = rgb4(4, 8, 11);
    c[14] = rgb4(13, 12, 8);
    c[15] = rgb4(6, 6, 7);
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, c[i]);
}

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    pal(vdp, PAL_HUD, {0, rgb4(15, 14, 10), rgb4(15, 8, 3), rgb4(12, 3, 2), rgb4(8, 14, 10)});
    pal(vdp, PAL_CAR, {0, rgb4(12, 2, 2), rgb4(5, 1, 1), rgb4(6, 10, 13), rgb4(2, 2, 2), rgb4(14, 12, 4),
                       rgb4(15, 15, 12), rgb4(9, 9, 9)});
    pal(vdp, PAL_GHOST, {0, rgb4(10, 11, 12), rgb4(4, 5, 6), rgb4(7, 9, 11), rgb4(2, 2, 3), rgb4(8, 8, 6),
                         rgb4(13, 13, 12), rgb4(6, 6, 7)});
    pal(vdp, PAL_ROCK, {0, rgb4(7, 6, 5), rgb4(12, 4, 2), rgb4(4, 3, 3)});
    pal(vdp, PAL_SIGN, {0, rgb4(14, 12, 3), rgb4(2, 2, 2), rgb4(15, 15, 15)});
    roadPal(vdp);
    vdp.setFogColor(rgb4(10, 8, 7));

    for (int i = 0; i < 3; i++) art.car[i] = gs::uploadMipped(vdp, carBmp(i));
    art.ghost = gs::uploadMipped(vdp, carBmp(0));
    art.post = gs::uploadMipped(vdp, postBmp());
    art.board = gs::uploadMipped(vdp, boardBmp());

    for (int ch = 0; ch < 96; ch++) {
        gs::Bitmap g(8, 8);
        const uint8_t* src = gs::glyph(char(ch + 32));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (src[y * 5 + x]) g.set(x + 1, y, 1);
        art.glyph[ch] = gs::uploadMipped(vdp, g);
    }
}

}  // namespace cliff
