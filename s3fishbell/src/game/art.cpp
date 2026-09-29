#include "game/art.h"

#include <initializer_list>

namespace fishbell {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Bitmap fishArt() {
    gs::Bitmap b(44, 22);
    b.ellipse(22.f, 11.f, 16.f, 8.f, 2);
    b.ellipse(20.f, 10.f, 13.f, 6.f, 3);
    b.poly({{2.f, 11.f}, {12.f, 4.f}, {12.f, 18.f}}, 4);
    b.poly({{40.f, 6.f}, {34.f, 11.f}, {40.f, 16.f}}, 5);
    b.ellipse(30.f, 9.f, 2.2f, 2.2f, 1);
    b.ellipse(30.6f, 9.f, 1.f, 1.f, 6);
    b.rect(16, 10, 10, 2, 7);
    b.line(18.f, 14.f, 28.f, 14.f, 5, 1.f);
    return b;
}

gs::Bitmap weedArt() {
    gs::Bitmap b(28, 26);
    b.ellipse(14.f, 16.f, 10.f, 7.f, 2);
    b.ellipse(8.f, 10.f, 5.f, 7.f, 3);
    b.ellipse(18.f, 8.f, 4.f, 8.f, 4);
    b.ellipse(14.f, 18.f, 6.f, 3.f, 5);
    b.line(14.f, 20.f, 14.f, 25.f, 3, 1.4f);
    return b;
}

gs::Bitmap hookArt() {
    gs::Bitmap b(12, 18);
    b.line(6.f, 0.f, 6.f, 10.f, 1, 1.4f);
    b.ellipse(7.2f, 12.f, 3.4f, 3.6f, 2);
    b.rect(5, 9, 4, 4, 0);
    b.line(9.f, 10.f, 11.f, 7.f, 3, 1.2f);
    return b;
}

gs::Bitmap bobberArt() {
    gs::Bitmap b(14, 16);
    b.ellipse(7.f, 8.f, 5.5f, 6.f, 2);
    b.ellipse(7.f, 6.f, 5.f, 3.2f, 1);
    b.rect(6, 1, 2, 3, 3);
    b.ellipse(5.f, 6.f, 1.4f, 1.2f, 4);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(34, 30);
    b.rect(15, 0, 4, 4, 5);
    b.ellipse(17.f, 4.f, 3.f, 2.2f, 5);
    b.ellipse(17.f, 15.f, 13.f, 11.f, 2);
    b.ellipse(17.f, 14.f, 10.f, 8.f, 3);
    b.ellipse(14.f, 12.f, 4.f, 3.4f, 4);
    b.ellipse(17.f, 23.f, 12.f, 3.2f, 6);
    b.line(7.f, 10.f, 10.f, 18.f, 4, 1.f);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(8, 12);
    b.line(4.f, 0.f, 4.f, 6.f, 1, 1.3f);
    b.ellipse(4.f, 9.f, 2.6f, 2.6f, 2);
    return b;
}

gs::Bitmap yokeArt() {
    gs::Bitmap b(28, 8);
    b.rect(0, 3, 28, 3, 2);
    b.rect(12, 1, 4, 6, 3);
    return b;
}

gs::Bitmap rodArt() {
    gs::Bitmap b(8, 48);
    b.rect(3, 0, 2, 48, 2);
    b.rect(2, 0, 2, 48, 1);
    b.rect(2, 40, 4, 6, 3);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 36);
    b.rect(2, 0, 6, 36, 2);
    b.rect(2, 0, 3, 36, 1);
    b.rect(1, 8, 8, 3, 3);
    return b;
}

gs::Bitmap rippleArt() {
    gs::Bitmap b(18, 6);
    b.ellipse(9.f, 3.f, 8.f, 2.f, 1);
    b.ellipse(9.f, 3.f, 5.f, 1.2f, 2);
    return b;
}

}  // namespace

void loadArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    setPal(vdp, PAL_HUD, {0, rgb4(14, 15, 15), rgb4(8, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, rgb4(1, 2, 3)});
    setPal(vdp, PAL_GOLD, {0, rgb4(15, 13, 4), rgb4(10, 7, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, rgb4(3, 2, 0)});
    setPal(vdp, PAL_ALERT, {0, rgb4(15, 5, 3), rgb4(8, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, rgb4(2, 0, 0)});
    setPal(vdp, PAL_FISH, {0, rgb4(15, 15, 14), rgb4(12, 8, 2), rgb4(15, 12, 3), rgb4(9, 5, 1), rgb4(4, 2, 1), rgb4(1, 1, 2), rgb4(15, 6, 2)});
    setPal(vdp, PAL_WEED, {0, rgb4(2, 6, 2), rgb4(1, 8, 3), rgb4(3, 12, 4), rgb4(6, 14, 5), rgb4(1, 4, 2)});
    setPal(vdp, PAL_BELL, {0, rgb4(6, 5, 3), rgb4(12, 9, 3), rgb4(15, 13, 6), rgb4(15, 15, 10), rgb4(8, 6, 2), rgb4(14, 10, 4), rgb4(4, 3, 2)});
    setPal(vdp, PAL_ROD, {0, rgb4(10, 6, 2), rgb4(6, 3, 1), rgb4(3, 2, 1), rgb4(14, 12, 8)});
    setPal(vdp, PAL_WIN, {0, rgb4(12, 15, 8), rgb4(4, 10, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, rgb4(1, 3, 1)});
    setPal(vdp, PAL_WATER, {0, rgb4(6, 12, 14), rgb4(3, 8, 12), rgb4(10, 15, 15)});
    setPal(vdp, PAL_SKY, {0, rgb4(15, 10, 4), rgb4(8, 4, 2)});

    vdp.setFogColor(rgb4(1, 3, 6));
    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);
    art.fish = gs::uploadMipped(vdp, fishArt());
    art.weed = gs::uploadMipped(vdp, weedArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.bobber = gs::uploadMipped(vdp, bobberArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.clapper = gs::uploadMipped(vdp, clapperArt());
    art.yoke = gs::uploadMipped(vdp, yokeArt());
    art.rod = gs::uploadMipped(vdp, rodArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.ripple = gs::uploadMipped(vdp, rippleArt());
}

}  // namespace fishbell
