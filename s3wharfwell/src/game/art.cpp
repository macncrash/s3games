#include "game/art.h"

namespace wharf {
namespace {

void pal(gs::VDP& vdp, int p, int i, int r, int g, int b) { vdp.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void keeperBmp(gs::Bitmap& b, int pose) {
    b.rect(10, 2, 12, 4, 3);
    b.rect(8, 5, 16, 3, 3);
    b.ellipse(16, 12, 5, 5, 1);
    b.rect(14, 11, 2, 2, 6);
    b.rect(9, 16, 14, 14, 2);
    b.rect(9, 16, 14, 3, 5);
    int leg = pose == 1 ? 3 : 0;
    b.rect(10, 30, 5, 8 + (pose == 1 ? -2 : 0), 4);
    b.rect(17, 30, 5, 8 + (pose == 1 ? 0 : 0), 4);
    b.rect(10, 36 + (pose == 1 ? -2 : 0), 5, 3, 7);
    b.rect(17, 36, 5, 3, 7);
    if (pose == 2) {
        b.line(18, 18, 31, 6, 5, 2);
        b.rect(28, 4, 4, 3, 8);
    } else {
        b.line(20, 18, 26, 36 - leg, 5, 2);
    }
    (void)leg;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    for (int i = 0; i < 16; i++) {
        pal(vdp, PAL_TEXT, i, 15, 15, 14);
        pal(vdp, PAL_GOLD, i, 15, 12, 3);
        pal(vdp, PAL_ALERT, i, 15, 4, 3);
    }
    pal(vdp, PAL_WOOD, 1, 10, 6, 2);
    pal(vdp, PAL_WOOD, 2, 7, 4, 1);
    pal(vdp, PAL_WOOD, 3, 13, 9, 4);
    pal(vdp, PAL_WOOD, 4, 4, 2, 1);
    pal(vdp, PAL_WOOD, 5, 5, 3, 1);

    pal(vdp, PAL_STONE, 1, 10, 10, 9);
    pal(vdp, PAL_STONE, 2, 7, 7, 6);
    pal(vdp, PAL_STONE, 3, 4, 4, 4);
    pal(vdp, PAL_STONE, 4, 1, 1, 2);
    pal(vdp, PAL_STONE, 5, 3, 6, 3);
    pal(vdp, PAL_STONE, 6, 13, 12, 10);
    pal(vdp, PAL_STONE, 7, 14, 10, 3);

    pal(vdp, PAL_KEEPER, 1, 13, 9, 6);
    pal(vdp, PAL_KEEPER, 2, 14, 11, 2);
    pal(vdp, PAL_KEEPER, 3, 2, 3, 7);
    pal(vdp, PAL_KEEPER, 4, 3, 2, 1);
    pal(vdp, PAL_KEEPER, 5, 8, 5, 2);
    pal(vdp, PAL_KEEPER, 6, 2, 2, 2);
    pal(vdp, PAL_KEEPER, 7, 1, 1, 1);
    pal(vdp, PAL_KEEPER, 8, 12, 12, 11);

    pal(vdp, PAL_CRAB, 1, 12, 3, 2);
    pal(vdp, PAL_CRAB, 2, 8, 2, 1);
    pal(vdp, PAL_CRAB, 3, 15, 8, 4);
    pal(vdp, PAL_CRAB, 4, 2, 1, 1);

    pal(vdp, PAL_BARREL, 1, 10, 6, 2);
    pal(vdp, PAL_BARREL, 2, 6, 3, 1);
    pal(vdp, PAL_BARREL, 3, 12, 10, 6);
    pal(vdp, PAL_BARREL, 4, 3, 2, 1);

    pal(vdp, PAL_FOAM, 1, 14, 15, 15);
    pal(vdp, PAL_FOAM, 2, 8, 12, 13);
    pal(vdp, PAL_FOAM, 3, 4, 8, 10);

    pal(vdp, PAL_BRUTE, 1, 6, 8, 4);
    pal(vdp, PAL_BRUTE, 2, 3, 5, 2);
    pal(vdp, PAL_BRUTE, 3, 12, 4, 2);
    pal(vdp, PAL_BRUTE, 4, 1, 2, 1);

    pal(vdp, PAL_GULL, 1, 15, 15, 15);
    pal(vdp, PAL_GULL, 2, 12, 12, 13);
    pal(vdp, PAL_GULL, 3, 14, 8, 2);
    pal(vdp, PAL_GULL, 4, 3, 3, 4);

    gs::Bitmap deck(304, 22);
    for (int x = 0; x < 304; x += 16) {
        int shade = ((x / 16) & 1) ? 1 : 3;
        deck.rect(float(x), 0, 15, 8, shade);
        deck.rect(float(x), 8, 15, 6, 2);
        deck.rect(float(x), 14, 15, 8, 4);
        deck.rect(float(x + 14), 0, 2, 22, 5);
    }
    art.deck = gs::uploadImage(vdp, deck);

    gs::Bitmap pile(10, 48);
    pile.rect(2, 0, 6, 48, 2);
    pile.rect(1, 0, 2, 48, 1);
    pile.rect(3, 6, 4, 3, 4);
    pile.rect(3, 22, 4, 3, 4);
    pile.rect(3, 38, 4, 3, 4);
    art.pile = gs::uploadImage(vdp, pile);

    gs::Bitmap well(52, 68);
    well.ellipse(26, 16, 20, 8, 6);
    well.ellipse(26, 16, 12, 5, 4);
    well.rect(6, 16, 40, 40, 1);
    well.rect(6, 16, 6, 40, 3);
    well.rect(40, 16, 6, 40, 2);
    for (int y = 22; y < 52; y += 8) well.rect(10, float(y), 32, 2, 2);
    well.rect(8, 50, 36, 8, 2);
    well.ellipse(26, 56, 18, 6, 3);
    well.rect(22, 8, 3, 10, 7);
    well.rect(18, 6, 11, 3, 7);
    well.rect(4, 28, 6, 4, 5);
    well.rect(42, 36, 6, 4, 5);
    art.well = gs::uploadImage(vdp, well);

    for (int pose = 0; pose < 3; pose++) {
        gs::Bitmap b(34, 42);
        keeperBmp(b, pose);
        art.keeper[pose] = gs::uploadImage(vdp, b);
    }

    gs::Bitmap crab(36, 20);
    crab.ellipse(18, 11, 10, 6, 1);
    crab.ellipse(18, 10, 6, 3, 2);
    crab.rect(6, 6, 5, 4, 3);
    crab.rect(25, 6, 5, 4, 3);
    crab.line(4, 8, 0, 2, 1, 2);
    crab.line(32, 8, 35, 2, 1, 2);
    crab.rect(8, 14, 3, 5, 4);
    crab.rect(14, 15, 3, 5, 4);
    crab.rect(20, 15, 3, 5, 4);
    crab.rect(26, 14, 3, 5, 4);
    crab.rect(12, 8, 2, 2, 4);
    crab.rect(22, 8, 2, 2, 4);
    art.crab = gs::uploadImage(vdp, crab);

    gs::Bitmap barrel(18, 24);
    barrel.ellipse(9, 4, 7, 3, 3);
    barrel.rect(2, 4, 14, 16, 1);
    barrel.rect(2, 8, 14, 3, 3);
    barrel.rect(2, 14, 14, 2, 3);
    barrel.ellipse(9, 20, 7, 3, 2);
    barrel.rect(1, 6, 2, 12, 4);
    barrel.rect(15, 6, 2, 12, 4);
    art.barrel = gs::uploadImage(vdp, barrel);

    gs::Bitmap brute(40, 26);
    brute.ellipse(20, 14, 14, 8, 1);
    brute.ellipse(20, 13, 8, 4, 2);
    brute.rect(4, 6, 8, 6, 3);
    brute.rect(28, 6, 8, 6, 3);
    brute.line(2, 10, 0, 2, 1, 2);
    brute.line(38, 10, 39, 2, 1, 2);
    brute.rect(6, 18, 4, 7, 4);
    brute.rect(14, 19, 4, 7, 4);
    brute.rect(22, 19, 4, 7, 4);
    brute.rect(30, 18, 4, 7, 4);
    brute.rect(12, 10, 3, 2, 3);
    brute.rect(24, 10, 3, 2, 3);
    art.brute = gs::uploadImage(vdp, brute);

    gs::Bitmap foam(20, 8);
    foam.ellipse(5, 4, 4, 2, 1);
    foam.ellipse(12, 3, 5, 2, 2);
    foam.ellipse(16, 5, 3, 2, 1);
    art.foam = gs::uploadImage(vdp, foam);

    gs::Bitmap gull(22, 10);
    gull.ellipse(8, 6, 5, 3, 1);
    gull.line(12, 5, 21, 2, 2, 2);
    gull.line(12, 6, 20, 8, 2, 1);
    gull.rect(4, 5, 2, 2, 3);
    gull.rect(6, 8, 3, 2, 4);
    art.gull = gs::uploadImage(vdp, gull);

    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.spacing = 1;
    for (int i = 0; i < 96; i++) {
        gs::Bitmap g = gs::textBitmap(std::string(1, char(32 + i)), st);
        art.glyph[i] = gs::uploadImage(vdp, g);
        art.gh = g.h;
        art.advance = g.w + 2;
    }
}

}  // namespace wharf
