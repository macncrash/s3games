#include "game/art.h"

#include <string>

namespace slip {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap bodyArt() {
    Bitmap b(100, 48);
    b.rect(18, 34, 50, 3, 6);
    b.rect(24, 30, 3, 6, 5);
    b.rect(58, 30, 3, 6, 5);
    b.ellipse(52, 18, 22, 12, 2);
    b.ellipse(52, 17, 20, 10, 1);
    b.ellipse(64, 15, 10, 6, 4);
    b.ellipse(66, 14, 5, 3, 8);
    b.rect(10, 16, 20, 4, 2);
    b.rect(2, 8, 10, 16, 3);
    b.rect(4, 12, 5, 8, 7);
    b.rect(48, 3, 4, 11, 5);
    b.rect(40, 20, 6, 7, 9);
    b.rect(26, 20, 12, 2, 10);
    b.outline(5, false);
    return b;
}

Bitmap rotorArt(int frame) {
    Bitmap b(108, 16);
    if (frame == 0) {
        b.rect(2, 7, 104, 3, 8);
        b.rect(50, 3, 8, 10, 5);
    } else if (frame == 1) {
        b.line(4, 13, 104, 2, 8, 3);
        b.rect(50, 3, 8, 10, 5);
    } else {
        b.ellipse(54, 8, 6, 5, 8);
        b.rect(50, 2, 8, 12, 5);
        b.rect(14, 7, 14, 2, 3);
        b.rect(78, 7, 14, 2, 3);
    }
    return b;
}

Bitmap rivalArt() {
    Bitmap b(68, 34);
    b.rect(12, 22, 34, 2, 3);
    b.rect(16, 19, 2, 5, 2);
    b.rect(40, 19, 2, 5, 2);
    b.ellipse(36, 13, 15, 8, 1);
    b.ellipse(44, 11, 6, 4, 4);
    b.rect(6, 11, 14, 3, 1);
    b.rect(1, 5, 7, 10, 2);
    b.rect(32, 2, 3, 8, 5);
    b.rect(4, 1, 56, 2, 6);
    b.outline(5, false);
    return b;
}

Bitmap pileArt() {
    Bitmap b(28, 96);
    b.rect(6, 0, 16, 96, 2);
    b.rect(6, 0, 4, 96, 1);
    b.rect(18, 0, 4, 96, 3);
    for (int y = 8; y < 90; y += 14) b.rect(6, y, 16, 2, 4);
    b.rect(2, 0, 24, 8, 5);
    b.rect(4, 2, 20, 3, 6);
    b.outline(7, false);
    return b;
}

Bitmap deckArt() {
    Bitmap b(72, 18);
    b.rect(0, 4, 72, 12, 2);
    b.rect(0, 2, 72, 4, 1);
    for (int x = 4; x < 70; x += 10) b.rect(x, 6, 2, 8, 4);
    b.rect(30, 0, 12, 4, 5);
    b.rect(34, 1, 4, 2, 6);
    return b;
}

Bitmap waveArt() {
    Bitmap b(80, 20);
    b.rect(0, 8, 80, 12, 1);
    for (int x = 0; x < 80; x += 16) {
        b.ellipse(float(x + 8), 8, 8, 3, 2);
        b.ellipse(float(x + 4), 13, 6, 2, 3);
    }
    return b;
}

Bitmap buoyArt() {
    Bitmap b(22, 36);
    b.rect(10, 16, 2, 18, 4);
    b.ellipse(11, 10, 8, 8, 1);
    b.ellipse(11, 10, 4, 4, 2);
    b.rect(9, 2, 4, 4, 3);
    return b;
}

Bitmap gullArt() {
    Bitmap b(28, 12);
    b.line(2, 8, 14, 4, 1, 1.5f);
    b.line(14, 4, 26, 8, 1, 1.5f);
    b.rect(13, 5, 3, 2, 2);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(64, 22);
    b.ellipse(20, 13, 14, 7, 1);
    b.ellipse(36, 11, 16, 8, 1);
    b.ellipse(28, 9, 9, 5, 2);
    return b;
}

Bitmap gaugeArt() {
    Bitmap b(14, 70);
    b.rect(4, 2, 6, 66, 3);
    b.rect(5, 4, 4, 62, 1);
    for (int y = 8; y < 64; y += 10) b.rect(2, y, 10, 2, 2);
    b.rect(1, 28, 12, 3, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 4), gs::rgb4(15, 6, 3),
                          gs::rgb4(8, 14, 12), gs::rgb4(7, 9, 12), gs::rgb4(2, 2, 4)});
    setPal(vdp, PAL_SHIP, {0, gs::rgb4(14, 14, 13), gs::rgb4(8, 9, 8), gs::rgb4(3, 4, 3),
                           gs::rgb4(6, 12, 15), gs::rgb4(2, 2, 2), gs::rgb4(12, 10, 3),
                           gs::rgb4(13, 3, 2), gs::rgb4(14, 15, 15), gs::rgb4(4, 5, 5),
                           gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_RIVAL, {0, gs::rgb4(9, 3, 3), gs::rgb4(4, 2, 2), gs::rgb4(12, 8, 4),
                            gs::rgb4(6, 9, 11), gs::rgb4(1, 1, 1), gs::rgb4(14, 7, 3)});
    setPal(vdp, PAL_SEA, {0, gs::rgb4(3, 8, 13), gs::rgb4(6, 12, 15), gs::rgb4(2, 5, 9),
                          gs::rgb4(12, 10, 4), gs::rgb4(15, 14, 8), gs::rgb4(15, 15, 15),
                          gs::rgb4(8, 8, 9)});
    setPal(vdp, PAL_PIER, {0, gs::rgb4(12, 10, 7), gs::rgb4(8, 6, 4), gs::rgb4(5, 4, 3),
                           gs::rgb4(3, 3, 2), gs::rgb4(14, 12, 8), gs::rgb4(11, 9, 5),
                           gs::rgb4(2, 2, 2), gs::rgb4(15, 4, 3)});

    art.body = gs::uploadMipped(vdp, bodyArt());
    for (int i = 0; i < 3; i++) art.rotor[i] = gs::uploadMipped(vdp, rotorArt(i));
    art.rival = gs::uploadMipped(vdp, rivalArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.deck = gs::uploadMipped(vdp, deckArt());
    art.wave = gs::uploadMipped(vdp, waveArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.gauge = gs::uploadMipped(vdp, gaugeArt());

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    for (int c = 32; c < 127; c++) {
        art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace slip
