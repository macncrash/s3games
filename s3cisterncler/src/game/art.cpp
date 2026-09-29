#include "game/art.h"

#include <initializer_list>
#include <string>

namespace ccler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void courses(gs::Bitmap& b, int x, int y, int w, int h, int stone, int shade, int joint) {
    b.rect(float(x), float(y), float(w), float(h), stone);
    for (int row = y + 4; row < y + h; row += 8) {
        b.rect(float(x), float(row), float(w), 1.f, joint);
        int shift = ((row - y) / 8) & 1 ? 10 : 3;
        for (int col = x + shift; col < x + w; col += 14) b.rect(float(col), float(row), 1.f, 8.f, joint);
    }
    b.rect(float(x), float(y), 2.f, float(h), shade);
}

gs::Bitmap wardArt(int step) {
    gs::Bitmap b(34, 56);
    b.ellipse(17, 8, 7, 4, 6);
    b.ellipse(17, 15, 6, 7, 5);
    b.rect(14, 12, 6, 3, 7);
    b.set(15, 15, 8);
    b.set(20, 15, 8);
    b.poly({{9, 22}, {25, 22}, {27, 40}, {7, 40}}, 1);
    b.rect(14, 24, 6, 7, 3);
    b.rect(11, 32, 12, 4, 2);
    if (step == 0) {
        b.rect(10, 40, 5, 12, 4);
        b.rect(19, 40, 5, 10, 4);
        b.rect(8, 50, 8, 3, 2);
        b.rect(17, 48, 8, 3, 2);
    } else {
        b.rect(10, 40, 5, 10, 4);
        b.rect(19, 40, 5, 12, 4);
        b.rect(9, 48, 8, 3, 2);
        b.rect(17, 50, 8, 3, 2);
    }
    b.outline(9, false);
    return b;
}

gs::Bitmap rakeArt() {
    gs::Bitmap b(30, 42);
    b.line(8, 2, 16, 26, 3, 2.0f);
    b.rect(10, 24, 16, 3, 1);
    b.line(12, 26, 12, 38, 2, 1.2f);
    b.line(16, 26, 16, 39, 2, 1.2f);
    b.line(20, 26, 20, 38, 2, 1.2f);
    b.line(24, 26, 24, 37, 2, 1.2f);
    b.outline(4, false);
    return b;
}

