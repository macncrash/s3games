#include "game/art.h"

#include <initializer_list>
#include <string>

namespace lot {
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
            if (c >= '1' && c <= '9') v = c - '0';
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
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

void paintLot(gs::VDP& vdp, gs::TileAlloc& tiles) {
    int sky = makeTile(tiles, vdp, {"11111111", "11111111", "11111111", "11111111", "11111111", "11111111",
                                    "11111111", "11111111"});
    int asphalt = makeTile(tiles, vdp, {"22222222", "22322232", "22222222", "23222223", "22232222", "22222222",
                                        "32222232", "22222222"});
    int crack = makeTile(tiles, vdp, {"22222222", "22232222", "22333222", "22232222", "22223222", "22222232",
                                      "22222222", "22222222"});
    int stall = makeTile(tiles, vdp, {"44444444", "22222222", "22222222", "22222222", "22222222", "22222222",
                                      "22222222", "22222222"});
    int curb = makeTile(tiles, vdp, {"55555555", "66666666", "22222222", "22222222", "22222222", "22222222",
                                     "22222222", "22222222"});
    vdp.A.clear();
    vdp.B.clear();
    for (int cy = 0; cy < 18; cy++)
        for (int cx = 0; cx < 64; cx++) vdp.B.set(cx, cy, gs::entry(sky, PAL_SKY));
    for (int cx = 0; cx < 64; cx++) {
        vdp.B.set(cx, 18, gs::entry(curb, PAL_LOT));
        for (int cy = 19; cy < 32; cy++) {
            int tile = asphalt;
            if (cy == 20 && (cx % 5) == 0) tile = stall;
            else if ((cx * 3 + cy) % 11 == 0) tile = crack;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_LOT));
        }
    }
    (void)sky;
}

struct Ink {
    gs::Bitmap b;
    explicit Ink(int w, int h) : b(w, h) {}
    void r(int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); }
    gs::Bitmap take() {
        b.outline(15, false);
        return b;
    }
};

gs::Bitmap runner(int pose) {
    Ink d(28, 46);
    int bob = (pose == 1) ? 1 : (pose == 2) ? -1 : 0;
    d.r(10, 2 + bob, 8, 7, 4);
    d.r(11, 4 + bob, 6, 2, 5);
    d.r(9, 9 + bob, 10, 12, 2);
    d.r(8, 11 + bob, 3, 8, 3);
    d.r(18, 12 + bob, 3, 8, 3);
    if (pose == 3) {
        d.r(6, 8, 4, 8, 3);
        d.r(19, 6, 4, 8, 3);
        d.r(10, 21, 3, 10, 1);
        d.r(15, 21, 3, 10, 1);
        d.r(9, 31, 5, 3, 6);
        d.r(14, 31, 5, 3, 6);
    } else if (pose == 1) {
        d.r(10, 21, 3, 12, 1);
        d.r(16, 23, 3, 8, 1);
        d.r(9, 33, 5, 3, 6);
        d.r(15, 31, 5, 3, 6);
    } else if (pose == 2) {
        d.r(11, 23, 3, 8, 1);
        d.r(15, 21, 3, 12, 1);
        d.r(10, 31, 5, 3, 6);
        d.r(14, 33, 5, 3, 6);
    } else {
        d.r(10, 21, 3, 12, 1);
        d.r(15, 21, 3, 12, 1);
        d.r(9, 33, 5, 3, 6);
        d.r(14, 33, 5, 3, 6);
    }
    d.r(17, 14 + bob, 6, 5, 8);
    return d.take();
}

gs::Bitmap pouchBmp() {
    Ink d(18, 16);
    d.r(3, 4, 12, 10, 2);
    d.r(4, 5, 10, 3, 3);
    d.r(6, 1, 6, 4, 4);
    d.r(7, 8, 4, 3, 5);
    return d.take();
}

gs::Bitmap carBmp(int kind) {
    Ink d(72, 28);
    int body = kind ? 3 : 2;
    d.r(6, 10, 60, 12, body);
    d.r(16, 4, 28, 8, body);
    d.r(18, 5, 10, 5, 5);
    d.r(30, 5, 12, 5, 5);
    d.r(4, 14, 6, 4, 6);
    d.r(62, 14, 6, 4, kind ? 4 : 7);
    d.r(12, 20, 10, 7, 1);
    d.r(48, 20, 10, 7, 1);
    d.r(14, 22, 6, 3, 8);
    d.r(50, 22, 6, 3, 8);
    return d.take();
}

