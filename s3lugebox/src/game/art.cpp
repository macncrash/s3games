#include "art.h"

#include <cmath>
#include <initializer_list>

namespace lugebox {
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

gs::Bitmap lugeArt() {
    gs::Bitmap b(96, 36);
    b.poly({{6.f, 22.f}, {86.f, 16.f}, {92.f, 20.f}, {88.f, 26.f}, {8.f, 28.f}}, 2);
    b.rect(10, 24, 74, 3, 3);
    b.line(8, 27, 90, 22, 4, 1.6f);
    b.ellipse(18, 26, 3.2f, 3.2f, 5);
    b.ellipse(78, 23, 3.2f, 3.2f, 5);
    b.ellipse(48, 14, 9, 7, 6);
    b.ellipse(58, 12, 7, 6, 7);
    b.rect(40, 16, 16, 4, 8);
    b.rect(62, 10, 10, 4, 1);
    b.ellipse(70, 9, 5, 4, 9);
    b.set(72, 8, 10);
    b.line(36, 18, 28, 14, 8, 1.5f);
    b.outline(11, false);
    return b;
}

gs::Bitmap sprayArt() {
    gs::Bitmap b(18, 10);
    b.ellipse(8, 6, 7, 3, 1);
    b.ellipse(12, 4, 4, 2, 2);
    b.ellipse(4, 5, 2, 1.4f, 3);
    return b;
}

gs::Bitmap flakeArt() {
    gs::Bitmap b(7, 7);
    b.line(3, 0, 3, 6, 1, 1.f);
    b.line(0, 3, 6, 3, 1, 1.f);
    b.set(1, 1, 2);
    b.set(5, 5, 2);
    return b;
}

gs::Bitmap bankArt() {
    gs::Bitmap b(48, 28);
    b.rect(0, 8, 48, 20, 1);
    b.poly({{0.f, 12.f}, {10.f, 4.f}, {22.f, 10.f}, {36.f, 2.f}, {48.f, 11.f}, {48.f, 28.f}, {0.f, 28.f}}, 2);
    for (int x = 2; x < 46; x += 7) b.rect(x, 16, 4, 2, 3);
    b.rect(0, 22, 48, 6, 4);
    return b;
}

gs::Bitmap pineArt() {
    gs::Bitmap b(28, 48);
    b.poly({{14.f, 2.f}, {24.f, 20.f}, {4.f, 20.f}}, 1);
    b.poly({{14.f, 12.f}, {26.f, 32.f}, {2.f, 32.f}}, 2);
    b.poly({{14.f, 22.f}, {27.f, 42.f}, {1.f, 42.f}}, 1);
    b.rect(12, 40, 4, 8, 3);
    b.rect(16, 8, 2, 6, 4);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 52);
    b.rect(4, 8, 4, 44, 1);
    b.rect(1, 2, 10, 8, 2);
    b.rect(3, 22, 6, 3, 3);
    b.rect(5, 44, 2, 6, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(16, 12);
    b.rect(0, 2, 16, 8, 1);
    b.rect(0, 2, 16, 2, 2);
    b.rect(0, 8, 16, 2, 3);
    return b;
}

gs::Bitmap hutArt() {
    gs::Bitmap b(52, 40);
    b.rect(8, 16, 36, 22, 1);
    b.poly({{4.f, 16.f}, {26.f, 4.f}, {48.f, 16.f}}, 2);
    b.rect(22, 24, 10, 14, 3);
    b.rect(12, 20, 7, 6, 4);
    b.rect(34, 20, 7, 6, 4);
    b.rect(24, 6, 3, 6, 5);
    for (int y = 18; y < 36; y += 4) b.rect(8, y, 36, 1, 6);
    b.outline(7, false);
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
    b.line(16, 16, 16 + std::cos(a) * 8.f, 16 + std::sin(a) * 8.f, 5, 1.5f);
    b.outline(6, false);
    return b;
}

gs::Bitmap crewArt(bool step) {
    gs::Bitmap b(22, 34);
    b.ellipse(11, 7, 4.2f, 4.2f, 1);
    b.rect(8, 12, 6, 9, 2);
    b.line(8, 14, step ? 3.f : 5.f, 22, 3, 1.6f);
    b.line(14, 14, step ? 19.f : 17.f, 22, 3, 1.6f);
    b.line(9, 21, step ? 6.f : 8.f, 32, 4, 1.7f);
    b.line(13, 21, step ? 16.f : 14.f, 32, 4, 1.7f);
    b.rect(7, 4, 8, 3, 5);
    b.set(9, 6, 6);
    b.set(13, 6, 6);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 30);
    b.rect(4, 10, 2, 20, 1);
    b.ellipse(5, 7, 4, 4, 2);
    b.ellipse(5, 7, 2, 2, 3);
    return b;
}

