#include "game/art.h"

#include <string>

namespace buslock {
namespace {

using gs::Bitmap;

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

Bitmap busArt() {
    Bitmap b(96, 48);
    b.rect(6, 10, 84, 26, 2);
    b.rect(8, 12, 70, 10, 4);
    b.rect(10, 14, 12, 6, 5);
    b.rect(26, 14, 12, 6, 5);
    b.rect(42, 14, 12, 6, 5);
    b.rect(58, 14, 12, 6, 5);
    b.rect(78, 16, 8, 16, 3);
    b.rect(4, 22, 6, 10, 6);
    b.rect(86, 20, 6, 8, 1);
    b.ellipse(24, 38, 8, 8, 1);
    b.ellipse(24, 38, 3, 3, 7);
    b.ellipse(72, 38, 8, 8, 1);
    b.ellipse(72, 38, 3, 3, 7);
    b.rect(18, 8, 28, 4, 6);
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
    Bitmap b(20, 88);
    b.rect(2, 0, 16, 88, 2);
    b.rect(5, 0, 4, 88, 4);
    for (int y = 6; y < 82; y += 12) b.rect(4, y, 12, 3, 1);
    b.rect(0, 0, 20, 6, 3);
    b.outline(1, false);
    return b;
}

Bitmap stoneArt() {
    Bitmap b(32, 20);
    b.rect(0, 0, 32, 20, 2);
    b.rect(1, 1, 14, 8, 3);
    b.rect(17, 1, 14, 8, 4);
    b.rect(1, 11, 14, 8, 4);
    b.rect(17, 11, 14, 8, 3);
    b.rect(0, 9, 32, 2, 1);
    return b;
}

Bitmap waterArt() {
    Bitmap b(36, 14);
    b.rect(0, 0, 36, 14, 1);
    b.rect(0, 0, 36, 3, 4);
    for (int x = 0; x < 36; x += 9) b.rect(x, 6, 5, 2, 2);
    for (int x = 4; x < 36; x += 9) b.rect(x, 10, 5, 2, 3);
    return b;
}

Bitmap lampArt() {
    Bitmap b(16, 40);
    b.rect(6, 12, 4, 28, 1);
    b.ellipse(8, 8, 6, 6, 3);
    b.ellipse(8, 8, 3, 3, 2);
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
    pal(vdp, PAL_BUS, 2, 14, 11, 2);
    pal(vdp, PAL_BUS, 3, 12, 4, 3);
    pal(vdp, PAL_BUS, 4, 8, 12, 14);
    pal(vdp, PAL_BUS, 5, 13, 15, 15);
    pal(vdp, PAL_BUS, 6, 15, 14, 8);
    pal(vdp, PAL_BUS, 7, 6, 6, 7);
    pal(vdp, PAL_GATE, 1, 3, 3, 4);
    pal(vdp, PAL_GATE, 2, 8, 7, 5);
    pal(vdp, PAL_GATE, 3, 12, 8, 3);
    pal(vdp, PAL_GATE, 4, 14, 13, 10);
    pal(vdp, PAL_STONE, 1, 5, 5, 5);
    pal(vdp, PAL_STONE, 2, 8, 8, 7);
    pal(vdp, PAL_STONE, 3, 11, 10, 8);
    pal(vdp, PAL_STONE, 4, 6, 8, 6);
    pal(vdp, PAL_WATER, 1, 1, 4, 9);
    pal(vdp, PAL_WATER, 2, 2, 7, 12);
    pal(vdp, PAL_WATER, 3, 4, 10, 14);
    pal(vdp, PAL_WATER, 4, 11, 14, 15);
    pal(vdp, PAL_BANK, 1, 4, 3, 2);
    pal(vdp, PAL_BANK, 2, 14, 12, 4);
    pal(vdp, PAL_BANK, 3, 6, 10, 4);
    pal(vdp, PAL_CREW, 1, 2, 2, 3);
    pal(vdp, PAL_CREW, 2, 12, 3, 3);
    pal(vdp, PAL_CREW, 3, 15, 14, 6);
    pal(vdp, PAL_CREW, 4, 6, 8, 12);
    pal(vdp, PAL_ALERT, 1, 15, 5, 2);
    pal(vdp, PAL_ALERT, 2, 15, 13, 6);

    art.bus = gs::uploadMipped(vdp, busArt());
    art.crew = gs::uploadMipped(vdp, crewArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    st.spacing = 1;
    for (int c = 32; c < 96; c++) {
        Bitmap g = gs::textBitmap(std::string(1, char(c)), st);
        art.glyph[c - 32] = gs::uploadMipped(vdp, g);
    }
    vdp.setFogColor(gs::rgb4(5, 7, 9));
}

}  // namespace buslock
