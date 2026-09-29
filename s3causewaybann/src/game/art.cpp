#include "game/art.h"

#include <string>

namespace causewaybann {
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
    gs::Bitmap b(28, 44);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose == 2 ? 1 : 0;
    R(8, 2 + bob, 12, 5, 6);   // cap
    R(8, 6 + bob, 12, 6, 4);   // face
    R(16, 7 + bob, 2, 2, 1);
    R(6, 12 + bob, 16, 13, 2); // slicker
    R(6, 12 + bob, 4, 13, 3);
    R(10, 20 + bob, 8, 2, 5);
    if (pose == 1) {
        R(7, 25, 5, 12, 2);
        R(16, 27, 5, 10, 2);
        R(6, 36, 7, 3, 7);
        R(15, 36, 7, 3, 7);
        R(20, 14, 4, 8, 3);
    } else if (pose == 2) {
        R(6, 27, 5, 10, 2);
        R(16, 25, 5, 12, 2);
        R(5, 36, 7, 3, 7);
        R(15, 36, 7, 3, 7);
        R(20, 15, 4, 8, 3);
    } else if (pose == 3) {
        R(9, 22, 4, 8, 2);
        R(15, 20, 5, 7, 2);
        R(8, 30, 5, 3, 7);
        R(16, 27, 6, 3, 7);
        R(18, 12, 6, 3, 3);
    } else {
        R(9, 25, 4, 12, 2);
        R(15, 25, 4, 12, 2);
        R(8, 36, 6, 3, 7);
        R(15, 36, 6, 3, 7);
        R(20, 15, 3, 9, 3);
    }
    return b;
}

gs::Bitmap watchman(int pose) {
    gs::Bitmap b(26, 42);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose ? 1 : 0;
    R(7, 1 + bob, 12, 6, 6);
    R(8, 6 + bob, 10, 5, 4);
    R(6, 11 + bob, 14, 14, 2);
    R(6, 11 + bob, 3, 14, 3);
    R(17, 14 + bob, 5, 5, 5); // lantern glow
    R(18, 15 + bob, 3, 3, 7);
    if (pose) {
        R(7, 25, 4, 11, 2);
        R(15, 26, 4, 10, 2);
        R(6, 35, 6, 3, 1);
        R(14, 35, 6, 3, 1);
    } else {
        R(8, 25, 4, 11, 2);
        R(14, 25, 4, 11, 2);
        R(7, 35, 6, 3, 1);
        R(13, 35, 6, 3, 1);
    }
    return b;
}

gs::Bitmap cloth() {
    gs::Bitmap b(26, 36);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(12, 1, 2, 32, 6);
    R(4, 4, 16, 5, 5);
    R(5, 9, 14, 16, 2);
    R(5, 9, 4, 16, 3);
    R(11, 12, 4, 10, 4);
    R(6, 25, 3, 6, 2);
    R(12, 25, 3, 5, 3);
    return b;
}

gs::Bitmap slab() {
    gs::Bitmap b(24, 16);
    b.rect(0, 2, 24, 12, 2);
    b.rect(0, 2, 24, 3, 4);
    b.rect(0, 11, 24, 3, 1);
    b.rect(11, 4, 2, 8, 3);
    return b;
}

gs::Bitmap pier() {
    gs::Bitmap b(10, 48);
    b.rect(3, 0, 4, 48, 2);
    b.rect(2, 0, 2, 48, 4);
    b.rect(1, 40, 8, 4, 1);
    return b;
}

gs::Bitmap wave() {
    gs::Bitmap b(32, 10);
    b.ellipse(8, 6, 7, 3, 3);
    b.ellipse(20, 7, 8, 3, 2);
    b.rect(4, 8, 22, 2, 4);
    return b;
}

gs::Bitmap lamp() {
    gs::Bitmap b(10, 28);
    b.rect(4, 8, 2, 20, 2);
    b.rect(2, 1, 6, 8, 3);
    b.rect(3, 2, 4, 5, 5);
    return b;
}

gs::Bitmap gate() {
    gs::Bitmap b(18, 52);
    b.rect(1, 8, 4, 44, 2);
    b.rect(13, 8, 4, 44, 2);
    b.rect(1, 8, 16, 4, 4);
    b.rect(1, 24, 16, 3, 3);
    return b;
}

gs::Bitmap boat() {
    gs::Bitmap b(36, 16);
    b.poly({{2, 8}, {8, 14}, {28, 14}, {34, 8}, {28, 8}, {8, 8}}, 2);
    b.rect(16, 2, 2, 8, 4);
    b.poly({{18, 3}, {28, 6}, {18, 8}}, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 15, 13), gs::rgb4(2, 4, 6), gs::rgb4(15, 12, 4),
                          gs::rgb4(12, 3, 3), gs::rgb4(6, 8, 9), gs::rgb4(1, 2, 3)});
    setPal(vdp, PAL_STONE, {gs::rgb4(0, 0, 0), gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 7), gs::rgb4(4, 5, 5),
                            gs::rgb4(9, 9, 8), gs::rgb4(2, 3, 3), gs::rgb4(8, 7, 5)});
    setPal(vdp, PAL_COAT, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(3, 5, 7), gs::rgb4(2, 3, 5),
                           gs::rgb4(12, 9, 7), gs::rgb4(8, 3, 3), gs::rgb4(10, 2, 2), gs::rgb4(2, 2, 2),
                           gs::rgb4(14, 13, 10)});
    setPal(vdp, PAL_BANNER, {gs::rgb4(0, 0, 0), gs::rgb4(2, 1, 1), gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 2),
                             gs::rgb4(14, 12, 3), gs::rgb4(6, 5, 2), gs::rgb4(5, 4, 3), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_WATCH, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(2, 3, 5), gs::rgb4(1, 2, 4),
                            gs::rgb4(9, 8, 6), gs::rgb4(14, 11, 3), gs::rgb4(4, 4, 3), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_WATER, {gs::rgb4(0, 0, 0), gs::rgb4(1, 3, 6), gs::rgb4(2, 6, 9), gs::rgb4(4, 9, 11),
                            gs::rgb4(8, 12, 13), gs::rgb4(1, 2, 4)});
    setPal(vdp, PAL_WOOD, {gs::rgb4(0, 0, 0), gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(8, 6, 3),
                           gs::rgb4(10, 8, 5), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_LAMP, {gs::rgb4(0, 0, 0), gs::rgb4(3, 3, 2), gs::rgb4(5, 5, 4), gs::rgb4(8, 7, 3),
                           gs::rgb4(12, 10, 4), gs::rgb4(15, 14, 6)});

    art.stand = gs::uploadMipped(vdp, runner(0));
    art.walkA = gs::uploadMipped(vdp, runner(1));
    art.walkB = gs::uploadMipped(vdp, runner(2));
    art.leap = gs::uploadMipped(vdp, runner(3));
    art.watch[0] = gs::uploadMipped(vdp, watchman(0));
    art.watch[1] = gs::uploadMipped(vdp, watchman(1));
    art.banner = gs::uploadMipped(vdp, cloth());
    art.slab = gs::uploadMipped(vdp, slab());
    art.pier = gs::uploadMipped(vdp, pier());
    art.wave = gs::uploadMipped(vdp, wave());
    art.lamp = gs::uploadMipped(vdp, lamp());
    art.gate = gs::uploadMipped(vdp, gate());
    art.boat = gs::uploadMipped(vdp, boat());
    loadFont(vdp, art, tiles);
    vdp.setFogColor(gs::rgb4(2, 4, 7));
}

}  // namespace causewaybann
