#include "game/art.h"

#include <string>

namespace bunkerbann {
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

gs::Bitmap soldier(int pose, int coat, int trim, int helm) {
    gs::Bitmap b(30, 46);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose == 2 ? 1 : 0;
    R(9, 3 + bob, 12, 7, 4);
    R(8, 1 + bob, 14, 4, helm);
    R(17, 5 + bob, 2, 2, 9);
    R(7, 10 + bob, 14, 14, coat);
    R(7, 10 + bob, 3, 14, trim);
    R(11, 18 + bob, 6, 2, 5);
    if (pose == 1) {
        R(8, 24, 5, 14, coat);
        R(16, 26, 5, 12, coat);
        R(7, 37, 7, 4, 6);
        R(15, 37, 7, 4, 6);
        R(20, 12, 3, 11, 4);
    } else if (pose == 2) {
        R(7, 26, 5, 12, coat);
        R(17, 24, 5, 14, coat);
        R(6, 37, 7, 4, 6);
        R(16, 37, 7, 4, 6);
        R(20, 13, 3, 11, 4);
    } else if (pose == 3) {
        R(10, 24, 4, 14, coat);
        R(16, 24, 4, 14, coat);
        R(9, 37, 6, 4, 6);
        R(16, 37, 6, 4, 6);
        R(20, 14, 8, 3, coat);
        R(26, 12, 3, 4, 7);
    } else {
        R(10, 24, 4, 14, coat);
        R(16, 24, 4, 14, coat);
        R(9, 37, 6, 4, 6);
        R(16, 37, 6, 4, 6);
        R(20, 12, 3, 11, 4);
    }
    return b;
}

gs::Bitmap cloth() {
    gs::Bitmap b(28, 38);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(3, 2, 20, 4, 6);
    R(5, 6, 16, 24, 2);
    R(5, 6, 4, 24, 3);
    R(13, 10, 4, 16, 4);
    R(7, 30, 3, 6, 2);
    R(13, 30, 3, 5, 2);
    R(18, 30, 3, 6, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 15, 12), gs::rgb4(3, 4, 3), gs::rgb4(15, 10, 3),
                          gs::rgb4(12, 3, 2), gs::rgb4(7, 8, 6), gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_CONCRETE, {gs::rgb4(0, 0, 0), gs::rgb4(5, 5, 4), gs::rgb4(7, 7, 6), gs::rgb4(3, 3, 3),
                               gs::rgb4(8, 8, 6), gs::rgb4(2, 3, 2), gs::rgb4(4, 5, 3), gs::rgb4(9, 8, 5),
                               gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_COAT, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(3, 5, 3), gs::rgb4(5, 7, 4),
                           gs::rgb4(10, 8, 6), gs::rgb4(6, 2, 2), gs::rgb4(2, 2, 1), gs::rgb4(12, 11, 8),
                           gs::rgb4(14, 12, 6), gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_BANNER, {gs::rgb4(0, 0, 0), gs::rgb4(2, 1, 1), gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1),
                             gs::rgb4(14, 12, 3), gs::rgb4(5, 3, 1), gs::rgb4(10, 7, 3), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_SENTRY, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(4, 4, 3), gs::rgb4(6, 6, 4),
                             gs::rgb4(9, 7, 5), gs::rgb4(8, 6, 2), gs::rgb4(2, 2, 1), gs::rgb4(11, 10, 6),
                             gs::rgb4(13, 9, 3), gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_SAND, {gs::rgb4(0, 0, 0), gs::rgb4(4, 3, 1), gs::rgb4(8, 6, 3), gs::rgb4(10, 8, 4),
                           gs::rgb4(5, 5, 4), gs::rgb4(11, 10, 7), gs::rgb4(2, 2, 1), gs::rgb4(13, 9, 4)});
    setPal(vdp, PAL_LAMP, {gs::rgb4(0, 0, 0), gs::rgb4(4, 3, 1), gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 7),
                           gs::rgb4(8, 5, 1), gs::rgb4(2, 2, 1), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_DUSK, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(2, 2, 4), gs::rgb4(3, 3, 5),
                           gs::rgb4(6, 6, 8), gs::rgb4(10, 10, 11)});

