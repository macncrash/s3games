#include "game/art.h"

#include <algorithm>
#include <string>

namespace mmaga {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* cs, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? cs[i] : 0);
}

gs::Bitmap millBody() {
    gs::Bitmap b(88, 108);
    b.rect(16, 34, 56, 70, 3);
    b.rect(20, 38, 48, 62, 4);
    for (int y = 42; y < 96; y += 7) b.rect(20, float(y), 48, 1, 2);
    b.rect(36, 72, 16, 28, 6);
    b.rect(40, 80, 8, 12, 1);
    b.rect(26, 48, 10, 12, 7);
    b.rect(52, 48, 10, 12, 7);
    b.poly({{14, 36}, {44, 6}, {74, 36}}, 5);
    b.rect(40, 0, 8, 14, 8);
    b.rect(38, 0, 12, 3, 9);
    b.outline(10, false);
    return b;
}

gs::Bitmap sailA() {
    gs::Bitmap b(64, 64);
    b.rect(29, 2, 6, 60, 1);
    b.rect(2, 29, 60, 6, 1);
    b.rect(30, 6, 4, 16, 2);
    b.rect(30, 42, 4, 16, 2);
    b.rect(6, 30, 16, 4, 2);
    b.rect(42, 30, 16, 4, 2);
    b.ellipse(32, 32, 5, 5, 3);
    return b;
}

gs::Bitmap sailB() {
    gs::Bitmap b(64, 64);
    b.line(8, 8, 56, 56, 1, 5);
    b.line(56, 8, 8, 56, 1, 5);
    b.line(14, 12, 36, 34, 2, 3);
    b.line(50, 14, 34, 30, 2, 3);
    b.ellipse(32, 32, 5, 5, 3);
    return b;
}

gs::Bitmap wheatRow() {
    gs::Bitmap b(28, 16);
    for (int i = 0; i < 5; i++) {
        float x = 3.f + i * 5.f;
        b.line(x, 14, x + 1, 3, 1, 1);
        b.ellipse(x + 1, 3, 2, 2, (i & 1) ? 2 : 3);
    }
    return b;
}

gs::Bitmap pathArt() {
    gs::Bitmap b(18, 40);
    b.poly({{2, 0}, {16, 0}, {14, 40}, {4, 40}}, 1);
    b.line(5, 8, 13, 8, 2, 1);
    b.line(5, 22, 13, 22, 2, 1);
    return b;
}

gs::Bitmap doorArt() {
    gs::Bitmap b(18, 28);
    b.rect(2, 2, 14, 24, 1);
    b.rect(4, 4, 10, 18, 2);
    b.ellipse(12, 14, 1, 1, 3);
    return b;
}

