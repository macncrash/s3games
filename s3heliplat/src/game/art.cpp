#include "game/art.h"

#include <string>

namespace heliplat {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap bodyArt() {
    Bitmap b(92, 36);
    b.ellipse(50, 16, 24, 10, 2);
    b.ellipse(50, 15, 20, 8, 1);
    b.ellipse(64, 14, 9, 5, 4);
    b.ellipse(66, 13, 4, 2, 8);
    b.rect(18, 15, 16, 4, 3);
    b.rect(8, 12, 12, 8, 5);
    b.rect(10, 14, 6, 4, 6);
    b.rect(46, 4, 3, 8, 5);
    b.rect(22, 24, 48, 3, 7);
    b.rect(26, 22, 2, 5, 5);
    b.rect(62, 22, 2, 5, 5);
    b.ellipse(42, 17, 2, 3, 9);
    b.outline(7, false);
    return b;
}

Bitmap rotorArt(int frame) {
    Bitmap b(100, 12);
    if (frame == 0) {
        b.rect(2, 5, 96, 2, 8);
        b.rect(46, 1, 8, 10, 5);
    } else if (frame == 1) {
        b.line(4, 10, 96, 2, 8, 2);
        b.rect(46, 1, 8, 10, 5);
    } else {
        b.ellipse(50, 6, 6, 5, 8);
        b.rect(46, 0, 8, 12, 5);
        b.rect(14, 5, 10, 2, 3);
        b.rect(76, 5, 10, 2, 3);
    }
    return b;
}

Bitmap deckArt() {
    Bitmap b(96, 18);
    b.rect(0, 4, 96, 12, 2);
    b.rect(0, 2, 96, 4, 1);
    for (int x = 4; x < 92; x += 16) b.rect(x, 8, 8, 3, 3);
    b.rect(0, 14, 96, 4, 4);
    b.outline(5, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 48);
    b.rect(5, 0, 6, 48, 1);
    b.rect(2, 0, 12, 4, 2);
    for (int y = 8; y < 48; y += 10) b.rect(6, y, 4, 3, 3);
    return b;
}

Bitmap hillArt() {
    Bitmap b(80, 40);
    b.poly({{0, 39}, {18, 16}, {36, 28}, {54, 8}, {80, 39}}, 1);
    b.poly({{20, 22}, {34, 30}, {48, 18}, {40, 39}, {16, 39}}, 2);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(52, 20);
    b.ellipse(16, 12, 12, 6, 1);
    b.ellipse(30, 10, 14, 7, 1);
    b.ellipse(24, 8, 8, 5, 2);
    return b;
}

Bitmap markArt() {
    Bitmap b(12, 16);
    b.rect(2, 0, 8, 16, 1);
    b.rect(4, 2, 4, 12, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 13), gs::rgb4(8, 10, 12), gs::rgb4(15, 13, 4),
                          gs::rgb4(15, 5, 3), gs::rgb4(4, 14, 8), gs::rgb4(5, 6, 7)});
    setPal(vdp, PAL_HELI, {0, gs::rgb4(15, 11, 3), gs::rgb4(12, 7, 2), gs::rgb4(7, 4, 2),
                           gs::rgb4(6, 12, 15), gs::rgb4(2, 2, 3), gs::rgb4(14, 14, 12),
                           gs::rgb4(3, 3, 3), gs::rgb4(1, 1, 1), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_DECK, {0, gs::rgb4(11, 11, 12), gs::rgb4(7, 8, 9), gs::rgb4(14, 12, 4),
                           gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(6, 6, 7), gs::rgb4(10, 10, 11), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_HILL, {0, gs::rgb4(3, 8, 4), gs::rgb4(5, 11, 5), gs::rgb4(2, 5, 3)});
    setPal(vdp, PAL_CLOUD, {0, gs::rgb4(14, 14, 15), gs::rgb4(11, 12, 14)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 13, 2), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_END, {0, gs::rgb4(14, 3, 2), gs::rgb4(8, 2, 2), gs::rgb4(15, 8, 3)});

    art.body = gs::uploadMipped(vdp, bodyArt());
    for (int i = 0; i < 3; i++) art.rotor[i] = gs::uploadMipped(vdp, rotorArt(i));
    art.deck = gs::uploadMipped(vdp, deckArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.mark = gs::uploadMipped(vdp, markArt());

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    for (int c = 32; c < 127; c++) {
        art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace heliplat
