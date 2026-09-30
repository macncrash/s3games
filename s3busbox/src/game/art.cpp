#include "art.h"

#include <cmath>
#include <initializer_list>

namespace busbox {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 1));
}

gs::Bitmap busArt() {
    gs::Bitmap b(128, 52);
    b.rect(8, 16, 108, 26, 1);
    b.poly({{8, 16}, {22, 6}, {104, 6}, {116, 16}}, 1);
    b.rect(18, 8, 22, 8, 2);
    b.rect(42, 8, 22, 8, 2);
    b.rect(66, 8, 22, 8, 2);
    b.rect(90, 8, 16, 8, 2);
    b.rect(24, 22, 16, 12, 3);
    b.rect(44, 22, 16, 12, 3);
    b.rect(64, 22, 16, 12, 3);
    b.rect(86, 20, 14, 18, 4);
    b.rect(90, 24, 6, 10, 5);
    b.rect(10, 28, 8, 6, 6);
    b.rect(108, 20, 10, 8, 7);
    b.rect(6, 38, 112, 6, 8);
    b.ellipse(28, 44, 8, 8, 9);
    b.ellipse(96, 44, 8, 8, 9);
    b.rect(46, 2, 28, 5, 10);
    b.outline(15, false);
    return b;
}

gs::Bitmap wheelArt(int phase) {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 1);
    b.ellipse(11, 11, 6, 6, 2);
    b.ellipse(11, 11, 2, 2, 3);
    float a0 = phase ? 0.5f : 0.05f;
    for (int i = 0; i < 3; i++) {
        float a = a0 + i * 1.0472f;
        b.line(11, 11, 11 + std::cos(a) * 6.f, 11 + std::sin(a) * 6.f, 4, 1.2f);
    }
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 10);
    b.ellipse(8, 6, 6, 3, 1);
    b.ellipse(5, 5, 2, 1.5f, 2);
    return b;
}

gs::Bitmap roadArt() {
    gs::Bitmap b(48, 24);
    b.rect(0, 0, 48, 24, 1);
    for (int x = 3; x < 46; x += 10) b.rect(x, 10, 5, 2, 2);
    b.rect(0, 0, 48, 2, 3);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(24, 8);
    b.rect(0, 1, 24, 6, 1);
    b.rect(2, 3, 8, 2, 2);
    return b;
}

