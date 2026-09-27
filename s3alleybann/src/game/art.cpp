#include "game/art.h"

#include <string>

namespace alleybann {
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

gs::Bitmap person(int pose, int coat, int trim, int cap) {
    gs::Bitmap b(28, 44);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose == 2 ? 1 : 0;
    R(10, 2 + bob, 8, 6, 4);
    R(9, 1 + bob, 10, 3, cap);
    R(16, 4 + bob, 2, 2, 9);
    R(8, 8 + bob, 12, 14, coat);
    R(8, 8 + bob, 3, 14, trim);
    R(11, 16 + bob, 6, 2, 5);
    if (pose == 1) {
        R(9, 22, 4, 14, coat);
        R(15, 24, 4, 12, coat);
        R(8, 35, 6, 4, 6);
        R(14, 35, 6, 4, 6);
        R(18, 10, 3, 10, 4);
    } else if (pose == 2) {
        R(8, 24, 4, 12, coat);
        R(16, 22, 4, 14, coat);
        R(7, 35, 6, 4, 6);
        R(15, 35, 6, 4, 6);
        R(18, 11, 3, 10, 4);
    } else if (pose == 3) {
        R(10, 22, 4, 14, coat);
        R(15, 22, 4, 14, coat);
        R(9, 35, 6, 4, 6);
        R(15, 35, 6, 4, 6);
        R(18, 12, 8, 3, coat);
        R(24, 11, 3, 3, 7);
    } else {
        R(10, 22, 4, 14, coat);
        R(15, 22, 4, 14, coat);
        R(9, 35, 6, 4, 6);
        R(15, 35, 6, 4, 6);
        R(18, 10, 3, 10, 4);
    }
    return b;
}

gs::Bitmap cloth() {
    gs::Bitmap b(26, 36);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(4, 2, 18, 4, 6);
    R(6, 6, 14, 22, 2);
    R(6, 6, 4, 22, 3);
    R(12, 10, 4, 14, 4);
    R(8, 28, 3, 6, 2);
    R(13, 28, 3, 5, 2);
    R(18, 28, 3, 6, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(15, 14, 10), gs::rgb4(4, 3, 2), gs::rgb4(15, 8, 3),
                          gs::rgb4(12, 3, 3), gs::rgb4(8, 8, 8), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_BRICK, {gs::rgb4(0, 0, 0), gs::rgb4(6, 3, 3), gs::rgb4(8, 4, 3), gs::rgb4(4, 2, 2),
                            gs::rgb4(9, 8, 7), gs::rgb4(3, 3, 4), gs::rgb4(5, 5, 6), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_COAT, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(3, 5, 8), gs::rgb4(5, 8, 12),
                           gs::rgb4(12, 8, 6), gs::rgb4(8, 2, 2), gs::rgb4(2, 2, 2), gs::rgb4(10, 10, 11),
                           gs::rgb4(14, 12, 6), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_BANNER, {gs::rgb4(0, 0, 0), gs::rgb4(2, 1, 1), gs::rgb4(12, 2, 2), gs::rgb4(8, 1, 1),
                             gs::rgb4(14, 12, 4), gs::rgb4(6, 4, 2), gs::rgb4(10, 8, 4), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_WATCH, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 8),
                            gs::rgb4(11, 8, 6), gs::rgb4(9, 7, 3), gs::rgb4(2, 2, 2), gs::rgb4(12, 11, 6),
                            gs::rgb4(14, 10, 3), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_WOOD, {gs::rgb4(0, 0, 0), gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(8, 6, 3),
                           gs::rgb4(4, 4, 4), gs::rgb4(10, 9, 7), gs::rgb4(2, 2, 2), gs::rgb4(12, 8, 3)});
    setPal(vdp, PAL_LAMP, {gs::rgb4(0, 0, 0), gs::rgb4(4, 3, 1), gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 8),
                           gs::rgb4(8, 6, 2), gs::rgb4(2, 2, 2), gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_NIGHT, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 3), gs::rgb4(2, 2, 5), gs::rgb4(4, 4, 7),
                            gs::rgb4(8, 8, 10), gs::rgb4(12, 12, 13)});

