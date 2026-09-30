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
    Bitmap b(104, 52);
    b.rect(20, 36, 52, 3, 6);  // skids
    b.rect(26, 32, 3, 7, 5);
    b.rect(62, 32, 3, 7, 5);
    b.ellipse(54, 20, 24, 13, 2);
    b.ellipse(54, 19, 22, 11, 1);
    b.ellipse(66, 17, 11, 7, 4);  // canopy
    b.ellipse(68, 16, 6, 4, 8);
    b.rect(12, 18, 22, 5, 2);  // boom
    b.rect(4, 10, 10, 14, 3);  // fin
    b.rect(6, 12, 5, 8, 7);
    b.rect(50, 4, 4, 12, 5);  // mast
    b.rect(42, 22, 6, 8, 9);  // door
    b.rect(28, 22, 10, 2, 10);  // stripe
    b.outline(5, false);
    return b;
}

Bitmap rotorArt(int frame) {
    Bitmap b(112, 18);
    if (frame == 0) {
        b.rect(2, 8, 108, 3, 8);
        b.rect(52, 4, 8, 10, 5);
    } else if (frame == 1) {
        b.line(6, 14, 106, 3, 8, 3);
        b.rect(52, 4, 8, 10, 5);
    } else {
        b.ellipse(56, 9, 7, 6, 8);
        b.rect(52, 3, 8, 12, 5);
        b.rect(16, 8, 16, 2, 3);
        b.rect(80, 8, 16, 2, 3);
    }
    return b;
}

Bitmap rivalArt() {
    Bitmap b(72, 36);
    b.rect(14, 24, 36, 2, 3);
    b.rect(18, 21, 2, 5, 2);
    b.rect(42, 21, 2, 5, 2);
    b.ellipse(38, 14, 16, 9, 1);
    b.ellipse(46, 12, 7, 5, 4);
    b.rect(8, 12, 16, 3, 1);
    b.rect(2, 6, 8, 10, 2);
    b.rect(34, 2, 3, 8, 5);
    b.rect(6, 1, 60, 2, 6);
    b.outline(5, false);
    return b;
}

Bitmap grassArt() {
    Bitmap b(96, 36);
    b.rect(0, 10, 96, 26, 2);
    b.rect(0, 8, 96, 6, 1);
    for (int x = 2; x < 96; x += 7) {
        b.line(float(x), 22, float(x + 2), 6, (x / 7) % 2 ? 3 : 4, 1.4f);
        b.rect(x, 16, 2, 4, 5);
    }
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

Bitmap discArt() {
    Bitmap b(80, 22);
    b.ellipse(40, 11, 36, 8, 2);
    b.ellipse(40, 11, 22, 5, 1);
    b.ellipse(40, 11, 8, 3, 3);
    return b;
}

Bitmap treeArt() {
    Bitmap b(28, 48);
    b.rect(12, 28, 4, 18, 4);
    b.ellipse(14, 16, 12, 14, 1);
    b.ellipse(10, 18, 7, 8, 2);
    b.ellipse(18, 14, 6, 6, 3);
    return b;
}

Bitmap sockArt() {
    Bitmap b(40, 36);
    b.rect(4, 4, 3, 30, 5);
    b.poly({{7, 6}, {34, 12}, {30, 18}, {7, 14}}, 2);
    b.poly({{7, 8}, {28, 13}, {26, 16}, {7, 13}}, 1);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(70, 26);
    b.ellipse(22, 15, 16, 8, 1);
    b.ellipse(40, 13, 18, 9, 1);
    b.ellipse(30, 11, 10, 6, 2);
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
    setPal(vdp, PAL_RIVAL, {0, gs::rgb4(8, 3, 3), gs::rgb4(4, 2, 2), gs::rgb4(12, 8, 4),
                            gs::rgb4(6, 8, 10), gs::rgb4(1, 1, 1), gs::rgb4(14, 6, 3)});
    setPal(vdp, PAL_WORLD, {0, gs::rgb4(14, 14, 15), gs::rgb4(12, 13, 14), gs::rgb4(5, 8, 12),
                            gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(6, 12, 4), gs::rgb4(3, 8, 2), gs::rgb4(8, 13, 5),
                            gs::rgb4(4, 10, 3), gs::rgb4(2, 6, 2), gs::rgb4(14, 14, 10),
                            gs::rgb4(15, 15, 15), gs::rgb4(12, 4, 3)});

    art.body = gs::uploadMipped(vdp, bodyArt());
    for (int i = 0; i < 3; i++) art.rotor[i] = gs::uploadMipped(vdp, rotorArt(i));
    art.rival = gs::uploadMipped(vdp, rivalArt());
    art.grass = gs::uploadMipped(vdp, grassArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.disc = gs::uploadMipped(vdp, discArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.sock = gs::uploadMipped(vdp, sockArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    for (int c = 32; c < 127; c++) {
        art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace heli
