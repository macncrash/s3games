#include "game/art.h"

#include <string>

namespace helilock {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap bodyArt() {
    Bitmap b(88, 40);
    b.ellipse(48, 20, 22, 11, 2);
    b.ellipse(48, 19, 19, 9, 1);
    b.ellipse(58, 18, 10, 6, 4);
    b.ellipse(60, 17, 5, 3, 8);
    b.rect(16, 18, 18, 4, 2);
    b.rect(8, 10, 10, 14, 3);
    b.rect(10, 13, 5, 7, 6);
    b.rect(42, 6, 3, 8, 5);
    b.rect(28, 28, 36, 2, 5);
    b.rect(32, 26, 2, 4, 5);
    b.rect(58, 26, 2, 4, 5);
    b.ellipse(40, 22, 2, 3, 9);
    b.outline(7, false);
    return b;
}

Bitmap rotorArt(int frame) {
    Bitmap b(96, 14);
    if (frame == 0) {
        b.rect(2, 6, 92, 2, 8);
        b.rect(44, 2, 8, 10, 5);
    } else if (frame == 1) {
        b.line(6, 11, 90, 2, 8, 2);
        b.rect(44, 2, 8, 10, 5);
    } else {
        b.ellipse(48, 7, 5, 5, 8);
        b.rect(44, 1, 8, 12, 5);
        b.rect(16, 6, 12, 2, 3);
        b.rect(68, 6, 12, 2, 3);
    }
    return b;
}

Bitmap gateArt() {
    Bitmap b(24, 64);
    b.rect(2, 0, 20, 64, 2);
    b.rect(4, 2, 16, 60, 1);
    for (int i = 0; i < 6; i++) {
        b.rect(6, 4 + i * 10, 12, 4, (i & 1) ? 6 : 3);
    }
    b.outline(5, false);
    return b;
}

Bitmap wallArt() {
    Bitmap b(48, 32);
    b.rect(0, 0, 48, 32, 1);
    b.rect(0, 0, 48, 4, 2);
    for (int x = 4; x < 48; x += 12) b.rect(x, 10, 6, 14, 3);
    return b;
}

Bitmap waterArt() {
    Bitmap b(64, 24);
    b.rect(0, 4, 64, 20, 1);
    b.rect(0, 2, 64, 4, 2);
    for (int x = 0; x < 64; x += 8) b.rect(x, 8, 4, 2, 3);
    return b;
}

Bitmap padArt() {
    Bitmap b(80, 20);
    b.rect(0, 6, 80, 14, 2);
    b.rect(4, 8, 72, 4, 6);
    b.rect(34, 8, 12, 8, 4);
    b.outline(5, false);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(56, 22);
    b.ellipse(20, 12, 14, 8, 1);
    b.ellipse(34, 11, 16, 8, 1);
    b.ellipse(28, 9, 10, 6, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(9, 11, 13), gs::rgb4(15, 12, 3),
                          gs::rgb4(15, 4, 3), gs::rgb4(4, 14, 7), gs::rgb4(7, 8, 9), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_SHIP, {0, gs::rgb4(14, 10, 3), gs::rgb4(11, 7, 2), gs::rgb4(6, 4, 2),
                           gs::rgb4(8, 13, 15), gs::rgb4(2, 2, 2), gs::rgb4(15, 13, 4),
                           gs::rgb4(12, 3, 2), gs::rgb4(14, 15, 15), gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(8, 8, 9), gs::rgb4(5, 5, 6), gs::rgb4(12, 12, 11),
                           gs::rgb4(15, 12, 2), gs::rgb4(1, 1, 1), gs::rgb4(14, 11, 2), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_WORLD, {0, gs::rgb4(3, 7, 12), gs::rgb4(6, 11, 15), gs::rgb4(8, 13, 14),
                            gs::rgb4(4, 8, 4), gs::rgb4(14, 13, 6), gs::rgb4(9, 9, 8), gs::rgb4(5, 5, 5)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(14, 14, 13), gs::rgb4(15, 15, 15), gs::rgb4(9, 10, 11)});

    art.body = gs::uploadMipped(vdp, bodyArt());
    for (int i = 0; i < 3; i++) art.rotor[i] = gs::uploadMipped(vdp, rotorArt(i));
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.wall = gs::uploadMipped(vdp, wallArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.pad = gs::uploadMipped(vdp, padArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    for (int c = 32; c < 127; c++) {
        art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace helilock
