#include "game/art.h"

#include <string>

namespace railpass {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap engineArt() {
    gs::Bitmap b(88, 40);
    b.rect(8, 16, 62, 16, 2);
    b.rect(10, 18, 40, 10, 3);
    b.rect(52, 10, 22, 22, 2);
    b.rect(56, 13, 14, 8, 8);
    b.rect(4, 20, 8, 8, 4);
    b.rect(70, 8, 10, 6, 5);
    b.line(75, 8, 75, 2, 6, 2.f);
    b.ellipse(22, 34, 7, 7, 1);
    b.ellipse(22, 34, 3, 3, 7);
    b.ellipse(48, 34, 7, 7, 1);
    b.ellipse(48, 34, 3, 3, 7);
    b.ellipse(68, 34, 6, 6, 1);
    b.rect(6, 30, 70, 3, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap plowArt() {
    gs::Bitmap b(28, 22);
    b.poly({{2, 18}, {24, 6}, {26, 10}, {8, 20}}, 3);
    b.rect(0, 16, 10, 5, 2);
    b.outline(1, false);
    return b;
}

gs::Bitmap driftArt() {
    gs::Bitmap b(48, 28);
    b.ellipse(24, 18, 22, 10, 2);
    b.ellipse(14, 16, 10, 8, 3);
    b.ellipse(32, 15, 12, 9, 4);
    b.rect(6, 20, 36, 6, 5);
    return b;
}

gs::Bitmap peakArt() {
    gs::Bitmap b(72, 64);
    b.poly({{4, 62}, {28, 8}, {40, 28}, {52, 4}, {70, 62}}, 2);
    b.poly({{22, 22}, {28, 8}, {36, 24}}, 3);
    b.poly({{46, 18}, {52, 4}, {60, 22}}, 3);
    b.rect(0, 56, 72, 8, 4);
    return b;
}

gs::Bitmap pineArt() {
    gs::Bitmap b(24, 40);
    b.rect(10, 26, 4, 14, 2);
    b.poly({{12, 2}, {2, 28}, {22, 28}}, 3);
    b.poly({{12, 10}, {5, 22}, {19, 22}}, 4);
    return b;
}

gs::Bitmap sleeperArt() {
    gs::Bitmap b(26, 10);
    b.rect(0, 3, 26, 5, 2);
    b.rect(1, 1, 24, 2, 3);
    b.rect(1, 7, 24, 2, 3);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(56, 24);
    b.ellipse(16, 14, 14, 8, 2);
    b.ellipse(32, 10, 16, 10, 3);
    b.ellipse(44, 14, 10, 7, 2);
    return b;
}

gs::Bitmap mouthArt() {
    gs::Bitmap b(36, 72);
    b.rect(0, 0, 10, 72, 2);
    b.rect(26, 0, 10, 72, 2);
    b.rect(8, 0, 20, 8, 3);
    b.rect(10, 10, 16, 50, 4);
    b.outline(1, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    setPal(vdp, PAL_HUD, {0, rgb4(15, 14, 12), rgb4(8, 9, 10), rgb4(4, 5, 6)});
    setPal(vdp, PAL_ENGINE, {0, rgb4(2, 2, 2), rgb4(12, 3, 2), rgb4(15, 8, 3), rgb4(6, 6, 7), rgb4(14, 12, 6),
                             rgb4(9, 9, 10), rgb4(4, 4, 5), rgb4(11, 14, 15)});
    setPal(vdp, PAL_SNOW, {0, rgb4(6, 7, 8), rgb4(13, 14, 15), rgb4(15, 15, 15), rgb4(9, 11, 13), rgb4(7, 8, 9)});
    setPal(vdp, PAL_ROCK, {0, rgb4(2, 2, 3), rgb4(6, 6, 7), rgb4(12, 13, 14), rgb4(4, 5, 6)});
    setPal(vdp, PAL_STORM, {0, rgb4(2, 3, 5), rgb4(5, 6, 8), rgb4(8, 9, 11), rgb4(11, 12, 13)});
    setPal(vdp, PAL_RAIL, {0, rgb4(2, 2, 2), rgb4(5, 4, 3), rgb4(10, 9, 8)});
    setPal(vdp, PAL_PINE, {0, rgb4(2, 2, 1), rgb4(4, 3, 2), rgb4(2, 6, 3), rgb4(4, 9, 4)});
    setPal(vdp, PAL_ALERT, {0, rgb4(15, 5, 2), rgb4(15, 11, 3), rgb4(8, 2, 1)});

    art.engine = gs::uploadMipped(vdp, engineArt());
    art.plow = gs::uploadMipped(vdp, plowArt());
    art.drift = gs::uploadMipped(vdp, driftArt());
    art.peak = gs::uploadMipped(vdp, peakArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.sleeper = gs::uploadMipped(vdp, sleeperArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.mouth = gs::uploadMipped(vdp, mouthArt());

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    for (int c = 32; c < 96; c++) {
        gs::Bitmap g = gs::textBitmap(std::string(1, char(c)), st);
        if (g.w < 1 || g.h < 1) g = gs::Bitmap(4, 7);
        art.glyph[c - 32] = gs::uploadMipped(vdp, g);
    }
}

}  // namespace railpass
