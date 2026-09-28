#include "game/art.h"

#include <cstdint>
#include <string>

namespace bunkerladd {
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
    body(4, 11, 1, 6, 1);
    body(5, 10, 2, 5, 2);
    pxSet(b, 6, 3, 3);
    pxSet(b, 9, 3, 3);
    body(4, 11, 7, 15, 4);
    body(5, 10, 8, 14, 5);
    pxSet(b, 3, 9, 4);
    pxSet(b, 12, 9, 4);
    if (pose == 0) {
        body(5, 7, 16, 22, 6);
        body(9, 11, 16, 22, 6);
    } else if (pose == 1) {
        body(4, 6, 16, 21, 6);
        body(10, 12, 16, 22, 6);
        pxSet(b, 2, 11, 4);
    } else if (pose == 2) {
        body(6, 8, 16, 22, 6);
        body(9, 12, 16, 20, 6);
        pxSet(b, 13, 10, 4);
    } else if (pose == 3) {
        body(5, 7, 15, 19, 6);
        body(9, 11, 15, 18, 6);
        pxSet(b, 1, 8, 4);
        pxSet(b, 14, 8, 4);
    } else {
        body(6, 8, 16, 22, 6);
        body(9, 11, 16, 21, 6);
        pxSet(b, 4, 12, 4);
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
        if ((y % 6) == 2) {
            for (int x = 3; x <= 12; x++) pxSet(b, x, y, 3);
            for (int x = 4; x <= 11; x++) pxSet(b, x, y + 1 < 32 ? y + 1 : y, 2);
        }
    }
    return b;
}

gs::Bitmap slabBmp() {
    gs::Bitmap b(16, 12);
    b.rect(0, 0, 16, 12, 1);
    b.rect(0, 0, 16, 3, 2);
    b.rect(0, 10, 16, 2, 3);
    for (int x = 2; x < 16; x += 5) b.rect(x, 4, 1, 6, 4);
    return b;
}

gs::Bitmap trolleyBmp() {
    gs::Bitmap b(28, 18);
    b.rect(2, 4, 24, 9, 1);
    b.rect(4, 5, 20, 6, 2);
    b.rect(0, 8, 4, 4, 3);
    b.rect(24, 8, 4, 4, 3);
    b.ellipse(6, 14, 4, 4, 4);
    b.ellipse(21, 14, 4, 4, 4);
    b.ellipse(6, 14, 2, 2, 5);
    b.ellipse(21, 14, 2, 2, 5);
    return b;
}

gs::Bitmap lampBmp() {
    gs::Bitmap b(14, 18);
    b.rect(6, 0, 2, 6, 1);
    b.rect(3, 6, 8, 6, 2);
    b.rect(4, 7, 6, 4, 3);
    b.rect(5, 12, 4, 2, 1);
    return b;
}

gs::Bitmap hatchBmp() {
    gs::Bitmap b(36, 16);
    b.rect(0, 4, 36, 10, 1);
    b.rect(2, 6, 32, 6, 2);
    b.rect(14, 0, 8, 6, 3);
    b.rect(16, 1, 4, 3, 4);
    for (int x = 4; x < 32; x += 6) pxSet(b, x, 8, 5);
    return b;
}

gs::Bitmap bagBmp() {
    gs::Bitmap b(18, 12);
    b.ellipse(9, 7, 8, 5, 1);
    b.ellipse(9, 6, 6, 3, 2);
    b.line(3, 7, 15, 5, 3, 1);
    return b;
}

gs::Bitmap slitBmp() {
    gs::Bitmap b(22, 8);
    b.rect(0, 2, 22, 4, 1);
    b.rect(2, 3, 18, 2, 2);
    return b;
}

