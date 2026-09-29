#include "game/art.h"

#include <string>

namespace orchardbann {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

int makeTile(gs::TileAlloc& al, gs::VDP& vdp, std::initializer_list<const char*> rows) {
    uint8_t px[64] = {};
    int y = 0;
    for (const char* row : rows) {
        if (y >= 8) break;
        for (int x = 0; x < 8 && row[x]; x++) {
            char c = row[x];
            int v = 0;
            if (c >= '0' && c <= '9') v = c - '0';
            else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
            px[y * 8 + x] = uint8_t(v);
        }
        y++;
    }
    int t = al.alloc(1);
    vdp.loadTile(t, px);
    return t;
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

gs::Bitmap picker(int pose) {
    gs::Bitmap b(30, 46);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose == 2 ? 1 : 0;
    R(9, 0, 12, 4, 6);
    R(8, 3, 14, 2, 7);
    R(11, 5 + bob, 8, 7, 4);
    R(17, 7 + bob, 2, 2, 1);
    R(9, 12 + bob, 12, 12, 2);
    R(9, 12 + bob, 4, 12, 3);
    R(12, 20 + bob, 6, 2, 5);
    if (pose == 1) {
        R(8, 24, 5, 14, 2);
        R(16, 26, 5, 12, 2);
        R(7, 37, 7, 4, 6);
        R(15, 37, 7, 4, 6);
        R(20, 14, 4, 8, 4);
    } else if (pose == 2) {
        R(8, 26, 5, 12, 2);
        R(17, 24, 5, 14, 2);
        R(7, 37, 7, 4, 6);
        R(16, 37, 7, 4, 6);
        R(20, 15, 4, 8, 4);
    } else if (pose == 3) {
        R(10, 24, 4, 14, 2);
        R(16, 24, 4, 14, 2);
        R(9, 37, 6, 4, 6);
        R(15, 37, 6, 4, 6);
        R(20, 16, 8, 3, 2);
        R(26, 14, 3, 4, 5);
    } else {
        R(10, 24, 4, 14, 2);
        R(16, 24, 4, 14, 2);
        R(9, 37, 6, 4, 6);
        R(15, 37, 6, 4, 6);
        R(20, 14, 3, 10, 4);
    }
    return b;
}

gs::Bitmap scare(int step) {
    gs::Bitmap b(34, 48);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(12, 2, 10, 4, 6);
    R(11, 5, 12, 3, 7);
    R(13, 8, 8, 7, 5);
    R(15, 10, 2, 2, 1);
    R(19, 10, 2, 2, 1);
    R(14, 15, 6, 16, 2);
    R(14, 15, 6, 4, 3);
    int arm = step ? 2 : 0;
    R(2, 16 + arm, 12, 3, 4);
    R(20, 18 - arm, 12, 3, 4);
    R(15, 31, 2, 12, 4);
    R(19, 31, 2, 12, 4);
    return b;
}

gs::Bitmap cloth() {
    gs::Bitmap b(28, 34);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(2, 0, 24, 4, 6);
    R(4, 4, 20, 22, 2);
    R(4, 4, 5, 22, 3);
    R(12, 8, 6, 14, 4);
    R(6, 26, 3, 6, 2);
    R(12, 26, 3, 5, 3);
    R(18, 26, 3, 7, 2);
    return b;
}

gs::Bitmap treeBmp() {
    gs::Bitmap b(64, 78);
    b.rect(28, 40, 8, 36, 5);
    b.rect(26, 70, 12, 6, 6);
    b.ellipse(32, 28, 28, 24, 2);
    b.ellipse(22, 24, 12, 10, 3);
    b.ellipse(42, 30, 10, 8, 4);
    b.ellipse(30, 18, 6, 5, 1);
    b.ellipse(18, 34, 3, 3, 7);
    b.ellipse(40, 16, 3, 3, 7);
    b.ellipse(48, 28, 3, 3, 7);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(15, 15, 12), gs::rgb4(3, 4, 2), gs::rgb4(14, 10, 3),
                          gs::rgb4(12, 3, 3), gs::rgb4(6, 8, 4), gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_LEAF, {gs::rgb4(0, 0, 0), gs::rgb4(6, 12, 4), gs::rgb4(3, 8, 2), gs::rgb4(8, 14, 5),
                           gs::rgb4(2, 5, 1), gs::rgb4(10, 8, 3), gs::rgb4(4, 6, 2), gs::rgb4(12, 14, 6)});
    setPal(vdp, PAL_PICK, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(2, 6, 3), gs::rgb4(4, 9, 4),
                           gs::rgb4(13, 9, 6), gs::rgb4(10, 4, 3), gs::rgb4(8, 5, 2), gs::rgb4(14, 12, 4),
                           gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_BANNER, {gs::rgb4(0, 0, 0), gs::rgb4(2, 1, 1), gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1),
                             gs::rgb4(15, 13, 3), gs::rgb4(6, 4, 2), gs::rgb4(10, 8, 3)});
    setPal(vdp, PAL_SCARE, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(9, 6, 2), gs::rgb4(12, 8, 3),
                            gs::rgb4(6, 4, 2), gs::rgb4(11, 9, 6), gs::rgb4(4, 3, 1), gs::rgb4(14, 11, 4)});
    setPal(vdp, PAL_WOOD, {gs::rgb4(0, 0, 0), gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(8, 5, 2),
                           gs::rgb4(4, 3, 2), gs::rgb4(5, 3, 1), gs::rgb4(3, 2, 1), gs::rgb4(12, 3, 2)});
    setPal(vdp, PAL_APPLE, {gs::rgb4(0, 0, 0), gs::rgb4(8, 1, 1), gs::rgb4(14, 2, 2), gs::rgb4(12, 4, 2),
                            gs::rgb4(15, 14, 8), gs::rgb4(4, 8, 2), gs::rgb4(2, 4, 1)});
    setPal(vdp, PAL_EARTH, {gs::rgb4(0, 0, 0), gs::rgb4(4, 3, 1), gs::rgb4(6, 5, 2), gs::rgb4(8, 6, 3),
                            gs::rgb4(3, 5, 2), gs::rgb4(5, 7, 3), gs::rgb4(2, 2, 1)});

    loadFont(vdp, art, tiles);

    int sky = makeTile(tiles, vdp, {"11111111", "11111111", "11112211", "11111111", "11211111", "11111111",
                                    "11111121", "11111111"});
    int grass = makeTile(tiles, vdp, {"44424442", "24242424", "55525552", "42424242", "44424442", "25252525",
                                      "55535553", "42424242"});
    int soil = makeTile(tiles, vdp, {"11221122", "22112211", "11121112", "21112111", "12211221", "11121112",
                                     "22112211", "11221122"});

    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    for (int cy = 0; cy < 18; cy++) {
        for (int cx = 0; cx < 64; cx++) vdp.A.set(cx, cy, gs::entry(sky, PAL_LEAF));
    }
    for (int cy = 18; cy < 24; cy++) {
        for (int cx = 0; cx < 64; cx++) vdp.B.set(cx, cy, gs::entry(grass, PAL_EARTH));
    }
    for (int cy = 24; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) vdp.B.set(cx, cy, gs::entry(soil, PAL_EARTH));
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int lift = y < 120 ? y / 18 : 7;
        vdp.lineBackdrop[y] = gs::rgb4(5, 8 + lift / 2, 14 - y / 28);
    }
    vdp.setFogColor(gs::rgb4(8, 10, 6));

    art.stand = gs::uploadMipped(vdp, picker(0));
    art.walkA = gs::uploadMipped(vdp, picker(1));
    art.walkB = gs::uploadMipped(vdp, picker(2));
    art.swing = gs::uploadMipped(vdp, picker(3));
    art.scare[0] = gs::uploadMipped(vdp, scare(0));
    art.scare[1] = gs::uploadMipped(vdp, scare(1));
    art.banner = gs::uploadMipped(vdp, cloth());
    art.tree = gs::uploadMipped(vdp, treeBmp());
    {
        gs::Bitmap b(14, 14);
        b.ellipse(7, 7, 6, 6, 2);
        b.ellipse(5, 5, 2, 2, 4);
        b.rect(6, 1, 2, 3, 5);
        art.apple = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(18, 70);
        b.rect(2, 8, 4, 62, 5);
        b.rect(12, 8, 4, 62, 5);
        b.rect(2, 8, 14, 4, 6);
        b.rect(2, 28, 14, 3, 3);
        b.rect(2, 48, 14, 3, 3);
        art.gate = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(16, 12);
        b.rect(1, 3, 14, 8, 5);
        b.rect(1, 3, 14, 2, 6);
        b.rect(4, 0, 8, 3, 3);
        art.basket = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(24, 8);
        b.ellipse(12, 4, 11, 3, 1);
        art.shadow = gs::uploadMipped(vdp, b);
    }
}

}  // namespace orchardbann
