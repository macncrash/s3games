#include "game/art.h"

#include <string>

namespace trenchbann {
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
    gs::Bitmap b(28, low ? 28 : 44);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    if (low) {
        R(4, 6, 16, 8, 2);
        R(6, 2, 10, 6, 4);
        R(14, 4, 2, 2, 9);
        R(2, 12, 20, 8, 3);
        R(3, 18, 8, 5, 6);
        R(14, 18, 9, 5, 6);
        R(18, 8, 8, 3, 2);
        return b;
    }
    int bob = pose == 2 ? 1 : 0;
    R(8, 2 + bob, 11, 6, 4);
    R(7, 0 + bob, 13, 3, 7);
    R(16, 4 + bob, 2, 2, 9);
    R(6, 8 + bob, 14, 13, 2);
    R(6, 8 + bob, 3, 13, 3);
    R(10, 16 + bob, 6, 2, 5);
    if (pose == 1) {
        R(7, 21, 5, 14, 2);
        R(15, 23, 5, 12, 2);
        R(6, 34, 7, 4, 6);
        R(14, 34, 7, 4, 6);
        R(18, 10, 3, 10, 4);
    } else if (pose == 2) {
        R(6, 23, 5, 12, 2);
        R(16, 21, 5, 14, 2);
        R(5, 34, 7, 4, 6);
        R(15, 34, 7, 4, 6);
        R(18, 11, 3, 10, 4);
    } else {
        R(9, 21, 4, 14, 2);
        R(15, 21, 4, 14, 2);
        R(8, 34, 6, 4, 6);
        R(15, 34, 6, 4, 6);
        R(18, 10, 3, 11, 4);
    }
    return b;
}

gs::Bitmap cloth() {
    gs::Bitmap b(26, 34);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(2, 1, 18, 3, 6);
    R(4, 4, 16, 22, 2);
    R(4, 4, 16, 4, 3);
    R(8, 10, 8, 8, 4);
    R(10, 12, 4, 4, 5);
    R(5, 26, 3, 6, 2);
    R(11, 26, 3, 5, 3);
    R(16, 26, 3, 6, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 9), gs::rgb4(3, 3, 2), gs::rgb4(15, 8, 2),
                          gs::rgb4(12, 3, 2), gs::rgb4(6, 6, 4), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_TIMBER, {gs::rgb4(0, 0, 0), gs::rgb4(4, 3, 1), gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1),
                             gs::rgb4(8, 6, 3), gs::rgb4(2, 2, 1), gs::rgb4(5, 4, 2), gs::rgb4(9, 7, 4),
                             gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_COAT, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(5, 6, 3), gs::rgb4(3, 4, 2),
                           gs::rgb4(9, 8, 5), gs::rgb4(7, 3, 2), gs::rgb4(2, 2, 1), gs::rgb4(11, 9, 6),
                           gs::rgb4(13, 11, 7), gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_BANNER, {gs::rgb4(0, 0, 0), gs::rgb4(2, 1, 1), gs::rgb4(12, 2, 1), gs::rgb4(8, 1, 1),
                             gs::rgb4(14, 13, 8), gs::rgb4(6, 5, 2), gs::rgb4(9, 6, 2), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_FLARE, {gs::rgb4(0, 0, 0), gs::rgb4(4, 2, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 8, 2),
                            gs::rgb4(12, 4, 1), gs::rgb4(6, 3, 1), gs::rgb4(14, 14, 8), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_MUD, {gs::rgb4(0, 0, 0), gs::rgb4(3, 2, 1), gs::rgb4(5, 4, 2), gs::rgb4(2, 2, 1),
                          gs::rgb4(6, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(1, 1, 1), gs::rgb4(7, 6, 3)});
    setPal(vdp, PAL_BAG, {gs::rgb4(0, 0, 0), gs::rgb4(5, 4, 2), gs::rgb4(8, 7, 4), gs::rgb4(6, 5, 3),
                          gs::rgb4(3, 3, 2), gs::rgb4(10, 8, 5), gs::rgb4(2, 2, 1), gs::rgb4(12, 9, 5)});
    setPal(vdp, PAL_SKY, {gs::rgb4(0, 0, 0), gs::rgb4(2, 2, 2), gs::rgb4(3, 3, 3), gs::rgb4(4, 4, 4),
                          gs::rgb4(6, 5, 4), gs::rgb4(8, 7, 6)});

    loadFont(vdp, art, tiles);

    int plank = makeTile(tiles, vdp, {"22332233", "22332233", "11111111", "33223322", "33223322", "11111111",
                                      "22332233", "22332233"});
    int post = makeTile(tiles, vdp, {"33111133", "33222233", "33222233", "33111133", "33222233", "33222233",
                                     "33111133", "11111111"});
    int duck = makeTile(tiles, vdp, {"44444444", "11111111", "66666666", "11111111", "55555555", "11111111",
                                     "66666666", "11111111"});
    int slop = makeTile(tiles, vdp, {"23232323", "32323232", "11111111", "22332233", "33223322", "11111111",
                                     "23232323", "32323232"});

    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    for (int cy = 8; cy < 21; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int tile = (cx % 7 == 0) ? post : plank;
            if (cy == 8 || cy == 20) tile = post;
            vdp.A.set(cx, cy, gs::entry(tile, PAL_TIMBER));
        }
    }
    for (int cy = 21; cy < 28; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int tile = (cy < 23) ? duck : slop;
            vdp.B.set(cx, cy, gs::entry(tile, cy < 23 ? PAL_TIMBER : PAL_MUD));
        }
    }

    art.stand = gs::uploadMipped(vdp, runner(0, false));
    art.walkA = gs::uploadMipped(vdp, runner(1, false));
    art.walkB = gs::uploadMipped(vdp, runner(2, false));
    art.crouch = gs::uploadMipped(vdp, runner(0, true));
    art.banner = gs::uploadMipped(vdp, cloth());

    {
        gs::Bitmap b(8, 48);
        b.rect(3, 0, 2, 48, 6);
        b.rect(2, 0, 4, 3, 7);
        art.pole = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(28, 16);
        b.rect(1, 4, 26, 10, 2);
        b.rect(1, 4, 26, 3, 3);
        b.rect(3, 8, 8, 4, 5);
        b.rect(15, 8, 8, 4, 1);
        art.bag = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(22, 8);
        b.rect(0, 2, 22, 4, 4);
        b.rect(0, 5, 22, 2, 1);
        b.rect(2, 1, 3, 6, 6);
        b.rect(17, 1, 3, 6, 6);
        art.duck = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(18, 28);
        b.ellipse(9, 8, 7, 6, 2);
        b.ellipse(9, 8, 4, 3, 6);
        b.rect(8, 12, 2, 14, 5);
        b.rect(4, 14, 10, 2, 3);
        art.flare = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(10, 36);
        b.rect(4, 0, 2, 36, 4);
        b.rect(2, 2, 6, 3, 6);
        art.stake = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(22, 8);
        b.ellipse(11, 4, 10, 3, 1);
        art.shadow = gs::uploadMipped(vdp, b);
    }

    for (int y = 0; y < gs::SCREEN_H; y++) {
        int k = y < 40 ? 3 : y < 90 ? 2 : 1;
        vdp.lineBackdrop[y] = gs::rgb4(k + 1, k + 1, k);
    }
    vdp.setFogColor(gs::rgb4(2, 2, 1));
    vdp.A.enabled = true;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) vdp.road[y].on = false;
}

}  // namespace trenchbann
