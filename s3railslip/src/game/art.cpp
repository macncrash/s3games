#include "game/art.h"

#include <string>

namespace railslip {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap boatArt() {
    gs::Bitmap b(96, 36);
    b.poly({{6, 16}, {78, 16}, {88, 22}, {80, 28}, {8, 28}}, 3);
    b.rect(14, 18, 58, 6, 4);
    b.rect(18, 8, 28, 10, 2);
    b.rect(22, 10, 10, 6, 8);
    b.rect(36, 10, 6, 6, 8);
    b.rect(46, 4, 3, 12, 5);
    b.poly({{46, 4}, {62, 8}, {46, 10}}, 6);
    b.ellipse(24, 30, 5, 5, 1);
    b.ellipse(24, 30, 2, 2, 7);
    b.ellipse(58, 30, 5, 5, 1);
    b.ellipse(58, 30, 2, 2, 7);
    b.rect(10, 26, 68, 3, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap sleeperArt() {
    gs::Bitmap b(28, 12);
    b.rect(0, 4, 28, 5, 2);
    b.rect(1, 2, 26, 2, 3);
    b.rect(1, 9, 26, 2, 3);
    return b;
}

gs::Bitmap pileArt() {
    gs::Bitmap b(16, 64);
    b.rect(4, 0, 8, 64, 2);
    b.rect(2, 0, 12, 6, 3);
    b.rect(5, 10, 2, 48, 4);
    b.rect(0, 28, 16, 4, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(48, 36);
    b.poly({{0, 14}, {24, 2}, {48, 14}}, 3);
    b.rect(4, 14, 40, 20, 2);
    b.rect(8, 18, 10, 8, 5);
    b.rect(28, 20, 10, 14, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(28, 12);
    b.poly({{2, 8}, {12, 4}, {14, 6}, {8, 8}}, 2);
    b.poly({{26, 8}, {16, 4}, {14, 6}, {20, 8}}, 2);
    b.ellipse(14, 7, 2, 2, 3);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(22, 28);
    b.rect(2, 2, 2, 24, 2);
    b.poly({{4, 3}, {20, 8}, {4, 13}}, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 40);
    b.rect(4, 8, 2, 32, 2);
    b.ellipse(5, 6, 4, 4, 3);
    b.ellipse(5, 6, 2, 2, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    setPal(vdp, PAL_HUD, {0, rgb4(15, 14, 11), rgb4(7, 8, 9), rgb4(3, 4, 5)});
    setPal(vdp, PAL_BOAT, {0, rgb4(2, 2, 2), rgb4(12, 8, 4), rgb4(14, 12, 8), rgb4(13, 3, 2), rgb4(4, 4, 5),
                           rgb4(15, 4, 3), rgb4(6, 5, 4), rgb4(10, 14, 15)});
    setPal(vdp, PAL_QUAY, {0, rgb4(2, 2, 2), rgb4(6, 5, 4), rgb4(10, 9, 7), rgb4(4, 4, 4)});
    setPal(vdp, PAL_PILE, {0, rgb4(1, 1, 1), rgb4(5, 4, 3), rgb4(8, 7, 5), rgb4(11, 10, 8)});
    setPal(vdp, PAL_FLAG, {0, rgb4(2, 2, 2), rgb4(6, 5, 4), rgb4(14, 3, 2), rgb4(15, 12, 4)});
    setPal(vdp, PAL_GULL, {0, rgb4(3, 3, 4), rgb4(14, 14, 13), rgb4(8, 7, 6)});
    setPal(vdp, PAL_ALERT, {0, rgb4(15, 5, 2), rgb4(15, 12, 4), rgb4(8, 2, 1)});
    setPal(vdp, PAL_WATER, {0, rgb4(1, 3, 6), rgb4(2, 5, 8), rgb4(3, 7, 10), rgb4(5, 9, 12), rgb4(8, 12, 13),
                            rgb4(2, 4, 5), rgb4(4, 6, 7), rgb4(10, 13, 14), rgb4(1, 2, 4), rgb4(6, 8, 9),
                            rgb4(3, 5, 6), rgb4(7, 10, 11), rgb4(9, 11, 12), rgb4(2, 6, 8), rgb4(4, 8, 10)});

    art.boat = gs::uploadMipped(vdp, boatArt());
    art.sleeper = gs::uploadMipped(vdp, sleeperArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    for (int c = 32; c < 96; c++) {
        gs::Bitmap g = gs::textBitmap(std::string(1, char(c)), st);
        if (g.w < 1 || g.h < 1) g = gs::Bitmap(4, 7);
        art.glyph[c - 32] = gs::uploadMipped(vdp, g);
    }
}

}  // namespace railslip
