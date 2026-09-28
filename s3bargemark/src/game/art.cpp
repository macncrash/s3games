#include "art.h"

#include <cmath>
#include <initializer_list>

namespace bargemark {
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
    gs::Bitmap b(128, 42);
    b.poly({{6.f, 22.f}, {108.f, 20.f}, {122.f, 28.f}, {114.f, 38.f}, {10.f, 38.f}, {2.f, 28.f}}, 2);
    b.poly({{12.f, 28.f}, {110.f, 27.f}, {116.f, 36.f}, {12.f, 36.f}}, 3);
    b.rect(18, 14, 78, 9, 1);
    b.rect(24, 8, 26, 8, 4);
    b.rect(54, 9, 20, 7, 5);
    b.rect(28, 10, 6, 5, 6);
    b.rect(38, 10, 6, 5, 6);
    b.rect(78, 6, 5, 14, 7);
    b.ellipse(20, 24, 3.f, 3.f, 8);
    b.ellipse(104, 24, 3.f, 3.f, 8);
    b.rect(46, 30, 32, 2, 9);
    gs::TextStyle st{1, 10, 0, 0, 0};
    b.blit(gs::textBitmap("IDA", st), 48, 16);
    b.outline(15, false);
    return b;
}

gs::Bitmap bankArt() {
    gs::Bitmap b(64, 28);
    b.rect(0, 12, 64, 16, 1);
    b.poly({{0.f, 16.f}, {14.f, 6.f}, {30.f, 12.f}, {48.f, 4.f}, {64.f, 14.f}, {64.f, 28.f}, {0.f, 28.f}}, 2);
    for (int x = 2; x < 60; x += 9) b.rect(x, 18, 5, 3, 3);
    b.rect(0, 24, 64, 4, 4);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(18, 22);
    b.line(4, 20, 6, 3, 1, 1.3f);
    b.line(9, 20, 8, 1, 2, 1.3f);
    b.line(14, 20, 15, 6, 1, 1.2f);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 52);
    b.rect(4, 8, 4, 44, 1);
    b.rect(1, 2, 10, 8, 2);
    b.rect(3, 20, 6, 3, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap crossArt() {
    gs::Bitmap b(28, 28);
    b.rect(12, 2, 4, 24, 1);
    b.rect(2, 12, 24, 4, 1);
    b.rect(11, 11, 6, 6, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(48, 16);
    b.ellipse(24, 8, 22, 6, 1);
    b.ellipse(24, 8, 16, 3, 0);
    b.rect(22, 6, 4, 4, 2);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(56, 40);
    b.rect(6, 16, 44, 22, 1);
    b.poly({{2.f, 16.f}, {28.f, 3.f}, {54.f, 16.f}}, 2);
    b.rect(24, 24, 10, 14, 3);
    b.rect(12, 20, 8, 7, 4);
    b.rect(36, 20, 8, 7, 4);
    b.rect(26, 5, 4, 8, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap clockArt(int hand) {
    gs::Bitmap b(32, 32);
    b.ellipse(16, 16, 14, 14, 1);
    b.ellipse(16, 16, 11, 11, 2);
    b.ellipse(16, 16, 2, 2, 3);
    for (int i = 0; i < 12; i++) {
        float a = i * 0.523599f - 1.5708f;
        b.set(int(16 + std::cos(a) * 9.f), int(16 + std::sin(a) * 9.f), 4);
    }
    float a = hand * 0.785398f - 1.5708f;
    b.line(16, 16, 16 + std::cos(a) * 8.f, 16 + std::sin(a) * 8.f, 5, 1.6f);
    b.outline(6, false);
    return b;
}

gs::Bitmap crewArt(bool step) {
    gs::Bitmap b(20, 32);
    b.ellipse(10, 6, 4, 4, 1);
    b.rect(7, 11, 6, 10, 2);
    b.line(7, 13, step ? 2.f : 4.f, 20, 3, 1.5f);
    b.line(13, 13, step ? 18.f : 16.f, 20, 3, 1.5f);
    b.line(8, 22, step ? 5.f : 7.f, 31, 4, 1.6f);
    b.line(12, 22, step ? 15.f : 13.f, 31, 4, 1.6f);
    b.set(8, 5, 5);
    b.set(12, 5, 5);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 40);
    b.rect(12, 22, 4, 16, 3);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(10, 12, 5, 5, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 28);
    b.rect(4, 8, 2, 20, 1);
    b.ellipse(5, 6, 4, 4, 2);
    b.ellipse(5, 6, 2, 2, 3);
    return b;
}

gs::Bitmap birdArt(bool up) {
    gs::Bitmap b(16, 8);
    if (up) {
        b.line(1, 6, 8, 2, 1, 1.2f);
        b.line(8, 2, 15, 6, 1, 1.2f);
    } else {
        b.line(1, 2, 8, 5, 1, 1.2f);
        b.line(8, 5, 15, 2, 1, 1.2f);
    }
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
                            gs::rgb4(6, 7, 6), gs::rgb4(14, 12, 6), gs::rgb4(3, 3, 3), gs::rgb4(13, 10, 3),
                            gs::rgb4(11, 4, 3), gs::rgb4(15, 14, 10), 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(5, 7, 4), gs::rgb4(3, 9, 4), gs::rgb4(7, 6, 4), gs::rgb4(3, 4, 3), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(7, 7, 7), gs::rgb4(14, 12, 3), gs::rgb4(12, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(14, 12, 3), gs::rgb4(15, 15, 12), gs::rgb4(8, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
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
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 7, 3), gs::rgb4(4, 10, 4), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(4, 8, 10), gs::rgb4(8, 12, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    art.barge = gs::uploadMipped(vdp, bargeArt());
    art.wake = gs::uploadMipped(vdp, wakeArt());
    art.smoke = gs::uploadMipped(vdp, smokeArt());
    art.bank = gs::uploadMipped(vdp, bankArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.cross = gs::uploadMipped(vdp, crossArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    for (int i = 0; i < 8; i++) art.clock[i] = gs::uploadMipped(vdp, clockArt(i));
    art.crew[0] = gs::uploadMipped(vdp, crewArt(false));
    art.crew[1] = gs::uploadMipped(vdp, crewArt(true));
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(true));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(false));
    art.title = words(vdp, "BARGE", 3, 1, 2);
    art.wordMark = words(vdp, "MARK", 3, 1, 2);
    art.setDown = words(vdp, "SET DOWN", 2, 1, 2);
    art.lost = words(vdp, "THEY TOOK IT", 2, 1, 2);
    art.paused = words(vdp, "HELD", 2, 1, 2);
    loadFont(vdp, art);
}

}  // namespace bargemark
