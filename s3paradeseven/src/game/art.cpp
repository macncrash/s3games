#include "game/art.h"

#include <initializer_list>

namespace paradeseven {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap marcher(int hat) {
    gs::Bitmap b(16, 26);
    b.ellipse(8, 5, 4.2f, 3.6f, 3);
    b.rect(5, 2, 6, 2, hat);
    b.rect(4, 1, 8, 2, 5);
    b.set(6, 5, 6);
    b.set(10, 5, 6);
    b.rect(6, 8, 4, 2, 2);
    b.rect(5, 10, 6, 8, 1);
    b.rect(6, 11, 2, 6, 2);
    b.rect(3, 11, 2, 6, 1);
    b.rect(11, 11, 2, 6, 1);
    b.rect(5, 18, 2, 7, 7);
    b.rect(9, 18, 2, 7, 7);
    b.rect(4, 24, 4, 2, 6);
    b.rect(9, 24, 4, 2, 6);
    return b;
}

gs::Bitmap floatArt() {
    gs::Bitmap b(40, 22);
    b.rect(1, 4, 38, 14, 1);
    b.rect(1, 4, 38, 3, 2);
    b.rect(2, 8, 10, 6, 3);
    b.rect(14, 8, 10, 6, 4);
    b.rect(26, 8, 10, 6, 3);
    b.rect(4, 16, 6, 5, 5);
    b.rect(30, 16, 6, 5, 5);
    b.ellipse(7, 19, 2.4f, 2.4f, 6);
    b.ellipse(33, 19, 2.4f, 2.4f, 6);
    b.rect(18, 1, 4, 4, 2);
    b.rect(16, 0, 8, 2, 4);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(14, 22);
    b.rect(2, 0, 2, 22, 3);
    b.poly({{4, 2}, {13, 6}, {4, 11}}, 1);
    b.poly({{5, 4}, {11, 6}, {5, 9}}, 2);
    return b;
}

gs::Bitmap curbArt() {
    gs::Bitmap b(36, 14);
    b.rect(0, 2, 36, 10, 2);
    b.rect(2, 4, 32, 6, 1);
    for (int i = 0; i < 7; i++) b.rect(4 + i * 4, 5, 2, 4, 4);
    return b;
}

gs::Bitmap confettiArt() {
    gs::Bitmap b(5, 5);
    b.rect(1, 0, 3, 2, 1);
    b.rect(0, 2, 2, 2, 2);
    b.set(3, 3, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7, 2.1f, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 28);
    b.rect(3, 6, 2, 22, 2);
    b.ellipse(4, 4, 3.2f, 3.2f, 1);
    b.ellipse(3.2f, 3.2f, 1.2f, 1.2f, 3);
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

gs::Image phrase(gs::VDP& vdp, const char* s) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {2, 1, 2, 0, 1}));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_INK, {0, ink, gs::rgb4(4, 4, 6), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(12, 3, 4), gs::rgb4(15, 13, 8), gs::rgb4(14, 10, 6), gs::rgb4(3, 2, 6),
                          gs::rgb4(8, 2, 3), gs::rgb4(2, 2, 2), gs::rgb4(4, 5, 10)});
    setPal(vdp, PAL_THEM, {0, gs::rgb4(3, 6, 13), gs::rgb4(14, 13, 10), gs::rgb4(10, 8, 6), gs::rgb4(2, 3, 8),
                           gs::rgb4(1, 3, 8), gs::rgb4(2, 2, 2), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(13, 2, 3), gs::rgb4(15, 12, 10), gs::rgb4(15, 14, 8), gs::rgb4(6, 1, 2),
                          gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 9), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BLUE, {0, gs::rgb4(2, 5, 13), gs::rgb4(12, 14, 15), gs::rgb4(8, 12, 15), gs::rgb4(1, 2, 6),
                           gs::rgb4(3, 3, 4), gs::rgb4(9, 9, 10), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(12, 8, 1), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 12),
                           gs::rgb4(6, 3, 0)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(14, 3, 3), gs::rgb4(15, 14, 12), gs::rgb4(6, 4, 2), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(15, 14, 10), gs::rgb4(12, 4, 6), gs::rgb4(15, 12, 3), gs::rgb4(4, 8, 12)});
    loadFont(vdp, art);
    art.you = gs::uploadMipped(vdp, marcher(4));
    art.them = gs::uploadMipped(vdp, marcher(5));
    art.float_ = gs::uploadMipped(vdp, floatArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.curb = gs::uploadMipped(vdp, curbArt());
    art.confetti = gs::uploadMipped(vdp, confettiArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.logo = phrase(vdp, "PARADE");
    art.done = phrase(vdp, "FIRST TO SEVEN");
    art.miss = phrase(vdp, "THEY LEAVE");
}

}  // namespace paradeseven