gs::Bitmap birdArt(bool up) {
    gs::Bitmap b(16, 8);
    if (up) {
        b.line(1, 6, 8, 2, 1, 1.3f);
        b.line(8, 2, 15, 6, 1, 1.3f);
    } else {
        b.line(1, 2, 8, 5, 1, 1.3f);
        b.line(8, 5, 15, 2, 1, 1.3f);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12), gs::rgb4(8, 9, 10), ink});
    setPal(vdp, PAL_LUGE,
           {0, gs::rgb4(14, 3, 2), gs::rgb4(12, 12, 13), gs::rgb4(6, 7, 8), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3),
            gs::rgb4(9, 6, 4), gs::rgb4(13, 10, 8), gs::rgb4(4, 5, 8), gs::rgb4(15, 12, 4), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_ICE,
           {0, gs::rgb4(10, 12, 13), gs::rgb4(14, 15, 15), gs::rgb4(7, 9, 11), gs::rgb4(12, 13, 14)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(8, 4, 2), gs::rgb4(14, 10, 3), gs::rgb4(12, 12, 10), gs::rgb4(4, 4, 5), ink});
    setPal(vdp, PAL_BOX, {0, gs::rgb4(12, 3, 2), gs::rgb4(15, 12, 4), gs::rgb4(6, 2, 2)});
    setPal(vdp, PAL_CREW,
           {0, gs::rgb4(13, 10, 8), gs::rgb4(3, 5, 10), gs::rgb4(8, 8, 9), gs::rgb4(2, 2, 3), gs::rgb4(12, 3, 2),
            gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_CLOCK,
           {0, gs::rgb4(10, 8, 4), gs::rgb4(15, 14, 11), gs::rgb4(2, 2, 3), gs::rgb4(6, 5, 4), gs::rgb4(12, 2, 2),
            ink});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(13, 14, 15), gs::rgb4(9, 12, 14), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(6, 14, 7), gs::rgb4(12, 15, 10), ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(15, 12, 6), ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(14, 13, 10), gs::rgb4(8, 9, 12), ink});
    setPal(vdp, PAL_HUT,
           {0, gs::rgb4(8, 5, 3), gs::rgb4(10, 3, 2), gs::rgb4(3, 3, 4), gs::rgb4(12, 13, 14), gs::rgb4(4, 4, 4),
            gs::rgb4(6, 4, 2), ink});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(2, 6, 3), gs::rgb4(3, 8, 4), gs::rgb4(5, 4, 2), gs::rgb4(12, 13, 14)});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 13, 14)});

    art.luge = gs::uploadMipped(vdp, lugeArt());
    art.spray = gs::uploadMipped(vdp, sprayArt());
    art.flake = gs::uploadMipped(vdp, flakeArt());
    art.bank = gs::uploadMipped(vdp, bankArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.hut = gs::uploadMipped(vdp, hutArt());
    for (int i = 0; i < 8; i++) art.clock[i] = gs::uploadMipped(vdp, clockArt(i));
    art.crew[0] = gs::uploadMipped(vdp, crewArt(false));
    art.crew[1] = gs::uploadMipped(vdp, crewArt(true));
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(true));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(false));
    loadFont(vdp, art);
    art.title = words(vdp, "LUGE", 3, 1, 2);
    art.boxWord = words(vdp, "BOX", 3, 1, 2);
    art.stopped = words(vdp, "STOPPED", 2, 1, 2);
    art.outside = words(vdp, "OUTSIDE", 2, 1, 2);
    art.late = words(vdp, "TOO LATE", 2, 1, 2);
    art.paused = words(vdp, "HELD", 2, 1, 2);
}

}  // namespace lugebox
