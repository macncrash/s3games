#include "game/art.h"

#include <string>

namespace buspass {
namespace {

using gs::Bitmap;

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

Bitmap busArt() {
    Bitmap b(96, 48);
    b.rect(6, 12, 84, 24, 2);
    b.rect(8, 14, 68, 9, 4);
    b.rect(10, 16, 12, 5, 5);
    b.rect(26, 16, 12, 5, 5);
    b.rect(42, 16, 12, 5, 5);
    b.rect(58, 16, 12, 5, 5);
    b.rect(78, 18, 8, 14, 3);
    b.rect(4, 22, 6, 10, 6);
    b.rect(86, 20, 6, 8, 1);
    b.ellipse(24, 38, 8, 8, 1);
    b.ellipse(24, 38, 3, 3, 7);
    b.ellipse(72, 38, 8, 8, 1);
    b.ellipse(72, 38, 3, 3, 7);
    b.rect(16, 8, 30, 5, 6);
    b.outline(1, false);
    return b;
}

Bitmap crewArt() {
    Bitmap b(48, 24);
    b.rect(2, 6, 42, 12, 2);
    b.rect(4, 8, 28, 5, 4);
    b.ellipse(12, 18, 4, 4, 1);
    b.ellipse(36, 18, 4, 4, 1);
    b.rect(40, 8, 4, 6, 3);
    return b;
}

Bitmap gateArt() {
    Bitmap b(16, 72);
    b.rect(2, 0, 12, 72, 2);
    b.rect(5, 0, 4, 72, 4);
    for (int y = 4; y < 68; y += 10) b.rect(3, y, 10, 3, 3);
    b.rect(0, 0, 16, 5, 1);
    b.outline(1, false);
    return b;
}

Bitmap pineArt() {
    Bitmap b(28, 40);
    b.poly({{14, 1}, {26, 22}, {2, 22}}, 2);
    b.poly({{14, 12}, {27, 34}, {1, 34}}, 3);
    b.rect(12, 32, 4, 8, 1);
    return b;
}

Bitmap rockArt() {
    Bitmap b(32, 22);
    b.poly({{2, 20}, {8, 6}, {18, 2}, {30, 14}, {24, 21}}, 2);
    b.poly({{10, 10}, {18, 6}, {22, 14}, {12, 16}}, 3);
    b.outline(1, false);
    return b;
}

Bitmap roadArt() {
    Bitmap b(40, 12);
    b.rect(0, 0, 40, 12, 2);
    b.rect(0, 0, 40, 2, 4);
    b.rect(0, 10, 40, 2, 1);
    b.rect(16, 5, 8, 2, 3);
    return b;
}

Bitmap peakArt() {
    Bitmap b(64, 36);
    b.poly({{0, 35}, {18, 10}, {32, 2}, {48, 16}, {63, 35}}, 2);
    b.poly({{22, 12}, {32, 4}, {40, 14}, {30, 16}}, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    for (int p = 0; p < 8; p++)
        for (int i = 0; i < 16; i++) pal(vdp, p, i, 0, 0, 0);

    pal(vdp, PAL_HUD, 1, 15, 15, 13);
    pal(vdp, PAL_HUD, 2, 15, 12, 3);
    pal(vdp, PAL_HUD, 3, 8, 14, 15);
    pal(vdp, PAL_HUD, 4, 6, 15, 8);
    pal(vdp, PAL_BUS, 1, 2, 2, 3);
    pal(vdp, PAL_BUS, 2, 13, 10, 2);
    pal(vdp, PAL_BUS, 3, 12, 4, 3);
    pal(vdp, PAL_BUS, 4, 8, 12, 14);
    pal(vdp, PAL_BUS, 5, 13, 15, 15);
    pal(vdp, PAL_BUS, 6, 15, 14, 8);
    pal(vdp, PAL_BUS, 7, 6, 6, 7);
    pal(vdp, PAL_SNOW, 1, 6, 7, 8);
    pal(vdp, PAL_SNOW, 2, 13, 14, 15);
    pal(vdp, PAL_SNOW, 3, 10, 12, 14);
    pal(vdp, PAL_SNOW, 4, 15, 15, 15);
    pal(vdp, PAL_ROCK, 1, 4, 4, 5);
    pal(vdp, PAL_ROCK, 2, 8, 8, 8);
    pal(vdp, PAL_ROCK, 3, 12, 12, 12);
    pal(vdp, PAL_PINE, 1, 3, 2, 1);
    pal(vdp, PAL_PINE, 2, 2, 7, 3);
    pal(vdp, PAL_PINE, 3, 4, 11, 5);
    pal(vdp, PAL_ROAD, 1, 3, 3, 3);
    pal(vdp, PAL_ROAD, 2, 6, 6, 6);
    pal(vdp, PAL_ROAD, 3, 14, 12, 3);
    pal(vdp, PAL_ROAD, 4, 9, 9, 9);
    pal(vdp, PAL_CREW, 1, 2, 2, 3);
    pal(vdp, PAL_CREW, 2, 12, 3, 3);
    pal(vdp, PAL_CREW, 3, 15, 14, 6);
    pal(vdp, PAL_CREW, 4, 6, 8, 12);
    pal(vdp, PAL_ALERT, 1, 15, 5, 2);
    pal(vdp, PAL_ALERT, 2, 15, 13, 6);

    art.bus = gs::uploadMipped(vdp, busArt());
    art.crew = gs::uploadMipped(vdp, crewArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.road = gs::uploadMipped(vdp, roadArt());
    art.peak = gs::uploadMipped(vdp, peakArt());

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    st.spacing = 1;
    for (int c = 32; c < 96; c++) {
        Bitmap g = gs::textBitmap(std::string(1, char(c)), st);
        art.glyph[c - 32] = gs::uploadMipped(vdp, g);
    }
    vdp.setFogColor(gs::rgb4(10, 11, 12));
}

}  // namespace buspass
