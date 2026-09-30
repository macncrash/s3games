#include "art.h"

#include <cmath>
#include <initializer_list>

namespace keelkilo {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 2, 3));
}

gs::Bitmap hullArt() {
    gs::Bitmap b(36, 88);
    b.poly({{18.f, 2.f}, {32.f, 18.f}, {33.f, 64.f}, {24.f, 84.f}, {12.f, 84.f}, {3.f, 64.f}, {4.f, 18.f}}, 1);
    b.poly({{18.f, 12.f}, {26.f, 24.f}, {26.f, 60.f}, {18.f, 74.f}, {10.f, 60.f}, {10.f, 24.f}}, 2);
    b.rect(16, 28, 4, 30, 3);
    b.ellipse(18, 42, 2.6f, 4.f, 4);
    b.rect(12, 70, 12, 5, 5);
    b.line(18, 6, 18, 80, 6, 1.3f);
    b.outline(7, false);
    return b;
}

gs::Bitmap sailArt() {
    gs::Bitmap b(52, 60);
    b.poly({{26.f, 2.f}, {48.f, 54.f}, {26.f, 46.f}}, 1);
    b.poly({{26.f, 6.f}, {6.f, 50.f}, {26.f, 42.f}}, 2);
    b.line(26, 0, 26, 58, 3, 2.f);
    b.rect(22, 54, 8, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(48, 48);
    b.ellipse(24, 24, 22, 22, 1);
    b.ellipse(24, 24, 16, 16, 2);
    b.ellipse(24, 24, 5, 5, 3);
    for (int i = 0; i < 8; i++) {
        float a = i * 0.785398f;
        float c = std::cos(a), s = std::sin(a);
        b.line(24 + c * 5.f, 24 + s * 5.f, 24 + c * 20.f, 24 + s * 20.f, 4, 1.6f);
    }
    b.ellipse(24, 24, 22, 22, 0);
    b.outline(5, false);
    return b;
}

gs::Bitmap millArt() {
    gs::Bitmap b(40, 36);
    b.poly({{2.f, 16.f}, {20.f, 2.f}, {38.f, 16.f}, {38.f, 34.f}, {2.f, 34.f}}, 1);
    b.rect(6, 18, 28, 16, 2);
    b.rect(16, 22, 8, 12, 3);
    b.rect(8, 20, 6, 6, 4);
    b.rect(26, 20, 6, 6, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(22, 30);
    b.rect(0, 20, 22, 10, 1);
    for (int i = 0; i < 6; i++) {
        float x = 2.f + i * 3.2f;
        b.line(x, 26.f, x + ((i & 1) ? 2.f : -2.f), 3.f + (i % 3), (i & 1) ? 2 : 3, 1.3f);
    }
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(14, 40);
    b.rect(5, 8, 4, 30, 1);
    b.rect(2, 2, 10, 8, 2);
    b.rect(3, 3, 8, 5, 3);
    b.rect(3, 36, 8, 3, 4);
    return b;
}

gs::Bitmap tapeArt() {
    gs::Bitmap b(64, 10);
    b.rect(0, 2, 64, 6, 1);
    for (int x = 0; x < 64; x += 8) b.rect(x, 2, 4, 6, 2);
    return b;
}

gs::Bitmap foamArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(14, 6, 12, 4, 1);
    b.ellipse(6, 6, 3, 2, 2);
    b.ellipse(21, 5, 4, 2, 2);
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
    const uint16_t ink = gs::rgb4(1, 2, 3);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 12, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(12, 13, 12), gs::rgb4(4, 7, 8), gs::rgb4(8, 4, 2), gs::rgb4(13, 10, 5), gs::rgb4(3, 3, 3),
            gs::rgb4(9, 8, 6), gs::rgb4(2, 2, 3), ink});
    setPal(vdp, PAL_SAIL, {0, gs::rgb4(15, 14, 11), gs::rgb4(10, 13, 14), gs::rgb4(5, 4, 3), gs::rgb4(8, 5, 2),
                           gs::rgb4(2, 3, 4), ink});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(12, 9, 4), gs::rgb4(14, 12, 7),
                            gs::rgb4(3, 2, 1), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_MILL, {0, gs::rgb4(9, 4, 3), gs::rgb4(12, 8, 5), gs::rgb4(4, 3, 3), gs::rgb4(14, 13, 8),
                           gs::rgb4(3, 2, 2), gs::rgb4(2, 1, 1), ink});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(3, 8, 3), gs::rgb4(8, 12, 4), gs::rgb4(2, 5, 2), gs::rgb4(10, 9, 4), ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(11, 11, 10), gs::rgb4(15, 12, 3), gs::rgb4(6, 5, 3), gs::rgb4(4, 4, 4), ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 12, 14), ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(15, 15, 12), gs::rgb4(1, 5, 2), ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 12, 5), gs::rgb4(5, 1, 1), ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(9, 6, 1), gs::rgb4(15, 15, 12), ink});

    vdp.setFogColor(gs::rgb4(4, 8, 10));
    loadFont(vdp, art);
    art.hull = gs::uploadMipped(vdp, hullArt());
    art.sail = gs::uploadMipped(vdp, sailArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.mill = gs::uploadMipped(vdp, millArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.tape = gs::uploadMipped(vdp, tapeArt());
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.title = words(vdp, "KEEL KILO", 3, 1, 2);
}

}  // namespace keelkilo
