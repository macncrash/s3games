#include "art.h"

#include <cmath>
#include <initializer_list>

namespace vanbox {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap vanArt() {
    gs::Bitmap b(96, 40);
    b.poly({{6.f, 22.f}, {18.f, 12.f}, {46.f, 10.f}, {58.f, 16.f}, {88.f, 16.f}, {90.f, 22.f}, {88.f, 32.f}, {8.f, 32.f}}, 1);
    b.rect(10, 22, 76, 10, 2);
    b.rect(20, 13, 22, 8, 3);
    b.rect(44, 14, 12, 7, 4);
    b.rect(62, 18, 16, 6, 5);
    b.rect(8, 24, 6, 6, 6);
    b.ellipse(24, 32, 6, 6, 7);
    b.ellipse(72, 32, 6, 6, 7);
    b.ellipse(24, 32, 2.4f, 2.4f, 8);
    b.ellipse(72, 32, 2.4f, 2.4f, 8);
    b.rect(12, 26, 4, 2, 9);
    gs::TextStyle st{1, 10, 0, 0, 0};
    b.blit(gs::textBitmap("MAIL", st), 62, 19);
    b.outline(11, false);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2, 2, 2);
    return b;
}

gs::Bitmap smokeArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4, 1);
    b.ellipse(4, 5, 2, 2, 2);
    return b;
}

gs::Bitmap asphaltArt() {
    gs::Bitmap b(48, 20);
    b.rect(0, 0, 48, 20, 1);
    b.rect(0, 0, 48, 3, 2);
    b.rect(0, 17, 48, 3, 3);
    for (int x = 2; x < 46; x += 10) b.rect(x, 9, 5, 2, 4);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(16, 12);
    b.rect(0, 0, 16, 12, 1);
    b.poly({{0.f, 12.f}, {6.f, 0.f}, {10.f, 0.f}, {4.f, 12.f}}, 2);
    b.poly({{8.f, 12.f}, {14.f, 0.f}, {16.f, 0.f}, {12.f, 12.f}}, 2);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 40);
    b.rect(3, 4, 4, 36, 1);
    b.rect(1, 1, 8, 6, 2);
    b.rect(2, 18, 6, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap officeArt() {
    gs::Bitmap b(64, 48);
    b.rect(4, 16, 56, 32, 1);
    b.poly({{2.f, 18.f}, {32.f, 4.f}, {62.f, 18.f}}, 2);
    b.rect(26, 28, 12, 20, 3);
    b.rect(10, 22, 10, 8, 4);
    b.rect(44, 22, 10, 8, 4);
    b.rect(30, 8, 4, 8, 5);
    b.rect(8, 36, 8, 6, 6);
    b.outline(7, false);
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
    b.outline(6, false);
    return b;
}

gs::Bitmap crewArt(bool step) {
    gs::Bitmap b(18, 30);
    b.ellipse(9, 6, 4, 4, 1);
    b.rect(6, 11, 6, 9, 2);
    b.line(6, 13, step ? 2.f : 4.f, 18, 3, 1.5f);
    b.line(12, 13, step ? 16.f : 14.f, 18, 3, 1.5f);
    b.line(7, 20, step ? 4.f : 6.f, 29, 4, 1.6f);
    b.line(11, 20, step ? 14.f : 12.f, 29, 4, 1.6f);
    b.rect(4, 3, 10, 3, 5);
    b.set(7, 6, 6);
    b.set(11, 6, 6);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 36);
    b.rect(12, 20, 4, 16, 1);
    b.ellipse(14, 12, 12, 10, 2);
    b.ellipse(10, 14, 6, 5, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 32);
    b.rect(5, 10, 2, 22, 1);
    b.ellipse(6, 6, 4, 4, 2);
    b.rect(2, 28, 8, 3, 3);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(16, 14);
    b.ellipse(8, 8, 6, 5, 1);
    b.rect(5, 2, 6, 4, 2);
    b.line(5, 4, 11, 4, 3, 1.2f);
    return b;
}

gs::Bitmap birdArt(bool up) {
    gs::Bitmap b(16, 8);
    if (up) {
        b.line(1, 6, 8, 2, 1, 1.4f);
        b.line(8, 2, 15, 6, 1, 1.4f);
    } else {
        b.line(1, 2, 8, 6, 1, 1.4f);
        b.line(8, 6, 15, 2, 1, 1.4f);
    }
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

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int fill, int edge) {
    gs::TextStyle st{scale, fill, edge, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 13), gs::rgb4(8, 9, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_VAN, {0, gs::rgb4(15, 12, 2), gs::rgb4(14, 13, 10), gs::rgb4(6, 10, 13), gs::rgb4(3, 5, 7),
                          gs::rgb4(12, 3, 3), gs::rgb4(15, 15, 8), gs::rgb4(2, 2, 2), gs::rgb4(9, 9, 9),
                          gs::rgb4(8, 6, 2), gs::rgb4(1, 1, 2), gs::rgb4(4, 3, 1), 0, 0, 0, ink});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(4, 4, 5), gs::rgb4(6, 6, 6), gs::rgb4(2, 2, 3), gs::rgb4(14, 12, 3),
                           0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(7, 7, 8), gs::rgb4(14, 12, 3), gs::rgb4(12, 4, 3), gs::rgb4(2, 2, 3),
                           0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BOX, {0, gs::rgb4(5, 5, 4), gs::rgb4(15, 13, 2), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, ink});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(12, 8, 5), gs::rgb4(3, 5, 10), gs::rgb4(10, 10, 9), gs::rgb4(2, 2, 3),
                           gs::rgb4(4, 6, 12), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(12, 10, 6), gs::rgb4(15, 15, 13), gs::rgb4(3, 3, 3), gs::rgb4(8, 7, 5),
                            gs::rgb4(12, 3, 2), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(8, 8, 8), gs::rgb4(13, 13, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 8), gs::rgb4(2, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 4), gs::rgb4(4, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 4), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_OFFICE, {0, gs::rgb4(9, 6, 4), gs::rgb4(6, 3, 2), gs::rgb4(3, 3, 4), gs::rgb4(11, 13, 14),
                             gs::rgb4(5, 5, 5), gs::rgb4(12, 4, 3), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(5, 3, 2), gs::rgb4(3, 8, 3), gs::rgb4(5, 11, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MAIL, {0, gs::rgb4(11, 8, 4), gs::rgb4(14, 12, 8), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});

    art.van = gs::uploadMipped(vdp, vanArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.smoke = gs::uploadMipped(vdp, smokeArt());
    art.asphalt = gs::uploadMipped(vdp, asphaltArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.office = gs::uploadMipped(vdp, officeArt());
    for (int i = 0; i < 8; i++) art.clock[i] = gs::uploadMipped(vdp, clockArt(i));
    art.crew[0] = gs::uploadMipped(vdp, crewArt(false));
    art.crew[1] = gs::uploadMipped(vdp, crewArt(true));
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(true));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(false));
    art.title = words(vdp, "MAIL VAN", 3, 1, 2);
    art.boxWord = words(vdp, "BOX", 3, 1, 2);
    art.stopped = words(vdp, "STOPPED", 2, 1, 2);
    art.outside = words(vdp, "OUTSIDE", 2, 1, 2);
    art.late = words(vdp, "TOO LATE", 2, 1, 2);
    art.paused = words(vdp, "HELD", 2, 1, 2);
    loadFont(vdp, art);
}

}  // namespace vanbox
