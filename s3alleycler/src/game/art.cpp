#include "game/art.h"

#include <initializer_list>
#include <string>

namespace acler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void courses(gs::Bitmap& b, int x, int y, int w, int h, int brick, int shade, int mortar) {
    b.rect(float(x), float(y), float(w), float(h), brick);
    for (int row = y; row < y + h; row += 6) {
        b.rect(float(x), float(row), float(w), 1.f, mortar);
        int shift = ((row - y) / 6) & 1 ? 8 : 2;
        for (int col = x + shift; col < x + w; col += 16) b.rect(float(col), float(row), 1.f, 6.f, mortar);
    }
    b.rect(float(x), float(y), 3.f, float(h), shade);
}

gs::Bitmap sweepArt(int step) {
    gs::Bitmap b(36, 58);
    b.ellipse(18, 8, 8, 4, 6);
    b.ellipse(18, 16, 7, 8, 5);
    b.rect(14, 13, 8, 3, 7);
    b.set(15, 16, 8);
    b.set(21, 16, 8);
    b.poly({{10, 24}, {26, 24}, {28, 42}, {8, 42}}, 1);
    b.rect(15, 25, 6, 8, 3);
    b.line(26, 30, 33, 44, 2, 2.2f);
    if (step == 0) {
        b.rect(11, 42, 5, 12, 4);
        b.rect(20, 42, 5, 10, 4);
        b.rect(9, 52, 8, 3, 2);
        b.rect(18, 50, 8, 3, 2);
    } else {
        b.rect(11, 42, 5, 10, 4);
        b.rect(20, 42, 5, 12, 4);
        b.rect(10, 50, 8, 3, 2);
        b.rect(18, 52, 8, 3, 2);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap broomArt() {
    gs::Bitmap b(28, 40);
    b.line(6, 4, 18, 28, 3, 2.2f);
    b.rect(12, 26, 12, 8, 1);
    b.line(13, 28, 13, 36, 2, 1.2f);
    b.line(17, 28, 17, 37, 2, 1.2f);
    b.line(21, 28, 21, 36, 2, 1.2f);
    b.outline(4, false);
    return b;
}

gs::Bitmap canArt() {
    gs::Bitmap b(18, 24);
    b.rect(4, 4, 10, 16, 1);
    b.rect(4, 4, 10, 3, 2);
    b.rect(5, 10, 8, 2, 3);
    b.ellipse(9, 4, 5, 2, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap paperArt() {
    gs::Bitmap b(28, 18);
    b.poly({{2, 4}, {22, 2}, {26, 14}, {6, 16}}, 1);
    b.line(6, 7, 20, 5, 2, 1.f);
    b.line(7, 11, 22, 9, 2, 1.f);
    b.outline(3, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(30, 24);
    b.poly({{3, 6}, {26, 4}, {28, 20}, {5, 22}}, 1);
    b.line(4, 12, 27, 10, 2, 1.4f);
    b.line(14, 5, 15, 21, 3, 1.4f);
    b.outline(4, false);
    return b;
}

gs::Bitmap bottleArt() {
    gs::Bitmap b(14, 28);
    b.rect(5, 2, 4, 6, 2);
    b.poly({{3, 8}, {11, 8}, {12, 24}, {2, 24}}, 1);
    b.rect(4, 14, 6, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap wallArt() {
    gs::Bitmap b(48, 96);
    courses(b, 0, 0, 48, 96, 1, 2, 3);
    b.rect(8, 28, 18, 22, 4);
    b.rect(10, 30, 14, 16, 5);
    b.rect(16, 30, 1, 16, 6);
    b.rect(10, 37, 14, 1, 6);
    b.rect(30, 60, 12, 18, 7);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(20, 36);
    b.rect(9, 0, 2, 14, 4);
    b.poly({{3, 14}, {17, 14}, {14, 26}, {6, 26}}, 1);
    b.rect(6, 16, 8, 6, 2);
    b.ellipse(10, 30, 3, 2, 3);
    b.outline(5, false);
    return b;
}

gs::Bitmap grateArt() {
    gs::Bitmap b(64, 18);
    b.rect(1, 2, 62, 14, 1);
    for (int x = 4; x < 60; x += 6) b.rect(float(x), 3, 2, 12, 2);
    b.rect(1, 2, 62, 2.0f, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(14, 14, 9, 9, 2);
    b.line(14, 14, 14, 7, 3, 1.4f);
    b.line(14, 14, 20, 16, 4, 1.4f);
    b.outline(5, false);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 10);
    b.ellipse(14, 5, 12, 3, 1);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4, 1);
    b.ellipse(6, 6, 2.5f, 2.0f, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& a) {
    setPal(vdp, PAL_TEXT, {gs::rgb4(0, 0, 0), gs::rgb4(14, 14, 13), gs::rgb4(8, 8, 9), gs::rgb4(15, 12, 4),
                           gs::rgb4(4, 12, 8), gs::rgb4(14, 4, 4)});
    setPal(vdp, PAL_BRICK, {gs::rgb4(0, 0, 0), gs::rgb4(8, 3, 3), gs::rgb4(4, 1, 2), gs::rgb4(3, 2, 2),
                            gs::rgb4(2, 3, 5), gs::rgb4(10, 12, 14), gs::rgb4(6, 8, 10), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_SWEEP, {gs::rgb4(0, 0, 0), gs::rgb4(2, 5, 9), gs::rgb4(10, 7, 3), gs::rgb4(13, 11, 6),
                            gs::rgb4(1, 1, 3), gs::rgb4(12, 8, 6), gs::rgb4(3, 2, 2), gs::rgb4(14, 12, 8),
                            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_JUNK, {gs::rgb4(0, 0, 0), gs::rgb4(11, 11, 12), gs::rgb4(6, 7, 8), gs::rgb4(12, 3, 3),
                           gs::rgb4(8, 8, 9), gs::rgb4(2, 2, 3), gs::rgb4(14, 13, 10), gs::rgb4(4, 6, 8)});
    setPal(vdp, PAL_WOOD, {gs::rgb4(0, 0, 0), gs::rgb4(9, 6, 2), gs::rgb4(6, 4, 1), gs::rgb4(12, 9, 4),
                           gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_LAMP, {gs::rgb4(0, 0, 0), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 10), gs::rgb4(8, 6, 2),
                           gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_STONE, {gs::rgb4(0, 0, 0), gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(7, 7, 6),
                            gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_GRATE, {gs::rgb4(0, 0, 0), gs::rgb4(3, 4, 4), gs::rgb4(1, 1, 2), gs::rgb4(6, 7, 6),
                            gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_GOOD, {gs::rgb4(0, 0, 0), gs::rgb4(6, 14, 8), gs::rgb4(12, 15, 12)});
    setPal(vdp, PAL_ALERT, {gs::rgb4(0, 0, 0), gs::rgb4(14, 4, 3), gs::rgb4(15, 10, 4)});
    setPal(vdp, PAL_NIGHT, {gs::rgb4(1, 1, 3), gs::rgb4(2, 2, 5), gs::rgb4(4, 4, 7)});

    a.sweep[0] = gs::uploadMipped(vdp, sweepArt(0));
    a.sweep[1] = gs::uploadMipped(vdp, sweepArt(1));
    a.broom = gs::uploadMipped(vdp, broomArt());
    a.can = gs::uploadMipped(vdp, canArt());
    a.paper = gs::uploadMipped(vdp, paperArt());
    a.crate = gs::uploadMipped(vdp, crateArt());
    a.bottle = gs::uploadMipped(vdp, bottleArt());
    a.wall = gs::uploadMipped(vdp, wallArt());
    a.lamp = gs::uploadMipped(vdp, lampArt());
    a.grate = gs::uploadMipped(vdp, grateArt());
    a.clock = gs::uploadMipped(vdp, clockArt());
    a.shadow = gs::uploadMipped(vdp, shadowArt());
    a.puff = gs::uploadMipped(vdp, puffArt());

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    for (int c = 32; c < 127; c++) a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));

    vdp.setFogColor(gs::rgb4(1, 1, 3));
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.HUD.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.road[y].on = false;
        vdp.lineFog[y] = 0;
    }
}

}  // namespace acler
