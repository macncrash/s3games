#include "game/art.h"

#include <cstdint>
#include <string>

namespace redoubtladd {
namespace {

void pal(gs::VDP& v, int p, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) v.setColor(p * 16 + i, i < n ? c[i] : 0);
}

void pxSet(gs::Bitmap& b, int x, int y, int c) { b.set(x, y, c); }

gs::Bitmap hero(int pose) {
    gs::Bitmap b(16, 24);
    auto body = [&](int x0, int x1, int y0, int y1, int c) {
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) pxSet(b, x, y, c);
    };
    // Shako and greatcoat. Distinct from the bunker fatigue suit.
    body(5, 10, 0, 2, 1);
    pxSet(b, 4, 1, 1);
    pxSet(b, 11, 1, 2);
    body(5, 10, 3, 7, 3);
    pxSet(b, 6, 5, 2);
    pxSet(b, 9, 5, 2);
    body(3, 12, 8, 16, 4);
    body(5, 10, 9, 15, 5);
    pxSet(b, 2, 11, 4);
    pxSet(b, 13, 10, 4);
    if (pose == 0) {
        body(5, 7, 17, 23, 6);
        body(9, 11, 17, 23, 6);
    } else if (pose == 1) {
        body(4, 6, 17, 22, 6);
        body(10, 12, 17, 23, 6);
        pxSet(b, 1, 12, 4);
    } else if (pose == 2) {
        body(6, 8, 17, 23, 6);
        body(9, 12, 17, 21, 6);
        pxSet(b, 14, 11, 4);
    } else if (pose == 3) {
        body(5, 7, 16, 20, 6);
        body(9, 11, 16, 19, 6);
        pxSet(b, 1, 9, 4);
        pxSet(b, 14, 9, 4);
    } else {
        body(6, 8, 17, 23, 6);
        body(9, 11, 17, 22, 6);
        pxSet(b, 3, 13, 4);
        pxSet(b, 12, 9, 5);
    }
    return b;
}

gs::Bitmap ladderBmp() {
    gs::Bitmap b(14, 32);
    for (int y = 0; y < 32; y++) {
        pxSet(b, 1, y, 1);
        pxSet(b, 2, y, 2);
        pxSet(b, 11, y, 2);
        pxSet(b, 12, y, 1);
        if ((y % 5) == 1) {
            for (int x = 2; x <= 11; x++) pxSet(b, x, y, 3);
        }
    }
    return b;
}

gs::Bitmap sodBmp() {
    gs::Bitmap b(16, 10);
    b.rect(0, 0, 16, 10, 1);
    b.rect(0, 0, 16, 3, 2);
    b.rect(0, 8, 16, 2, 3);
    for (int x = 1; x < 16; x += 4) pxSet(b, x, 1, 4);
    return b;
}

gs::Bitmap gabionBmp() {
    gs::Bitmap b(22, 26);
    b.ellipse(11, 13, 10, 12, 1);
    b.ellipse(11, 12, 7, 9, 2);
    for (int y = 4; y < 22; y += 3)
        for (int x = 3; x < 19; x++)
            if (((x + y) & 3) == 0) pxSet(b, x, y, 3);
    b.rect(4, 2, 14, 2, 4);
    b.rect(5, 22, 12, 2, 4);
    return b;
}

gs::Bitmap flagBmp() {
    gs::Bitmap b(20, 22);
    b.rect(2, 0, 2, 22, 1);
    b.rect(4, 2, 14, 8, 2);
    b.rect(6, 4, 8, 4, 3);
    return b;
}

gs::Bitmap stakeBmp() {
    gs::Bitmap b(8, 16);
    b.rect(3, 0, 2, 16, 1);
    b.line(1, 2, 6, 6, 2, 1);
    b.line(6, 2, 1, 6, 2, 1);
    return b;
}

gs::Bitmap lampBmp() {
    gs::Bitmap b(12, 16);
    b.rect(5, 0, 2, 5, 1);
    b.rect(2, 5, 8, 7, 2);
    b.rect(3, 6, 6, 5, 3);
    b.rect(4, 12, 4, 2, 1);
    return b;
}

