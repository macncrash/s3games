#include "game/art.h"

#include <string>

namespace bikelock {
namespace {

using gs::Bitmap;

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

Bitmap bikeArt(bool duck) {
    Bitmap b(64, 52);
    b.ellipse(16, 38, 11, 11, 1);
    b.ellipse(16, 38, 5, 5, 5);
    b.ellipse(48, 38, 11, 11, 1);
    b.ellipse(48, 38, 5, 5, 5);
    b.line(16, 38, 48, 38, 2, 2.5f);
    b.line(16, 38, 30, 22, 2, 2.5f);
    b.line(48, 38, 34, 18, 2, 2.5f);
    b.line(24, 24, 46, 22, 4, 2.5f);
    b.rect(42, 16, 4, 6, 6);
    float hy = duck ? 20.f : 8.f;
    float neck = duck ? 24.f : 14.f;
    b.ellipse(34, hy, 5, 5, 3);
    b.ellipse(36, hy - 1, 2, 2, 7);
    b.line(34, neck, 32, duck ? 30.f : 26.f, 3, 3.f);
    b.line(32, duck ? 22.f : 18.f, 44, 20, 3, 2.5f);
    b.line(32, duck ? 30.f : 26.f, 22, 36, 3, 2.5f);
    b.line(32, duck ? 30.f : 26.f, 40, 36, 3, 2.5f);
    b.outline(1, false);
    return b;
}

Bitmap gateArt() {
    Bitmap b(24, 96);
    b.rect(2, 0, 20, 96, 2);
    b.rect(4, 0, 4, 96, 4);
    for (int y = 8; y < 92; y += 14) {
        b.rect(6, y, 12, 3, 1);
        b.ellipse(12, float(y + 8), 2.2f, 2.2f, 3);
    }
    b.outline(1, false);
    return b;
}

Bitmap stoneArt() {
    Bitmap b(32, 24);
    b.rect(0, 0, 32, 24, 2);
    b.rect(1, 1, 14, 10, 3);
    b.rect(17, 1, 14, 10, 4);
    b.rect(1, 13, 14, 10, 4);
    b.rect(17, 13, 14, 10, 3);
    b.rect(0, 11, 32, 2, 1);
    return b;
}

Bitmap waterArt() {
    Bitmap b(32, 16);
    b.rect(0, 0, 32, 16, 1);
    b.rect(0, 0, 32, 3, 4);
    for (int x = 0; x < 32; x += 8) b.rect(x, 6, 5, 2, 2);
    for (int x = 4; x < 32; x += 8) b.rect(x, 11, 5, 2, 3);
    return b;
}

Bitmap treeArt() {
    Bitmap b(40, 56);
    b.rect(17, 30, 6, 26, 1);
    b.ellipse(20, 20, 16, 16, 2);
    b.ellipse(14, 24, 8, 8, 3);
    b.ellipse(26, 16, 6, 6, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    for (int i = 0; i < 16; i++) {
        pal(vdp, PAL_HUD, i, 0, 0, 0);
        pal(vdp, PAL_BIKE, i, 0, 0, 0);
        pal(vdp, PAL_GATE, i, 0, 0, 0);
        pal(vdp, PAL_STONE, i, 0, 0, 0);
        pal(vdp, PAL_WATER, i, 0, 0, 0);
        pal(vdp, PAL_TREE, i, 0, 0, 0);
        pal(vdp, PAL_ALERT, i, 0, 0, 0);
    }
    pal(vdp, PAL_HUD, 1, 15, 15, 14);
    pal(vdp, PAL_HUD, 2, 15, 12, 3);
    pal(vdp, PAL_HUD, 3, 15, 4, 3);
    pal(vdp, PAL_HUD, 4, 6, 15, 8);
    pal(vdp, PAL_BIKE, 1, 2, 2, 3);
    pal(vdp, PAL_BIKE, 2, 4, 5, 7);
    pal(vdp, PAL_BIKE, 3, 13, 8, 5);
    pal(vdp, PAL_BIKE, 4, 12, 3, 2);
    pal(vdp, PAL_BIKE, 5, 8, 9, 10);
    pal(vdp, PAL_BIKE, 6, 14, 10, 2);
    pal(vdp, PAL_BIKE, 7, 4, 3, 2);
    pal(vdp, PAL_GATE, 1, 3, 4, 5);
    pal(vdp, PAL_GATE, 2, 7, 8, 9);
    pal(vdp, PAL_GATE, 3, 10, 6, 3);
    pal(vdp, PAL_GATE, 4, 13, 14, 14);
    pal(vdp, PAL_STONE, 1, 6, 6, 5);
    pal(vdp, PAL_STONE, 2, 9, 8, 7);
    pal(vdp, PAL_STONE, 3, 11, 10, 8);
    pal(vdp, PAL_STONE, 4, 7, 9, 6);
    pal(vdp, PAL_WATER, 1, 2, 5, 10);
    pal(vdp, PAL_WATER, 2, 3, 8, 13);
    pal(vdp, PAL_WATER, 3, 5, 11, 15);
    pal(vdp, PAL_WATER, 4, 12, 14, 15);
    pal(vdp, PAL_TREE, 1, 5, 3, 1);
    pal(vdp, PAL_TREE, 2, 3, 8, 3);
    pal(vdp, PAL_TREE, 3, 5, 11, 4);
    pal(vdp, PAL_TREE, 4, 8, 13, 6);
    pal(vdp, PAL_ALERT, 1, 15, 6, 2);
    pal(vdp, PAL_ALERT, 2, 15, 14, 8);

    art.bike = gs::uploadMipped(vdp, bikeArt(false));
    art.duck = gs::uploadMipped(vdp, bikeArt(true));
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.tree = gs::uploadMipped(vdp, treeArt());

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    st.spacing = 1;
    for (int c = 32; c < 96; c++) {
        Bitmap g = gs::textBitmap(std::string(1, char(c)), st);
        art.glyph[c - 32] = gs::uploadMipped(vdp, g);
    }
    vdp.setFogColor(gs::rgb4(6, 8, 10));
}

}  // namespace bikelock
