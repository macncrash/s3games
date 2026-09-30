#include "game/art.h"

#include <string>

namespace raillock {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap carArt(bool dropped) {
    gs::Bitmap b(96, 56);
    b.rect(6, 22, 78, 22, 2);
    b.rect(8, 24, 74, 16, 3);
    b.rect(48, 12, 28, 14, 2);
    b.rect(50, 14, 24, 10, 4);
    b.rect(54, 16, 8, 6, 8);
    b.rect(68, 16, 4, 6, 5);
    if (dropped) {
        b.rect(58, 6, 18, 4, 6);
        b.line(62, 10, 70, 14, 6, 2.f);
    } else {
        b.line(64, 2, 58, 14, 6, 2.f);
        b.line(64, 2, 74, 14, 6, 2.f);
        b.rect(60, 0, 10, 3, 6);
    }
    b.ellipse(22, 46, 8, 8, 1);
    b.ellipse(22, 46, 4, 4, 7);
    b.ellipse(70, 46, 8, 8, 1);
    b.ellipse(70, 46, 4, 4, 7);
    b.rect(10, 42, 72, 3, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(24, 80);
    b.rect(2, 0, 20, 80, 2);
    for (int y = 6; y < 76; y += 10) b.rect(4, y, 16, 4, 3);
    b.rect(0, 0, 24, 6, 4);
    b.rect(8, 0, 8, 80, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(36, 28);
    b.rect(0, 0, 36, 28, 2);
    b.rect(2, 2, 15, 11, 3);
    b.rect(19, 2, 15, 11, 4);
    b.rect(2, 15, 15, 11, 4);
    b.rect(19, 15, 15, 11, 3);
    return b;
}

gs::Bitmap waterArt() {
    gs::Bitmap b(40, 18);
    b.rect(0, 0, 40, 18, 2);
    b.rect(0, 8, 40, 4, 3);
    b.line(2, 4, 16, 4, 4, 1.5f);
    b.line(22, 12, 36, 12, 5, 1.5f);
    return b;
}

gs::Bitmap sleeperArt() {
    gs::Bitmap b(28, 12);
    b.rect(0, 4, 28, 6, 2);
    b.rect(2, 1, 24, 3, 3);
    b.rect(2, 8, 24, 3, 3);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 48);
    b.rect(12, 28, 4, 18, 2);
    b.ellipse(14, 16, 12, 14, 3);
    b.ellipse(14, 14, 7, 8, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    setPal(vdp, PAL_HUD, {0, rgb4(15, 14, 11), rgb4(8, 7, 5), rgb4(4, 3, 2)});
    setPal(vdp, PAL_CAR, {0, rgb4(2, 2, 2), rgb4(10, 3, 2), rgb4(13, 6, 3), rgb4(15, 12, 8), rgb4(3, 3, 3),
                          rgb4(14, 13, 6), rgb4(6, 6, 7), rgb4(9, 14, 15)});
    setPal(vdp, PAL_GATE, {0, rgb4(1, 1, 1), rgb4(4, 5, 4), rgb4(7, 8, 6), rgb4(11, 10, 6), rgb4(13, 12, 8)});
    setPal(vdp, PAL_STONE, {0, rgb4(2, 2, 2), rgb4(7, 6, 5), rgb4(9, 8, 7), rgb4(6, 5, 4)});
    setPal(vdp, PAL_WATER, {0, rgb4(1, 3, 6), rgb4(2, 6, 9), rgb4(4, 9, 12), rgb4(8, 13, 14), rgb4(12, 15, 15)});
    setPal(vdp, PAL_RAIL, {0, rgb4(2, 2, 2), rgb4(5, 4, 3), rgb4(9, 8, 7)});
    setPal(vdp, PAL_TREE, {0, rgb4(2, 1, 1), rgb4(4, 3, 1), rgb4(2, 7, 3), rgb4(5, 11, 4)});
    setPal(vdp, PAL_ALERT, {0, rgb4(15, 4, 2), rgb4(15, 10, 3), rgb4(8, 1, 1)});

    art.car = gs::uploadMipped(vdp, carArt(false));
    art.low = gs::uploadMipped(vdp, carArt(true));
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.sleeper = gs::uploadMipped(vdp, sleeperArt());
    art.tree = gs::uploadMipped(vdp, treeArt());

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    for (int c = 32; c < 96; c++) {
        gs::Bitmap g = gs::textBitmap(std::string(1, char(c)), st);
        art.glyph[c - 32] = gs::uploadMipped(vdp, g);
    }
}

}  // namespace raillock
