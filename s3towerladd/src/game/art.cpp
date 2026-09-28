#include "game/art.h"

#include <cstdint>
#include <string>

namespace towerladd {
namespace {

void pal(gs::VDP& v, int p, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) v.setColor(p * 16 + i, i < n ? c[i] : 0);
}

void fill(gs::Bitmap& b, int x0, int x1, int y0, int y1, int c) {
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) b.set(x, y, c);
}

gs::Bitmap hero(int pose) {
    gs::Bitmap b(18, 28);
    fill(b, 6, 11, 1, 3, 1);
    fill(b, 5, 12, 3, 7, 1);
    fill(b, 7, 10, 4, 6, 2);
    b.set(7, 5, 3);
    b.set(10, 5, 3);
    fill(b, 4, 13, 8, 18, 4);
    fill(b, 6, 11, 9, 16, 5);
    b.set(3, 11, 4);
    b.set(14, 11, 4);
    if (pose == 0) {
        fill(b, 6, 8, 19, 26, 6);
        fill(b, 10, 12, 19, 26, 6);
    } else if (pose == 1) {
        fill(b, 4, 7, 19, 25, 6);
        fill(b, 11, 14, 19, 26, 6);
        b.set(2, 13, 4);
    } else if (pose == 2) {
        fill(b, 7, 10, 19, 26, 6);
        fill(b, 11, 15, 18, 24, 6);
        b.set(15, 12, 4);
    } else if (pose == 3) {
        fill(b, 6, 8, 16, 21, 6);
        fill(b, 11, 13, 16, 20, 6);
        b.set(2, 10, 4);
        b.set(15, 10, 4);
    } else {
        fill(b, 6, 8, 19, 26, 6);
        fill(b, 10, 12, 18, 25, 6);
        b.set(4, 14, 5);
        b.set(13, 10, 4);
    }
    b.set(8, 2, 7);
    return b;
}

gs::Bitmap ladderBmp() {
    gs::Bitmap b(14, 40);
    for (int y = 0; y < 40; y++) {
        b.set(1, y, 1);
        b.set(2, y, 2);
        b.set(11, y, 2);
        b.set(12, y, 1);
        if ((y % 7) == 3) {
            for (int x = 2; x <= 11; x++) b.set(x, y, 3);
        }
    }
    return b;
}

gs::Bitmap merlonBmp() {
    gs::Bitmap b(16, 18);
    b.rect(0, 6, 16, 12, 1);
    b.rect(0, 0, 6, 10, 2);
    b.rect(10, 0, 6, 10, 2);
    b.rect(0, 15, 16, 3, 3);
    b.line(6, 8, 10, 8, 4, 1);
    return b;
}

gs::Bitmap torchBmp() {
    gs::Bitmap b(10, 18);
    b.rect(4, 8, 2, 10, 1);
    b.ellipse(5, 6, 3, 4, 2);
    b.ellipse(5, 6, 2, 2, 3);
    b.set(5, 3, 4);
    return b;
}

gs::Bitmap bannerBmp() {
    gs::Bitmap b(14, 26);
    b.rect(1, 0, 12, 3, 1);
    for (int y = 3; y < 24; y++) {
        int sag = (y / 6) % 2;
        for (int x = 2 + sag; x < 12; x++) b.set(x, y, (x + y) & 1 ? 2 : 3);
    }
    b.line(3, 24, 7, 25, 2, 1);
    b.line(7, 25, 11, 23, 3, 1);
    return b;
}

gs::Bitmap moonBmp() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 1);
    b.ellipse(10, 7, 4, 4, 0);
    b.set(5, 6, 2);
    b.set(6, 9, 2);
    return b;
}

gs::Bitmap hatchBmp() {
    gs::Bitmap b(22, 12);
    b.rect(0, 4, 22, 8, 1);
    b.rect(2, 6, 18, 4, 2);
    b.rect(8, 0, 6, 6, 3);
    b.set(10, 2, 4);
    return b;
}

