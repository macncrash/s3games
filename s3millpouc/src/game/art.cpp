#include "game/art.h"

#include <algorithm>
#include <string>

namespace mpouc {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* cs, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? cs[i] : 0);
}

gs::Bitmap millBody() {
    gs::Bitmap b(72, 96);
    b.rect(14, 28, 44, 64, 3);
    b.rect(18, 32, 36, 56, 4);
    for (int y = 36; y < 84; y += 8) b.rect(18, float(y), 36, 2, 2);
    b.rect(30, 64, 12, 24, 6);
    b.rect(33, 72, 6, 10, 1);
    b.rect(22, 42, 8, 10, 7);
    b.rect(42, 42, 8, 10, 7);
    b.poly({{10, 30}, {36, 4}, {62, 30}}, 5);
    b.rect(33, 0, 6, 12, 8);
    b.rect(30, 0, 12, 3, 9);
    b.outline(10, false);
    return b;
}

gs::Bitmap capArt() {
    gs::Bitmap b(28, 10);
    b.poly({{0, 8}, {14, 0}, {28, 8}}, 1);
    b.rect(12, 2, 4, 6, 2);
    return b;
}

gs::Bitmap sailArt() {
    gs::Bitmap b(14, 48);
    b.rect(4, 2, 6, 44, 1);
    b.rect(2, 6, 10, 14, 2);
    b.rect(2, 26, 10, 14, 3);
    b.line(3, 8, 11, 18, 4, 1);
    return b;
}

gs::Bitmap bucketArt() {
    gs::Bitmap b(16, 12);
    b.poly({{1, 2}, {15, 2}, {12, 11}, {4, 11}}, 1);
    b.rect(4, 4, 8, 3, 2);
    b.line(2, 2, 14, 2, 3, 1);
    return b;
}

gs::Bitmap hubArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 1);
    b.ellipse(9, 9, 3, 3, 2);
    b.rect(8, 1, 2, 16, 3);
    b.rect(1, 8, 16, 2, 3);
    return b;
}

gs::Bitmap millerArt(bool jump) {
    gs::Bitmap b(20, 30);
    b.ellipse(10, 6, 5, 5, 3);
    b.rect(6, 4, 8, 3, 4);
    b.rect(5, 11, 10, 9, 1);
    b.rect(2, 13, 4, 7, 2);
    b.rect(14, 12, 4, 7, 5);
    if (jump) {
        b.rect(5, 20, 4, 6, 6);
        b.rect(11, 20, 4, 5, 6);
    } else {
        b.rect(6, 20, 3, 8, 6);
        b.rect(11, 20, 3, 8, 6);
    }
    b.outline(7, false);
    return b;
}

gs::Bitmap pouchArt() {
    gs::Bitmap b(14, 12);
    b.poly({{2, 3}, {12, 3}, {13, 11}, {1, 11}}, 1);
    b.rect(3, 4, 8, 5, 2);
    b.line(4, 1, 7, 4, 3, 1);
    b.line(10, 1, 7, 4, 3, 1);
    b.ellipse(7, 7, 1, 1, 4);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(18, 22);
    b.ellipse(9, 12, 7, 8, 1);
    b.rect(6, 3, 6, 6, 2);
    b.line(5, 8, 13, 8, 3, 1);
    b.ellipse(9, 13, 2, 2, 4);
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(22, 8);
    b.rect(0, 1, 22, 6, 1);
    b.line(0, 2, 22, 2, 2, 1);
    b.line(6, 1, 6, 7, 3, 1);
    b.line(15, 1, 15, 7, 3, 1);
    return b;
}