    loadFont(vdp, art, tiles);

    int brick = makeTile(tiles, vdp, {"11221122", "11221122", "33333333", "22112211", "22112211", "33333333",
                                      "11221122", "11221122"});
    int mortar = makeTile(tiles, vdp, {"33333333", "22112211", "22112211", "33333333", "11221122", "11221122",
                                       "33333333", "22112211"});
    int sill = makeTile(tiles, vdp, {"44444444", "55555555", "33333333", "11221122", "11221122", "33333333",
                                     "22112211", "22112211"});
    int pane = makeTile(tiles, vdp, {"11221122", "16666122", "16666122", "33333333", "22116611", "22116611",
                                     "33333333", "11221122"});
    int cobble = makeTile(tiles, vdp, {"44554455", "45544554", "33333333", "54455445", "44554455", "33333333",
                                       "55445544", "44554455"});
    int wet = makeTile(tiles, vdp, {"55445544", "33333333", "44554455", "54455445", "33333333", "45544554",
                                    "44554455", "33333333"});

    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    for (int cy = 0; cy < 28; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int tile = ((cx / 3 + cy) & 1) ? brick : mortar;
            if (cy == 8 || cy == 16) tile = sill;
            if ((cy == 6 || cy == 7 || cy == 14 || cy == 15) && (cx % 7 == 2 || cx % 7 == 3)) tile = pane;
            vdp.A.set(cx, cy, gs::entry(tile, PAL_BRICK));
        }
    }
    for (int cy = 22; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int tile = ((cx + cy) & 1) ? cobble : wet;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_BRICK));
        }
    }

    art.stand = gs::uploadMipped(vdp, person(0, 2, 3, 5));
    art.walkA = gs::uploadMipped(vdp, person(1, 2, 3, 5));
    art.walkB = gs::uploadMipped(vdp, person(2, 2, 3, 5));
    art.shove = gs::uploadMipped(vdp, person(3, 2, 3, 5));
    art.watcher[0] = gs::uploadMipped(vdp, person(0, 2, 3, 5));
    art.watcher[1] = gs::uploadMipped(vdp, person(1, 2, 3, 5));
    art.banner = gs::uploadMipped(vdp, cloth());

    {
        gs::Bitmap b(8, 48);
        b.rect(3, 0, 2, 48, 6);
        b.rect(2, 0, 4, 3, 7);
        art.pole = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(28, 22);
        b.rect(2, 4, 24, 16, 2);
        b.rect(2, 4, 24, 3, 3);
        b.rect(6, 8, 6, 6, 4);
        art.crate = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(22, 26);
        b.ellipse(11, 12, 9, 11, 4);
        b.rect(4, 2, 14, 4, 5);
        b.ellipse(11, 16, 6, 5, 1);
        art.bin = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(16, 28);
        b.rect(7, 8, 2, 20, 1);
        b.rect(4, 2, 8, 8, 4);
        b.rect(6, 4, 4, 4, 2);
        art.lamp = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(14, 18);
        b.rect(2, 0, 10, 4, 5);
        b.rect(0, 4, 14, 3, 5);
        b.rect(3, 7, 8, 8, 4);
        art.fire = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(24, 8);
        b.ellipse(12, 4, 11, 3, 1);
        art.shadow = gs::uploadMipped(vdp, b);
    }

    for (int y = 0; y < gs::SCREEN_H; y++) {
        int k = y < 70 ? 1 : y < 150 ? 2 : 3;
        vdp.lineBackdrop[y] = gs::rgb4(k, k, k + 2);
    }
    vdp.setFogColor(gs::rgb4(1, 1, 2));
    vdp.A.enabled = true;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
}

}  // namespace alleybann
