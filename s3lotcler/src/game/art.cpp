#include "game/art.h"

#include <initializer_list>
#include <string>

namespace lotcler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap rigArt(int step) {
    gs::Bitmap b(44, 36);
    b.poly({{4, 10}, {34, 8}, {38, 22}, {6, 24}}, 1);
    b.rect(8, 12, 16, 8, 2);
    b.rect(10, 14, 6, 4, 3);
    b.rect(18, 14, 4, 4, 3);
    b.ellipse(12, 26, 5, 4, 4);
    b.ellipse(30, 25, 5, 4, 4);
    b.rect(6, 22, 28, 4, 5);
    if (step == 0) {
        b.rect(4, 26, 6, 3, 6);
        b.rect(12, 28, 8, 3, 6);
        b.rect(22, 26, 8, 3, 6);
    } else {
        b.rect(6, 28, 8, 3, 6);
        b.rect(16, 26, 8, 3, 6);
        b.rect(26, 28, 6, 3, 6);
    }
    b.rect(32, 12, 4, 6, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap tireArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 9, 9, 1);
    b.ellipse(11, 11, 5, 5, 2);
    b.ellipse(11, 11, 2, 2, 3);
    b.line(11, 4, 11, 8, 3, 1.2f);
    b.line(11, 14, 11, 18, 3, 1.2f);
    b.line(4, 11, 8, 11, 3, 1.2f);
    b.line(14, 11, 18, 11, 3, 1.2f);
    return b;
}

