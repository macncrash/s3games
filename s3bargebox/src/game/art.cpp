#include "art.h"

#include <cmath>
#include <initializer_list>

namespace bargebox {
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

gs::Bitmap bargeArt() {
    gs::Bitmap b(128, 40);
    b.poly({{4.f, 22.f}, {112.f, 20.f}, {124.f, 28.f}, {116.f, 36.f}, {8.f, 36.f}, {2.f, 28.f}}, 2);
    b.poly({{10.f, 28.f}, {114.f, 27.f}, {118.f, 35.f}, {10.f, 35.f}}, 3);
    b.rect(16, 16, 86, 8, 1);
    b.rect(22, 10, 28, 8, 4);
    b.rect(54, 11, 22, 7, 5);
    b.rect(26, 12, 7, 5, 6);
    b.rect(36, 12, 7, 5, 6);
    b.rect(80, 8, 6, 12, 7);
    b.rect(78, 4, 10, 5, 8);
    b.ellipse(20, 24, 3.f, 3.f, 9);
    b.ellipse(108, 24, 3.f, 3.f, 9);
    b.rect(48, 30, 28, 2, 10);
    gs::TextStyle st{1, 11, 0, 0, 0};
    b.blit(gs::textBitmap("IDA", st), 50, 18);
    b.outline(15, false);
    return b;
}

gs::Bitmap bankArt() {
    gs::Bitmap b(64, 28);
    b.rect(0, 10, 64, 18, 1);
    b.poly({{0.f, 14.f}, {12.f, 6.f}, {28.f, 12.f}, {44.f, 4.f}, {64.f, 12.f}, {64.f, 28.f}, {0.f, 28.f}}, 2);
    for (int x = 3; x < 60; x += 8) b.rect(x, 16, 5, 3, 3);
    b.rect(0, 24, 64, 4, 4);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(20, 24);
    b.line(4, 22, 6, 4, 1, 1.4f);
    b.line(10, 22, 8, 2, 2, 1.4f);
    b.line(15, 22, 16, 7, 1, 1.3f);
    b.ellipse(7, 5, 2.2f, 2.f, 3);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(14, 48);
    b.rect(5, 6, 4, 42, 1);
    b.rect(2, 2, 10, 7, 2);
    b.rect(4, 18, 6, 3, 3);
    b.rect(6, 40, 2, 6, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap padArt() {
    gs::Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 1);
    b.rect(1, 1, 14, 2, 2);
    b.rect(1, 13, 14, 2, 3);
    return b;
}

gs::Bitmap hatchArt() {
    gs::Bitmap b(16, 8);
    b.rect(0, 2, 16, 4, 1);
    b.rect(2, 3, 4, 2, 2);
    b.rect(10, 3, 4, 2, 2);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(56, 40);
    b.rect(6, 16, 44, 22, 1);
    b.poly({{2.f, 16.f}, {28.f, 4.f}, {54.f, 16.f}}, 2);
    b.rect(24, 24, 10, 14, 3);
    b.rect(12, 20, 8, 7, 4);
    b.rect(38, 20, 8, 7, 4);
    b.rect(26, 6, 4, 8, 5);
    b.outline(6, false);
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
    gs::Bitmap b(20, 32);
    b.ellipse(10, 6, 4, 4, 1);
    b.rect(7, 11, 6, 10, 2);
    b.line(7, 13, step ? 2.f : 4.f, 20, 3, 1.6f);
    b.line(13, 13, step ? 18.f : 16.f, 20, 3, 1.6f);
    b.line(8, 22, step ? 5.f : 7.f, 31, 4, 1.7f);
    b.line(12, 22, step ? 15.f : 13.f, 31, 4, 1.7f);
    b.set(8, 5, 5);
    b.set(12, 5, 5);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 36);
    b.rect(12, 20, 4, 16, 3);
    b.ellipse(14, 14, 10, 10, 1);
    b.ellipse(10, 12, 5, 5, 2);
    b.outline(4, false);
    return b;
}

gs::Bitmap birdArt(bool up) {
    gs::Bitmap b(22, 12);
    float tip = up ? 2.f : 9.f;
    b.line(11, 6, 2, tip, 1, 1.4f);
    b.line(11, 6, 20, tip, 1, 1.4f);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 28);
    b.rect(4, 8, 2, 20, 1);
    b.ellipse(5, 6, 4, 4, 2);
    b.ellipse(5, 6, 2, 2, 3);
    return b;
}

