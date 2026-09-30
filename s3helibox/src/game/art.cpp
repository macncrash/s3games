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
    b.rect(18, 28, 46, 3, 6);             // skids
    b.rect(22, 26, 3, 6, 5);
    b.rect(56, 26, 3, 6, 5);
    b.ellipse(46, 18, 22, 12, 2);         // cabin
    b.ellipse(46, 17, 20, 10, 1);
    b.ellipse(56, 16, 10, 7, 4);          // glass
    b.ellipse(58, 15, 6, 4, 8);
    b.rect(14, 16, 16, 5, 2);             // tail boom
    b.rect(8, 12, 8, 10, 3);              // fin
    b.rect(10, 14, 4, 6, 7);
    b.rect(44, 4, 3, 10, 5);              // mast
    b.ellipse(40, 20, 3, 3, 9);           // door
    b.outline(5, false);
    return b;
}

Bitmap rotorArt(int frame) {
    Bitmap b(100, 16);
    if (frame == 0) {
        b.rect(2, 7, 96, 3, 8);
        b.rect(46, 4, 8, 9, 5);
    } else if (frame == 1) {
        b.line(8, 12, 92, 3, 8, 3);
        b.rect(46, 4, 8, 9, 5);
    } else {
        b.ellipse(50, 8, 6, 6, 8);
        b.rect(46, 3, 8, 11, 5);
        b.rect(18, 7, 14, 2, 3);
        b.rect(68, 7, 14, 2, 3);
    }
    return b;
}

Bitmap boxArt() {
    Bitmap b(128, 88);
    b.rect(0, 0, 14, 80, 2);
    b.rect(2, 2, 10, 76, 1);
    b.rect(114, 0, 14, 80, 2);
    b.rect(116, 2, 10, 76, 1);
    b.rect(0, 72, 128, 16, 3);
    b.rect(14, 74, 100, 4, 6);  // pad stripe
    b.rect(48, 78, 32, 6, 7);
    // Chevrons on the inner faces so the opening reads as a box.
    for (int i = 0; i < 4; i++) {
        b.rect(4, 8 + i * 16, 6, 3, 8);
        b.rect(118, 8 + i * 16, 6, 3, 8);
    }
    b.outline(5, false);
    return b;
}

Bitmap groundArt() {
    Bitmap b(160, 32);
    b.rect(0, 8, 160, 24, 2);
    b.rect(0, 8, 160, 4, 1);
    for (int x = 0; x < 160; x += 16) b.rect(x, 18, 8, 3, 3);
    return b;
}

Bitmap hillArt() {
    Bitmap b(80, 40);
    b.poly({{0, 40}, {20, 14}, {40, 28}, {58, 6}, {80, 40}}, 1);
    b.poly({{6, 40}, {22, 18}, {36, 30}, {40, 40}}, 2);
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 12, 14), gs::rgb4(15, 12, 3),
                          gs::rgb4(15, 4, 3), gs::rgb4(4, 14, 6), gs::rgb4(8, 8, 9), shadow});
    setPal(vdp, PAL_SHIP, {0, gs::rgb4(12, 13, 8), gs::rgb4(8, 9, 5), gs::rgb4(4, 5, 3),
                           gs::rgb4(6, 10, 14), gs::rgb4(1, 1, 1), gs::rgb4(14, 12, 3),
                           gs::rgb4(14, 3, 2), gs::rgb4(13, 15, 15), gs::rgb4(3, 3, 3), shadow});
    setPal(vdp, PAL_BOX, {0, gs::rgb4(12, 8, 3), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1),
                          gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 1), gs::rgb4(14, 12, 4),
                          gs::rgb4(13, 3, 2), gs::rgb4(15, 14, 8), shadow});
    setPal(vdp, PAL_WORLD, {0, gs::rgb4(5, 9, 4), gs::rgb4(3, 6, 3), gs::rgb4(7, 8, 4),
                            gs::rgb4(15, 14, 15), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 10), shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(14, 14, 12), gs::rgb4(15, 15, 15), gs::rgb4(10, 10, 8), shadow});

    art.body = gs::uploadMipped(vdp, bodyArt());
    for (int i = 0; i < 3; i++) art.rotor[i] = gs::uploadMipped(vdp, rotorArt(i));
    art.box = gs::uploadMipped(vdp, boxArt());
    art.ground = gs::uploadMipped(vdp, groundArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    for (int c = 32; c < 127; c++) {
        art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace heli