gs::Bitmap drumArt() {
    gs::Bitmap b(20, 28);
    b.ellipse(10, 6, 7, 3, 2);
    b.rect(3, 6, 14, 16, 1);
    b.rect(3, 10, 14, 3, 3);
    b.rect(3, 18, 14, 2, 3);
    b.ellipse(10, 22, 7, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap palletArt() {
    gs::Bitmap b(36, 18);
    b.rect(2, 3, 32, 4, 1);
    b.rect(2, 9, 32, 4, 1);
    b.rect(4, 3, 3, 12, 2);
    b.rect(16, 3, 3, 12, 2);
    b.rect(28, 3, 3, 12, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap leafArt() {
    gs::Bitmap b(28, 16);
    b.ellipse(10, 9, 8, 5, 1);
    b.ellipse(18, 7, 7, 4, 2);
    b.ellipse(14, 11, 6, 3, 3);
    b.line(8, 8, 16, 6, 4, 1.f);
    return b;
}

gs::Bitmap coneArt() {
    gs::Bitmap b(16, 28);
    b.poly({{8, 2}, {14, 22}, {2, 22}}, 1);
    b.rect(4, 10, 8, 3, 2);
    b.rect(3, 16, 10, 3, 2);
    b.rect(2, 22, 12, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(22, 26);
    b.poly({{6, 4}, {16, 4}, {18, 10}, {19, 22}, {3, 22}, {4, 10}}, 1);
    b.rect(7, 2, 8, 4, 2);
    b.line(8, 12, 14, 16, 3, 1.2f);
    b.outline(4, false);
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(40, 12);
    b.poly({{2, 3}, {36, 2}, {38, 9}, {4, 10}}, 1);
    b.line(6, 5, 32, 4, 2, 1.f);
    b.ellipse(8, 6, 1.2f, 1.2f, 3);
    b.ellipse(30, 5, 1.2f, 1.2f, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(16, 64);
    b.rect(7, 8, 2, 56, 1);
    b.poly({{2, 8}, {14, 8}, {12, 16}, {4, 16}}, 2);
    b.rect(4, 9, 8, 4, 3);
    b.ellipse(8, 62, 4, 2, 4);
    return b;
}

gs::Bitmap stallArt() {
    gs::Bitmap b(8, 48);
    for (int y = 0; y < 48; y += 8) b.rect(2, float(y), 4, 4, 1);
    return b;
}

gs::Bitmap fenceArt() {
    gs::Bitmap b(80, 20);
    b.rect(0, 4, 80, 3, 1);
    b.rect(0, 12, 80, 3, 1);
    for (int x = 2; x < 80; x += 10) b.rect(float(x), 2, 3, 16, 2);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(26, 26);
    b.ellipse(13, 13, 11, 11, 1);
    b.ellipse(13, 13, 8, 8, 2);
    b.line(13, 13, 13, 6, 3, 1.3f);
    b.line(13, 13, 19, 15, 4, 1.3f);
    b.outline(5, false);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(30, 10);
    b.ellipse(15, 5, 13, 3, 1);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 3, 1);
    b.ellipse(5, 5, 2, 1.5f, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& a) {
    setPal(vdp, PAL_INK, {gs::rgb4(0, 0, 0), gs::rgb4(14, 14, 12), gs::rgb4(8, 8, 7), gs::rgb4(15, 12, 3),
                          gs::rgb4(4, 12, 7), gs::rgb4(14, 4, 3)});
    setPal(vdp, PAL_YARD, {gs::rgb4(0, 0, 0), gs::rgb4(12, 12, 10), gs::rgb4(5, 5, 5), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_RIG, {gs::rgb4(0, 0, 0), gs::rgb4(14, 11, 2), gs::rgb4(8, 9, 11), gs::rgb4(12, 14, 15),
                          gs::rgb4(2, 2, 2), gs::rgb4(6, 6, 6), gs::rgb4(3, 3, 2), gs::rgb4(13, 4, 3),
                          gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_TIRE, {gs::rgb4(0, 0, 0), gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 5), gs::rgb4(9, 8, 7)});
    setPal(vdp, PAL_DRUM, {gs::rgb4(0, 0, 0), gs::rgb4(3, 6, 10), gs::rgb4(6, 9, 13), gs::rgb4(12, 10, 2),
                           gs::rgb4(2, 4, 7), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_WOOD, {gs::rgb4(0, 0, 0), gs::rgb4(9, 6, 2), gs::rgb4(6, 4, 1), gs::rgb4(3, 2, 1),
                           gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_LEAF, {gs::rgb4(0, 0, 0), gs::rgb4(4, 8, 2), gs::rgb4(8, 10, 3), gs::rgb4(6, 5, 2),
                           gs::rgb4(10, 8, 3)});
    setPal(vdp, PAL_CONE, {gs::rgb4(0, 0, 0), gs::rgb4(14, 6, 1), gs::rgb4(15, 14, 12), gs::rgb4(4, 4, 4),
                           gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_SACK, {gs::rgb4(0, 0, 0), gs::rgb4(10, 8, 5), gs::rgb4(6, 5, 3), gs::rgb4(4, 3, 2),
                           gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_POLE, {gs::rgb4(0, 0, 0), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4), gs::rgb4(14, 13, 6),
                           gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_DUST, {gs::rgb4(0, 0, 0), gs::rgb4(8, 8, 7), gs::rgb4(12, 12, 10)});
    setPal(vdp, PAL_GOOD, {gs::rgb4(0, 0, 0), gs::rgb4(6, 14, 7), gs::rgb4(12, 15, 12)});
    setPal(vdp, PAL_HOT, {gs::rgb4(0, 0, 0), gs::rgb4(14, 4, 3), gs::rgb4(15, 10, 3)});

    a.rig[0] = gs::uploadMipped(vdp, rigArt(0));
    a.rig[1] = gs::uploadMipped(vdp, rigArt(1));
    a.tire = gs::uploadMipped(vdp, tireArt());
    a.drum = gs::uploadMipped(vdp, drumArt());
    a.pallet = gs::uploadMipped(vdp, palletArt());
    a.leaves = gs::uploadMipped(vdp, leafArt());
    a.cone = gs::uploadMipped(vdp, coneArt());
    a.sack = gs::uploadMipped(vdp, sackArt());
    a.plank = gs::uploadMipped(vdp, plankArt());
    a.pole = gs::uploadMipped(vdp, poleArt());
    a.stall = gs::uploadMipped(vdp, stallArt());
    a.fence = gs::uploadMipped(vdp, fenceArt());
    a.clock = gs::uploadMipped(vdp, clockArt());
    a.shadow = gs::uploadMipped(vdp, shadowArt());
    a.dust = gs::uploadMipped(vdp, dustArt());

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    for (int c = 32; c < 127; c++) a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));

    vdp.setFogColor(gs::rgb4(2, 2, 3));
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.HUD.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.road[y].on = false;
        vdp.lineFog[y] = 0;
    }
}

}  // namespace lotcler
