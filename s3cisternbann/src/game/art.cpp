#include "game/art.h"

#include <string>

namespace cisternbann {
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

gs::Bitmap keeper(int pose) {
    gs::Bitmap b(26, 42);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose == 2 ? 1 : 0;
    R(8, 2 + bob, 10, 8, 4);
    R(9, 4 + bob, 3, 2, 5);
    R(6, 10 + bob, 14, 14, 2);
    R(6, 10 + bob, 3, 14, 3);
    R(10, 16 + bob, 6, 2, 6);
    if (pose == 1) {
        R(7, 24, 5, 13, 2);
        R(15, 26, 5, 11, 2);
        R(6, 36, 7, 4, 7);
        R(14, 36, 7, 4, 7);
    } else if (pose == 2) {
        R(6, 26, 5, 11, 2);
        R(15, 24, 5, 13, 2);
        R(5, 36, 7, 4, 7);
        R(14, 36, 7, 4, 7);
    } else {
        R(8, 24, 4, 13, 2);
        R(14, 24, 4, 13, 2);
        R(7, 36, 6, 4, 7);
        R(14, 36, 6, 4, 7);
    }
    R(18, 12, 3, 10, 3);
    return b;
}

gs::Bitmap cloth() {
    gs::Bitmap b(22, 30);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(2, 1, 16, 3, 5);
    R(4, 4, 12, 20, 2);
    R(4, 4, 3, 20, 3);
    R(8, 8, 4, 10, 4);
    R(6, 24, 2, 5, 2);
    R(10, 24, 2, 4, 3);
    R(14, 24, 2, 5, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(13, 15, 14), gs::rgb4(2, 4, 5), gs::rgb4(15, 12, 4),
                          gs::rgb4(12, 3, 2), gs::rgb4(6, 8, 8), gs::rgb4(1, 2, 3)});
    setPal(vdp, PAL_STONE, {gs::rgb4(0, 0, 0), gs::rgb4(4, 5, 5), gs::rgb4(6, 7, 6), gs::rgb4(3, 3, 3),
                            gs::rgb4(8, 8, 7), gs::rgb4(2, 3, 3), gs::rgb4(5, 6, 4), gs::rgb4(9, 8, 6),
                            gs::rgb4(1, 2, 2)});
    setPal(vdp, PAL_WATER, {gs::rgb4(0, 0, 0), gs::rgb4(1, 3, 5), gs::rgb4(2, 5, 8), gs::rgb4(3, 7, 10),
                            gs::rgb4(5, 9, 12), gs::rgb4(8, 12, 13), gs::rgb4(1, 2, 4), gs::rgb4(4, 6, 7)});
    setPal(vdp, PAL_KEEPER, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(2, 4, 5), gs::rgb4(3, 6, 6),
                             gs::rgb4(10, 8, 6), gs::rgb4(2, 2, 2), gs::rgb4(8, 3, 2), gs::rgb4(2, 2, 1),
                             gs::rgb4(12, 11, 8)});
    setPal(vdp, PAL_BANNER, {gs::rgb4(0, 0, 0), gs::rgb4(2, 1, 1), gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1),
                             gs::rgb4(14, 12, 3), gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 3)});
    setPal(vdp, PAL_EEL, {gs::rgb4(0, 0, 0), gs::rgb4(1, 2, 1), gs::rgb4(2, 5, 3), gs::rgb4(4, 8, 4),
                          gs::rgb4(8, 12, 6), gs::rgb4(14, 14, 8), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_MOSS, {gs::rgb4(0, 0, 0), gs::rgb4(1, 3, 1), gs::rgb4(2, 5, 2), gs::rgb4(4, 7, 3),
                           gs::rgb4(6, 8, 4), gs::rgb4(3, 4, 2)});

    loadFont(vdp, art, tiles);

    int block = makeTile(tiles, vdp, {"11221133", "12211332", "22113322", "33333333", "11332211", "13322113",
                                      "33221122", "22223333"});
    int seam = makeTile(tiles, vdp, {"33333333", "11221122", "11221122", "55555555", "22112211", "22112211",
                                     "33333333", "11221122"});
    int moss = makeTile(tiles, vdp, {"11221122", "12211331", "22113322", "33333333", "11332211", "13322112",
                                     "33221122", "12123232"});
    int wet = makeTile(tiles, vdp, {"11223344", "12233445", "22334455", "23344551", "33445511", "34455112",
                                    "44551122", "45511223"});
    int deep = makeTile(tiles, vdp, {"11112222", "11222233", "12223331", "22233311", "22333111", "23331112",
                                     "33311122", "33111222"});

    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    for (int cy = 0; cy < 28; cy++) {
        for (int cx = 0; cx < 40; cx++) {
            bool wall = cx < 2 || cx >= 38;
            bool lip = cy < 2 && (cx < 8 || cx >= 32);
            if (!wall && !lip) continue;
            int tile = ((cx + cy) & 1) ? block : seam;
            if (cy == 6 || cy == 14 || cy == 20) tile = moss;
            vdp.A.set(cx, cy, gs::entry(tile, PAL_STONE));
        }
    }
    for (int cy = 22; cy < 28; cy++) {
        for (int cx = 2; cx < 38; cx++) {
            int tile = (cy >= 25) ? deep : wet;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_WATER));
        }
    }

    art.stand = gs::uploadMipped(vdp, keeper(0));
    art.walkA = gs::uploadMipped(vdp, keeper(1));
    art.walkB = gs::uploadMipped(vdp, keeper(2));
    art.banner = gs::uploadMipped(vdp, cloth());

    {
        gs::Bitmap b(36, 12);
        b.ellipse(18, 6, 16, 5, 2);
        b.ellipse(10, 6, 4, 3, 3);
        b.ellipse(24, 5, 3, 2, 4);
        b.set(30, 5, 5);
        art.eel = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(6, 10);
        b.rect(2, 0, 2, 6, 4);
        b.rect(1, 6, 4, 3, 5);
        art.drip = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(48, 10);
        b.rect(0, 4, 48, 6, 2);
        b.rect(0, 2, 48, 3, 4);
        b.rect(4, 6, 8, 3, 6);
        b.rect(20, 6, 10, 3, 3);
        b.rect(36, 6, 8, 3, 1);
        art.lip = gs::uploadMipped(vdp, b);
    }
}

}  // namespace cisternbann
