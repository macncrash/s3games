#include "game/art.h"

#include <cstdint>

namespace paradechime {
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
    gs::Bitmap b(18, 30);
    b.rect(7, 0, 4, 5, 4);
    b.ellipse(9, 7, 3.4f, 3.2f, 3);
    b.set(7, 6, 6);
    b.set(11, 6, 6);
    b.rect(5, 10, 8, 10, 1);
    b.rect(7, 11, 4, 8, 2);
    b.rect(2, 11, 3, 7, 1);
    b.rect(13, 11, 3, 7, 1);
    b.rect(5, 20, 2, 7, 5);
    b.rect(11, 20, 2, 7, 5);
    b.rect(4, 26, 4, 2, 6);
    b.rect(10, 26, 4, 2, 6);
    b.line(14, 12, 17, 2, 4, 1.2f);
    return b;
}

gs::Bitmap wagonArt() {
    gs::Bitmap b(48, 20);
    b.rect(1, 1, 46, 3, 4);
    b.rect(3, 4, 42, 9, 1);
    b.rect(6, 6, 8, 5, 2);
    b.rect(18, 6, 10, 5, 3);
    b.rect(32, 6, 8, 5, 2);
    b.ellipse(12, 15, 3.6f, 3.6f, 6);
    b.ellipse(36, 15, 3.6f, 3.6f, 6);
    b.ellipse(12, 15, 1.4f, 1.4f, 5);
    b.ellipse(36, 15, 1.4f, 1.4f, 5);
    return b;
}

gs::Bitmap horseArt() {
    gs::Bitmap b(34, 20);
    b.ellipse(14, 10, 9.f, 4.6f, 1);
    b.ellipse(26, 6, 4.2f, 3.f, 1);
    b.rect(28, 3, 2, 3, 2);
    b.rect(6, 12, 2, 6, 1);
    b.rect(11, 12, 2, 6, 1);
    b.rect(18, 12, 2, 6, 1);
    b.rect(23, 12, 2, 6, 1);
    b.rect(4, 8, 5, 2, 3);
    b.set(27, 5, 6);
    return b;
}

gs::Bitmap drumArt() {
    gs::Bitmap b(22, 20);
    b.ellipse(11, 8, 8.f, 4.f, 1);
    b.rect(3, 8, 16, 6, 2);
    b.ellipse(11, 14, 8.f, 3.f, 3);
    b.line(4, 6, 8, 1, 4, 1.f);
    b.line(18, 6, 14, 1, 4, 1.f);
    b.ellipse(11, 8, 3.f, 1.4f, 4);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(28, 40);
    b.poly({{14, 0}, {26, 10}, {2, 10}}, 2);
    b.rect(4, 10, 20, 26, 1);
    b.ellipse(14, 18, 6.f, 6.f, 3);
    b.ellipse(14, 18, 4.2f, 4.2f, 4);
    b.rect(12, 16, 1, 5, 5);
    b.rect(13, 17, 4, 1, 5);
    b.rect(10, 28, 8, 8, 2);
    b.rect(12, 32, 4, 4, 6);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(14, 16);
    b.rect(6, 0, 2, 3, 3);
    b.poly({{2, 5}, {12, 5}, {13, 11}, {1, 11}}, 1);
    b.rect(1, 11, 12, 2, 4);
    b.ellipse(7, 13, 1.2f, 1.4f, 2);
    return b;
}

gs::Bitmap personArt() {
    gs::Bitmap b(10, 16);
    b.ellipse(5, 3, 2.1f, 2.1f, 3);
    b.rect(3, 6, 4, 5, 1);
    b.rect(2, 7, 1, 4, 2);
    b.rect(7, 7, 1, 4, 2);
    b.rect(3, 11, 1, 4, 4);
    b.rect(6, 11, 1, 4, 4);
    return b;
}

gs::Bitmap pennantArt() {
    gs::Bitmap b(14, 18);
    b.rect(0, 0, 1, 18, 3);
    b.poly({{1, 1}, {13, 5}, {1, 9}}, 1);
    b.poly({{2, 3}, {9, 5}, {2, 7}}, 2);
    return b;
}