    loadFont(vdp, art, tiles);

    int pour = makeTile(tiles, vdp, {"11221133", "12211331", "22113311", "33333333", "11332211", "13322112",
                                     "33221122", "11113333"});
    int seam = makeTile(tiles, vdp, {"33333333", "11221122", "11221122", "33333333", "22112211", "22112211",
                                     "33333333", "11221122"});
    int slit = makeTile(tiles, vdp, {"11221122", "18888122", "18888122", "33333333", "22118811", "22118811",
                                     "33333333", "11221122"});
    int earth = makeTile(tiles, vdp, {"55665566", "56655665", "22222222", "65566556", "55665566", "22222222",
                                      "66556655", "55665566"});
    int mud = makeTile(tiles, vdp, {"66556655", "22222222", "55665566", "65566556", "22222222", "56655665",
                                    "55665566", "22222222"});

    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    for (int cy = 0; cy < 28; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int tile = ((cx / 4 + cy) & 1) ? pour : seam;
            if (cy == 9 || cy == 17) tile = seam;
            if ((cy == 6 || cy == 7 || cy == 14 || cy == 15) && (cx % 9 == 4)) tile = slit;
            vdp.A.set(cx, cy, gs::entry(tile, PAL_CONCRETE));
        }
    }
    for (int cy = 22; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int tile = ((cx + cy) & 1) ? earth : mud;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_SAND));
        }
    }

    art.stand = gs::uploadMipped(vdp, soldier(0, 2, 3, 7));
    art.walkA = gs::uploadMipped(vdp, soldier(1, 2, 3, 7));
    art.walkB = gs::uploadMipped(vdp, soldier(2, 2, 3, 7));
    art.shove = gs::uploadMipped(vdp, soldier(3, 2, 3, 7));
    art.sentry[0] = gs::uploadMipped(vdp, soldier(0, 2, 5, 6));
    art.sentry[1] = gs::uploadMipped(vdp, soldier(1, 2, 5, 6));
    art.banner = gs::uploadMipped(vdp, cloth());

    {
        gs::Bitmap b(8, 52);
        b.rect(3, 0, 2, 52, 6);
        b.rect(2, 0, 4, 3, 7);
        art.staff = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(30, 18);
        b.rect(1, 3, 28, 12, 2);
        b.rect(1, 3, 28, 3, 3);
        b.rect(4, 7, 8, 5, 5);
        b.rect(16, 7, 8, 5, 1);
        art.bag = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(22, 16);
        b.rect(0, 4, 22, 8, 1);
        b.rect(0, 6, 22, 2, 4);
        b.rect(2, 2, 2, 12, 3);
        b.rect(18, 2, 2, 12, 3);
        art.slit = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(26, 20);
        b.line(0, 4, 26, 8, 4, 1);
        b.line(0, 10, 26, 6, 4, 1);
        b.line(0, 16, 26, 12, 5, 1);
        b.rect(2, 0, 2, 18, 6);
        b.rect(22, 0, 2, 18, 6);
        art.wire = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(14, 22);
        b.rect(6, 8, 2, 14, 1);
        b.rect(3, 1, 8, 8, 4);
        b.rect(5, 3, 4, 4, 2);
        art.lamp = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(24, 8);
        b.ellipse(12, 4, 11, 3, 1);
        art.shadow = gs::uploadMipped(vdp, b);
    }

    for (int y = 0; y < gs::SCREEN_H; y++) {
        int k = y < 60 ? 1 : y < 140 ? 2 : 3;
        vdp.lineBackdrop[y] = gs::rgb4(k, k + 1, k + 2);
    }
    vdp.setFogColor(gs::rgb4(1, 2, 2));
    vdp.A.enabled = true;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) vdp.road[y].on = false;
}

}  // namespace bunkerbann
