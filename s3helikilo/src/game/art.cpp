#include "game/art.h"

#include <string>

namespace heli {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap bodyArt() {
    Bitmap b(96, 48);
    b.rect(20, 30, 50, 3, 6);
    b.rect(26, 26, 3, 7, 5);
    b.rect(58, 26, 3, 7, 5);
    b.ellipse(28, 34, 4, 4, 9);
    b.ellipse(60, 34, 4, 4, 9);
    b.ellipse(50, 18, 20, 11, 2);
    b.ellipse(50, 17, 18, 9, 1);
    b.ellipse(60, 16, 9, 6, 4);
    b.ellipse(62, 15, 5, 3, 8);
    b.rect(16, 16, 18, 4, 2);
    b.rect(8, 10, 8, 12, 3);
    b.rect(10, 12, 4, 6, 7);
    b.rect(48, 4, 3, 10, 5);
    b.ellipse(42, 20, 3, 3, 9);
    b.outline(5, false);
    return b;
}

Bitmap rotorArt(int frame) {
    Bitmap b(108, 16);
    if (frame == 0) {
        b.rect(2, 7, 104, 3, 8);
        b.rect(50, 3, 8, 10, 5);
    } else if (frame == 1) {
        b.line(6, 12, 102, 3, 8, 3);
        b.rect(50, 3, 8, 10, 5);
    } else {
        b.ellipse(54, 8, 7, 6, 8);
        b.rect(50, 2, 8, 12, 5);
        b.rect(16, 7, 16, 2, 3);
        b.rect(76, 7, 16, 2, 3);
    }
    return b;
}

Bitmap wheelArt() {
    Bitmap b(40, 40);
    b.ellipse(20, 20, 18, 18, 2);
    b.ellipse(20, 20, 12, 12, 1);
    b.ellipse(20, 20, 4, 4, 6);
    b.rect(18, 4, 4, 32, 3);
    b.rect(4, 18, 32, 4, 3);
    b.outline(5, false);
    return b;
}

Bitmap cableArt() {
    Bitmap b(6, 48);
    b.rect(2, 0, 2, 48, 1);
    return b;
}

Bitmap groundArt() {
    Bitmap b(160, 36);
    b.rect(0, 8, 160, 28, 2);
    b.rect(0, 8, 160, 5, 1);
    for (int x = 0; x < 160; x += 18) b.rect(x, 18, 8, 3, 3);
    return b;
}

Bitmap hillArt() {
    Bitmap b(80, 40);
    b.poly({{0, 40}, {18, 16}, {38, 28}, {56, 8}, {80, 40}}, 1);
    b.poly({{8, 40}, {22, 20}, {34, 30}, {42, 40}}, 2);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(64, 28);
    b.ellipse(22, 16, 16, 9, 1);
    b.ellipse(38, 14, 18, 10, 1);
    b.ellipse(30, 12, 12, 8, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 10, 10, 1);
    b.ellipse(16, 16, 6, 6, 2);
    return b;
}

Bitmap bannerArt() {
    Bitmap b(28, 64);
    b.rect(10, 0, 6, 64, 2);
    b.rect(4, 8, 20, 16, 3);
    b.rect(6, 10, 16, 12, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 12, 14), gs::rgb4(15, 12, 3),
                          gs::rgb4(15, 4, 3), gs::rgb4(4, 14, 6), gs::rgb4(8, 8, 9), shadow});
    setPal(vdp, PAL_SHIP, {0, gs::rgb4(13, 14, 9), gs::rgb4(8, 9, 5), gs::rgb4(4, 5, 3),
                           gs::rgb4(6, 11, 14), gs::rgb4(1, 1, 1), gs::rgb4(14, 12, 3),
                           gs::rgb4(14, 3, 2), gs::rgb4(13, 15, 15), gs::rgb4(2, 2, 2), shadow});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(3, 3, 3), gs::rgb4(6, 6, 6), gs::rgb4(9, 9, 8),
                            gs::rgb4(14, 12, 4), gs::rgb4(12, 4, 2), gs::rgb4(15, 14, 8), shadow});
    setPal(vdp, PAL_WORLD, {0, gs::rgb4(5, 9, 4), gs::rgb4(3, 6, 2), gs::rgb4(8, 9, 4),
                            gs::rgb4(15, 14, 15), gs::rgb4(15, 13, 4), gs::rgb4(14, 10, 3), shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(14, 14, 12), gs::rgb4(15, 15, 15), gs::rgb4(10, 10, 8), shadow});

    art.body = gs::uploadMipped(vdp, bodyArt());
    for (int i = 0; i < 3; i++) art.rotor[i] = gs::uploadMipped(vdp, rotorArt(i));
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.cable = gs::uploadMipped(vdp, cableArt());
    art.ground = gs::uploadMipped(vdp, groundArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    for (int c = 32; c < 127; c++) {
        art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace heli
