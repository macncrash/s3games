#include "game/art.h"

#include <string>

namespace cisternpouc {
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
    gs::Bitmap b(24, 40);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose == 2 ? 1 : 0;
    R(8, 1 + bob, 8, 7, 4);
    R(9, 3 + bob, 2, 2, 5);
    R(6, 8 + bob, 12, 13, 2);
    R(6, 8 + bob, 3, 13, 3);
    R(9, 14 + bob, 6, 2, 6);
    R(4, 10 + bob, 3, 8, 3);
    R(17, 10 + bob, 3, 8, 3);
    if (pose == 1) {
        R(7, 21, 4, 13, 2);
        R(14, 23, 4, 11, 2);
        R(6, 33, 6, 4, 7);
        R(13, 33, 6, 4, 7);
    } else if (pose == 2) {
        R(6, 23, 4, 11, 2);
        R(14, 21, 4, 13, 2);
        R(5, 33, 6, 4, 7);
        R(13, 33, 6, 4, 7);
    } else {
        R(8, 21, 3, 13, 2);
        R(13, 21, 3, 13, 2);
        R(7, 33, 5, 4, 7);
        R(12, 33, 5, 4, 7);
    }
    return b;
}

gs::Bitmap pouchBmp() {
    gs::Bitmap b(18, 20);
    b.ellipse(9, 11, 7, 7, 2);
    b.ellipse(9, 12, 4, 4, 3);
    b.rect(6, 3, 6, 4, 4);
    b.rect(7, 1, 4, 3, 5);
    b.set(8, 8, 6);
    b.set(11, 9, 6);
    b.rect(4, 10, 2, 3, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 15, 13), gs::rgb4(2, 3, 4), gs::rgb4(15, 13, 5),
                          gs::rgb4(12, 4, 2), gs::rgb4(6, 7, 7), gs::rgb4(1, 2, 2)});
    setPal(vdp, PAL_STONE, {gs::rgb4(0, 0, 0), gs::rgb4(3, 4, 4), gs::rgb4(6, 6, 5), gs::rgb4(2, 3, 3),
                            gs::rgb4(8, 8, 6), gs::rgb4(1, 2, 2), gs::rgb4(5, 6, 4), gs::rgb4(9, 8, 5),
                            gs::rgb4(4, 5, 3)});
    setPal(vdp, PAL_WATER, {gs::rgb4(0, 0, 0), gs::rgb4(1, 3, 6), gs::rgb4(2, 5, 8), gs::rgb4(3, 7, 11),
                            gs::rgb4(5, 10, 12), gs::rgb4(9, 13, 14), gs::rgb4(1, 2, 4), gs::rgb4(4, 6, 8)});
    setPal(vdp, PAL_KEEPER, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(3, 5, 6), gs::rgb4(2, 3, 4),
                             gs::rgb4(11, 8, 6), gs::rgb4(2, 2, 2), gs::rgb4(9, 3, 2), gs::rgb4(3, 2, 1),
                             gs::rgb4(13, 12, 9)});
    setPal(vdp, PAL_POUCH, {gs::rgb4(0, 0, 0), gs::rgb4(2, 1, 0), gs::rgb4(10, 6, 2), gs::rgb4(7, 4, 1),
                            gs::rgb4(13, 9, 4), gs::rgb4(14, 12, 6), gs::rgb4(15, 14, 8), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_ROPE, {gs::rgb4(0, 0, 0), gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(8, 6, 3),
                           gs::rgb4(4, 4, 4), gs::rgb4(9, 9, 8), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_MOSS, {gs::rgb4(0, 0, 0), gs::rgb4(1, 3, 1), gs::rgb4(2, 5, 2), gs::rgb4(3, 6, 2),
                           gs::rgb4(5, 8, 3), gs::rgb4(2, 4, 2)});

    loadFont(vdp, art, tiles);

    int block = makeTile(tiles, vdp, {"11223322", "12233221", "22332211", "33333333", "22112233", "21122332",
                                      "11223322", "22333311"});
    int seam = makeTile(tiles, vdp, {"55555555", "11221122", "22112211", "33333333", "12211221", "21122112",
                                     "55555555", "11221122"});
    int moss = makeTile(tiles, vdp, {"11221122", "12211321", "22113222", "33333333", "11332211", "13222112",
                                     "32211221", "21232312"});
    int wet = makeTile(tiles, vdp, {"11223344", "12233445", "22334451", "23344511", "33445112", "34451122",
                                    "44511223", "45112233"});
    int deep = makeTile(tiles, vdp, {"11112222", "11122233", "11222331", "12223311", "22233111", "22331112",
                                     "23311122", "33111222"});

    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    for (int cy = 0; cy < 28; cy++) {
        for (int cx = 0; cx < 40; cx++) {
            bool wall = cx < 2 || cx >= 38;
            bool crown = cy < 2;
            if (!wall && !crown) continue;
            int tile = ((cx + cy) & 1) ? block : seam;
            if (cy == 8 || cy == 16) tile = moss;
            vdp.A.set(cx, cy, gs::entry(tile, PAL_STONE));
        }
    }
    for (int cy = 23; cy < 28; cy++) {
        for (int cx = 2; cx < 38; cx++) {
            int tile = (cy >= 26) ? deep : wet;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_WATER));
        }
    }

    art.stand = gs::uploadMipped(vdp, keeper(0));
    art.walkA = gs::uploadMipped(vdp, keeper(1));
    art.walkB = gs::uploadMipped(vdp, keeper(2));
    art.pouch = gs::uploadMipped(vdp, pouchBmp());

    {
        gs::Bitmap b(16, 16);
        b.rect(6, 0, 4, 3, 2);
        b.rect(3, 3, 10, 10, 4);
        b.rect(4, 4, 8, 8, 5);
        b.rect(5, 12, 6, 3, 3);
        art.bucket = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(5, 10);
        b.rect(2, 0, 1, 6, 4);
        b.rect(1, 6, 3, 3, 5);
        art.drip = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(40, 10);
        b.rect(0, 3, 40, 7, 2);
        b.rect(0, 1, 40, 3, 4);
        b.rect(2, 6, 8, 3, 6);
        b.rect(16, 6, 8, 3, 3);
        b.rect(28, 6, 8, 3, 7);
        art.lip = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(14, 16);
        b.ellipse(7, 8, 6, 6, 5);
        b.ellipse(7, 8, 4, 4, 1);
        b.rect(6, 1, 2, 3, 4);
        b.line(7, 8, 7, 5, 3, 1);
        b.line(7, 8, 10, 8, 3, 1);
        art.watch = gs::uploadMipped(vdp, b);
    }
}

}  // namespace cisternpouc
