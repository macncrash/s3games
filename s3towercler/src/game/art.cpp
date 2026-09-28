#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace tower {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap towerArt() {
    gs::Bitmap b(64, 120);
    b.rect(18, 18, 28, 96, 2);
    b.rect(18, 18, 8, 96, 1);
    b.rect(16, 14, 32, 8, 3);
    b.rect(14, 10, 6, 8, 4);
    b.rect(24, 8, 6, 10, 4);
    b.rect(34, 10, 6, 8, 4);
    b.rect(44, 10, 6, 8, 4);
    b.rect(22, 4, 4, 8, 5);
    b.rect(23, 0, 2, 6, 6);
    b.ellipse(32, 36, 9, 9, 7);
    b.ellipse(32, 36, 7, 7, 8);
    b.rect(31, 30, 2, 7, 9);
    b.rect(32, 35, 5, 2, 9);
    b.rect(26, 52, 6, 10, 10);
    b.rect(38, 52, 6, 10, 10);
    b.rect(26, 78, 6, 8, 10);
    b.rect(38, 78, 6, 8, 10);
    b.poly({{26, 100}, {38, 100}, {40, 114}, {24, 114}}, 11);
    b.rect(29, 104, 6, 10, 12);
    b.rect(10, 108, 44, 8, 3);
    b.outline(13, false);
    return b;
}

gs::Bitmap handArt(int step) {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 2, 2, 1);
    float ang = step * 0.785398f;
    float x1 = 8.f + std::cos(ang) * 6.f;
    float y1 = 8.f + std::sin(ang) * 6.f;
    b.line(8, 8, x1, y1, 2, 1.4f);
    return b;
}

gs::Bitmap keeperArt(int step) {
    gs::Bitmap b(28, 40);
    b.ellipse(14, 6, 7, 3, 3);
    b.rect(10, 6, 8, 3, 2);
    b.ellipse(14, 12, 5, 5, 4);
    b.set(12, 12, 5);
    b.set(16, 12, 5);
    b.poly({{8, 16}, {20, 16}, {22, 30}, {6, 30}}, 1);
    b.rect(12, 18, 4, 6, 6);
    if (step == 0) {
        b.rect(8, 30, 5, 8, 2);
        b.rect(15, 30, 5, 7, 2);
        b.rect(7, 36, 7, 2, 3);
        b.rect(14, 35, 7, 2, 3);
    } else {
        b.rect(8, 30, 5, 7, 2);
        b.rect(15, 30, 5, 8, 2);
        b.rect(7, 35, 7, 2, 3);
        b.rect(14, 36, 7, 2, 3);
    }
    b.outline(7, false);
    return b;
}

gs::Bitmap broomArt() {
    gs::Bitmap b(10, 28);
    b.rect(4, 0, 2, 16, 1);
    b.poly({{1, 14}, {9, 14}, {8, 26}, {2, 26}}, 2);
    b.line(2, 16, 2, 25, 3, 1.f);
    b.line(5, 16, 5, 26, 3, 1.f);
    b.line(8, 16, 7, 25, 3, 1.f);
    return b;
}

gs::Bitmap ivyArt() {
    gs::Bitmap b(36, 22);
    b.ellipse(10, 14, 9, 6, 1);
    b.ellipse(22, 12, 10, 7, 2);
    b.ellipse(16, 8, 5, 4, 3);
    b.line(8, 16, 28, 10, 4, 1.2f);
    b.outline(5, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(28, 24);
    b.rect(3, 6, 22, 16, 1);
    b.line(3, 6, 25, 22, 2, 1.2f);
    b.line(25, 6, 3, 22, 2, 1.2f);
    b.rect(3, 13, 22, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(32, 20);
    b.poly({{4, 14}, {12, 4}, {22, 8}, {28, 16}, {8, 18}}, 1);
    b.poly({{14, 10}, {24, 6}, {26, 14}, {16, 16}}, 2);
    b.ellipse(16, 16, 12, 4, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap barrelArt() {
    gs::Bitmap b(22, 26);
    b.ellipse(11, 6, 8, 4, 2);
    b.rect(3, 6, 16, 14, 1);
    b.ellipse(11, 20, 8, 4, 1);
    b.rect(3, 10, 16, 2, 3);
    b.rect(3, 16, 16, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(44, 14);
    b.rect(2, 3, 40, 4, 1);
    b.rect(4, 8, 36, 3, 2);
    b.rect(10, 4, 2, 8, 3);
    b.rect(28, 3, 2, 8, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(18, 28);
    b.rect(8, 2, 2, 10, 1);
    b.poly({{4, 12}, {14, 12}, {12, 22}, {6, 22}}, 2);
    b.rect(7, 14, 4, 4, 3);
    b.ellipse(9, 24, 6, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap moteArt() {
    gs::Bitmap b(6, 6);
    b.ellipse(3, 3, 2, 2, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TextStyle big{2, 1, 0, 0, 1};
    for (int c = 32; c < 128; c++) {
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 4), gs::rgb4(15, 4, 3), gs::rgb4(8, 8, 7)});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(9, 9, 8), gs::rgb4(7, 7, 7), gs::rgb4(5, 5, 5), gs::rgb4(11, 11, 10), gs::rgb4(12, 4, 3),
            gs::rgb4(14, 12, 6), gs::rgb4(13, 12, 8), gs::rgb4(15, 14, 10), gs::rgb4(2, 2, 2), gs::rgb4(3, 4, 6),
            gs::rgb4(6, 5, 4), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_KEEPER,
           {0, gs::rgb4(4, 6, 10), gs::rgb4(3, 4, 7), gs::rgb4(6, 4, 2), gs::rgb4(13, 9, 6), gs::rgb4(2, 2, 2),
            gs::rgb4(12, 10, 4), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_MOSS, {0, gs::rgb4(3, 8, 3), gs::rgb4(5, 10, 4), gs::rgb4(2, 5, 2), gs::rgb4(8, 12, 5), gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(11, 8, 3), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_RUST, {0, gs::rgb4(8, 4, 2), gs::rgb4(6, 3, 2), gs::rgb4(12, 8, 3), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(12, 9, 2), gs::rgb4(15, 13, 4), gs::rgb4(8, 6, 1)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(12, 11, 8)});
    setPal(vdp, PAL_IVY, {0, gs::rgb4(2, 7, 3), gs::rgb4(4, 9, 3), gs::rgb4(6, 11, 4), gs::rgb4(1, 4, 2), gs::rgb4(1, 2, 1)});

    art.tower = gs::uploadMipped(vdp, towerArt());
    for (int i = 0; i < 8; i++) art.hand[i] = gs::uploadMipped(vdp, handArt(i));
    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.broom = gs::uploadMipped(vdp, broomArt());
    art.ivy = gs::uploadMipped(vdp, ivyArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.mote = gs::uploadMipped(vdp, moteArt());
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(6, 7, 9));
}

}  // namespace tower