gs::Bitmap boothBmp() {
    Ink d(48, 64);
    d.r(6, 16, 36, 44, 2);
    d.r(10, 20, 28, 16, 5);
    d.r(4, 8, 40, 10, 3);
    d.r(20, 2, 8, 8, 4);
    d.r(18, 44, 12, 16, 1);
    d.r(8, 52, 8, 8, 6);
    return d.take();
}

gs::Bitmap lampBmp() {
    Ink d(16, 56);
    d.r(6, 10, 4, 44, 2);
    d.r(2, 4, 12, 8, 3);
    d.r(4, 6, 8, 4, 4);
    return d.take();
}

gs::Bitmap coneBmp() {
    Ink d(14, 18);
    d.r(6, 1, 2, 4, 1);
    d.r(4, 5, 6, 4, 2);
    d.r(2, 9, 10, 4, 3);
    d.r(1, 13, 12, 4, 2);
    return d.take();
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    setPal(vdp, PAL_HUD, {rgb4(0, 0, 0), rgb4(15, 15, 12), rgb4(8, 8, 6), rgb4(15, 12, 2)});
    setPal(vdp, PAL_LOT, {rgb4(0, 0, 0), rgb4(2, 2, 3), rgb4(4, 4, 5), rgb4(6, 6, 7), rgb4(13, 11, 2),
                          rgb4(9, 9, 8), rgb4(7, 7, 6), rgb4(3, 3, 4)});
    setPal(vdp, PAL_SKY, {rgb4(0, 0, 0), rgb4(2, 2, 6), rgb4(4, 3, 8), rgb4(8, 6, 10), rgb4(12, 8, 6)});
    setPal(vdp, PAL_PLAYER, {rgb4(0, 0, 0), rgb4(1, 2, 6), rgb4(12, 5, 1), rgb4(14, 8, 3), rgb4(12, 8, 5),
                             rgb4(2, 2, 2), rgb4(3, 3, 3), rgb4(15, 15, 15), rgb4(10, 7, 3), rgb4(0, 0, 0)});
    setPal(vdp, PAL_CAR, {rgb4(0, 0, 0), rgb4(1, 1, 1), rgb4(12, 2, 2), rgb4(2, 4, 11), rgb4(14, 12, 2),
                          rgb4(8, 12, 14), rgb4(6, 6, 6), rgb4(15, 4, 1), rgb4(10, 10, 11), rgb4(0, 0, 0)});
    setPal(vdp, PAL_POUCH, {rgb4(0, 0, 0), rgb4(4, 2, 1), rgb4(10, 6, 2), rgb4(13, 9, 4), rgb4(8, 5, 2),
                            rgb4(15, 12, 4), rgb4(6, 3, 1)});
    setPal(vdp, PAL_BOOTH, {rgb4(0, 0, 0), rgb4(3, 3, 4), rgb4(5, 7, 8), rgb4(2, 8, 4), rgb4(14, 12, 3),
                            rgb4(10, 14, 15), rgb4(12, 4, 2)});
    setPal(vdp, PAL_FX, {rgb4(0, 0, 0), rgb4(15, 15, 8), rgb4(15, 8, 1), rgb4(15, 15, 15)});

    vdp.setFogColor(rgb4(2, 2, 5));
    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    paintLot(vdp, tiles);

    art.stand = gs::uploadMipped(vdp, runner(0));
    art.runA = gs::uploadMipped(vdp, runner(1));
    art.runB = gs::uploadMipped(vdp, runner(2));
    art.leap = gs::uploadMipped(vdp, runner(3));
    art.pouch = gs::uploadMipped(vdp, pouchBmp());
    art.car = gs::uploadMipped(vdp, carBmp(0));
    art.carB = gs::uploadMipped(vdp, carBmp(1));
    art.booth = gs::uploadMipped(vdp, boothBmp());
    art.lamp = gs::uploadMipped(vdp, lampBmp());
    art.cone = gs::uploadMipped(vdp, coneBmp());

    gs::TextStyle big{3, 1, 15, 2, 1};
    gs::TextStyle small{1, 3, 0, 1, 1};
    art.title = gs::uploadMipped(vdp, gs::textBitmap("LOT POUC", big));
    art.sub = gs::uploadMipped(vdp, gs::textBitmap("CARRY IT ACROSS", small));
}

}  // namespace lot
