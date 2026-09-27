#include "game/art.h"

namespace harborladd {

static void pal(gs::VDP& vdp, int bank, const uint16_t* c) {
    for (int i = 0; i < 16; i++) vdp.setColor(bank * 16 + i, c[i]);
}

static gs::Image phrase(gs::VDP& vdp, const char* s, int color, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = color;
    st.outline = 15;
    st.spacing = 1;
    return gs::uploadImage(vdp, gs::textBitmap(s, st));
}

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[16] = {
        0, gs::rgb4(15, 14, 11), gs::rgb4(9, 8, 6), gs::rgb4(15, 12, 4),
        gs::rgb4(12, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 3)};
    const uint16_t hero[16] = {
        0, gs::rgb4(2, 2, 3), gs::rgb4(14, 10, 7), gs::rgb4(2, 4, 8),
        gs::rgb4(14, 12, 3), gs::rgb4(3, 2, 2), gs::rgb4(13, 13, 12), gs::rgb4(12, 3, 2),
        0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t wood[16] = {
        0, gs::rgb4(5, 3, 1), gs::rgb4(9, 6, 2), gs::rgb4(13, 9, 4),
        gs::rgb4(3, 2, 1), gs::rgb4(7, 7, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t pilePal[16] = {
        0, gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 8), gs::rgb4(11, 11, 10),
        gs::rgb4(3, 4, 3), gs::rgb4(6, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t iron[16] = {
        0, gs::rgb4(3, 4, 5), gs::rgb4(8, 9, 10), gs::rgb4(13, 14, 14),
        gs::rgb4(10, 7, 2), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t water[16] = {
        0, gs::rgb4(2, 6, 9), gs::rgb4(5, 10, 12), gs::rgb4(10, 14, 14),
        gs::rgb4(1, 3, 6), gs::rgb4(14, 14, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t buoyPal[16] = {
        0, gs::rgb4(12, 3, 2), gs::rgb4(15, 14, 12), gs::rgb4(4, 4, 5),
        gs::rgb4(14, 10, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t lampPal[16] = {
        0, gs::rgb4(4, 3, 2), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 12),
        gs::rgb4(8, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    pal(vdp, PAL_HUD, hud);
    pal(vdp, PAL_HERO, hero);
    pal(vdp, PAL_WOOD, wood);
    pal(vdp, PAL_PILE, pilePal);
    pal(vdp, PAL_IRON, iron);
    pal(vdp, PAL_WATER, water);
    pal(vdp, PAL_BUOY, buoyPal);
    pal(vdp, PAL_LAMP, lampPal);
    vdp.setFogColor(gs::rgb4(3, 6, 9));

    auto heroBmp = [](int step) {
        gs::Bitmap b(16, 24);
        b.ellipse(8, 4, 5, 3, 4);
        b.rect(5, 5, 6, 5, 2);
        b.rect(6, 6, 2, 1, 1);
        b.rect(9, 6, 2, 1, 1);
        b.rect(4, 10, 8, 8, 3);
        b.rect(5, 11, 3, 3, 6);
        b.rect(6, 15, 4, 2, 7);
        b.rect(2, 11, 2, 5, 3);
        b.rect(12, 11, 2, 5, 3);
        int lx = step ? 4 : 5;
        int rx = step ? 9 : 8;
        b.rect(lx, 18, 3, 5, 5);
        b.rect(rx, 18, 3, 5, 5);
        b.rect(lx, 22, 3, 2, 1);
        b.rect(rx, 22, 3, 2, 1);
        b.outline(1, false);
        return b;
    };
    art.hero[0] = gs::uploadMipped(vdp, heroBmp(0));
    art.hero[1] = gs::uploadMipped(vdp, heroBmp(1));

    gs::Bitmap climb(14, 24);
    climb.ellipse(7, 4, 5, 3, 4);
    climb.rect(4, 5, 6, 5, 2);
    climb.rect(3, 10, 8, 8, 3);
    climb.rect(1, 11, 3, 3, 2);
    climb.rect(10, 14, 3, 3, 2);
    climb.rect(4, 18, 3, 5, 5);
    climb.rect(8, 18, 3, 5, 5);
    climb.outline(1, false);
    art.climb = gs::uploadMipped(vdp, climb);

    gs::Bitmap plank(24, 10);
    plank.rect(0, 1, 24, 8, 2);
    for (int x = 0; x < 24; x += 6) plank.rect(x, 1, 1, 8, 1);
    plank.rect(0, 1, 24, 1, 3);
    plank.rect(0, 8, 24, 1, 4);
    art.plank = gs::uploadMipped(vdp, plank);

    gs::Bitmap pile(14, 48);
    pile.rect(3, 0, 8, 48, 2);
    pile.rect(2, 0, 10, 4, 3);
    pile.rect(4, 0, 2, 48, 5);
    for (int y = 8; y < 46; y += 8) pile.rect(3, y, 8, 1, 1);
    pile.rect(1, 18, 12, 3, 4);
    art.pile = gs::uploadMipped(vdp, pile);

    gs::Bitmap rung(16, 8);
    rung.rect(1, 1, 2, 7, 2);
    rung.rect(13, 1, 2, 7, 2);
    rung.rect(2, 3, 12, 2, 3);
    rung.rect(2, 3, 12, 1, 4);
    art.rung = gs::uploadMipped(vdp, rung);

    gs::Bitmap buoy(12, 16);
    buoy.ellipse(6, 7, 5, 6, 1);
    buoy.rect(4, 2, 4, 5, 2);
    buoy.rect(5, 12, 2, 4, 3);
    buoy.outline(3, false);
    art.buoy = gs::uploadMipped(vdp, buoy);

    gs::Bitmap crate(16, 14);
    crate.rect(1, 1, 14, 12, 3);
    crate.line(1, 1, 14, 12, 1, 1);
    crate.line(14, 1, 1, 12, 1, 1);
    crate.rect(1, 1, 14, 1, 4);
    crate.outline(1, false);
    art.crate = gs::uploadMipped(vdp, crate);

    gs::Bitmap gull(14, 8);
    gull.line(1, 4, 7, 3, 5, 1);
    gull.line(7, 3, 13, 5, 5, 1);
    gull.rect(6, 3, 2, 2, 1);
    art.gull = gs::uploadMipped(vdp, gull);

    gs::Bitmap lamp(10, 14);
    lamp.rect(4, 0, 2, 4, 1);
    lamp.rect(2, 4, 6, 6, 2);
    lamp.rect(3, 5, 4, 4, 3);
    lamp.rect(4, 10, 2, 4, 4);
    lamp.outline(1, false);
    art.lamp = gs::uploadMipped(vdp, lamp);

    art.wordHarbor = phrase(vdp, "HARBOR", 1, 2);
    art.wordFar = phrase(vdp, "FAR LADDER", 3, 2);
    art.wordGo = phrase(vdp, "START", 1, 1);
    art.wordHeld = phrase(vdp, "WATCH HELD", 3, 2);
    art.wordOver = phrase(vdp, "WATCH OVER", 4, 2);
}

}  // namespace harborladd