void earthTile(gs::VDP& v, int index, uint32_t seed, bool crest) {
    uint8_t px[64];
    uint32_t r = seed;
    for (int i = 0; i < 64; i++) {
        r = r * 1664525u + 1013904223u;
        int n = int(r >> 28) & 7;
        px[i] = uint8_t(n < 5 ? 1 : n < 7 ? 2 : 3);
    }
    if (crest) {
        for (int x = 0; x < 8; x++) px[x] = 4;
    }
    v.loadTile(index, px);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 12, 8), gs::rgb4(8, 7, 4), gs::rgb4(15, 14, 10), gs::rgb4(4, 3, 2)};
    const uint16_t earth[] = {0, gs::rgb4(6, 4, 2), gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(3, 6, 2)};
    const uint16_t heroC[] = {0, gs::rgb4(2, 2, 2), gs::rgb4(12, 10, 4), gs::rgb4(9, 7, 5), gs::rgb4(4, 5, 6),
                              gs::rgb4(6, 7, 8), gs::rgb4(3, 3, 3)};
    const uint16_t timber[] = {0, gs::rgb4(4, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(11, 8, 4)};
    const uint16_t gab[] = {0, gs::rgb4(7, 5, 2), gs::rgb4(10, 8, 4), gs::rgb4(5, 4, 2), gs::rgb4(3, 3, 2)};
    const uint16_t flag[] = {0, gs::rgb4(3, 3, 2), gs::rgb4(10, 2, 2), gs::rgb4(14, 12, 6)};
    const uint16_t ok[] = {0, gs::rgb4(3, 6, 2), gs::rgb4(6, 10, 4), gs::rgb4(12, 13, 7), gs::rgb4(14, 14, 8)};
    const uint16_t alert[] = {0, gs::rgb4(12, 3, 2), gs::rgb4(15, 8, 3), gs::rgb4(6, 2, 1)};
    const uint16_t sod[] = {0, gs::rgb4(5, 4, 2), gs::rgb4(4, 7, 2), gs::rgb4(3, 3, 1), gs::rgb4(7, 9, 3)};
    const uint16_t water[] = {0, gs::rgb4(1, 2, 4), gs::rgb4(2, 4, 6), gs::rgb4(4, 6, 8)};
    pal(vdp, PAL_HUD, hud, 5);
    pal(vdp, PAL_EARTH, earth, 5);
    pal(vdp, PAL_HERO, heroC, 7);
    pal(vdp, PAL_TIMBER, timber, 4);
    pal(vdp, PAL_GABION, gab, 5);
    pal(vdp, PAL_FLAG, flag, 4);
    pal(vdp, PAL_OK, ok, 5);
    pal(vdp, PAL_ALERT, alert, 4);
    pal(vdp, PAL_SOD, sod, 5);
    pal(vdp, PAL_WATER, water, 4);

    for (int i = 0; i < 4; i++) earthTile(vdp, 1 + i, 0xA11u + uint32_t(i) * 77u, i == 0);

    art.stand = gs::uploadMipped(vdp, hero(0));
    art.walkA = gs::uploadMipped(vdp, hero(1));
    art.walkB = gs::uploadMipped(vdp, hero(2));
    art.jump = gs::uploadMipped(vdp, hero(3));
    art.climbA = gs::uploadMipped(vdp, hero(4));
    art.climbB = gs::uploadMipped(vdp, hero(0));
    art.ladder = gs::uploadMipped(vdp, ladderBmp());
    art.sod = gs::uploadMipped(vdp, sodBmp());
    art.gabion = gs::uploadMipped(vdp, gabionBmp());
    art.flag = gs::uploadMipped(vdp, flagBmp());
    art.stake = gs::uploadMipped(vdp, stakeBmp());
    art.lamp = gs::uploadMipped(vdp, lampBmp());

    for (int i = 0; i < 96; i++) {
        gs::TextStyle st;
        st.scale = 1;
        st.color = 1;
        st.spacing = 0;
        std::string s(1, char(32 + i));
        gs::Bitmap g = gs::textBitmap(s, st);
        if (g.w < 1) g = gs::Bitmap(4, 7);
        art.glyph[i] = gs::uploadImage(vdp, g);
    }
}

}  // namespace redoubtladd
