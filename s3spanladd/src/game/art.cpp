#include "game/art.h"

namespace spanladd {

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
        0, gs::rgb4(15, 14, 12), gs::rgb4(8, 7, 6), gs::rgb4(15, 12, 4),
        gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)};
    const uint16_t hero[16] = {
        0, gs::rgb4(2, 1, 1), gs::rgb4(14, 10, 7), gs::rgb4(10, 4, 3),
        gs::rgb4(6, 3, 2), gs::rgb4(3, 3, 4), gs::rgb4(12, 11, 9), gs::rgb4(15, 8, 4),
        0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t wood[16] = {
        0, gs::rgb4(6, 3, 1), gs::rgb4(10, 6, 2), gs::rgb4(13, 9, 4),
        gs::rgb4(4, 2, 1), gs::rgb4(8, 7, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t stone[16] = {
        0, gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(11, 11, 12),
        gs::rgb4(3, 3, 4), gs::rgb4(7, 6, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t iron[16] = {
        0, gs::rgb4(3, 4, 5), gs::rgb4(7, 8, 9), gs::rgb4(12, 13, 14),
        gs::rgb4(9, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t water[16] = {
        0, gs::rgb4(2, 5, 8), gs::rgb4(4, 8, 11), gs::rgb4(8, 12, 14),
        gs::rgb4(1, 3, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t flag[16] = {
        0, gs::rgb4(12, 2, 2), gs::rgb4(15, 12, 3), gs::rgb4(14, 14, 13),
        gs::rgb4(4, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t sky[16] = {
        0, gs::rgb4(6, 8, 12), gs::rgb4(10, 12, 14), gs::rgb4(14, 14, 12),
        gs::rgb4(3, 5, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    pal(vdp, PAL_HUD, hud);
    pal(vdp, PAL_HERO, hero);
    pal(vdp, PAL_WOOD, wood);
    pal(vdp, PAL_STONE, stone);
    pal(vdp, PAL_IRON, iron);
    pal(vdp, PAL_WATER, water);
    pal(vdp, PAL_FLAG, flag);
    pal(vdp, PAL_SKY, sky);
    vdp.setFogColor(gs::rgb4(6, 8, 12));

    auto heroBmp = [](int step) {
        gs::Bitmap b(16, 22);
        b.rect(4, 1, 8, 7, 2);
        b.rect(5, 3, 2, 2, 1);
        b.rect(9, 3, 2, 2, 1);
        b.rect(6, 6, 4, 1, 1);
        b.rect(3, 8, 10, 8, 3);
        b.rect(4, 9, 3, 4, 7);
        b.set(3, 10, 6);
        b.rect(2, 10, 2, 5, 2);
        b.rect(12, 10, 2, 5, 2);
        int lx = step ? 4 : 5;
        int rx = step ? 9 : 8;
        b.rect(lx, 16, 3, 5, 5);
        b.rect(rx, 16, 3, 5, 5);
        b.rect(lx, 20, 3, 2, 1);
        b.rect(rx, 20, 3, 2, 1);
        b.outline(1, false);
        return b;
    };
    art.hero[0] = gs::uploadMipped(vdp, heroBmp(0));
    art.hero[1] = gs::uploadMipped(vdp, heroBmp(1));

    gs::Bitmap climb(14, 22);
    climb.rect(3, 1, 8, 7, 2);
    climb.rect(4, 8, 6, 8, 3);
    climb.rect(1, 9, 3, 3, 2);
    climb.rect(10, 12, 3, 3, 2);
    climb.rect(4, 16, 3, 5, 5);
    climb.rect(8, 16, 3, 5, 5);
    climb.outline(1, false);
    art.climb = gs::uploadMipped(vdp, climb);

    gs::Bitmap plank(28, 12);
    plank.rect(0, 2, 28, 8, 2);
    for (int x = 0; x < 28; x += 7) plank.rect(x, 2, 1, 8, 1);
    plank.rect(0, 2, 28, 1, 3);
    plank.rect(0, 9, 28, 1, 4);
    art.plank = gs::uploadMipped(vdp, plank);

    gs::Bitmap pier(36, 64);
    pier.rect(4, 0, 28, 64, 2);
    pier.rect(2, 0, 32, 6, 3);
    for (int y = 10; y < 62; y += 10) {
        pier.rect(6, y, 24, 2, 4);
        pier.rect(8, y + 4, 8, 4, 1);
        pier.rect(20, y + 4, 8, 4, 5);
    }
    pier.rect(0, 58, 36, 6, 1);
    art.pier = gs::uploadMipped(vdp, pier);

    gs::Bitmap rung(16, 8);
    rung.rect(1, 0, 2, 8, 2);
    rung.rect(13, 0, 2, 8, 2);
    rung.rect(1, 3, 14, 2, 3);
    art.rung = gs::uploadMipped(vdp, rung);

    gs::Bitmap rail(6, 18);
    rail.rect(2, 0, 2, 18, 1);
    rail.rect(0, 0, 6, 3, 4);
    art.rail = gs::uploadMipped(vdp, rail);

    gs::Bitmap flagBmp(18, 16);
    flagBmp.rect(1, 1, 2, 14, 4);
    flagBmp.poly({{3, 2}, {16, 5}, {3, 9}}, 1);
    flagBmp.poly({{4, 3}, {13, 5}, {4, 8}}, 2);
    art.flag = gs::uploadMipped(vdp, flagBmp);

    gs::Bitmap gull(16, 8);
    gull.poly({{0, 4}, {7, 2}, {8, 3}, {15, 1}, {8, 4}, {7, 5}}, 3);
    gull.set(6, 3, 1);
    art.gull = gs::uploadMipped(vdp, gull);

    art.wordSpan = phrase(vdp, "ONE SPAN", 1, 2);
    art.wordFar = phrase(vdp, "FAR LADDER", 3, 2);
    art.wordGo = phrase(vdp, "START", 1, 1);
    art.wordDone = phrase(vdp, "DONE", 3, 3);
    art.wordHint = phrase(vdp, "CROSS  CLIMB", 2, 1);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        int band = y < 120 ? 0 : (y < 168 ? 1 : 2);
        uint16_t c = band == 0 ? gs::rgb4(5, 8, 13) : band == 1 ? gs::rgb4(8, 11, 14) : gs::rgb4(2, 5, 8);
        vdp.lineBackdrop[y] = c;
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
}

}  // namespace spanladd
