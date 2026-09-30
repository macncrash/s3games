#include "game/art.h"

#include <string>

namespace trenchladd {
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

gs::Bitmap soldier(int pose) {
    gs::Bitmap b(28, 48);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    if (pose == 3) {
        R(2, 14, 20, 8, 2);
        R(4, 8, 12, 7, 4);
        R(6, 6, 14, 3, 7);
        R(14, 10, 2, 2, 8);
        R(1, 20, 24, 5, 3);
        R(3, 24, 8, 4, 6);
        R(14, 24, 9, 4, 6);
        R(18, 12, 8, 3, 5);
        return b;
    }
    if (pose == 4) {
        R(8, 2, 10, 7, 4);
        R(7, 1, 12, 3, 7);
        R(16, 4, 2, 2, 8);
        R(6, 9, 13, 12, 2);
        R(6, 9, 3, 12, 5);
        R(17, 11, 8, 3, 6);
        R(4, 18, 8, 5, 2);
        R(14, 16, 6, 6, 2);
        R(3, 22, 7, 3, 6);
        R(15, 21, 6, 3, 6);
        return b;
    }
    int bob = pose == 2 ? 1 : 0;
    R(8, 1 + bob, 11, 8, 4);
    R(7, 0 + bob, 13, 3, 7);
    R(17, 4 + bob, 2, 2, 8);
    R(6, 9 + bob, 14, 13, 2);
    R(6, 9 + bob, 3, 13, 5);
    R(10, 17 + bob, 6, 2, 3);
    R(18, 11 + bob, 8, 3, 6);
    if (pose == 1) {
        R(7, 22, 5, 16, 2);
        R(15, 24, 5, 14, 2);
        R(6, 37, 7, 4, 6);
        R(14, 37, 7, 4, 6);
    } else if (pose == 2) {
        R(6, 24, 5, 14, 2);
        R(15, 22, 5, 16, 2);
        R(5, 37, 7, 4, 6);
        R(14, 37, 7, 4, 6);
    } else {
        R(9, 22, 4, 16, 2);
        R(15, 22, 4, 16, 2);
        R(8, 37, 6, 4, 6);
        R(14, 37, 6, 4, 6);
    }
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 10), gs::rgb4(2, 3, 2), gs::rgb4(15, 9, 2),
                          gs::rgb4(12, 3, 2), gs::rgb4(5, 6, 4), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CHALK, {gs::rgb4(0, 0, 0), gs::rgb4(7, 7, 5), gs::rgb4(10, 10, 7), gs::rgb4(4, 4, 3),
                            gs::rgb4(5, 5, 3), gs::rgb4(2, 3, 2), gs::rgb4(8, 7, 4), gs::rgb4(12, 11, 8),
                            gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_KHAKI, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(5, 6, 3), gs::rgb4(3, 4, 2),
                            gs::rgb4(9, 8, 5), gs::rgb4(3, 3, 2), gs::rgb4(2, 2, 1), gs::rgb4(4, 3, 1),
                            gs::rgb4(1, 1, 2), gs::rgb4(12, 10, 6)});
    setPal(vdp, PAL_LADDER, {gs::rgb4(0, 0, 0), gs::rgb4(4, 2, 1), gs::rgb4(8, 4, 2), gs::rgb4(11, 6, 2),
                             gs::rgb4(14, 10, 4), gs::rgb4(6, 4, 2), gs::rgb4(2, 1, 1), gs::rgb4(13, 12, 8)});
    setPal(vdp, PAL_TRACE, {gs::rgb4(0, 0, 0), gs::rgb4(6, 4, 1), gs::rgb4(15, 14, 6), gs::rgb4(15, 8, 2),
                            gs::rgb4(12, 3, 1), gs::rgb4(7, 2, 1), gs::rgb4(15, 15, 12), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_MUD, {gs::rgb4(0, 0, 0), gs::rgb4(2, 2, 1), gs::rgb4(4, 3, 2), gs::rgb4(1, 1, 1),
                          gs::rgb4(6, 5, 2), gs::rgb4(3, 2, 1), gs::rgb4(0, 1, 1), gs::rgb4(7, 6, 3)});
    setPal(vdp, PAL_IRON, {gs::rgb4(0, 0, 0), gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 5), gs::rgb4(8, 8, 7),
                           gs::rgb4(1, 1, 1), gs::rgb4(4, 5, 3), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_SKY, {gs::rgb4(0, 0, 0), gs::rgb4(1, 2, 2), gs::rgb4(2, 3, 3), gs::rgb4(3, 4, 4),
                          gs::rgb4(4, 5, 4), gs::rgb4(7, 7, 5)});

    loadFont(vdp, art, tiles);

    int board = makeTile(tiles, vdp, {"11221122", "22112211", "55555555", "11221122", "22112211", "33333333",
                                      "11221122", "22112211"});
    int revet = makeTile(tiles, vdp, {"66666666", "44444444", "22222222", "33333333", "22222222", "44444444",
                                      "66666666", "11111111"});
    int duck = makeTile(tiles, vdp, {"33333333", "11111111", "55555555", "11111111", "22222222", "11111111",
                                     "55555555", "11111111"});
    int slop = makeTile(tiles, vdp, {"12121212", "21212121", "14141414", "41414141", "12121212", "11111111",
                                     "21212121", "12121212"});
    int night = makeTile(tiles, vdp, {"11111111", "11121111", "11111111", "31111111", "11111111", "11111141",
                                      "11111111", "11111111"});

    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    for (int cy = 0; cy < 8; cy++) {
        for (int cx = 0; cx < 64; cx++) vdp.A.set(cx, cy, gs::entry(night, PAL_SKY, (cx * 3 + cy) & 1, 0));
    }
    for (int cy = 8; cy < 18; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int tile = (cy & 1) ? revet : board;
            vdp.A.set(cx, cy, gs::entry(tile, PAL_CHALK, (cx + cy) & 1, 0));
        }
    }
    for (int cy = 18; cy < 28; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int tile = (cy < 22) ? duck : slop;
            vdp.B.set(cx, cy, gs::entry(tile, cy < 22 ? PAL_CHALK : PAL_MUD));
        }
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int k = y < 70 ? 1 : 2;
        vdp.lineBackdrop[y] = gs::rgb4(k, k + 1, k + 1);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.setFogColor(gs::rgb4(1, 2, 2));

    art.stand = gs::uploadMipped(vdp, soldier(0));
    art.walkA = gs::uploadMipped(vdp, soldier(1));
    art.walkB = gs::uploadMipped(vdp, soldier(2));
    art.crouch = gs::uploadMipped(vdp, soldier(3));
    art.leap = gs::uploadMipped(vdp, soldier(4));

    {
        gs::Bitmap b(18, 78);
        b.rect(1, 0, 3, 78, 3);
        b.rect(14, 0, 3, 78, 2);
        for (int i = 0; i < 9; i++) b.rect(1, 4 + i * 8, 16, 2, 4);
        art.ladder = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(16, 6);
        b.rect(0, 2, 16, 2, 4);
        b.rect(0, 4, 16, 1, 2);
        art.rung = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(48, 16);
        b.ellipse(24, 8, 22, 7, 1);
        b.ellipse(24, 9, 14, 4, 3);
        b.ellipse(18, 7, 4, 2, 6);
        art.crater = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(10, 36);
        b.rect(3, 6, 4, 30, 2);
        b.rect(1, 4, 8, 5, 3);
        b.rect(0, 8, 10, 3, 6);
        art.post = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(14, 6);
        b.rect(0, 2, 14, 2, 2);
        b.rect(8, 1, 5, 4, 6);
        art.tracer = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(28, 14);
        b.rect(1, 4, 26, 8, 2);
        b.rect(1, 4, 26, 3, 1);
        b.rect(4, 7, 7, 4, 5);
        b.rect(15, 7, 8, 4, 3);
        art.bag = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(22, 6);
        b.ellipse(11, 3, 10, 2, 1);
        art.shadow = gs::uploadMipped(vdp, b);
    }
}

}  // namespace trenchladd
