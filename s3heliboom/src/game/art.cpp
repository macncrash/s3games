#include "game/art.h"

#include <string>

namespace heliboom {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap bodyArt() {
    Bitmap b(104, 48);
    b.rect(22, 34, 50, 3, 6);
    b.rect(28, 30, 3, 6, 5);
    b.rect(62, 30, 3, 6, 5);
    b.ellipse(54, 18, 24, 12, 2);
    b.ellipse(54, 17, 22, 10, 1);
    b.ellipse(66, 15, 11, 7, 4);
    b.ellipse(68, 14, 6, 4, 8);
    b.rect(10, 16, 24, 5, 2);
    b.rect(2, 8, 10, 16, 3);
    b.rect(4, 10, 5, 10, 7);
    b.rect(50, 2, 4, 12, 5);
    b.rect(42, 20, 6, 7, 9);
    b.rect(28, 20, 10, 2, 10);
    b.outline(5, false);
    return b;
}

Bitmap rotorArt(int frame) {
    Bitmap b(112, 16);
    if (frame == 0) {
        b.rect(2, 7, 108, 3, 8);
        b.rect(52, 3, 8, 10, 5);
    } else if (frame == 1) {
        b.line(6, 13, 106, 2, 8, 3);
        b.rect(52, 3, 8, 10, 5);
    } else {
        b.ellipse(56, 8, 7, 5, 8);
        b.rect(52, 2, 8, 12, 5);
        b.rect(16, 7, 16, 2, 3);
        b.rect(80, 7, 16, 2, 3);
    }
    return b;
}

Bitmap driveArt() {
    Bitmap b(40, 28);
    b.rect(4, 6, 32, 18, 2);
    b.rect(6, 8, 28, 14, 1);
    b.ellipse(20, 15, 7, 5, 3);
    b.ellipse(20, 15, 3, 2, 4);
    b.rect(8, 4, 4, 4, 5);
    b.rect(28, 4, 4, 4, 5);
    b.rect(6, 22, 28, 3, 6);
    b.outline(5, false);
    return b;
}

Bitmap slingArt() {
    Bitmap b(6, 36);
    b.rect(2, 0, 2, 36, 1);
    return b;
}

Bitmap girderArt() {
    Bitmap b(48, 16);
    b.rect(0, 2, 48, 12, 2);
    b.rect(0, 2, 48, 3, 1);
    for (int x = 2; x < 46; x += 8) b.line(float(x), 14, float(x + 6), 4, 3, 1.2f);
    b.outline(4, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 64);
    b.rect(4, 0, 8, 64, 2);
    b.rect(5, 0, 3, 64, 1);
    for (int y = 4; y < 60; y += 10) b.rect(2, y, 12, 2, 3);
    return b;
}

Bitmap padArt() {
    Bitmap b(96, 14);
    b.rect(0, 2, 96, 10, 2);
    b.rect(4, 4, 88, 4, 1);
    for (int x = 8; x < 88; x += 14) b.rect(x, 4, 6, 4, 3);
    return b;
}

Bitmap waterArt() {
    Bitmap b(96, 28);
    b.rect(0, 6, 96, 22, 1);
    for (int x = 0; x < 96; x += 12) {
        b.ellipse(float(x + 6), 10, 6, 2, 2);
        b.ellipse(float(x + 2), 16, 5, 2, 3);
    }
    return b;
}

Bitmap cloudArt() {
    Bitmap b(70, 24);
    b.ellipse(22, 14, 16, 8, 1);
    b.ellipse(40, 12, 18, 9, 1);
    b.ellipse(30, 10, 10, 6, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 4), gs::rgb4(15, 5, 3),
                          gs::rgb4(6, 14, 8), gs::rgb4(8, 10, 12), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_SHIP, {0, gs::rgb4(14, 14, 12), gs::rgb4(9, 10, 7), gs::rgb4(4, 5, 3),
                           gs::rgb4(7, 12, 15), gs::rgb4(2, 2, 2), gs::rgb4(13, 11, 3),
                           gs::rgb4(12, 3, 2), gs::rgb4(14, 15, 15), gs::rgb4(3, 4, 4),
                           gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_DRIVE, {0, gs::rgb4(12, 10, 6), gs::rgb4(7, 6, 3), gs::rgb4(3, 8, 12),
                            gs::rgb4(10, 14, 15), gs::rgb4(2, 2, 2), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_BOOM, {0, gs::rgb4(14, 12, 6), gs::rgb4(10, 8, 4), gs::rgb4(6, 5, 3),
                           gs::rgb4(3, 3, 2), gs::rgb4(15, 14, 8), gs::rgb4(12, 4, 2)});
    setPal(vdp, PAL_WORLD, {0, gs::rgb4(4, 8, 12), gs::rgb4(8, 12, 15), gs::rgb4(3, 6, 9),
                            gs::rgb4(14, 14, 15), gs::rgb4(11, 12, 13)});

    art.body = gs::uploadMipped(vdp, bodyArt());
    for (int i = 0; i < 3; i++) art.rotor[i] = gs::uploadMipped(vdp, rotorArt(i));
    art.drive = gs::uploadMipped(vdp, driveArt());
    art.sling = gs::uploadMipped(vdp, slingArt());
    art.girder = gs::uploadMipped(vdp, girderArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.pad = gs::uploadMipped(vdp, padArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    for (int c = 32; c < 127; c++) {
        art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace heliboom
