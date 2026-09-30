#include "game/art.h"

#include <string>

namespace culvertbann {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    gs::TextStyle big{2, 1, 0, 15, 1};
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

gs::Bitmap runner(int pose) {
    gs::Bitmap b(24, 40);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose == 2 ? 1 : 0;
    R(8, 1 + bob, 8, 3, 6);
    R(9, 3 + bob, 6, 6, 4);
    R(13, 5 + bob, 2, 2, 9);
    R(14, 4 + bob, 3, 2, 8);
    R(7, 9 + bob, 10, 13, 2);
    R(7, 9 + bob, 3, 13, 3);
    R(10, 14 + bob, 5, 2, 5);
    if (pose == 1) {
        R(7, 22, 4, 12, 2);
        R(13, 23, 4, 11, 2);
        R(6, 33, 6, 4, 6);
        R(12, 33, 6, 4, 6);
    } else if (pose == 2) {
        R(8, 22, 4, 11, 2);
        R(13, 22, 4, 13, 2);
        R(7, 32, 6, 4, 6);
        R(13, 34, 6, 4, 6);
    } else {
        R(8, 22, 4, 13, 2);
        R(13, 22, 4, 13, 2);
        R(7, 34, 6, 4, 6);
        R(13, 34, 6, 4, 6);
    }
    R(16, 11, 3, 8, 4);
    return b;
}

gs::Bitmap cloth() {
    gs::Bitmap b(22, 30);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(10, 0, 2, 30, 6);
    R(4, 3, 12, 20, 2);
    R(4, 3, 3, 20, 3);
    R(8, 8, 4, 10, 4);
    R(5, 23, 3, 5, 2);
    R(10, 23, 3, 4, 3);
    R(2, 2, 3, 3, 5);
    return b;
}

gs::Bitmap vermin() {
    gs::Bitmap b(28, 14);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(4, 5, 16, 6, 2);
    R(18, 4, 7, 5, 3);
    R(23, 5, 2, 2, 8);
    R(22, 7, 3, 1, 4);
    R(2, 6, 6, 2, 5);
    R(6, 11, 2, 3, 6);
    R(12, 11, 2, 3, 6);
    R(17, 11, 2, 3, 6);
    R(8, 3, 2, 2, 4);
    return b;
}

gs::Bitmap ribBlock() {
    gs::Bitmap b(18, 48);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(2, 0, 14, 48, 2);
    R(2, 0, 3, 48, 4);
    R(13, 0, 3, 48, 1);
    for (int y = 6; y < 48; y += 12) R(4, y, 10, 2, 3);
    R(6, 20, 4, 6, 5);
    return b;
}

gs::Bitmap slab() {
    gs::Bitmap b(32, 12);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(0, 2, 32, 8, 2);
    R(0, 2, 32, 2, 4);
    R(0, 8, 32, 2, 1);
    R(4, 5, 6, 2, 3);
    R(18, 6, 8, 2, 3);
    return b;
}

gs::Bitmap flow() {
    gs::Bitmap b(20, 14);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(0, 2, 20, 12, 2);
    R(0, 2, 20, 3, 4);
    R(2, 6, 6, 2, 5);
    R(12, 8, 5, 2, 3);
    R(6, 10, 4, 2, 1);
    return b;
}

gs::Bitmap bars() {
    gs::Bitmap b(28, 64);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(0, 0, 28, 4, 2);
    R(0, 58, 28, 6, 2);
    for (int x = 3; x < 26; x += 6) R(x, 2, 2, 58, 3);
    R(1, 28, 26, 3, 4);
    return b;
}

gs::Bitmap drip() {
    gs::Bitmap b(6, 12);
    b.rect(2, 0, 2, 7, 2);
    b.ellipse(3, 9, 2, 2, 3);
    return b;
}

gs::Bitmap lamp() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 2);
    b.ellipse(5, 5, 2, 2, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 9), gs::rgb4(8, 8, 7), gs::rgb4(4, 6, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(3, 3, 3), gs::rgb4(6, 6, 6), gs::rgb4(4, 5, 4), gs::rgb4(9, 9, 8), gs::rgb4(5, 7, 5), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(2, 2, 1), gs::rgb4(12, 10, 2), gs::rgb4(8, 6, 1), gs::rgb4(13, 10, 7), gs::rgb4(6, 4, 2), gs::rgb4(3, 3, 3), 0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(4, 1, 1), gs::rgb4(13, 2, 2), gs::rgb4(9, 1, 1), gs::rgb4(14, 11, 3), gs::rgb4(15, 13, 6), gs::rgb4(5, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(1, 3, 5), gs::rgb4(2, 6, 9), gs::rgb4(4, 9, 12), gs::rgb4(7, 12, 14), gs::rgb4(10, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_RAT, {0, gs::rgb4(2, 1, 1), gs::rgb4(6, 4, 3), gs::rgb4(8, 6, 4), gs::rgb4(10, 8, 6), gs::rgb4(4, 3, 2), gs::rgb4(3, 2, 2), 0, gs::rgb4(14, 4, 3), 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(2, 2, 2), gs::rgb4(5, 6, 6), gs::rgb4(8, 9, 8), gs::rgb4(3, 5, 3), gs::rgb4(6, 8, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_MOSS, {0, gs::rgb4(2, 4, 2), gs::rgb4(4, 7, 3), gs::rgb4(6, 9, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    art.stand = gs::uploadMipped(vdp, runner(0));
    art.walkA = gs::uploadMipped(vdp, runner(1));
    art.walkB = gs::uploadMipped(vdp, runner(2));
    art.banner = gs::uploadMipped(vdp, cloth());
    art.rat = gs::uploadMipped(vdp, vermin());
    art.rib = gs::uploadMipped(vdp, ribBlock());
    art.slab = gs::uploadMipped(vdp, slab());
    art.water = gs::uploadMipped(vdp, flow());
    art.grate = gs::uploadMipped(vdp, bars());
    art.drip = gs::uploadMipped(vdp, drip());
    art.lamp = gs::uploadMipped(vdp, lamp());

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);
    vdp.setFogColor(gs::rgb4(1, 2, 3));
}

}  // namespace culvertbann