gs::Bitmap doorArt() {
    gs::Bitmap b(20, 32);
    b.rect(1, 1, 18, 30, 1);
    b.rect(4, 4, 12, 22, 2);
    b.rect(5, 6, 4, 6, 3);
    b.rect(11, 6, 4, 6, 3);
    b.ellipse(13, 16, 1, 1, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(10, 16);
    b.line(3, 15, 2, 3, 1, 1);
    b.line(6, 15, 8, 2, 2, 1);
    b.ellipse(2, 3, 2, 2, 3);
    b.ellipse(8, 2, 2, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 5, 4)};
    const uint16_t stone[] = {0, gs::rgb4(3, 2, 2), gs::rgb4(6, 5, 4), gs::rgb4(9, 8, 7), gs::rgb4(12, 11, 9),
                              gs::rgb4(8, 4, 3), gs::rgb4(4, 3, 2), gs::rgb4(10, 12, 13), gs::rgb4(5, 5, 6),
                              gs::rgb4(13, 12, 8), gs::rgb4(2, 2, 2)};
    const uint16_t sail[] = {0, gs::rgb4(14, 13, 11), gs::rgb4(11, 8, 5), gs::rgb4(7, 5, 3), gs::rgb4(15, 14, 10)};
    const uint16_t water[] = {0, gs::rgb4(3, 6, 8), gs::rgb4(5, 9, 11), gs::rgb4(8, 12, 13), gs::rgb4(2, 4, 5)};
    const uint16_t miller[] = {0, gs::rgb4(4, 6, 9), gs::rgb4(7, 8, 10), gs::rgb4(13, 10, 7), gs::rgb4(6, 4, 3),
                               gs::rgb4(12, 9, 5), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 1)};
    const uint16_t pouch[] = {0, gs::rgb4(10, 6, 3), gs::rgb4(13, 9, 4), gs::rgb4(6, 4, 2), gs::rgb4(14, 12, 6)};
    const uint16_t wood[] = {0, gs::rgb4(9, 6, 3), gs::rgb4(12, 9, 5), gs::rgb4(5, 3, 2), gs::rgb4(14, 11, 6),
                             gs::rgb4(3, 2, 1)};
    const uint16_t wheat[] = {0, gs::rgb4(11, 10, 3), gs::rgb4(7, 8, 2), gs::rgb4(13, 12, 6)};
    const uint16_t alert[] = {0, gs::rgb4(14, 4, 3)};
    const uint16_t ok[] = {0, gs::rgb4(6, 12, 5)};
    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_STONE, stone, 11);
    setPal(vdp, PAL_SAIL, sail, 5);
    setPal(vdp, PAL_WATER, water, 5);
    setPal(vdp, PAL_MILLER, miller, 8);
    setPal(vdp, PAL_POUCH, pouch, 5);
    setPal(vdp, PAL_WOOD, wood, 6);
    setPal(vdp, PAL_WHEAT, wheat, 4);
    setPal(vdp, PAL_ALERT, alert, 2);
    setPal(vdp, PAL_OK, ok, 2);
    vdp.setFogColor(gs::rgb4(8, 9, 8));

    for (int i = 0; i < 96; i++) {
        gs::TextStyle st;
        st.scale = 1;
        st.color = 1;
        gs::Bitmap g = gs::textBitmap(std::string(1, char(32 + i)), st);
        art.gw[i] = std::max(3, g.w);
        art.gh = std::max(art.gh, g.h);
        art.glyph[i] = gs::uploadImage(vdp, g.w > 0 ? g : gs::Bitmap(4, 8));
    }
    art.mill = gs::uploadMipped(vdp, millBody());
    art.cap = gs::uploadMipped(vdp, capArt());
    art.sail = gs::uploadMipped(vdp, sailArt());
    art.bucket = gs::uploadMipped(vdp, bucketArt());
    art.hub = gs::uploadMipped(vdp, hubArt());
    art.miller = gs::uploadMipped(vdp, millerArt(false));
    art.millerJ = gs::uploadMipped(vdp, millerArt(true));
    art.pouch = gs::uploadMipped(vdp, pouchArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.door = gs::uploadMipped(vdp, doorArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
}

}  // namespace mpouc
