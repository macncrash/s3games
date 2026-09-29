#include "game/art.h"

#include <initializer_list>
#include <string>

namespace ccler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap keeperArt(int step) {
    gs::Bitmap b(32, 52);
    b.ellipse(16, 7, 6, 5, 6);
    b.rect(12, 11, 8, 3, 7);
    b.set(13, 8, 8);
    b.set(19, 8, 8);
    b.poly({{9, 16}, {23, 16}, {25, 34}, {7, 34}}, 1);
    b.rect(13, 18, 6, 7, 3);
    b.rect(6, 18, 4, 12, 2);
    b.rect(22, 18, 4, 12, 2);
    if (step == 0) {
        b.rect(10, 34, 5, 12, 4);
        b.rect(18, 34, 5, 10, 4);
        b.rect(9, 45, 7, 3, 5);
        b.rect(17, 43, 7, 3, 5);
    } else {
        b.rect(10, 34, 5, 10, 4);
        b.rect(18, 34, 5, 12, 4);
        b.rect(9, 43, 7, 3, 5);
        b.rect(17, 45, 7, 3, 5);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap rakeArt() {
    gs::Bitmap b(26, 36);
    b.line(8, 2, 16, 22, 3, 2.0f);
    b.rect(6, 22, 16, 3, 1);
    b.line(8, 24, 8, 33, 2, 1.2f);
    b.line(13, 24, 13, 34, 2, 1.2f);
    b.line(18, 24, 18, 33, 2, 1.2f);
    b.outline(4, false);
    return b;
}

gs::Bitmap timberArt() {
    gs::Bitmap b(36, 16);
    b.ellipse(8, 8, 5, 5, 2);
    b.rect(8, 3, 20, 10, 1);
    b.ellipse(28, 8, 5, 5, 2);
    b.line(12, 6, 24, 6, 3, 1.f);
    b.outline(4, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(28, 22);
    b.poly({{3, 5}, {24, 3}, {26, 18}, {5, 20}}, 1);
    b.line(4, 11, 25, 9, 2, 1.3f);
    b.line(13, 4, 14, 19, 3, 1.3f);
    b.outline(4, false);
    return b;
}

gs::Bitmap netArt() {
    gs::Bitmap b(30, 22);
    b.poly({{2, 4}, {28, 6}, {24, 18}, {6, 20}}, 1);
    for (int i = 0; i < 4; i++) b.line(4.f + i * 6, 5.f, 8.f + i * 5, 18.f, 2, 1.f);
    for (int i = 0; i < 3; i++) b.line(4, 8.f + i * 4, 26, 9.f + i * 3, 3, 1.f);
    b.outline(4, false);
    return b;
}

gs::Bitmap barrelArt() {
    gs::Bitmap b(20, 26);
    b.ellipse(10, 6, 7, 3, 2);
    b.rect(3, 6, 14, 16, 1);
    b.ellipse(10, 22, 7, 3, 2);
    b.rect(3, 10, 14, 2, 3);
    b.rect(3, 16, 14, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap buoyArt() {
    gs::Bitmap b(16, 28);
    b.ellipse(8, 10, 6, 7, 1);
    b.ellipse(8, 10, 3, 4, 2);
    b.rect(7, 16, 2, 8, 3);
    b.ellipse(8, 24, 3, 2, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(22, 18);
    b.ellipse(11, 9, 8, 6, 1);
    b.ellipse(11, 9, 4, 3, 2);
    b.line(4, 6, 16, 13, 3, 1.4f);
    b.outline(4, false);
    return b;
}

gs::Bitmap slabArt() {
    gs::Bitmap b(48, 20);
    b.rect(1, 2, 46, 16, 1);
    b.rect(1, 2, 46, 3, 2);
    for (int x = 8; x < 44; x += 12) b.rect(float(x), 3, 1, 14, 3);
    b.rect(1, 15, 46, 2, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 28);
    b.rect(3, 2, 4, 22, 1);
    b.rect(1, 2, 8, 3, 2);
    b.rect(2, 22, 6, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 32);
    b.rect(7, 0, 2, 12, 4);
    b.poly({{2, 12}, {14, 12}, {11, 22}, {5, 22}}, 1);
    b.rect(5, 14, 6, 5, 2);
    b.ellipse(8, 26, 2, 2, 3);
    b.outline(5, false);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(26, 26);
    b.ellipse(13, 13, 11, 11, 1);
    b.ellipse(13, 13, 8, 8, 2);
    b.line(13, 13, 13, 6, 3, 1.4f);
    b.line(13, 13, 19, 15, 4, 1.4f);
    b.outline(5, false);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(26, 8);
    b.ellipse(13, 4, 11, 3, 1);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4, 1);
    b.ellipse(6, 6, 2.4f, 2.0f, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& a) {
    setPal(vdp, PAL_TEXT, {gs::rgb4(0, 0, 0), gs::rgb4(14, 14, 12), gs::rgb4(7, 8, 9), gs::rgb4(15, 12, 4),
                           gs::rgb4(4, 12, 8), gs::rgb4(14, 4, 3)});
    setPal(vdp, PAL_STONE, {gs::rgb4(0, 0, 0), gs::rgb4(7, 7, 8), gs::rgb4(10, 10, 11), gs::rgb4(4, 4, 5),
                            gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_KEEPER, {gs::rgb4(0, 0, 0), gs::rgb4(2, 5, 10), gs::rgb4(1, 3, 6), gs::rgb4(12, 11, 8),
                             gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 2), gs::rgb4(13, 10, 7), gs::rgb4(8, 5, 3),
                             gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WOOD, {gs::rgb4(0, 0, 0), gs::rgb4(10, 6, 2), gs::rgb4(7, 4, 1), gs::rgb4(13, 9, 4),
                           gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_NET, {gs::rgb4(0, 0, 0), gs::rgb4(9, 10, 6), gs::rgb4(5, 6, 3), gs::rgb4(12, 13, 8),
                          gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_BUOY, {gs::rgb4(0, 0, 0), gs::rgb4(14, 3, 2), gs::rgb4(15, 14, 12), gs::rgb4(6, 6, 7),
                           gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WATER, {gs::rgb4(0, 0, 0), gs::rgb4(2, 6, 10), gs::rgb4(3, 8, 12), gs::rgb4(1, 3, 6),
                            gs::rgb4(8, 12, 14)});
    setPal(vdp, PAL_LAMP, {gs::rgb4(0, 0, 0), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 11), gs::rgb4(8, 6, 2),
                           gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_GOOD, {gs::rgb4(0, 0, 0), gs::rgb4(6, 14, 8), gs::rgb4(12, 15, 12)});
    setPal(vdp, PAL_ALERT, {gs::rgb4(0, 0, 0), gs::rgb4(14, 4, 3), gs::rgb4(15, 10, 4)});

    a.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    a.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    a.rake = gs::uploadMipped(vdp, rakeArt());
    a.timber = gs::uploadMipped(vdp, timberArt());
    a.crate = gs::uploadMipped(vdp, crateArt());
    a.net = gs::uploadMipped(vdp, netArt());
    a.barrel = gs::uploadMipped(vdp, barrelArt());
    a.buoy = gs::uploadMipped(vdp, buoyArt());
    a.rope = gs::uploadMipped(vdp, ropeArt());
    a.slab = gs::uploadMipped(vdp, slabArt());
    a.post = gs::uploadMipped(vdp, postArt());
    a.lamp = gs::uploadMipped(vdp, lampArt());
    a.clock = gs::uploadMipped(vdp, clockArt());
    a.shadow = gs::uploadMipped(vdp, shadowArt());
    a.puff = gs::uploadMipped(vdp, puffArt());

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    for (int c = 32; c < 127; c++) a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));

    vdp.setFogColor(gs::rgb4(1, 2, 4));
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.HUD.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.road[y].on = false;
        vdp.lineFog[y] = 0;
    }
}

}  // namespace ccler