gs::Bitmap wakeArt() {
    gs::Bitmap b(18, 8);
    b.ellipse(9, 4, 7, 3, 1);
    b.ellipse(6, 4, 2, 1.2f, 2);
    return b;
}

gs::Bitmap smokeArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4, 1);
    b.ellipse(4, 5, 2, 2, 2);
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
    setPal(vdp, PAL_BARGE, {0, gs::rgb4(12, 8, 4), gs::rgb4(4, 6, 5), gs::rgb4(2, 3, 3), gs::rgb4(9, 10, 8),
                            gs::rgb4(6, 7, 6), gs::rgb4(14, 12, 6), gs::rgb4(3, 3, 3), gs::rgb4(8, 8, 8),
                            gs::rgb4(13, 10, 3), gs::rgb4(11, 4, 3), gs::rgb4(15, 14, 10), gs::rgb4(7, 4, 2),
                            0, 0, ink});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(5, 7, 4), gs::rgb4(3, 9, 4), gs::rgb4(7, 6, 4), gs::rgb4(3, 4, 3),
                           gs::rgb4(10, 9, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(7, 7, 7), gs::rgb4(14, 12, 3), gs::rgb4(12, 4, 3), gs::rgb4(4, 4, 4),
                           gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BOX, {0, gs::rgb4(14, 12, 3), gs::rgb4(15, 15, 12), gs::rgb4(8, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, ink});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(12, 8, 5), gs::rgb4(4, 5, 8), gs::rgb4(9, 9, 8), gs::rgb4(3, 3, 4),
                           gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(12, 9, 4), gs::rgb4(15, 14, 11), gs::rgb4(6, 4, 2), gs::rgb4(2, 2, 2),
                            gs::rgb4(12, 2, 2), gs::rgb4(3, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(13, 15, 14), gs::rgb4(8, 11, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(15, 15, 13), gs::rgb4(2, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(15, 14, 8), gs::rgb4(6, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 6), gs::rgb4(4, 3, 2), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, ink});
    setPal(vdp, PAL_SHED, {0, gs::rgb4(10, 6, 4), gs::rgb4(7, 3, 2), gs::rgb4(4, 3, 3), gs::rgb4(12, 12, 8),
                           gs::rgb4(5, 5, 5), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 7, 3), gs::rgb4(4, 10, 4), gs::rgb4(6, 4, 2), gs::rgb4(1, 3, 1), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    art.barge = gs::uploadMipped(vdp, bargeArt());
    art.wake = gs::uploadMipped(vdp, wakeArt());
    art.smoke = gs::uploadMipped(vdp, smokeArt());
    art.bank = gs::uploadMipped(vdp, bankArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.pad = gs::uploadMipped(vdp, padArt());
    art.hatch = gs::uploadMipped(vdp, hatchArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    for (int i = 0; i < 8; i++) art.clock[i] = gs::uploadMipped(vdp, clockArt(i));
    art.crew[0] = gs::uploadMipped(vdp, crewArt(false));
    art.crew[1] = gs::uploadMipped(vdp, crewArt(true));
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(true));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(false));
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.title = words(vdp, "BARGE", 3, 1, 2);
    art.boxWord = words(vdp, "BOX", 3, 1, 2);
    art.stopped = words(vdp, "IN THE BOX", 2, 1, 2);
    art.outside = words(vdp, "OUTSIDE", 2, 1, 2);
    art.late = words(vdp, "OTHER CREW", 2, 1, 2);
    art.paused = words(vdp, "HELD", 2, 1, 2);
    loadFont(vdp, art);
}

}  // namespace bargebox
