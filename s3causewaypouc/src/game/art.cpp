#include "game/art.h"

namespace cwpouc {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* cs, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? cs[i] : 0);
}

gs::Bitmap walker(bool step, bool air) {
    gs::Bitmap b(28, 44);
    b.ellipse(14, 7, 5, 5, 2);
    b.rect(12, 12, 5, 12, 1);
    if (air) {
        b.rect(6, 12, 6, 3, 3);
        b.rect(16, 12, 6, 3, 3);
        b.rect(10, 24, 3, 10, 4);
        b.rect(15, 24, 3, 8, 4);
    } else if (step) {
        b.rect(8, 14, 4, 3, 3);
        b.rect(16, 16, 5, 3, 3);
        b.rect(10, 24, 3, 14, 4);
        b.rect(16, 24, 3, 10, 4);
    } else {
        b.rect(7, 16, 5, 3, 3);
        b.rect(16, 14, 4, 3, 3);
        b.rect(11, 24, 3, 10, 4);
        b.rect(15, 24, 3, 14, 4);
    }
    b.rect(11, 16, 6, 4, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap pouchArt() {
    gs::Bitmap b(22, 18);
    b.rect(4, 6, 14, 10, 1);
    b.rect(7, 3, 8, 4, 2);
    b.rect(9, 8, 4, 3, 3);
    b.line(4, 6, 10, 2, 2, 1);
    b.line(18, 6, 12, 2, 2, 1);
    b.outline(4, false);
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(32, 14);
    b.rect(1, 2, 30, 10, 1);
    b.rect(4, 4, 2, 6, 2);
    b.rect(26, 4, 2, 6, 2);
    b.line(1, 7, 30, 7, 3, 1);
    b.outline(4, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 36);
    b.rect(4, 2, 4, 30, 1);
    b.rect(2, 2, 8, 4, 2);
    b.rect(3, 30, 6, 4, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(28, 48);
    b.rect(2, 8, 4, 38, 1);
    b.rect(22, 8, 4, 38, 1);
    b.rect(2, 6, 24, 5, 2);
    b.rect(10, 16, 8, 10, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(24, 10);
    b.line(2, 6, 12, 3, 1, 2);
    b.line(12, 3, 22, 7, 1, 2);
    b.ellipse(12, 5, 2, 2, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    big.outline = 2;
    for (int c = 32; c < 127; c++) {
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(15, 15, 13), gs::rgb4(3, 3, 5)};
    const uint16_t walkerC[] = {0, gs::rgb4(12, 8, 5), gs::rgb4(14, 11, 8), gs::rgb4(8, 6, 4), gs::rgb4(3, 3, 4),
                                gs::rgb4(13, 10, 4), gs::rgb4(1, 1, 2)};
    const uint16_t pouch[] = {0, gs::rgb4(10, 5, 3), gs::rgb4(14, 10, 4), gs::rgb4(15, 13, 6), gs::rgb4(2, 1, 1)};
    const uint16_t plank[] = {0, gs::rgb4(8, 6, 4), gs::rgb4(5, 4, 3), gs::rgb4(11, 9, 6), gs::rgb4(2, 2, 2)};
    const uint16_t post[] = {0, gs::rgb4(7, 7, 7), gs::rgb4(11, 11, 10), gs::rgb4(4, 4, 4), gs::rgb4(2, 2, 2)};
    const uint16_t gate[] = {0, gs::rgb4(6, 7, 8), gs::rgb4(12, 11, 6), gs::rgb4(14, 13, 8), gs::rgb4(2, 2, 3)};
    const uint16_t gull[] = {0, gs::rgb4(14, 14, 13), gs::rgb4(8, 8, 9)};
    const uint16_t alert[] = {0, gs::rgb4(15, 6, 3), gs::rgb4(4, 2, 2)};
    const uint16_t good[] = {0, gs::rgb4(8, 15, 7), gs::rgb4(2, 5, 3)};
    const uint16_t title[] = {0, gs::rgb4(15, 13, 7), gs::rgb4(5, 4, 2)};
    const uint16_t water[] = {0, gs::rgb4(3, 7, 11), gs::rgb4(6, 11, 13)};
    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_WALKER, walkerC, 7);
    setPal(vdp, PAL_POUCH, pouch, 5);
    setPal(vdp, PAL_PLANK, plank, 5);
    setPal(vdp, PAL_POST, post, 5);
    setPal(vdp, PAL_GATE, gate, 5);
    setPal(vdp, PAL_GULL, gull, 3);
    setPal(vdp, PAL_ALERT, alert, 3);
    setPal(vdp, PAL_GOOD, good, 3);
    setPal(vdp, PAL_TITLE, title, 3);
    setPal(vdp, PAL_WATER, water, 3);
    vdp.setFogColor(gs::rgb4(4, 7, 11));

    loadFont(vdp, art);
    art.stand = gs::uploadMipped(vdp, walker(false, false));
    art.runA = gs::uploadMipped(vdp, walker(true, false));
    art.runB = gs::uploadMipped(vdp, walker(false, false));
    art.leap = gs::uploadMipped(vdp, walker(false, true));
    art.pouch = gs::uploadMipped(vdp, pouchArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
}

}  // namespace cwpouc
