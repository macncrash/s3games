#include "game/art.h"

#include <initializer_list>
#include <string>

namespace harbormaga {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap cutterArt() {
    gs::Bitmap b(56, 28);
    b.ellipse(28, 20, 24, 6, 1);
    b.rect(8, 16, 40, 6, 2);
    b.rect(6, 18, 44, 3, 1);
    b.poly({{10, 18}, {4, 20}, {12, 22}}, 3);
    b.rect(22, 8, 16, 10, 4);
    b.rect(24, 10, 5, 4, 5);
    b.rect(32, 10, 4, 4, 6);
    b.rect(36, 4, 3, 8, 7);
    b.rect(39, 4, 6, 3, 8);
    b.rect(18, 14, 8, 3, 3);
    b.ellipse(14, 22, 3, 2, 9);
    b.outline(7, false);
    return b;
}

gs::Bitmap wreckArt() {
    gs::Bitmap b(52, 18);
    b.ellipse(26, 12, 22, 5, 1);
    b.rect(8, 8, 28, 5, 2);
    b.rect(30, 6, 10, 6, 3);
    b.rect(14, 10, 6, 3, 4);
    b.line(8, 14, 44, 8, 9, 1);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(48, 40);
    b.poly({{2, 16}, {24, 4}, {46, 16}}, 1);
    b.rect(4, 16, 40, 20, 2);
    b.rect(4, 16, 8, 20, 3);
    b.rect(18, 22, 12, 14, 4);
    b.rect(20, 24, 8, 6, 5);
    b.rect(8, 20, 6, 5, 6);
    b.rect(34, 20, 6, 5, 6);
    b.rect(0, 34, 48, 4, 7);
    return b;
}

gs::Bitmap lightArt() {
    gs::Bitmap b(28, 64);
    b.rect(10, 18, 8, 40, 1);
    b.rect(8, 18, 4, 40, 2);
    b.rect(6, 54, 16, 8, 3);
    b.rect(8, 10, 12, 10, 4);
    b.rect(10, 12, 8, 6, 5);
    b.ellipse(14, 8, 6, 4, 6);
    b.rect(13, 2, 2, 6, 7);
    return b;
}

gs::Bitmap buoyArt() {
    gs::Bitmap b(16, 24);
    b.ellipse(8, 8, 6, 6, 1);
    b.rect(6, 8, 4, 8, 2);
    b.rect(4, 16, 8, 3, 3);
    b.ellipse(8, 20, 5, 2, 4);
    return b;
}

gs::Bitmap craneArt() {
    gs::Bitmap b(40, 48);
    b.rect(6, 28, 6, 16, 1);
    b.rect(4, 12, 30, 4, 2);
    b.rect(28, 16, 2, 14, 3);
    b.rect(24, 28, 10, 6, 4);
    b.rect(2, 42, 16, 4, 5);
    return b;
}

gs::Bitmap sightArt() {
    gs::Bitmap b(17, 17);
    b.rect(8, 0, 1, 17, 1);
    b.rect(0, 8, 17, 1, 1);
    b.rect(7, 7, 3, 3, 2);
    return b;
}

gs::Bitmap roundArt(int fill) {
    gs::Bitmap b(8, 14);
    b.rect(2, 2, 4, 10, fill);
    b.rect(2, 1, 4, 2, 3);
    b.rect(1, 10, 6, 2, 2);
    return b;
}

gs::Bitmap splashArt() {
    gs::Bitmap b(20, 16);
    b.ellipse(10, 10, 8, 4, 1);
    b.ellipse(6, 6, 3, 4, 2);
    b.ellipse(14, 5, 2, 4, 2);
    b.rect(9, 2, 2, 6, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 9), gs::rgb4(3, 3, 4), gs::rgb4(8, 7, 5)});
    setPal(vdp, PAL_HULL, {0, gs::rgb4(2, 3, 4), gs::rgb4(4, 5, 6), gs::rgb4(8, 8, 7), gs::rgb4(6, 5, 3),
                           gs::rgb4(10, 12, 13), gs::rgb4(2, 2, 3), ink, gs::rgb4(12, 3, 3), gs::rgb4(9, 10, 11)});
    setPal(vdp, PAL_WAKE, {0, gs::rgb4(3, 4, 5), gs::rgb4(5, 6, 7), gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 3),
                           gs::rgb4(6, 8, 9), gs::rgb4(2, 2, 2), ink, gs::rgb4(8, 2, 2), gs::rgb4(6, 7, 8)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(12, 9, 3), gs::rgb4(6, 5, 3), gs::rgb4(14, 12, 6), gs::rgb4(4, 4, 4)});
    setPal(vdp, PAL_PIER, {0, gs::rgb4(8, 4, 3), gs::rgb4(10, 8, 6), gs::rgb4(6, 5, 4), gs::rgb4(2, 2, 2),
                           gs::rgb4(12, 10, 6), gs::rgb4(5, 8, 10), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_LIGHT, {0, gs::rgb4(12, 12, 11), gs::rgb4(8, 8, 8), gs::rgb4(5, 5, 5), gs::rgb4(4, 4, 5),
                            gs::rgb4(14, 13, 6), gs::rgb4(14, 14, 12), gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(14, 4, 3), gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(14, 13, 10), gs::rgb4(8, 10, 12), gs::rgb4(14, 14, 14)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 5, 8), gs::rgb4(3, 7, 10), gs::rgb4(4, 8, 11), gs::rgb4(6, 10, 12),
                            gs::rgb4(1, 3, 5), gs::rgb4(8, 7, 5), gs::rgb4(5, 6, 4), gs::rgb4(9, 8, 6)});
    vdp.setFogColor(gs::rgb4(6, 8, 11));

    art.cutter = gs::uploadMipped(vdp, cutterArt());
    art.wreck = gs::uploadMipped(vdp, wreckArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.light = gs::uploadMipped(vdp, lightArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
    art.round = gs::uploadMipped(vdp, roundArt(1));
    art.spent = gs::uploadMipped(vdp, roundArt(2));
    art.splash = gs::uploadMipped(vdp, splashArt());

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    st.spacing = 1;
    for (int c = 32; c < 127; c++) {
        std::string s(1, char(c));
        gs::Bitmap g = gs::textBitmap(s, st);
        art.cellW = g.w;
        art.cellH = g.h;
        art.glyph[c - 32] = gs::uploadImage(vdp, g);
    }
}

}  // namespace harbormaga