gs::Bitmap kerbArt() {
    gs::Bitmap b(32, 10);
    b.rect(0, 2, 32, 6, 1);
    for (int x = 0; x < 32; x += 8) b.rect(x, 2, 4, 6, 2);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(14, 48);
    b.rect(5, 10, 4, 36, 1);
    b.rect(1, 2, 12, 10, 2);
    b.rect(3, 14, 8, 4, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap hatchArt() {
    gs::Bitmap b(18, 10);
    b.rect(1, 2, 16, 6, 1);
    b.line(2, 8, 16, 2, 2, 1.2f);
    b.line(2, 2, 16, 8, 2, 1.2f);
    return b;
}

gs::Bitmap shelterArt() {
    gs::Bitmap b(56, 44);
    b.rect(4, 10, 48, 4, 1);
    b.rect(6, 14, 4, 26, 2);
    b.rect(46, 14, 4, 26, 2);
    b.rect(10, 16, 34, 16, 3);
    b.rect(12, 32, 22, 6, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap barrierArt() {
    gs::Bitmap b(16, 40);
    b.rect(6, 8, 4, 30, 1);
    for (int y = 8; y < 36; y += 6) b.rect(4, y, 8, 3, (y / 6) & 1 ? 2 : 3);
    b.rect(2, 4, 12, 4, 2);
    return b;
}

gs::Bitmap blockArt() {
    gs::Bitmap b(40, 36);
    b.rect(2, 12, 36, 22, 1);
    b.rect(6, 16, 8, 8, 2);
    b.rect(18, 16, 8, 8, 2);
    b.rect(30, 16, 6, 8, 3);
    b.poly({{2, 12}, {8, 4}, {34, 4}, {38, 12}}, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 40);
    b.rect(7, 12, 2, 26, 1);
    b.ellipse(8, 8, 5, 4, 2);
    b.rect(4, 10, 8, 2, 3);
    return b;
}

gs::Bitmap riderArt(int wave) {
    gs::Bitmap b(16, 28);
    b.ellipse(8, 6, 4, 4, 1);
    b.rect(5, 11, 6, 10, 2);
    b.line(5, 14, 1, 20 + wave, 2, 1.4f);
    b.line(11, 14, 15, 18, 2, 1.4f);
    b.line(6, 21, 4, 27, 3, 1.4f);
    b.line(10, 21, 12, 27, 3, 1.4f);
    return b;
}

gs::Bitmap wordArt(const char* s, int scale, int color) {
    gs::TextStyle st{scale, color, 0, 0, 1};
    gs::Bitmap t = gs::textBitmap(s, st);
    gs::Bitmap b(t.w + 8, t.h + 8);
    b.blit(t, 4, 4);
    b.outline(15, false);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
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
        art.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12), gs::rgb4(5, 5, 6)});
    setPal(vdp, PAL_BUS, {0, gs::rgb4(14, 11, 2), gs::rgb4(8, 10, 12), gs::rgb4(6, 8, 10), gs::rgb4(3, 4, 5),
                          gs::rgb4(12, 12, 8), gs::rgb4(13, 4, 2), gs::rgb4(15, 14, 6), gs::rgb4(4, 4, 4),
                          gs::rgb4(1, 1, 1), gs::rgb4(2, 3, 6)});
    setPal(vdp, PAL_GLASS, {0, gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 6), gs::rgb4(12, 12, 11), gs::rgb4(8, 8, 7)});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(3, 3, 4), gs::rgb4(12, 11, 4), gs::rgb4(6, 6, 5)});
    setPal(vdp, PAL_BOX, {0, gs::rgb4(13, 11, 2), gs::rgb4(15, 15, 8), gs::rgb4(5, 4, 1)});
    setPal(vdp, PAL_STOP, {0, gs::rgb4(8, 8, 8), gs::rgb4(14, 12, 2), gs::rgb4(12, 3, 2), gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_TOWN, {0, gs::rgb4(7, 6, 6), gs::rgb4(10, 12, 13), gs::rgb4(4, 5, 7), gs::rgb4(9, 5, 4),
                           gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_END, {0, gs::rgb4(6, 6, 6), gs::rgb4(14, 13, 12), gs::rgb4(13, 3, 2)});
    setPal(vdp, PAL_FOLK, {0, gs::rgb4(13, 10, 7), gs::rgb4(4, 6, 10), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 5)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 14, 6), gs::rgb4(3, 6, 3)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(6, 2, 1)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(14, 12, 3), gs::rgb4(4, 3, 1)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(14, 13, 6), gs::rgb4(8, 8, 6), gs::rgb4(3, 3, 3)});

    art.bus = gs::uploadMipped(vdp, busArt());
    art.wheel[0] = gs::uploadMipped(vdp, wheelArt(0));
    art.wheel[1] = gs::uploadMipped(vdp, wheelArt(1));
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.road = gs::uploadMipped(vdp, roadArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.kerb = gs::uploadMipped(vdp, kerbArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.hatch = gs::uploadMipped(vdp, hatchArt());
    art.shelter = gs::uploadMipped(vdp, shelterArt());
    art.barrier = gs::uploadMipped(vdp, barrierArt());
    art.block = gs::uploadMipped(vdp, blockArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.rider = gs::uploadMipped(vdp, riderArt(0));
    art.title = gs::uploadMipped(vdp, wordArt("BUS", 3, 1));
    art.boxWord = gs::uploadMipped(vdp, wordArt("BOX", 3, 1));
    art.stopped = gs::uploadMipped(vdp, wordArt("IN", 3, 1));
    art.missed = gs::uploadMipped(vdp, wordArt("END", 3, 1));
    art.outside = gs::uploadMipped(vdp, wordArt("OUT", 3, 1));
    art.paused = gs::uploadMipped(vdp, wordArt("HOLD", 2, 1));
    loadFont(vdp, art);
}

}  // namespace busbox
