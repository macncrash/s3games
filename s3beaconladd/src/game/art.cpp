#include "game/art.h"

namespace beaconladd {

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
        0, gs::rgb4(15, 14, 10), gs::rgb4(9, 8, 7), gs::rgb4(15, 12, 4),
        gs::rgb4(3, 3, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)};
    const uint16_t keeper[16] = {
        0, gs::rgb4(2, 2, 3), gs::rgb4(12, 9, 6), gs::rgb4(4, 5, 8),
        gs::rgb4(8, 8, 10), gs::rgb4(3, 3, 4), gs::rgb4(15, 13, 6), gs::rgb4(15, 10, 3),
        0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t stone[16] = {
        0, gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10),
        gs::rgb4(2, 2, 3), gs::rgb4(7, 6, 5), gs::rgb4(12, 11, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t lamp[16] = {
        0, gs::rgb4(15, 14, 6), gs::rgb4(15, 10, 2), gs::rgb4(15, 15, 12),
        gs::rgb4(8, 5, 1), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t iron[16] = {
        0, gs::rgb4(3, 4, 5), gs::rgb4(8, 9, 10), gs::rgb4(13, 13, 12),
        gs::rgb4(10, 7, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t night[16] = {
        0, gs::rgb4(14, 14, 12), gs::rgb4(6, 7, 12), gs::rgb4(10, 11, 14),
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t sea[16] = {
        0, gs::rgb4(1, 3, 6), gs::rgb4(2, 6, 9), gs::rgb4(4, 9, 11),
        gs::rgb4(8, 12, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    pal(vdp, PAL_HUD, hud);
    pal(vdp, PAL_KEEPER, keeper);
    pal(vdp, PAL_STONE, stone);
    pal(vdp, PAL_LAMP, lamp);
    pal(vdp, PAL_IRON, iron);
    pal(vdp, PAL_NIGHT, night);
    pal(vdp, PAL_SEA, sea);
    vdp.setFogColor(gs::rgb4(1, 2, 5));

    auto keeperBmp = [](int step) {
        gs::Bitmap b(16, 24);
        b.rect(5, 1, 6, 5, 2);
        b.rect(4, 0, 8, 2, 5);
        b.rect(6, 3, 2, 1, 1);
        b.rect(9, 3, 1, 1, 1);
        b.rect(4, 6, 8, 9, 3);
        b.rect(6, 8, 3, 4, 6);
        b.rect(11, 9, 3, 3, 2);
        b.set(13, 10, 7);
        b.rect(3, 10, 2, 4, 4);
        int lx = step ? 4 : 5;
        int rx = step ? 9 : 8;
        b.rect(lx, 15, 3, 7, 5);
        b.rect(rx, 15, 3, 7, 5);
        b.rect(lx, 21, 3, 2, 1);
        b.rect(rx, 21, 3, 2, 1);
        b.outline(1, false);
        return b;
    };
    art.keeper[0] = gs::uploadMipped(vdp, keeperBmp(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperBmp(1));

    gs::Bitmap climb(14, 24);
    climb.rect(4, 1, 6, 5, 2);
    climb.rect(3, 0, 8, 2, 5);
    climb.rect(3, 6, 8, 9, 3);
    climb.rect(1, 8, 3, 3, 2);
    climb.rect(10, 12, 3, 3, 2);
    climb.set(11, 13, 7);
    climb.rect(4, 15, 3, 7, 5);
    climb.rect(8, 15, 3, 7, 5);
    climb.outline(1, false);
    art.climb = gs::uploadMipped(vdp, climb);

    gs::Bitmap beacon(32, 96);
    beacon.rect(10, 28, 12, 68, 2);
    beacon.rect(8, 26, 16, 6, 3);
    beacon.rect(6, 88, 20, 8, 1);
    for (int y = 40; y < 86; y += 12) beacon.rect(12, y, 8, 3, 4);
    beacon.rect(8, 8, 16, 18, 5);
    beacon.rect(10, 10, 12, 14, 6);
    beacon.rect(14, 12, 4, 8, 3);
    beacon.rect(14, 4, 4, 6, 1);
    beacon.rect(12, 0, 8, 4, 3);
    beacon.ellipse(16, 16, 3, 4, 3);
    art.beacon = gs::uploadMipped(vdp, beacon);

    auto beamOf = [](int kind) {
        gs::Bitmap b(48, 20);
        if (kind == 0) b.poly({{0, 8}, {46, 1}, {46, 6}, {0, 12}}, 1);
        else if (kind == 1) b.poly({{0, 6}, {46, 6}, {46, 12}, {0, 12}}, 1);
        else b.poly({{0, 4}, {46, 12}, {46, 17}, {0, 8}}, 1);
        b.rect(0, 8, 4, 4, 3);
        return b;
    };
    for (int i = 0; i < 3; i++) art.beam[i] = gs::uploadMipped(vdp, beamOf(i));

    gs::Bitmap cliff(32, 28);
    cliff.rect(0, 4, 32, 24, 2);
    cliff.rect(0, 2, 32, 4, 3);
    cliff.rect(0, 24, 32, 4, 1);
    for (int x = 4; x < 30; x += 8) cliff.rect(x, 10, 2, 12, 4);
    cliff.rect(2, 6, 6, 2, 5);
    art.cliff = gs::uploadMipped(vdp, cliff);

    gs::Bitmap rung(18, 10);
    rung.rect(1, 0, 2, 10, 2);
    rung.rect(15, 0, 2, 10, 2);
    rung.rect(1, 4, 16, 2, 3);
    rung.rect(6, 4, 2, 2, 4);
    art.rung = gs::uploadMipped(vdp, rung);

    gs::Bitmap star(5, 5);
    star.set(2, 0, 1);
    star.set(2, 4, 1);
    star.set(0, 2, 1);
    star.set(4, 2, 1);
    star.set(2, 2, 3);
    art.star = gs::uploadMipped(vdp, star);

    gs::Bitmap lampBmp(8, 10);
    lampBmp.rect(3, 0, 2, 3, 5);
    lampBmp.ellipse(4, 6, 3, 3, 1);
    lampBmp.set(4, 6, 3);
    art.lamp = gs::uploadMipped(vdp, lampBmp);

    art.wordOne = phrase(vdp, "ONE BEACON", 1, 2);
    art.wordFar = phrase(vdp, "FAR LADDER", 3, 2);
    art.wordGo = phrase(vdp, "START", 1, 1);
    art.wordDone = phrase(vdp, "DONE", 3, 3);
    art.wordHint = phrase(vdp, "LEAVE IT  CLIMB", 2, 1);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 70) c = gs::rgb4(1, 1, 4);
        else if (y < 130) c = gs::rgb4(1, 2, 6);
        else if (y < 186) c = gs::rgb4(1, 3, 7);
        else c = gs::rgb4(1, 4, 8);
        vdp.lineBackdrop[y] = c;
        vdp.lineFog[y] = y > 190 ? 3 : 0;
        vdp.road[y].on = false;
    }
    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace beaconladd
