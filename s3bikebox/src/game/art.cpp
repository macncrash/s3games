#include "art.h"

#include <cmath>
#include <initializer_list>

namespace bikebox {
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

gs::Bitmap bikeArt() {
    gs::Bitmap b(96, 56);
    b.line(18, 40, 48, 22, 1, 2.2f);
    b.line(48, 22, 78, 40, 1, 2.2f);
    b.line(48, 22, 44, 40, 1, 2.0f);
    b.line(30, 40, 62, 40, 2, 2.4f);
    b.rect(46, 16, 3, 10, 3);
    b.rect(42, 14, 12, 3, 3);
    b.ellipse(22, 42, 12, 12, 4);
    b.ellipse(74, 42, 12, 12, 4);
    b.ellipse(22, 42, 3, 3, 5);
    b.ellipse(74, 42, 3, 3, 5);
    b.line(70, 40, 84, 34, 6, 1.6f);
    b.outline(15, false);
    return b;
}

gs::Bitmap wheelArt(int spokes) {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(14, 14, 9, 9, 2);
    b.ellipse(14, 14, 2.4f, 2.4f, 3);
    float a0 = spokes ? 0.4f : 0.f;
    for (int i = 0; i < 4; i++) {
        float a = a0 + i * 0.785398f;
        b.line(14, 14, 14 + std::cos(a) * 9.f, 14 + std::sin(a) * 9.f, 4, 1.2f);
    }
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 10);
    b.ellipse(8, 5, 6, 3, 1);
    b.ellipse(5, 5, 2, 1.4f, 2);
    return b;
}

gs::Bitmap roadArt() {
    gs::Bitmap b(64, 28);
    b.rect(0, 0, 64, 28, 1);
    for (int x = 2; x < 62; x += 9) b.rect(x, 6, 4, 2, 2);
    b.rect(0, 0, 64, 3, 3);
    b.rect(0, 25, 64, 3, 4);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(28, 6);
    b.rect(0, 1, 28, 4, 1);
    b.rect(2, 2, 10, 2, 2);
    return b;
}