void concTile(gs::VDP& v, int index, uint32_t seed, bool mortar) {
    uint8_t px[64];
    uint32_t r = seed;
    for (int i = 0; i < 64; i++) {
        r = r * 1664525u + 1013904223u;
        int n = int(r >> 28) & 7;
        px[i] = uint8_t(n < 5 ? 1 : n < 7 ? 2 : 3);
    }
    if (mortar) {
        for (int x = 0; x < 8; x++) px[7 * 8 + x] = 4;
    }
    for (int y = 0; y < 8; y++) px[y * 8] = 5;
    v.loadTile(index, px);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 8), gs::rgb4(8, 10, 6), gs::rgb4(15, 15, 12), gs::rgb4(4, 4, 3)};
    const uint16_t conc[] = {0, gs::rgb4(5, 6, 5), gs::rgb4(7, 8, 7), gs::rgb4(3, 4, 3), gs::rgb4(2, 2, 2),
                             gs::rgb4(4, 5, 4)};
    const uint16_t heroC[] = {0, gs::rgb4(3, 5, 2), gs::rgb4(6, 8, 4), gs::rgb4(12, 10, 6), gs::rgb4(4, 6, 3),
                              gs::rgb4(8, 9, 5), gs::rgb4(2, 2, 2)};
    const uint16_t iron[] = {0, gs::rgb4(3, 4, 4), gs::rgb4(8, 9, 9), gs::rgb4(12, 12, 10), gs::rgb4(5, 6, 6)};
    const uint16_t rust[] = {0, gs::rgb4(8, 4, 2), gs::rgb4(11, 7, 3), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 2),
                             gs::rgb4(6, 6, 6)};
    const uint16_t lamp[] = {0, gs::rgb4(4, 4, 3), gs::rgb4(10, 8, 3), gs::rgb4(15, 13, 5), gs::rgb4(6, 5, 2)};
    const uint16_t ok[] = {0, gs::rgb4(3, 6, 3), gs::rgb4(6, 10, 5), gs::rgb4(10, 12, 8), gs::rgb4(14, 14, 8),
                           gs::rgb4(2, 3, 2)};
    const uint16_t alert[] = {0, gs::rgb4(12, 3, 2), gs::rgb4(15, 8, 3), gs::rgb4(6, 2, 2)};
    const uint16_t sand[] = {0, gs::rgb4(8, 7, 3), gs::rgb4(11, 9, 5), gs::rgb4(5, 4, 2)};
    const uint16_t pit[] = {0, gs::rgb4(1, 1, 1), gs::rgb4(2, 3, 3), gs::rgb4(0, 2, 2)};
    pal(vdp, PAL_HUD, hud, 5);
    pal(vdp, PAL_CONC, conc, 6);
    pal(vdp, PAL_HERO, heroC, 7);
    pal(vdp, PAL_IRON, iron, 5);
    pal(vdp, PAL_RUST, rust, 6);
    pal(vdp, PAL_LAMP, lamp, 5);
    pal(vdp, PAL_OK, ok, 6);
    pal(vdp, PAL_ALERT, alert, 4);
    pal(vdp, PAL_SAND, sand, 4);
    pal(vdp, PAL_PIT, pit, 4);

    for (int i = 0; i < 4; i++) concTile(vdp, 1 + i, 0xB001u + uint32_t(i) * 99u, i & 1);
    concTile(vdp, 5, 0x51u, true);

    art.stand = gs::uploadMipped(vdp, hero(0));
    art.walkA = gs::uploadMipped(vdp, hero(1));
    art.walkB = gs::uploadMipped(vdp, hero(2));
    art.jump = gs::uploadMipped(vdp, hero(3));
    art.climbA = gs::uploadMipped(vdp, hero(4));
    art.climbB = gs::uploadMipped(vdp, hero(0));
    art.ladder = gs::uploadMipped(vdp, ladderBmp());
    art.slab = gs::uploadMipped(vdp, slabBmp());
    art.trolley = gs::uploadMipped(vdp, trolleyBmp());
    art.lamp = gs::uploadMipped(vdp, lampBmp());
    art.hatch = gs::uploadMipped(vdp, hatchBmp());
    art.bag = gs::uploadMipped(vdp, bagBmp());
    art.slit = gs::uploadMipped(vdp, slitBmp());

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

}  // namespace bunkerladd
