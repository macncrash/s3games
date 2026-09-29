#include "game/art.h"

#include <string>

namespace orchardwell {
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

gs::Bitmap keeperBmp(bool shove) {
    gs::Bitmap b(28, 44);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(8, 0, 12, 3, 6);
    R(7, 3, 14, 2, 7);
    R(10, 5, 8, 7, 4);
    R(16, 7, 2, 2, 1);
    R(9, 12, 10, 12, 2);
    R(9, 12, 3, 12, 3);
    R(11, 20, 6, 2, 5);
    R(10, 24, 3, 13, 2);
    R(15, 24, 3, 13, 2);
    R(9, 36, 5, 4, 6);
    R(14, 36, 5, 4, 6);
    if (shove) {
        R(18, 14, 9, 3, 5);
        R(25, 12, 2, 6, 7);
    } else {
        R(18, 14, 3, 10, 4);
        R(19, 22, 2, 6, 5);
    }
    return b;
}

gs::Bitmap wellBmp() {
    gs::Bitmap b(48, 56);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(6, 8, 4, 36, 3);
    R(38, 8, 4, 36, 3);
    R(4, 6, 40, 5, 4);
    R(18, 2, 12, 6, 5);
    R(22, 0, 4, 4, 6);
    b.ellipse(24, 38, 18, 8, 2);
    b.ellipse(24, 38, 12, 5, 1);
    b.ellipse(24, 36, 6, 2, 7);
    R(10, 42, 28, 10, 2);
    R(8, 50, 32, 4, 3);
    R(22, 14, 2, 18, 6);
    b.ellipse(20, 32, 3, 3, 5);
    return b;
}

gs::Bitmap wellCrackBmp() {
    gs::Bitmap b(48, 56);
    b.line(16, 44, 22, 50, 2, 1);
    b.line(22, 50, 20, 54, 2, 1);
    b.line(28, 42, 32, 50, 2, 1);
    return b;
}

gs::Bitmap wellFellBmp() {
    gs::Bitmap b(56, 28);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(2, 14, 40, 8, 2);
    R(4, 20, 36, 5, 3);
    R(30, 6, 6, 12, 4);
    R(36, 4, 14, 4, 5);
    b.ellipse(18, 16, 8, 4, 1);
    return b;
}

gs::Bitmap treeBmp() {
    gs::Bitmap b(52, 70);
    b.rect(22, 36, 8, 30, 5);
    b.rect(20, 62, 12, 5, 6);
    b.ellipse(26, 26, 22, 20, 2);
    b.ellipse(16, 22, 10, 8, 3);
    b.ellipse(34, 28, 9, 7, 4);
    b.ellipse(22, 14, 4, 3, 1);
    b.ellipse(12, 30, 3, 3, 7);
    b.ellipse(36, 16, 3, 3, 7);
    b.ellipse(40, 30, 3, 3, 7);
    return b;
}

gs::Bitmap boarBmp() {
    gs::Bitmap b(40, 24);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    b.ellipse(20, 13, 14, 8, 2);
    b.ellipse(16, 12, 8, 5, 3);
    R(28, 8, 10, 8, 2);
    R(36, 10, 3, 3, 4);
    R(30, 9, 2, 2, 1);
    R(8, 16, 3, 6, 5);
    R(16, 16, 3, 6, 5);
    R(24, 16, 3, 6, 5);
    R(4, 10, 4, 2, 6);
    return b;
}

gs::Bitmap crateBmp() {
    gs::Bitmap b(28, 26);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(2, 6, 24, 18, 2);
    R(2, 6, 24, 3, 3);
    R(2, 14, 24, 2, 4);
    R(12, 6, 2, 18, 4);
    b.ellipse(8, 4, 3, 3, 5);
    b.ellipse(16, 3, 3, 3, 6);
    b.ellipse(22, 5, 3, 3, 5);
    return b;
}

gs::Bitmap ramBmp() {
    gs::Bitmap b(44, 30);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    b.ellipse(22, 16, 14, 9, 2);
    b.ellipse(18, 14, 8, 5, 3);
    R(30, 8, 10, 10, 2);
    R(38, 10, 4, 4, 4);
    R(32, 10, 2, 2, 1);
    b.ellipse(34, 6, 5, 3, 5);
    b.ellipse(28, 5, 4, 3, 5);
    R(8, 18, 3, 8, 6);
    R(16, 18, 3, 8, 6);
    R(24, 18, 3, 8, 6);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(15, 15, 12), gs::rgb4(2, 3, 1), gs::rgb4(14, 11, 4),
                          gs::rgb4(12, 3, 2), gs::rgb4(5, 7, 3)});
    setPal(vdp, PAL_LEAF, {gs::rgb4(0, 0, 0), gs::rgb4(8, 14, 5), gs::rgb4(3, 8, 2), gs::rgb4(6, 12, 3),
                           gs::rgb4(2, 5, 1), gs::rgb4(8, 6, 2), gs::rgb4(4, 3, 1), gs::rgb4(14, 4, 2)});
    setPal(vdp, PAL_KEEPER, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(3, 7, 3), gs::rgb4(5, 10, 4),
                             gs::rgb4(13, 9, 6), gs::rgb4(9, 5, 2), gs::rgb4(6, 4, 2), gs::rgb4(14, 12, 5)});
    setPal(vdp, PAL_STONE, {gs::rgb4(0, 0, 0), gs::rgb4(2, 3, 4), gs::rgb4(7, 8, 8), gs::rgb4(5, 5, 6),
                            gs::rgb4(9, 8, 6), gs::rgb4(6, 4, 2), gs::rgb4(12, 11, 8), gs::rgb4(4, 8, 10)});
    setPal(vdp, PAL_BOAR, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(6, 3, 2), gs::rgb4(8, 5, 3),
                           gs::rgb4(12, 8, 6), gs::rgb4(4, 2, 1), gs::rgb4(10, 9, 7), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_WOOD, {gs::rgb4(0, 0, 0), gs::rgb4(2, 1, 1), gs::rgb4(8, 5, 2), gs::rgb4(10, 7, 3),
                           gs::rgb4(6, 4, 2), gs::rgb4(14, 2, 2), gs::rgb4(12, 4, 2), gs::rgb4(4, 8, 2)});
    setPal(vdp, PAL_APPLE, {gs::rgb4(0, 0, 0), gs::rgb4(8, 1, 1), gs::rgb4(14, 2, 2), gs::rgb4(12, 5, 2),
                            gs::rgb4(15, 14, 8), gs::rgb4(3, 8, 2), gs::rgb4(2, 4, 1)});
    setPal(vdp, PAL_FX, {gs::rgb4(0, 0, 0), gs::rgb4(15, 14, 8), gs::rgb4(12, 4, 2), gs::rgb4(8, 8, 7),
                         gs::rgb4(14, 12, 6), gs::rgb4(6, 5, 4)});

    loadFont(vdp, art, tiles);

    int sky = makeTile(tiles, vdp, {"11111111", "11111111", "11112211", "11111111", "11211111", "11111111",
                                    "11111121", "11111111"});
    int grass = makeTile(tiles, vdp, {"22232223", "32323232", "44424442", "23232323", "22232223", "34343434",
                                      "44434443", "23232323"});
    int soil = makeTile(tiles, vdp, {"55665566", "66556655", "55565556", "65556555", "56655665", "55565556",
                                     "66556655", "55665566"});

    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    for (int cy = 0; cy < 32; cy++)
        for (int cx = 0; cx < 64; cx++) vdp.B.set(cx, cy, gs::entry(cy < 10 ? sky : cy < 22 ? grass : soil, PAL_LEAF));
    (void)sky;

    art.keeper = gs::uploadMipped(vdp, keeperBmp(false));
    art.shove = gs::uploadMipped(vdp, keeperBmp(true));
    art.well = gs::uploadMipped(vdp, wellBmp());
    art.wellCrack = gs::uploadMipped(vdp, wellCrackBmp());
    art.wellFell = gs::uploadMipped(vdp, wellFellBmp());
    art.tree = gs::uploadMipped(vdp, treeBmp());
    {
        gs::Bitmap b(12, 12);
        b.ellipse(6, 6, 5, 5, 2);
        b.ellipse(4, 4, 2, 2, 4);
        b.rect(5, 1, 2, 3, 5);
        art.apple = gs::uploadMipped(vdp, b);
    }
    art.boar = gs::uploadMipped(vdp, boarBmp());
    art.crate = gs::uploadMipped(vdp, crateBmp());
    art.ram = gs::uploadMipped(vdp, ramBmp());
    {
        gs::Bitmap b(16, 10);
        b.ellipse(8, 5, 6, 3, 1);
        b.ellipse(4, 6, 2, 2, 3);
        art.dust = gs::uploadMipped(vdp, b);
    }
}

}  // namespace orchardwell
