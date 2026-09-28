#include "game/art.h"

#include <cstdint>
#include <string>

namespace millladd {
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
    body(5, 10, 0, 2, 1);
    body(4, 11, 2, 6, 2);
    pxSet(b, 6, 4, 3);
    pxSet(b, 9, 4, 3);
    body(4, 11, 7, 14, 4);
    body(6, 9, 8, 13, 5);
    pxSet(b, 3, 9, 4);
    pxSet(b, 12, 9, 4);
    if (pose == 0) {
        body(5, 7, 15, 22, 6);
        body(9, 11, 15, 22, 6);
    } else if (pose == 1) {
        body(4, 6, 15, 21, 6);
        body(10, 13, 15, 22, 6);
        pxSet(b, 2, 10, 4);
    } else if (pose == 2) {
        body(6, 8, 15, 22, 6);
        body(9, 12, 16, 20, 6);
        pxSet(b, 13, 9, 4);
    } else if (pose == 3) {
        body(5, 7, 14, 18, 6);
        body(9, 11, 14, 17, 6);
        pxSet(b, 1, 8, 4);
        pxSet(b, 14, 7, 4);
    } else {
        body(6, 8, 15, 22, 6);
        body(9, 11, 15, 21, 6);
        pxSet(b, 3, 11, 4);
        pxSet(b, 12, 8, 5);
    }
    return b;
}

gs::Bitmap ladderBmp() {
    gs::Bitmap b(16, 32);
    for (int y = 0; y < 32; y++) {
        pxSet(b, 2, y, 1);
        pxSet(b, 3, y, 2);
        pxSet(b, 12, y, 2);
        pxSet(b, 13, y, 1);
        if ((y % 6) == 1) {
            for (int x = 3; x <= 12; x++) pxSet(b, x, y, 3);
        }
    }
    return b;
}

gs::Bitmap plankBmp() {
    gs::Bitmap b(20, 10);
    b.rect(0, 0, 20, 10, 1);
    b.rect(0, 0, 20, 2, 2);
    b.rect(0, 8, 20, 2, 3);
    for (int x = 3; x < 20; x += 6) b.rect(x, 3, 1, 5, 4);
    return b;
}

gs::Bitmap hopperBmp() {
    gs::Bitmap b(26, 20);
    b.rect(2, 0, 22, 8, 1);
    b.rect(4, 2, 18, 4, 2);
    b.poly({{2, 8}, {24, 8}, {18, 16}, {8, 16}}, 3);
    b.rect(11, 15, 4, 4, 4);
    b.ellipse(6, 18, 3, 2, 5);
    b.ellipse(20, 18, 3, 2, 5);
    return b;
}

gs::Bitmap sackBmp() {
    gs::Bitmap b(16, 14);
    b.ellipse(8, 8, 7, 6, 1);
    b.ellipse(8, 7, 5, 4, 2);
    b.line(4, 4, 12, 3, 3, 1);
    b.rect(6, 1, 4, 3, 3);
    return b;
}

gs::Bitmap wheelBmp() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 13, 13, 1);
    b.ellipse(14, 14, 10, 10, 0);
    b.ellipse(14, 14, 3, 3, 2);
    b.rect(13, 2, 2, 24, 3);
    b.rect(2, 13, 24, 2, 3);
    b.line(4, 4, 24, 24, 3, 1);
    b.line(24, 4, 4, 24, 3, 1);
    return b;
}

gs::Bitmap hatchBmp() {
    gs::Bitmap b(34, 14);
    b.rect(0, 4, 34, 8, 1);
    b.rect(2, 6, 30, 4, 2);
    b.poly({{12, 4}, {22, 4}, {20, 0}, {14, 0}}, 3);
    for (int x = 4; x < 30; x += 5) pxSet(b, x, 8, 4);
    return b;
}

gs::Bitmap stoneBmp() {
    gs::Bitmap b(22, 12);
    b.ellipse(11, 7, 10, 5, 1);
    b.ellipse(11, 6, 6, 3, 2);
    b.rect(10, 2, 2, 8, 3);
    return b;
}

gs::Bitmap lampBmp() {
    gs::Bitmap b(12, 16);
    b.rect(5, 0, 2, 5, 1);
    b.rect(2, 5, 8, 6, 2);
    b.rect(3, 6, 6, 4, 3);
    b.rect(4, 11, 4, 2, 1);
    return b;
}

