#include "game/art.h"

#include <string>

namespace trenchpouc {
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

gs::Bitmap runner(int pose, bool low) {
    gs::Bitmap b(30, low ? 26 : 46);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    if (low) {
        R(3, 8, 18, 7, 2);
        R(5, 3, 11, 6, 4);
        R(14, 5, 2, 2, 8);
        R(2, 14, 22, 6, 3);
        R(4, 19, 8, 4, 6);
        R(15, 19, 8, 4, 6);
        R(20, 9, 7, 3, 2);
        R(1, 12, 4, 3, 5);
        return b;
    }
    int bob = pose == 2 ? 1 : 0;
    R(9, 1 + bob, 11, 7, 4);
    R(8, 0 + bob, 13, 3, 7);
    R(17, 4 + bob, 2, 2, 8);
    R(7, 8 + bob, 14, 14, 2);
    R(7, 8 + bob, 3, 14, 5);
    R(11, 16 + bob, 6, 2, 3);
    R(19, 10 + bob, 4, 8, 4);
    if (pose == 1) {
        R(8, 22, 5, 15, 2);
        R(16, 24, 5, 13, 2);
        R(7, 36, 7, 4, 6);
        R(15, 36, 7, 4, 6);
    } else if (pose == 2) {
        R(7, 24, 5, 13, 2);
        R(16, 22, 5, 15, 2);
        R(6, 36, 7, 4, 6);
        R(15, 36, 7, 4, 6);
    } else {
        R(10, 22, 4, 15, 2);
        R(16, 22, 4, 15, 2);
        R(9, 36, 6, 4, 6);
        R(15, 36, 6, 4, 6);
    }
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 14, 11), gs::rgb4(3, 3, 2), gs::rgb4(15, 10, 3),
                          gs::rgb4(12, 3, 2), gs::rgb4(6, 7, 5), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CHALK, {gs::rgb4(0, 0, 0), gs::rgb4(8, 8, 6), gs::rgb4(11, 11, 8), gs::rgb4(5, 5, 4),
                            gs::rgb4(6, 5, 3), gs::rgb4(3, 3, 2), gs::rgb4(9, 8, 5), gs::rgb4(13, 12, 9),
                            gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_KHAKI, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(6, 6, 3), gs::rgb4(4, 4, 2),
                            gs::rgb4(10, 8, 5), gs::rgb4(3, 4, 2), gs::rgb4(2, 2, 1), gs::rgb4(4, 3, 2),
                            gs::rgb4(1, 1, 2), gs::rgb4(12, 10, 7)});
    setPal(vdp, PAL_POUCH, {gs::rgb4(0, 0, 0), gs::rgb4(4, 2, 1), gs::rgb4(8, 4, 1), gs::rgb4(11, 6, 2),
                            gs::rgb4(14, 11, 4), gs::rgb4(6, 5, 2), gs::rgb4(2, 1, 1), gs::rgb4(13, 12, 8)});
    setPal(vdp, PAL_FLARE, {gs::rgb4(0, 0, 0), gs::rgb4(5, 3, 1), gs::rgb4(15, 13, 4), gs::rgb4(15, 8, 2),
                            gs::rgb4(12, 4, 1), gs::rgb4(7, 3, 1), gs::rgb4(15, 15, 10), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_MUD, {gs::rgb4(0, 0, 0), gs::rgb4(3, 2, 1), gs::rgb4(5, 4, 2), gs::rgb4(2, 2, 1),
                          gs::rgb4(6, 5, 2), gs::rgb4(4, 3, 1), gs::rgb4(1, 1, 0), gs::rgb4(7, 6, 3)});
    setPal(vdp, PAL_WIRE, {gs::rgb4(0, 0, 0), gs::rgb4(3, 3, 3), gs::rgb4(6, 6, 5), gs::rgb4(9, 9, 7),
                           gs::rgb4(2, 2, 2), gs::rgb4(4, 5, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_SKY, {gs::rgb4(0, 0, 0), gs::rgb4(1, 2, 3), gs::rgb4(2, 3, 4), gs::rgb4(3, 4, 5),
                          gs::rgb4(5, 5, 4), gs::rgb4(8, 7, 5)});

    loadFont(vdp, art, tiles);

    int chalk = makeTile(tiles, vdp, {"11221122", "22112211", "33333333", "11221122", "22112211", "44444444",
                                      "11221122", "22112211"});
    int revet = makeTile(tiles, vdp, {"66666666", "55555555", "22222222", "33333333", "22222222", "55555555",
                                      "66666666", "11111111"});
    int duck = makeTile(tiles, vdp, {"44444444", "11111111", "66666666", "11111111", "55555555", "11111111",
                                     "66666666", "11111111"});
    int slop = makeTile(tiles, vdp, {"23232323", "32323232", "14141414", "41414141", "23232323", "11111111",
                                     "32323232", "23232323"});

    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    for (int cy = 6; cy < 20; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int tile = (cy & 1) ? revet : chalk;
            vdp.A.set(cx, cy, gs::entry(tile, PAL_CHALK, (cx + cy) & 1, 0));
        }
    }
    for (int cy = 20; cy < 28; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int tile = (cy < 23) ? duck : slop;
            vdp.B.set(cx, cy, gs::entry(tile, cy < 23 ? PAL_CHALK : PAL_MUD));
        }
    }

    art.stand = gs::uploadMipped(vdp, runner(0, false));
    art.walkA = gs::uploadMipped(vdp, runner(1, false));
    art.walkB = gs::uploadMipped(vdp, runner(2, false));
    art.crouch = gs::uploadMipped(vdp, runner(0, true));

    {
        gs::Bitmap b(22, 16);
        b.rect(2, 3, 16, 11, 2);
        b.rect(2, 3, 16, 3, 3);
        b.rect(6, 6, 8, 5, 1);
        b.rect(8, 1, 4, 4, 4);
        b.rect(7, 0, 6, 2, 7);
        b.ellipse(17, 8, 4, 5, 5);
        art.pouch = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(36, 18);
        for (int i = 0; i < 5; i++) b.ellipse(6.f + i * 6.f, 9.f, 5.f, 5.f, 2);
        for (int i = 0; i < 4; i++) b.ellipse(9.f + i * 6.f, 9.f, 3.f, 3.f, 3);
        b.rect(0, 14, 36, 2, 4);
        art.coil = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(8, 40);
        b.rect(3, 0, 2, 40, 2);
        b.rect(1, 2, 6, 3, 3);
        art.stake = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(20, 20);
        b.ellipse(10, 10, 8, 8, 2);
        b.ellipse(10, 10, 4, 4, 6);
        b.rect(9, 0, 2, 20, 3);
        b.rect(0, 9, 20, 2, 3);
        art.burst = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(40, 10);
        b.rect(0, 2, 40, 5, 4);
        b.rect(0, 6, 40, 2, 1);
        b.rect(4, 1, 4, 8, 6);
        b.rect(32, 1, 4, 8, 6);
        art.plank = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(26, 14);
        b.rect(1, 3, 24, 9, 2);
        b.rect(1, 3, 24, 3, 1);
        b.rect(4, 6, 6, 4, 5);
        b.rect(14, 6, 7, 4, 3);
        art.bag = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(22, 8);
        b.ellipse(11, 4, 10, 3, 1);
        art.shadow = gs::uploadMipped(vdp, b);
    }

    for (int y = 0; y < gs::SCREEN_H; y++) {
        int k = y < 36 ? 3 : y < 80 ? 2 : 1;
        vdp.lineBackdrop[y] = gs::rgb4(k, k + 1, k + 2);
    }
    vdp.setFogColor(gs::rgb4(2, 2, 2));
    vdp.A.enabled = true;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) vdp.road[y].on = false;
}

}  // namespace trenchpouc