gs::Bitmap millerArt() {
    gs::Bitmap b(22, 32);
    b.ellipse(11, 6, 5, 5, 3);
    b.rect(7, 10, 8, 3, 4);
    b.rect(6, 13, 10, 10, 1);
    b.rect(4, 15, 3, 8, 2);
    b.rect(15, 15, 3, 8, 5);
    b.rect(7, 23, 3, 7, 6);
    b.rect(12, 23, 3, 7, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap man(int step, int kind) {
    gs::Bitmap b(24, 34);
    b.ellipse(12, 6, 5, 5, 3);
    b.rect(8, 10, 8, 3, 4);
    b.rect(6, 13, 12, 10, kind ? 2 : 1);
    b.rect(float(step ? 3 : 7), 14, 4, 8, 5);
    b.rect(float(step ? 15 : 13), 14, 4, 8, 5);
    b.rect(float(step ? 7 : 9), 23, 3, 8, 6);
    b.rect(float(step ? 13 : 12), 23, 3, 8, 6);
    if (!kind) b.line(16, 6, 22, 16, 8, 2);
    else b.rect(16, 14, 6, 2, 8);
    b.outline(7, false);
    return b;
}

gs::Bitmap downArt() {
    gs::Bitmap b(30, 12);
    b.ellipse(8, 6, 5, 4, 3);
    b.rect(12, 4, 14, 5, 1);
    b.rect(18, 8, 8, 2, 6);
    return b;
}

gs::Bitmap roundArt() {
    gs::Bitmap b(8, 16);
    b.rect(2, 2, 4, 10, 1);
    b.rect(1, 11, 6, 3, 2);
    b.rect(3, 0, 2, 2, 3);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(16, 10);
    b.poly({{0, 5}, {8, 0}, {16, 5}, {8, 10}}, 1);
    b.ellipse(8, 5, 3, 2, 2);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(12, 8);
    b.poly({{6, 0}, {12, 8}, {0, 8}}, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 9), gs::rgb4(5, 4, 3)};
    const uint16_t mill[] = {0, gs::rgb4(4, 3, 2), gs::rgb4(7, 6, 4), gs::rgb4(11, 10, 7), gs::rgb4(13, 12, 9),
                             gs::rgb4(9, 4, 3), gs::rgb4(3, 2, 1), gs::rgb4(8, 11, 13), gs::rgb4(5, 5, 5),
                             gs::rgb4(12, 11, 8), gs::rgb4(2, 1, 1)};
    const uint16_t sail[] = {0, gs::rgb4(14, 14, 12), gs::rgb4(9, 7, 4), gs::rgb4(5, 4, 3)};
    const uint16_t wheat[] = {0, gs::rgb4(10, 9, 3), gs::rgb4(7, 8, 2), gs::rgb4(13, 12, 5)};
    const uint16_t foe[] = {0, gs::rgb4(7, 2, 2), gs::rgb4(4, 4, 3), gs::rgb4(12, 8, 6), gs::rgb4(3, 2, 2),
                            gs::rgb4(9, 7, 4), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 1), gs::rgb4(11, 11, 8)};
    const uint16_t peel[] = {0, gs::rgb4(5, 5, 6), gs::rgb4(8, 7, 5), gs::rgb4(12, 9, 7), gs::rgb4(3, 3, 3),
                             gs::rgb4(8, 8, 6), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 2), gs::rgb4(10, 10, 8)};
    const uint16_t brass[] = {0, gs::rgb4(13, 10, 3), gs::rgb4(8, 6, 2), gs::rgb4(15, 14, 8)};
    const uint16_t fx[] = {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 15, 12)};
    const uint16_t ok[] = {0, gs::rgb4(7, 13, 5)};
    const uint16_t alert[] = {0, gs::rgb4(14, 4, 3)};
    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_MILL, mill, 11);
    setPal(vdp, PAL_SAIL, sail, 4);
    setPal(vdp, PAL_WHEAT, wheat, 4);
    setPal(vdp, PAL_FOE, foe, 9);
    setPal(vdp, PAL_PEEL, peel, 9);
    setPal(vdp, PAL_BRASS, brass, 4);
    setPal(vdp, PAL_FX, fx, 3);
    setPal(vdp, PAL_OK, ok, 2);
    setPal(vdp, PAL_ALERT, alert, 2);
    vdp.setFogColor(gs::rgb4(8, 8, 6));

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
    art.sailA = gs::uploadMipped(vdp, sailA());
    art.sailB = gs::uploadMipped(vdp, sailB());
    art.wheat = gs::uploadMipped(vdp, wheatRow());
    art.path = gs::uploadMipped(vdp, pathArt());
    art.door = gs::uploadMipped(vdp, doorArt());
    art.miller = gs::uploadMipped(vdp, millerArt());
    art.foe[0] = gs::uploadMipped(vdp, man(0, 0));
    art.foe[1] = gs::uploadMipped(vdp, man(1, 0));
    art.peel[0] = gs::uploadMipped(vdp, man(0, 1));
    art.peel[1] = gs::uploadMipped(vdp, man(1, 1));
    art.down = gs::uploadMipped(vdp, downArt());
    art.round = gs::uploadMipped(vdp, roundArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.mark = gs::uploadMipped(vdp, markArt());
}

}  // namespace mmaga