gs::Bitmap kerbArt() {
    gs::Bitmap b(32, 12);
    b.rect(0, 2, 32, 8, 1);
    for (int x = 0; x < 32; x += 8) b.rect(x, 2, 4, 8, 2);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 52);
    b.rect(4, 8, 4, 42, 1);
    b.rect(1, 2, 10, 8, 2);
    b.rect(3, 22, 6, 3, 3);
    b.rect(5, 44, 2, 6, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap hatchArt() {
    gs::Bitmap b(20, 10);
    b.rect(0, 2, 20, 6, 1);
    b.rect(2, 3, 6, 3, 2);
    b.rect(12, 3, 6, 3, 2);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(52, 40);
    b.rect(4, 16, 44, 22, 1);
    b.poly({{2.f, 16.f}, {26.f, 4.f}, {50.f, 16.f}}, 2);
    b.rect(22, 24, 10, 14, 3);
    b.rect(10, 20, 8, 6, 4);
    b.rect(34, 20, 8, 6, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap clockArt(int hand) {
    gs::Bitmap b(32, 32);
    b.ellipse(16, 16, 13, 13, 1);
    b.ellipse(16, 16, 11, 11, 2);
    b.ellipse(16, 16, 2, 2, 3);
    for (int i = 0; i < 12; i++) {
        float a = i * 0.523599f - 1.5708f;
        b.set(int(16 + std::cos(a) * 9.f), int(16 + std::sin(a) * 9.f), 4);
    }
    float a = hand * 0.785398f - 1.5708f;
    b.line(16, 16, 16 + std::cos(a) * 9.f, 16 + std::sin(a) * 9.f, 5, 1.6f);
    b.outline(15, false);
    return b;
}

gs::Bitmap crewArt(bool step) {
    gs::Bitmap b(22, 34);
    b.ellipse(11, 6, 4, 4, 1);
    b.rect(8, 11, 6, 10, 2);
    b.line(8, 13, step ? 3.f : 5.f, 20, 3, 1.5f);
    b.line(14, 13, step ? 19.f : 17.f, 20, 3, 1.5f);
    b.line(9, 22, step ? 6.f : 8.f, 32, 4, 1.6f);
    b.line(13, 22, step ? 16.f : 14.f, 32, 4, 1.6f);
    b.set(9, 5, 5);
    b.set(13, 5, 5);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 36);
    b.rect(5, 10, 2, 24, 1);
    b.ellipse(6, 7, 4, 4, 2);
    b.ellipse(6, 7, 2, 2, 3);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 36);
    b.rect(12, 18, 4, 16, 3);
    b.ellipse(14, 14, 11, 10, 1);
    b.ellipse(10, 12, 5, 4, 2);
    b.outline(15, false);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12), gs::rgb4(6, 6, 6)});
    setPal(vdp, PAL_BIKE, {0, gs::rgb4(2, 3, 4), gs::rgb4(8, 8, 7), gs::rgb4(12, 4, 2), gs::rgb4(1, 1, 1),
                           gs::rgb4(10, 10, 9), gs::rgb4(13, 11, 4)});
    setPal(vdp, PAL_RIDER, {0, gs::rgb4(12, 3, 2), gs::rgb4(14, 10, 6), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(3, 3, 4), gs::rgb4(5, 5, 5), gs::rgb4(8, 8, 6), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_BOX, {0, gs::rgb4(13, 12, 4), gs::rgb4(15, 15, 8), gs::rgb4(6, 5, 1)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(9, 9, 8), gs::rgb4(14, 12, 3), gs::rgb4(12, 3, 2), gs::rgb4(4, 4, 4)});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(13, 9, 6), gs::rgb4(4, 6, 10), gs::rgb4(13, 11, 4), gs::rgb4(2, 2, 3),
                           gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(12, 10, 6), gs::rgb4(15, 14, 10), gs::rgb4(2, 2, 2), gs::rgb4(8, 2, 2),
                            gs::rgb4(14, 3, 2)});
    setPal(vdp, PAL_TOWN, {0, gs::rgb4(2, 6, 3), gs::rgb4(4, 9, 4), gs::rgb4(6, 4, 2), gs::rgb4(8, 8, 7),
                           gs::rgb4(14, 12, 4), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(8, 7, 5), gs::rgb4(12, 11, 8)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(6, 14, 6), gs::rgb4(12, 15, 10)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 12, 6)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(14, 12, 5), gs::rgb4(8, 10, 14)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(10, 12, 14)});

    art.bike = gs::uploadMipped(vdp, bikeArt());
    art.wheel[0] = gs::uploadMipped(vdp, wheelArt(0));
    art.wheel[1] = gs::uploadMipped(vdp, wheelArt(1));
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.road = gs::uploadMipped(vdp, roadArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.kerb = gs::uploadMipped(vdp, kerbArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.hatch = gs::uploadMipped(vdp, hatchArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    for (int i = 0; i < 8; i++) art.clock[i] = gs::uploadMipped(vdp, clockArt(i));
    art.crew[0] = gs::uploadMipped(vdp, crewArt(false));
    art.crew[1] = gs::uploadMipped(vdp, crewArt(true));
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.title = gs::uploadMipped(vdp, wordArt("BIKE", 3, 1));
    art.boxWord = gs::uploadMipped(vdp, wordArt("BOX", 3, 1));
    art.stopped = gs::uploadMipped(vdp, wordArt("STOPPED", 2, 1));
    art.outside = gs::uploadMipped(vdp, wordArt("OUTSIDE", 2, 1));
    art.late = gs::uploadMipped(vdp, wordArt("TOO LATE", 2, 1));
    art.paused = gs::uploadMipped(vdp, wordArt("PAUSED", 2, 1));
    loadFont(vdp, art);
}

}  // namespace bikebox
