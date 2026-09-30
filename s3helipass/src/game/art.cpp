#include "game/art.h"

#include <string>

namespace helipass {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap bodyArt() {
    Bitmap b(88, 40);
    b.ellipse(46, 20, 22, 11, 2);
    b.ellipse(46, 19, 18, 8, 1);
    b.ellipse(58, 17, 10, 6, 4);
    b.ellipse(60, 16, 5, 3, 8);
    b.rect(14, 18, 16, 4, 2);
    b.rect(6, 12, 10, 12, 3);
    b.rect(8, 14, 5, 6, 6);
    b.rect(40, 6, 3, 8, 5);
    b.rect(30, 28, 32, 2, 5);
    b.ellipse(36, 22, 2, 3, 9);
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

Bitmap rockArt() {
    Bitmap b(48, 40);
    b.rect(0, 0, 48, 40, 1);
    b.poly({{0, 8}, {12, 0}, {28, 6}, {48, 0}, {48, 40}, {0, 40}}, 2);
    for (int x = 4; x < 48; x += 14) b.rect(x, 16, 6, 10, 3);
    b.rect(0, 34, 48, 6, 4);
    return b;
}

Bitmap snowArt() {
    Bitmap b(48, 24);
    b.rect(0, 8, 48, 16, 1);
    b.poly({{0, 14}, {10, 4}, {22, 12}, {36, 2}, {48, 10}, {48, 24}, {0, 24}}, 2);
    b.rect(0, 18, 48, 6, 3);
    return b;
}

Bitmap padArt() {
    Bitmap b(72, 16);
    b.rect(0, 4, 72, 12, 2);
    b.rect(4, 6, 64, 3, 5);
    b.rect(30, 6, 12, 8, 4);
    b.outline(6, false);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(64, 28);
    b.ellipse(22, 16, 16, 9, 1);
    b.ellipse(40, 14, 18, 10, 1);
    b.ellipse(32, 12, 12, 7, 2);
    b.ellipse(48, 18, 10, 6, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 10, 12), gs::rgb4(15, 13, 4),
                          gs::rgb4(15, 5, 3), gs::rgb4(6, 14, 8), gs::rgb4(6, 7, 9), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_SHIP, {0, gs::rgb4(13, 14, 15), gs::rgb4(8, 10, 12), gs::rgb4(4, 5, 6),
                           gs::rgb4(6, 12, 15), gs::rgb4(2, 2, 2), gs::rgb4(15, 12, 3),
                           gs::rgb4(12, 3, 2), gs::rgb4(15, 15, 15), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(5, 5, 6), gs::rgb4(7, 6, 6), gs::rgb4(3, 3, 4),
                           gs::rgb4(4, 5, 3), gs::rgb4(9, 8, 7), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(12, 13, 14), gs::rgb4(15, 15, 15), gs::rgb4(9, 11, 13),
                           gs::rgb4(14, 10, 4), gs::rgb4(6, 8, 6), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(7, 8, 10), gs::rgb4(11, 12, 13), gs::rgb4(4, 5, 7),
                         gs::rgb4(14, 14, 15)});

    art.body = gs::uploadMipped(vdp, bodyArt());
    for (int i = 0; i < 3; i++) art.rotor[i] = gs::uploadMipped(vdp, rotorArt(i));
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.snow = gs::uploadMipped(vdp, snowArt());
    art.pad = gs::uploadMipped(vdp, padArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    for (int c = 32; c < 127; c++) {
        art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace helipass
