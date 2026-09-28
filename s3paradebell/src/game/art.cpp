#include "game/art.h"

#include <cstdint>

namespace paradebell {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* c, int n) {
    for (int i = 0; i < n; i++) vdp.setColor(pal * 16 + i, c[i]);
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

gs::Bitmap majorArt() {
    gs::Bitmap b(16, 28);
    b.rect(5, 0, 6, 3, 4);
    b.rect(4, 2, 8, 2, 5);
    b.ellipse(8, 7, 3.2f, 3.f, 3);
    b.set(6, 6, 6);
    b.set(10, 6, 6);
    b.rect(4, 10, 8, 9, 1);
    b.rect(6, 11, 4, 7, 2);
    b.rect(2, 11, 2, 7, 1);
    b.rect(12, 11, 2, 7, 1);
    b.line(13, 12, 15, 4, 4, 1.f);
    b.rect(5, 19, 2, 8, 5);
    b.rect(9, 19, 2, 8, 5);
    b.rect(4, 25, 3, 2, 6);
    b.rect(9, 25, 3, 2, 6);
    return b;
}

gs::Bitmap wagonArt() {
    gs::Bitmap b(52, 22);
    b.rect(2, 2, 48, 4, 3);
    b.rect(4, 6, 44, 9, 1);
    b.rect(8, 8, 10, 5, 2);
    b.rect(22, 8, 10, 5, 4);
    b.rect(36, 8, 8, 5, 2);
    b.ellipse(12, 17, 4.f, 4.f, 6);
    b.ellipse(40, 17, 4.f, 4.f, 6);
    b.ellipse(12, 17, 1.6f, 1.6f, 5);
    b.ellipse(40, 17, 1.6f, 1.6f, 5);
    return b;
}

gs::Bitmap horseArt() {
    gs::Bitmap b(36, 22);
    b.ellipse(16, 11, 10.f, 5.f, 1);
    b.ellipse(28, 7, 4.f, 3.2f, 1);
    b.rect(30, 4, 2, 3, 2);
    b.rect(8, 13, 2, 7, 1);
    b.rect(13, 13, 2, 7, 1);
    b.rect(20, 13, 2, 7, 1);
    b.rect(25, 13, 2, 7, 1);
    b.rect(6, 10, 6, 2, 3);
    b.set(29, 6, 6);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(18, 24);
    b.rect(8, 0, 2, 5, 3);
    b.poly({{3, 8}, {15, 8}, {16, 16}, {2, 16}}, 1);
    b.poly({{5, 9}, {13, 9}, {14, 14}, {4, 14}}, 2);
    b.rect(2, 16, 14, 2, 4);
    b.ellipse(9, 18, 1.4f, 1.6f, 3);
    b.rect(7, 5, 4, 2, 4);
    return b;
}

gs::Bitmap personArt() {
    gs::Bitmap b(10, 16);
    b.ellipse(5, 3, 2.2f, 2.2f, 3);
    b.rect(3, 6, 4, 5, 1);
    b.rect(2, 7, 1, 4, 2);
    b.rect(7, 7, 1, 4, 2);
    b.rect(3, 11, 1, 4, 4);
    b.rect(6, 11, 1, 4, 4);
    return b;
}

gs::Bitmap pennantArt() {
    gs::Bitmap b(12, 16);
    b.rect(0, 0, 1, 16, 3);
    b.poly({{1, 1}, {11, 4}, {1, 8}}, 1);
    b.poly({{2, 3}, {8, 4}, {2, 6}}, 2);
    return b;
}

gs::Bitmap confettiArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 1, 3, 2, 1);
    b.set(3, 0, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7.f, 2.f, 1);
    return b;
}

