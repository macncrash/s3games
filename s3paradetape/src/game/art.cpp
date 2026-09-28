#include "game/art.h"

#include <initializer_list>

namespace paradetape {
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

gs::Bitmap majorArt() {
    gs::Bitmap b(16, 28);
    b.rect(5, 1, 6, 3, 4);
    b.ellipse(8, 6, 3.6f, 3.2f, 3);
    b.set(6, 6, 6);
    b.set(10, 6, 6);
    b.rect(6, 9, 4, 2, 2);
    b.rect(4, 11, 8, 8, 1);
    b.rect(6, 12, 4, 6, 2);
    b.rect(2, 12, 2, 7, 1);
    b.rect(12, 12, 2, 7, 1);
    b.rect(5, 19, 2, 6, 7);
    b.rect(9, 19, 2, 6, 7);
    b.rect(4, 25, 4, 2, 5);
    b.rect(9, 25, 4, 2, 5);
    b.rect(13, 8, 2, 8, 4);
    return b;
}

gs::Bitmap floatArt() {
    gs::Bitmap b(44, 18);
    b.rect(2, 4, 40, 10, 1);
    b.rect(2, 4, 40, 3, 2);
    b.rect(6, 8, 8, 4, 3);
    b.rect(18, 8, 8, 4, 4);
    b.rect(30, 8, 8, 4, 3);
    b.rect(6, 13, 5, 4, 5);
    b.rect(33, 13, 5, 4, 5);
    b.ellipse(8, 16, 2.2f, 2.2f, 6);
    b.ellipse(36, 16, 2.2f, 2.2f, 6);
    b.rect(20, 0, 4, 5, 2);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(16, 20);
    b.rect(2, 0, 2, 20, 3);
    b.poly({{4, 2}, {15, 6}, {4, 10}}, 1);
    b.poly({{5, 4}, {12, 6}, {5, 8}}, 2);
    return b;
}

gs::Bitmap curbArt() {
    gs::Bitmap b(48, 10);
    b.rect(0, 1, 48, 8, 1);
    b.rect(2, 3, 44, 4, 2);
    for (int i = 0; i < 8; i++) b.rect(3 + i * 5, 3, 2, 4, 3);
    return b;
}

gs::Bitmap drumArt() {
    gs::Bitmap b(14, 16);
    b.ellipse(7, 8, 6, 5, 1);
    b.ellipse(7, 6, 5, 2.4f, 2);
    b.rect(6, 1, 2, 6, 3);
    b.rect(3, 0, 8, 2, 4);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(18, 6);
    b.ellipse(9, 3, 8, 2, 1);
    return b;
}

gs::Bitmap confettiArt() {
    gs::Bitmap b(5, 5);
    b.rect(1, 0, 3, 2, 1);
    b.rect(0, 2, 2, 2, 2);
    b.set(3, 3, 3);
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
    setPal(vdp, PAL_YOU, {0, gs::rgb4(14, 3, 4), gs::rgb4(15, 13, 9), gs::rgb4(12, 8, 5), gs::rgb4(15, 12, 3),
                          gs::rgb4(2, 2, 2), gs::rgb4(8, 2, 2), gs::rgb4(5, 4, 8)});
    setPal(vdp, PAL_CROWD, {0, gs::rgb4(4, 7, 12), gs::rgb4(14, 12, 9), gs::rgb4(6, 4, 3), gs::rgb4(12, 3, 4),
                            gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(13, 2, 3), gs::rgb4(15, 12, 9), gs::rgb4(15, 14, 6), gs::rgb4(5, 1, 2),
                          gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 9), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 2), gs::rgb4(12, 8, 1), gs::rgb4(15, 15, 12), gs::rgb4(8, 5, 1),
                           gs::rgb4(4, 3, 2), gs::rgb4(9, 9, 8), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BLUE, {0, gs::rgb4(2, 5, 13), gs::rgb4(12, 14, 15), gs::rgb4(8, 11, 15), gs::rgb4(1, 2, 6),
                           gs::rgb4(3, 3, 4), gs::rgb4(9, 9, 10), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(15, 14, 11), gs::rgb4(13, 4, 5), gs::rgb4(4, 8, 13), gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_CURB, {0, gs::rgb4(7, 7, 8), gs::rgb4(11, 11, 12), gs::rgb4(15, 14, 6), gs::rgb4(4, 4, 5)});
    loadFont(vdp, art);
    art.you = gs::uploadMipped(vdp, majorArt());
    art.float_ = gs::uploadMipped(vdp, floatArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.curb = gs::uploadMipped(vdp, curbArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.confetti = gs::uploadMipped(vdp, confettiArt());
    art.logo = phrase(vdp, "PARADE");
    art.done = phrase(vdp, "DRAWER MATCHES");
    art.open = phrase(vdp, "TAPE OPEN");
}

}  // namespace paradetape