void stoneTile(gs::VDP& v, int index, uint32_t seed, bool course) {
    uint8_t px[64];
    uint32_t r = seed;
    for (int i = 0; i < 64; i++) {
        r = r * 1664525u + 1013904223u;
        int n = int(r >> 28) & 7;
        px[i] = uint8_t(n < 5 ? 1 : n < 7 ? 2 : 3);
    }
    if (course) {
        for (int x = 0; x < 8; x++) px[7 * 8 + x] = 4;
    }
    px[0] = 5;
    v.loadTile(index, px);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 12, 7), gs::rgb4(8, 9, 11), gs::rgb4(15, 15, 13), gs::rgb4(3, 3, 4)};
    const uint16_t stone[] = {0, gs::rgb4(5, 5, 6), gs::rgb4(7, 7, 8), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3),
                              gs::rgb4(8, 7, 6)};
    const uint16_t keep[] = {0, gs::rgb4(3, 3, 5), gs::rgb4(8, 7, 5), gs::rgb4(14, 12, 8), gs::rgb4(4, 3, 5),
                             gs::rgb4(6, 5, 7), gs::rgb4(2, 2, 3), gs::rgb4(12, 10, 4)};
    const uint16_t iron[] = {0, gs::rgb4(3, 4, 5), gs::rgb4(7, 8, 9), gs::rgb4(12, 11, 8), gs::rgb4(4, 5, 6)};
    const uint16_t torch[] = {0, gs::rgb4(5, 3, 2), gs::rgb4(12, 5, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 8)};
    const uint16_t cloth[] = {0, gs::rgb4(6, 2, 2), gs::rgb4(10, 3, 3), gs::rgb4(13, 8, 4), gs::rgb4(4, 1, 1)};
    const uint16_t ok[] = {0, gs::rgb4(3, 6, 4), gs::rgb4(6, 11, 6), gs::rgb4(12, 14, 8), gs::rgb4(15, 15, 10),
                           gs::rgb4(2, 3, 2)};
    const uint16_t alert[] = {0, gs::rgb4(12, 3, 2), gs::rgb4(15, 8, 3), gs::rgb4(5, 2, 2)};
    const uint16_t dusk[] = {0, gs::rgb4(10, 10, 12), gs::rgb4(14, 13, 8), gs::rgb4(6, 6, 9)};
    const uint16_t pit[] = {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 2, 3), gs::rgb4(0, 1, 2)};
    pal(vdp, PAL_HUD, hud, 5);
    pal(vdp, PAL_STONE, stone, 6);
    pal(vdp, PAL_KEEP, keep, 8);
    pal(vdp, PAL_IRON, iron, 5);
    pal(vdp, PAL_TORCH, torch, 5);
    pal(vdp, PAL_CLOTH, cloth, 5);
    pal(vdp, PAL_OK, ok, 6);
    pal(vdp, PAL_ALERT, alert, 4);
    pal(vdp, PAL_DUSK, dusk, 4);
    pal(vdp, PAL_PIT, pit, 4);

    for (int i = 0; i < 4; i++) stoneTile(vdp, 1 + i, 0x70E1u + uint32_t(i) * 131u, i & 1);

    art.stand = gs::uploadMipped(vdp, hero(0));
    art.walkA = gs::uploadMipped(vdp, hero(1));
    art.walkB = gs::uploadMipped(vdp, hero(2));
    art.jump = gs::uploadMipped(vdp, hero(3));
    art.climbA = gs::uploadMipped(vdp, hero(4));
    art.climbB = gs::uploadMipped(vdp, hero(0));
    art.ladder = gs::uploadMipped(vdp, ladderBmp());
    art.merlon = gs::uploadMipped(vdp, merlonBmp());
    art.torch = gs::uploadMipped(vdp, torchBmp());
    art.banner = gs::uploadMipped(vdp, bannerBmp());
    art.moon = gs::uploadMipped(vdp, moonBmp());
    art.hatch = gs::uploadMipped(vdp, hatchBmp());

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

}  // namespace towerladd
