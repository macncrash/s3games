#include "game/art.h"

#include <string>

namespace towerbann {
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

gs::Bitmap climber(int pose) {
    gs::Bitmap b(26, 42);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose == 2 ? 1 : 0;
    R(9, 2 + bob, 8, 7, 4);
    R(10, 3 + bob, 6, 2, 7);
    R(11, 5 + bob, 2, 2, 8);
    R(8, 8 + bob, 10, 3, 6);
    R(7, 11 + bob, 12, 12, 2);
    R(7, 11 + bob, 3, 12, 3);
    R(10, 16 + bob, 6, 2, 6);
    if (pose == 1) {
        R(6, 22, 5, 13, 2);
        R(15, 23, 5, 12, 2);
        R(5, 34, 7, 4, 5);
        R(14, 34, 7, 4, 5);
        R(18, 13, 3, 9, 4);
    } else if (pose == 2) {
        R(7, 23, 5, 12, 2);
        R(14, 22, 5, 13, 2);
        R(6, 34, 7, 4, 5);
        R(14, 34, 7, 4, 5);
        R(4, 14, 3, 9, 4);
    } else if (pose == 3) {
        R(8, 21, 4, 8, 2);
        R(14, 21, 4, 8, 2);
        R(7, 28, 6, 3, 5);
        R(14, 28, 6, 3, 5);
        R(18, 12, 4, 3, 2);
        R(4, 13, 4, 3, 4);
    } else {
        R(8, 23, 4, 12, 2);
        R(14, 23, 4, 12, 2);
        R(7, 34, 6, 4, 5);
        R(14, 34, 6, 4, 5);
        R(18, 13, 3, 10, 4);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap crow(int flap) {
    gs::Bitmap b(22, 14);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(6, 5, 12, 5, 2);
    R(4, 6, 4, 3, 3);
    R(16, 6, 4, 2, 4);
    R(2, flap ? 3 : 7, 6, 2, 2);
    R(14, flap ? 2 : 8, 6, 2, 3);
    R(8, 9, 2, 3, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap flag() {
    gs::Bitmap b(22, 28);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(3, 2, 3, 24, 5);
    R(6, 3, 13, 14, 2);
    R(6, 3, 13, 3, 3);
    R(10, 8, 5, 5, 4);
    R(16, 14, 3, 4, 2);
    b.outline(1, false);
    return b;
}

gs::Bitmap doorBmp() {
    gs::Bitmap b(28, 40);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(4, 6, 20, 32, 2);
    R(4, 4, 20, 6, 3);
    R(8, 14, 5, 8, 4);
    R(15, 22, 2, 2, 5);
    R(7, 30, 14, 8, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap ledgeBmp() {
    gs::Bitmap b(16, 8);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(0, 1, 16, 6, 2);
    R(0, 1, 16, 2, 3);
    R(0, 5, 16, 2, 4);
    R(7, 2, 1, 5, 5);
    return b;
}

gs::Bitmap wallBmp() {
    gs::Bitmap b(16, 32);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(0, 0, 16, 32, 2);
    R(0, 0, 16, 2, 3);
    R(0, 15, 16, 2, 4);
    R(0, 30, 16, 2, 4);
    R(2, 4, 5, 8, 5);
    R(9, 18, 5, 8, 5);
    return b;
}

gs::Bitmap slitBmp() {
    gs::Bitmap b(10, 16);
    b.rect(2, 1, 6, 14, 2);
    b.rect(3, 2, 4, 6, 3);
    b.rect(3, 9, 4, 4, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap poleBmp() {
    gs::Bitmap b(6, 36);
    b.rect(2, 0, 2, 36, 2);
    b.rect(1, 0, 4, 3, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 14, 12), gs::rgb4(2, 2, 3), gs::rgb4(15, 12, 6),
                          gs::rgb4(12, 3, 3), gs::rgb4(6, 7, 8), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_STONE, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9),
                            gs::rgb4(3, 3, 4), gs::rgb4(12, 9, 4), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_HERO, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(2, 7, 8), gs::rgb4(1, 4, 5),
                           gs::rgb4(12, 8, 6), gs::rgb4(5, 3, 2), gs::rgb4(13, 10, 4), gs::rgb4(3, 2, 2),
                           gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_BANNER, {gs::rgb4(0, 0, 0), gs::rgb4(2, 0, 1), gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 2),
                             gs::rgb4(14, 12, 4), gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 3)});
    setPal(vdp, PAL_CROW, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5),
                           gs::rgb4(14, 11, 3), gs::rgb4(6, 4, 2), gs::rgb4(8, 8, 8)});
    setPal(vdp, PAL_DOOR, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(4, 3, 2), gs::rgb4(7, 5, 3),
                           gs::rgb4(14, 12, 5), gs::rgb4(12, 9, 3), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_AMBER, {gs::rgb4(0, 0, 0), gs::rgb4(3, 2, 1), gs::rgb4(14, 10, 3), gs::rgb4(15, 14, 7),
                            gs::rgb4(8, 5, 1), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_NIGHT, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 3), gs::rgb4(2, 2, 5), gs::rgb4(4, 4, 7),
                            gs::rgb4(8, 8, 10), gs::rgb4(12, 12, 13)});

    loadFont(vdp, art, tiles);
    art.stand = gs::uploadMipped(vdp, climber(0));
    art.walkA = gs::uploadMipped(vdp, climber(1));
    art.walkB = gs::uploadMipped(vdp, climber(2));
    art.jump = gs::uploadMipped(vdp, climber(3));
    art.crow[0] = gs::uploadMipped(vdp, crow(0));
    art.crow[1] = gs::uploadMipped(vdp, crow(1));
    art.banner = gs::uploadMipped(vdp, flag());
    art.pole = gs::uploadMipped(vdp, poleBmp());
    art.door = gs::uploadMipped(vdp, doorBmp());
    art.ledge = gs::uploadMipped(vdp, ledgeBmp());
    art.wall = gs::uploadMipped(vdp, wallBmp());
    art.slit = gs::uploadMipped(vdp, slitBmp());

    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) vdp.road[y].on = false;
    vdp.setFogColor(gs::rgb4(1, 1, 2));
}

}  // namespace towerbann
