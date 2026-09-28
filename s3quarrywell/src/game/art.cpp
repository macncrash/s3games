#include "game/art.h"

namespace quarry {
namespace {

gs::Image up(gs::VDP& vdp, const gs::Bitmap& b) { return gs::uploadImage(vdp, b); }

void pal(gs::VDP& vdp, int p, int i, int r, int g, int b) {
    vdp.setColor(p * 16 + i, gs::rgb4(r, g, b));
}

void paintWell(gs::Bitmap& b) {
    b.rect(10, 8, 28, 58, 3);
    b.rect(12, 10, 24, 54, 4);
    b.rect(16, 18, 16, 22, 8);
    b.ellipse(24, 28, 6, 4, 9);
    b.rect(8, 6, 32, 6, 5);
    b.rect(6, 4, 4, 62, 5);
    b.rect(38, 4, 4, 62, 5);
    b.rect(4, 62, 40, 6, 2);
    b.rect(18, 0, 4, 10, 6);
    b.rect(26, 0, 4, 10, 6);
    b.line(18, 2, 30, 2, 6, 2);
    b.outline(1, true);
}

void paintMan(gs::Bitmap& b) {
    b.ellipse(12, 7, 5, 5, 4);
    b.rect(9, 3, 6, 3, 5);
    b.rect(8, 12, 8, 12, 7);
    b.rect(6, 14, 3, 8, 6);
    b.rect(15, 14, 3, 8, 6);
    b.rect(9, 24, 3, 8, 2);
    b.rect(13, 24, 3, 8, 2);
    b.rect(8, 31, 5, 2, 1);
    b.rect(13, 31, 5, 2, 1);
    b.set(10, 6, 1);
    b.set(14, 6, 1);
    b.outline(1, false);
}

void paintRock(gs::Bitmap& b) {
    b.ellipse(8, 8, 7, 6, 3);
    b.ellipse(6, 7, 3, 2, 4);
    b.outline(1, true);
}

void paintCart(gs::Bitmap& b) {
    b.rect(2, 4, 24, 8, 4);
    b.rect(4, 2, 16, 4, 5);
    b.ellipse(8, 13, 3, 3, 2);
    b.ellipse(20, 13, 3, 3, 2);
    b.rect(22, 5, 4, 3, 6);
    b.outline(1, true);
}

void paintBoulder(gs::Bitmap& b) {
    b.ellipse(16, 16, 14, 12, 3);
    b.ellipse(12, 12, 5, 4, 5);
    b.line(8, 18, 22, 10, 2, 1);
    b.outline(1, true);
}

void paintCliff(gs::Bitmap& b) {
    b.rect(0, 0, b.w, b.h, 3);
    for (int y = 8; y < b.h; y += 14) b.rect(0, y, b.w, 3, 2);
    for (int i = 0; i < 8; i++) b.rect(4 + (i * 9) % 28, 10 + i * 12, 8, 5, 4);
    b.outline(1, false);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    for (int p = 0; p < 16; p++) {
        pal(vdp, p, 1, 1, 1, 1);
        pal(vdp, p, 12, 15, 15, 14);
    }
    pal(vdp, PAL_HUD, 1, 15, 13, 6);
    pal(vdp, PAL_HUD, 2, 14, 4, 3);
    pal(vdp, PAL_HUD, 3, 8, 12, 6);

    pal(vdp, PAL_WELL, 2, 5, 4, 3);
    pal(vdp, PAL_WELL, 3, 8, 7, 5);
    pal(vdp, PAL_WELL, 4, 11, 10, 7);
    pal(vdp, PAL_WELL, 5, 7, 5, 2);
    pal(vdp, PAL_WELL, 6, 9, 7, 3);
    pal(vdp, PAL_WELL, 8, 2, 5, 7);
    pal(vdp, PAL_WELL, 9, 4, 10, 12);

    pal(vdp, PAL_MAN, 2, 4, 3, 2);
    pal(vdp, PAL_MAN, 4, 13, 10, 7);
    pal(vdp, PAL_MAN, 5, 12, 9, 3);
    pal(vdp, PAL_MAN, 6, 6, 8, 11);
    pal(vdp, PAL_MAN, 7, 4, 6, 10);

    pal(vdp, PAL_ROCK, 2, 5, 5, 5);
    pal(vdp, PAL_ROCK, 3, 8, 8, 7);
    pal(vdp, PAL_ROCK, 4, 12, 11, 9);

    pal(vdp, PAL_CART, 2, 3, 3, 3);
    pal(vdp, PAL_CART, 4, 10, 5, 2);
    pal(vdp, PAL_CART, 5, 13, 8, 3);
    pal(vdp, PAL_CART, 6, 14, 12, 4);

    pal(vdp, PAL_BOULDER, 2, 4, 4, 4);
    pal(vdp, PAL_BOULDER, 3, 7, 6, 5);
    pal(vdp, PAL_BOULDER, 5, 10, 9, 7);

    pal(vdp, PAL_FX, 1, 12, 10, 7);
    pal(vdp, PAL_FX, 3, 14, 12, 8);
    pal(vdp, PAL_CLIFF, 2, 5, 4, 3);
    pal(vdp, PAL_CLIFF, 3, 8, 6, 4);
    pal(vdp, PAL_CLIFF, 4, 10, 8, 5);

    const int road = PAL_ROAD;
    pal(vdp, road, 1, 8, 7, 4);
    pal(vdp, road, 2, 6, 5, 3);
    pal(vdp, road, 3, 9, 8, 5);
    pal(vdp, road, 4, 7, 6, 4);
    pal(vdp, road, 5, 5, 5, 3);
    pal(vdp, road, 6, 7, 7, 6);
    pal(vdp, road, 7, 5, 5, 5);
    pal(vdp, road, 8, 4, 4, 4);
    pal(vdp, road, 9, 3, 3, 3);
    pal(vdp, road, 10, 9, 8, 6);
    pal(vdp, road, 14, 12, 11, 8);
    pal(vdp, road, 15, 11, 10, 8);
    vdp.setFogColor(gs::rgb4(6, 6, 6));

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    st.spacing = 1;
    for (int c = 32; c < 127; c++) art.glyph[c - 32] = up(vdp, gs::textBitmap(std::string(1, char(c)), st));

    gs::Bitmap well(48, 72);
    paintWell(well);
    art.well = up(vdp, well);

    gs::Bitmap man(24, 34);
    paintMan(man);
    art.man = up(vdp, man);

    gs::Bitmap rock(16, 16);
    paintRock(rock);
    art.rock = up(vdp, rock);

    gs::Bitmap cart(28, 18);
    paintCart(cart);
    art.cart = up(vdp, cart);

    gs::Bitmap boulder(32, 30);
    paintBoulder(boulder);
    art.boulder = up(vdp, boulder);

    gs::Bitmap timber(22, 6);
    timber.rect(0, 1, 22, 4, 5);
    timber.outline(1, false);
    art.timber = up(vdp, timber);

    gs::Bitmap puff(12, 12);
    puff.ellipse(6, 6, 5, 4, 3);
    puff.outline(1, true);
    art.puff = up(vdp, puff);

    gs::Bitmap pip(1, 1);
    pip.set(0, 0, 1);
    art.pip = up(vdp, pip);

    gs::Bitmap cliff(36, 110);
    paintCliff(cliff);
    art.cliff = up(vdp, cliff);

    gs::Bitmap lamp(6, 6);
    lamp.ellipse(3, 3, 2, 2, 3);
    art.lamp = up(vdp, lamp);
}

}  // namespace quarry
