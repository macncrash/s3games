#include "game/art.h"

#include <cstring>

namespace purs {
namespace {

void glyphBmp(gs::Bitmap& b, char ch) {
    const uint8_t* g = gs::glyph(ch);
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 5; x++)
            if (g[y * 5 + x]) b.set(x, y, 1);
}

void paintGun(gs::Bitmap& b) {
    b.rect(6, 16, 40, 10, 2);
    b.rect(8, 18, 36, 6, 3);
    b.rect(44, 18, 16, 4, 4);
    b.rect(58, 19, 4, 2, 5);
    b.rect(18, 12, 8, 6, 6);
    b.rect(14, 26, 6, 10, 7);
    b.rect(28, 26, 6, 10, 7);
    b.rect(16, 34, 4, 4, 8);
    b.rect(30, 34, 4, 4, 8);
    b.rect(10, 22, 14, 3, 9);
    b.ellipse(22, 15, 3, 3, 5);
}

void paintDead(gs::Bitmap& b) {
    b.rect(2, 10, 28, 7, 2);
    b.rect(28, 12, 12, 3, 3);
    b.line(8, 8, 18, 18, 4, 1);
    b.rect(8, 17, 4, 8, 5);
    b.rect(16, 17, 4, 6, 5);
}

void paintFoe(gs::Bitmap& b) {
    b.ellipse(10, 6, 5, 4, 2);
    b.rect(6, 4, 8, 3, 3);
    b.rect(7, 10, 6, 10, 4);
    b.rect(5, 12, 3, 8, 5);
    b.rect(13, 12, 3, 8, 5);
    b.rect(6, 20, 3, 7, 6);
    b.rect(11, 20, 3, 7, 6);
    b.rect(14, 13, 6, 2, 7);
    b.set(8, 6, 8);
    b.set(11, 6, 8);
}

void paintFlash(gs::Bitmap& b) {
    b.ellipse(8, 6, 7, 5, 2);
    b.ellipse(8, 6, 3, 2, 3);
    b.rect(0, 5, 4, 2, 4);
}

void paintBag(gs::Bitmap& b) {
    b.ellipse(12, 7, 11, 5, 2);
    b.ellipse(12, 6, 8, 3, 3);
    b.line(4, 7, 20, 7, 4, 1);
}

void paintPost(gs::Bitmap& b) {
    b.rect(3, 0, 2, 16, 2);
    b.line(0, 4, 7, 8, 3, 1);
    b.line(0, 10, 7, 12, 3, 1);
}

void paintStar(gs::Bitmap& b) { b.set(1, 1, 1); }

void paintFlare(gs::Bitmap& b) {
    b.ellipse(6, 6, 5, 5, 2);
    b.ellipse(6, 6, 2, 2, 3);
}

void ink(gs::VDP& vdp, int pal, int i, int r, int g, int b) { vdp.setColor(pal * 16 + i, gs::rgb4(r, g, b)); }

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    for (int i = 0; i < 96; i++) {
        gs::Bitmap b(5, 7);
        glyphBmp(b, char(i + 32));
        art.glyph[i] = gs::uploadMipped(vdp, b);
    }
    gs::Bitmap gun(64, 40);
    paintGun(gun);
    art.gun = gs::uploadMipped(vdp, gun);
    gs::Bitmap dead(44, 26);
    paintDead(dead);
    art.dead = gs::uploadMipped(vdp, dead);
    gs::Bitmap foe(20, 28);
    paintFoe(foe);
    art.foe = gs::uploadMipped(vdp, foe);
    gs::Bitmap flash(16, 12);
    paintFlash(flash);
    art.flash = gs::uploadMipped(vdp, flash);
    gs::Bitmap bag(24, 14);
    paintBag(bag);
    art.bag = gs::uploadMipped(vdp, bag);
    gs::Bitmap post(8, 16);
    paintPost(post);
    art.post = gs::uploadMipped(vdp, post);
    gs::Bitmap star(3, 3);
    paintStar(star);
    art.star = gs::uploadMipped(vdp, star);
    gs::Bitmap flare(12, 12);
    paintFlare(flare);
    art.flare = gs::uploadMipped(vdp, flare);

    ink(vdp, PAL_INK, 1, 14, 13, 11);
    ink(vdp, PAL_AMBER, 1, 15, 11, 4);
    ink(vdp, PAL_ALERT, 1, 15, 4, 3);

    ink(vdp, PAL_GUN, 1, 0, 0, 0);
    ink(vdp, PAL_GUN, 2, 4, 4, 4);
    ink(vdp, PAL_GUN, 3, 7, 8, 6);
    ink(vdp, PAL_GUN, 4, 3, 3, 3);
    ink(vdp, PAL_GUN, 5, 12, 10, 6);
    ink(vdp, PAL_GUN, 6, 10, 8, 4);
    ink(vdp, PAL_GUN, 7, 5, 4, 3);
    ink(vdp, PAL_GUN, 8, 8, 6, 3);
    ink(vdp, PAL_GUN, 9, 13, 11, 5);

    ink(vdp, PAL_FOE, 2, 3, 4, 3);
    ink(vdp, PAL_FOE, 3, 5, 6, 4);
    ink(vdp, PAL_FOE, 4, 6, 6, 5);
    ink(vdp, PAL_FOE, 5, 4, 5, 3);
    ink(vdp, PAL_FOE, 6, 3, 3, 2);
    ink(vdp, PAL_FOE, 7, 8, 7, 4);
    ink(vdp, PAL_FOE, 8, 2, 2, 2);

    ink(vdp, PAL_FX, 2, 15, 13, 5);
    ink(vdp, PAL_FX, 3, 15, 15, 12);
    ink(vdp, PAL_FX, 4, 15, 8, 2);

    ink(vdp, PAL_BAG, 2, 6, 5, 3);
    ink(vdp, PAL_BAG, 3, 8, 7, 4);
    ink(vdp, PAL_BAG, 4, 4, 3, 2);

    ink(vdp, PAL_DEAD, 2, 3, 3, 3);
    ink(vdp, PAL_DEAD, 3, 5, 5, 4);
    ink(vdp, PAL_DEAD, 4, 8, 3, 2);
    ink(vdp, PAL_DEAD, 5, 2, 2, 2);

    for (int i = 1; i < 16; i++) ink(vdp, PAL_ROAD, i, 3 + (i % 3), 3, 2);
    ink(vdp, PAL_ROAD, 6, 4, 4, 2);
    ink(vdp, PAL_ROAD, 7, 5, 4, 2);
    ink(vdp, PAL_ROAD, 8, 3, 3, 2);
    ink(vdp, PAL_ROAD, 9, 2, 2, 1);
    ink(vdp, PAL_ROAD, 10, 3, 4, 2);
    ink(vdp, PAL_ROAD, 11, 2, 3, 4);
    ink(vdp, PAL_ROAD, 12, 3, 4, 5);
    ink(vdp, PAL_ROAD, 13, 6, 7, 8);
    ink(vdp, PAL_ROAD, 14, 4, 5, 3);
    ink(vdp, PAL_ROAD, 15, 7, 6, 4);

    vdp.setFogColor(gs::rgb4(1, 1, 2));
}

}  // namespace purs