void timberTile(gs::VDP& v, int index, uint32_t seed, bool beam) {
    uint8_t px[64];
    uint32_t r = seed;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            r = r * 1664525u + 1013904223u;
            int n = int(r >> 28) & 7;
            int c = (y & 1) ? 1 : 2;
            if (n > 5) c = 3;
            px[y * 8 + x] = uint8_t(c);
        }
    }
    if (beam) {
        for (int x = 0; x < 8; x++) px[3 * 8 + x] = 4;
        for (int x = 0; x < 8; x++) px[4 * 8 + x] = 5;
    }
    v.loadTile(index, px);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 12, 7), gs::rgb4(8, 6, 3), gs::rgb4(15, 14, 10), gs::rgb4(4, 3, 2)};
    const uint16_t wood[] = {0, gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(3, 2, 1), gs::rgb4(4, 3, 2),
                             gs::rgb4(8, 5, 2)};
    const uint16_t heroC[] = {0, gs::rgb4(5, 3, 1), gs::rgb4(10, 7, 4), gs::rgb4(2, 2, 2), gs::rgb4(4, 6, 8),
                              gs::rgb4(7, 9, 11), gs::rgb4(3, 2, 2)};
    const uint16_t iron[] = {0, gs::rgb4(3, 3, 3), gs::rgb4(7, 7, 6), gs::rgb4(11, 10, 7), gs::rgb4(4, 4, 4)};
    const uint16_t grain[] = {0, gs::rgb4(9, 7, 2), gs::rgb4(12, 10, 4), gs::rgb4(6, 4, 1), gs::rgb4(3, 2, 1),
                              gs::rgb4(2, 2, 2)};
    const uint16_t lamp[] = {0, gs::rgb4(4, 3, 2), gs::rgb4(11, 8, 3), gs::rgb4(15, 13, 6), gs::rgb4(5, 4, 2)};
    const uint16_t ok[] = {0, gs::rgb4(2, 6, 3), gs::rgb4(5, 10, 4), gs::rgb4(9, 12, 6), gs::rgb4(14, 14, 8),
                           gs::rgb4(2, 3, 2)};
    const uint16_t alert[] = {0, gs::rgb4(12, 3, 2), gs::rgb4(15, 8, 3), gs::rgb4(6, 2, 1)};
    const uint16_t water[] = {0, gs::rgb4(2, 4, 8), gs::rgb4(4, 7, 11), gs::rgb4(1, 2, 4), gs::rgb4(8, 10, 12)};
    const uint16_t stone[] = {0, gs::rgb4(5, 5, 5), gs::rgb4(8, 8, 7), gs::rgb4(3, 3, 3)};
    pal(vdp, PAL_HUD, hud, 5);
    pal(vdp, PAL_WOOD, wood, 6);
    pal(vdp, PAL_HERO, heroC, 7);
    pal(vdp, PAL_IRON, iron, 5);
    pal(vdp, PAL_GRAIN, grain, 6);
    pal(vdp, PAL_LAMP, lamp, 5);
    pal(vdp, PAL_OK, ok, 6);
    pal(vdp, PAL_ALERT, alert, 4);
    pal(vdp, PAL_WATER, water, 5);
    pal(vdp, PAL_STONE, stone, 4);

    for (int i = 0; i < 4; i++) timberTile(vdp, 1 + i, 0xA11u + uint32_t(i) * 77u, false);
    timberTile(vdp, 5, 0xBEEFu, true);

    art.stand = gs::uploadMipped(vdp, hero(0));
    art.walkA = gs::uploadMipped(vdp, hero(1));
    art.walkB = gs::uploadMipped(vdp, hero(2));
    art.jump = gs::uploadMipped(vdp, hero(3));
    art.climbA = gs::uploadMipped(vdp, hero(4));
    art.climbB = gs::uploadMipped(vdp, hero(0));
    art.ladder = gs::uploadMipped(vdp, ladderBmp());
    art.plank = gs::uploadMipped(vdp, plankBmp());
    art.hopper = gs::uploadMipped(vdp, hopperBmp());
    art.sack = gs::uploadMipped(vdp, sackBmp());
    art.wheel = gs::uploadMipped(vdp, wheelBmp());
    art.hatch = gs::uploadMipped(vdp, hatchBmp());
    art.stone = gs::uploadMipped(vdp, stoneBmp());
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

}  // namespace millladd