gs::Bitmap confettiArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 1, 3, 1, 1);
    b.set(2, 0, 2);
    b.set(1, 3, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7.f, 2.f, 1);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(8, 4);
    b.rect(0, 0, 8, 4, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t z = 0;
    const uint16_t hud[] = {z, gs::rgb4(15, 15, 13), gs::rgb4(7, 7, 8), gs::rgb4(3, 3, 4), gs::rgb4(15, 12, 3),
                            gs::rgb4(13, 3, 3), gs::rgb4(3, 9, 5), gs::rgb4(5, 7, 12), 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(2, 2, 3)};
    const uint16_t major[] = {z, gs::rgb4(2, 4, 10), gs::rgb4(15, 14, 11), gs::rgb4(13, 9, 6), gs::rgb4(15, 13, 3),
                              gs::rgb4(3, 3, 6), gs::rgb4(2, 2, 2), gs::rgb4(15, 15, 15)};
    const uint16_t wagon[] = {z, gs::rgb4(10, 3, 4), gs::rgb4(15, 14, 12), gs::rgb4(4, 8, 12), gs::rgb4(14, 9, 2),
                              gs::rgb4(8, 6, 3), gs::rgb4(2, 2, 2), gs::rgb4(6, 4, 2)};
    const uint16_t horse[] = {z, gs::rgb4(7, 4, 2), gs::rgb4(3, 2, 1), gs::rgb4(12, 2, 3), gs::rgb4(15, 14, 12),
                              gs::rgb4(1, 1, 1), gs::rgb4(2, 2, 2)};
    const uint16_t bell[] = {z, gs::rgb4(15, 13, 4), gs::rgb4(10, 8, 2), gs::rgb4(5, 4, 2), gs::rgb4(15, 15, 10)};
    const uint16_t crowd[] = {z, gs::rgb4(4, 6, 11), gs::rgb4(11, 3, 3), gs::rgb4(13, 9, 6), gs::rgb4(2, 2, 4)};
    const uint16_t flag[] = {z, gs::rgb4(12, 2, 3), gs::rgb4(15, 14, 12), gs::rgb4(4, 3, 2), gs::rgb4(2, 7, 3)};
    const uint16_t gold[] = {z, gs::rgb4(15, 12, 2), gs::rgb4(9, 6, 1), gs::rgb4(15, 15, 12)};
    const uint16_t tower[] = {z, gs::rgb4(9, 8, 6), gs::rgb4(6, 5, 4), gs::rgb4(12, 10, 7), gs::rgb4(14, 13, 9),
                              gs::rgb4(2, 2, 3), gs::rgb4(3, 5, 8)};
    const uint16_t cream[] = {z, gs::rgb4(15, 14, 11), gs::rgb4(9, 8, 5)};
    const uint16_t ink[] = {z, gs::rgb4(15, 15, 14), gs::rgb4(3, 3, 5)};
    const uint16_t drum[] = {z, gs::rgb4(12, 4, 3), gs::rgb4(6, 2, 2), gs::rgb4(14, 10, 4), gs::rgb4(15, 14, 8)};
    const uint16_t stone[] = {z, gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 4)};
    const uint16_t sky[] = {z, gs::rgb4(6, 8, 12)};
    const uint16_t conf[] = {z, gs::rgb4(15, 13, 3), gs::rgb4(12, 3, 5), gs::rgb4(3, 10, 6)};
    const uint16_t shade[] = {z, gs::rgb4(0, 0, 0)};
    setPal(vdp, PAL_HUD, hud, 16);
    setPal(vdp, PAL_MAJOR, major, 8);
    setPal(vdp, PAL_WAGON, wagon, 8);
    setPal(vdp, PAL_HORSE, horse, 7);
    setPal(vdp, PAL_BELL, bell, 5);
    setPal(vdp, PAL_CROWD, crowd, 5);
    setPal(vdp, PAL_FLAG, flag, 5);
    setPal(vdp, PAL_GOLD, gold, 4);
    setPal(vdp, PAL_TOWER, tower, 7);
    setPal(vdp, PAL_CREAM, cream, 3);
    setPal(vdp, PAL_INK, ink, 3);
    setPal(vdp, PAL_DRUM, drum, 5);
    setPal(vdp, PAL_STONE, stone, 3);
    setPal(vdp, PAL_SKY, sky, 2);
    setPal(vdp, PAL_CONF, conf, 4);
    setPal(vdp, PAL_SHADE, shade, 2);
    loadFont(vdp, art);
    art.major = gs::uploadMipped(vdp, majorArt());
    art.wagon = gs::uploadMipped(vdp, wagonArt());
    art.horse = gs::uploadMipped(vdp, horseArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.person = gs::uploadMipped(vdp, personArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.confetti = gs::uploadMipped(vdp, confettiArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("PARADE CHIME", {2, 1, 2, 0, 1}));
}

}  // namespace paradechime