gs::Bitmap siltArt() {
    gs::Bitmap b(26, 16);
    b.ellipse(13, 10, 11, 5, 1);
    b.ellipse(8, 8, 5, 3, 2);
    b.ellipse(17, 9, 4, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap mossArt() {
    gs::Bitmap b(22, 20);
    b.ellipse(11, 14, 8, 4, 1);
    b.poly({{11, 2}, {15, 12}, {7, 12}}, 2);
    b.poly({{6, 6}, {9, 14}, {3, 13}}, 3);
    b.poly({{16, 5}, {19, 13}, {12, 12}}, 2);
    b.outline(4, false);
    return b;
}

gs::Bitmap brickArt() {
    gs::Bitmap b(24, 14);
    b.rect(2, 3, 20, 8, 1);
    b.rect(2, 3, 20, 2, 2);
    b.line(12, 3, 12, 11, 3, 1.f);
    b.outline(4, false);
    return b;
}

gs::Bitmap flaskArt() {
    gs::Bitmap b(14, 26);
    b.rect(5, 1, 4, 6, 2);
    b.poly({{2, 7}, {12, 7}, {13, 22}, {1, 22}}, 1);
    b.rect(3, 13, 8, 4, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(16, 32);
    b.line(8, 30, 7, 4, 1, 1.6f);
    b.line(8, 18, 13, 8, 2, 1.2f);
    b.line(7, 14, 3, 6, 3, 1.2f);
    b.ellipse(7, 4, 2, 3, 2);
    return b;
}

gs::Bitmap pailArt() {
    gs::Bitmap b(20, 22);
    b.poly({{3, 6}, {17, 6}, {15, 18}, {5, 18}}, 1);
    b.rect(3, 5, 14, 3, 2);
    b.line(5, 6, 8, 1, 3, 1.2f);
    b.line(15, 6, 12, 1, 3, 1.2f);
    b.outline(4, false);
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(32, 12);
    b.poly({{2, 3}, {30, 2}, {29, 9}, {1, 10}}, 1);
    b.line(4, 5, 26, 4, 2, 1.f);
    b.outline(3, false);
    return b;
}

gs::Bitmap pierArt() {
    gs::Bitmap b(40, 110);
    courses(b, 0, 0, 40, 110, 1, 2, 3);
    b.rect(6, 18, 28, 10, 4);
    b.rect(8, 20, 24, 6, 5);
    b.ellipse(20, 70, 10, 6, 6);
    b.ellipse(20, 70, 6, 3, 5);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(26, 30);
    b.rect(4, 2, 18, 22, 1);
    b.rect(6, 4, 14, 16, 2);
    b.rect(11, 8, 2.2f, 8, 3);
    b.rect(11, 14, 6, 2.2f, 4);
    b.rect(8, 24, 10, 4, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(220, 28);
    b.rect(0, 6, 220, 16, 1);
    b.rect(0, 6, 220, 3, 2);
    for (int x = 8; x < 212; x += 18) b.rect(float(x), 8, 1.f, 12.f, 3);
    b.rect(0, 18, 220, 3, 4);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(26, 10);
    b.ellipse(13, 5, 11, 3, 1);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4, 1);
    b.ellipse(6, 6, 2.4f, 1.8f, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& a) {
    setPal(vdp, PAL_TEXT, {gs::rgb4(0, 0, 0), gs::rgb4(14, 14, 12), gs::rgb4(7, 8, 9), gs::rgb4(15, 12, 5),
                           gs::rgb4(5, 12, 9), gs::rgb4(14, 5, 4)});
    setPal(vdp, PAL_STONE, {gs::rgb4(0, 0, 0), gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 4), gs::rgb4(9, 8, 6),
                            gs::rgb4(2, 2, 3), gs::rgb4(5, 6, 7), gs::rgb4(3, 4, 5)});
    setPal(vdp, PAL_WARD, {gs::rgb4(0, 0, 0), gs::rgb4(4, 7, 8), gs::rgb4(8, 6, 3), gs::rgb4(12, 10, 6),
                           gs::rgb4(2, 2, 3), gs::rgb4(11, 8, 6), gs::rgb4(3, 2, 2), gs::rgb4(13, 12, 9),
                           gs::rgb4(1, 1, 1), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_SILT, {gs::rgb4(0, 0, 0), gs::rgb4(8, 6, 3), gs::rgb4(6, 5, 2), gs::rgb4(10, 8, 4),
                           gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_MOSS, {gs::rgb4(0, 0, 0), gs::rgb4(3, 7, 3), gs::rgb4(5, 10, 4), gs::rgb4(2, 5, 2),
                           gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_WATER, {gs::rgb4(0, 0, 0), gs::rgb4(3, 7, 10), gs::rgb4(6, 11, 13), gs::rgb4(2, 4, 7),
                            gs::rgb4(8, 12, 10), gs::rgb4(1, 2, 4), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_WOOD, {gs::rgb4(0, 0, 0), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(11, 8, 3),
                           gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_RIM, {gs::rgb4(0, 0, 0), gs::rgb4(6, 6, 6), gs::rgb4(9, 9, 8), gs::rgb4(3, 3, 4),
                          gs::rgb4(2, 3, 4), gs::rgb4(4, 6, 7), gs::rgb4(1, 2, 3)});
    setPal(vdp, PAL_GOOD, {gs::rgb4(0, 0, 0), gs::rgb4(6, 14, 8), gs::rgb4(12, 15, 12)});
    setPal(vdp, PAL_ALERT, {gs::rgb4(0, 0, 0), gs::rgb4(14, 4, 3), gs::rgb4(15, 10, 4)});
    setPal(vdp, PAL_DUSK, {gs::rgb4(1, 2, 4), gs::rgb4(2, 3, 6), gs::rgb4(4, 5, 8)});

    a.ward[0] = gs::uploadMipped(vdp, wardArt(0));
    a.ward[1] = gs::uploadMipped(vdp, wardArt(1));
    a.rake = gs::uploadMipped(vdp, rakeArt());
    a.silt = gs::uploadMipped(vdp, siltArt());
    a.moss = gs::uploadMipped(vdp, mossArt());
    a.brick = gs::uploadMipped(vdp, brickArt());
    a.flask = gs::uploadMipped(vdp, flaskArt());
    a.reed = gs::uploadMipped(vdp, reedArt());
    a.pail = gs::uploadMipped(vdp, pailArt());
    a.plank = gs::uploadMipped(vdp, plankArt());
    a.pier = gs::uploadMipped(vdp, pierArt());
    a.clock = gs::uploadMipped(vdp, clockArt());
    a.ring = gs::uploadMipped(vdp, ringArt());
    a.shadow = gs::uploadMipped(vdp, shadowArt());
    a.puff = gs::uploadMipped(vdp, puffArt());

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    for (int c = 32; c < 127; c++) a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));

    vdp.setFogColor(gs::rgb4(1, 2, 4));
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.HUD.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.road[y].on = false;
        vdp.lineFog[y] = 0;
    }
}

}  // namespace ccler