gs::Bitmap blockArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t z = 0;
    const uint16_t hud[] = {z, gs::rgb4(15, 15, 13), gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 5), gs::rgb4(15, 12, 2),
                            gs::rgb4(13, 3, 3), gs::rgb4(3, 8, 4), gs::rgb4(4, 6, 10), 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(2, 2, 3)};
    const uint16_t major[] = {z, gs::rgb4(12, 2, 3), gs::rgb4(15, 14, 10), gs::rgb4(13, 9, 6), gs::rgb4(15, 12, 2),
                              gs::rgb4(3, 3, 8), gs::rgb4(2, 2, 2), gs::rgb4(15, 15, 15)};
    const uint16_t wagon[] = {z, gs::rgb4(11, 3, 4), gs::rgb4(15, 14, 11), gs::rgb4(14, 8, 2), gs::rgb4(4, 7, 12),
                              gs::rgb4(15, 12, 3), gs::rgb4(2, 2, 2), gs::rgb4(6, 4, 3)};
    const uint16_t horse[] = {z, gs::rgb4(8, 5, 2), gs::rgb4(4, 2, 1), gs::rgb4(12, 2, 3), gs::rgb4(15, 14, 12),
                              gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)};
    const uint16_t bell[] = {z, gs::rgb4(15, 12, 3), gs::rgb4(12, 9, 2), gs::rgb4(6, 5, 2), gs::rgb4(15, 14, 8)};
    const uint16_t crowd[] = {z, gs::rgb4(3, 5, 10), gs::rgb4(10, 2, 3), gs::rgb4(13, 9, 6), gs::rgb4(2, 2, 4)};
    const uint16_t flag[] = {z, gs::rgb4(13, 2, 3), gs::rgb4(15, 14, 12), gs::rgb4(5, 3, 2), gs::rgb4(2, 6, 3)};
    const uint16_t gold[] = {z, gs::rgb4(15, 12, 2), gs::rgb4(10, 7, 1), gs::rgb4(15, 15, 12)};
    const uint16_t red[] = {z, gs::rgb4(14, 3, 3), gs::rgb4(8, 1, 1)};
    const uint16_t cream[] = {z, gs::rgb4(15, 14, 11), gs::rgb4(10, 8, 5)};
    const uint16_t ink[] = {z, gs::rgb4(15, 15, 13), gs::rgb4(3, 3, 4)};
    const uint16_t plaza[] = {z, gs::rgb4(10, 8, 5), gs::rgb4(7, 6, 4)};
    const uint16_t drum[] = {z, gs::rgb4(6, 3, 2), gs::rgb4(12, 8, 3)};
    const uint16_t sky[] = {z, gs::rgb4(7, 9, 13)};
    const uint16_t conf[] = {z, gs::rgb4(15, 13, 3), gs::rgb4(13, 3, 5), gs::rgb4(4, 10, 6)};
    const uint16_t shade[] = {z, gs::rgb4(0, 0, 0)};
    setPal(vdp, PAL_HUD, hud, 16);
    setPal(vdp, PAL_MAJOR, major, 8);
    setPal(vdp, PAL_FLOAT, wagon, 8);
    setPal(vdp, PAL_HORSE, horse, 7);
    setPal(vdp, PAL_BELL, bell, 5);
    setPal(vdp, PAL_CROWD, crowd, 5);
    setPal(vdp, PAL_FLAG, flag, 5);
    setPal(vdp, PAL_GOLD, gold, 4);
    setPal(vdp, PAL_RED, red, 3);
    setPal(vdp, PAL_CREAM, cream, 3);
    setPal(vdp, PAL_INK, ink, 3);
    setPal(vdp, PAL_PLAZA, plaza, 3);
    setPal(vdp, PAL_DRUM, drum, 3);
    setPal(vdp, PAL_SKY, sky, 2);
    setPal(vdp, PAL_CONF, conf, 4);
    setPal(vdp, PAL_SHADE, shade, 2);
    loadFont(vdp, art);
    art.major = gs::uploadMipped(vdp, majorArt());
    art.wagon = gs::uploadMipped(vdp, wagonArt());
    art.horse = gs::uploadMipped(vdp, horseArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.person = gs::uploadMipped(vdp, personArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.confetti = gs::uploadMipped(vdp, confettiArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.block = gs::uploadMipped(vdp, blockArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("PARADE BELL", {2, 1, 2, 0, 1}));
}

}  // namespace paradebell
