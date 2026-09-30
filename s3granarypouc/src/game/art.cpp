#include "game/art.h"

#include <initializer_list>
#include <string>

namespace granarypouc {
namespace {

void pal(gs::VDP& vdp, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(p * 16 + i++, c);
    }
    while (i < 15) vdp.setColor(p * 16 + i++, 0);
    if (i == 15) vdp.setColor(p * 16 + 15, gs::rgb4(1, 1, 1));
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

void paintLoft(gs::VDP& vdp, gs::TileAlloc& tiles) {
    int beam = makeTile(tiles, vdp, {"11111111", "12222111", "11111111", "00000000", "11111111", "11222211",
                                     "11111111", "00000000"});
    int plank = makeTile(tiles, vdp, {"34343434", "43434343", "22222222", "34343434", "43434343", "55555555",
                                      "34343434", "43434343"});
    int grain = makeTile(tiles, vdp, {"67676767", "76767676", "66676667", "76767676", "67676767", "76667676",
                                      "67676767", "76767676"});
    int mote = makeTile(tiles, vdp, {"00000000", "00080000", "00000000", "00000000", "08000000", "00000000",
                                     "00000080", "00000000"});
    vdp.A.clear();
    vdp.B.clear();
    for (int cx = 0; cx < 64; cx++) {
        vdp.A.set(cx, 2, gs::entry(beam, PAL_WOOD));
        vdp.A.set(cx, 6, gs::entry(beam, PAL_WOOD));
        if ((cx % 7) == 2) {
            for (int cy = 3; cy <= 5; cy++) vdp.A.set(cx, cy, gs::entry(beam, PAL_WOOD));
        }
        if ((cx * 3 + 1) % 9 == 0) vdp.A.set(cx, 10, gs::entry(mote, PAL_DUST));
        vdp.B.set(cx, 22, gs::entry(plank, PAL_WOOD));
        for (int cy = 23; cy < 28; cy++) vdp.B.set(cx, cy, gs::entry(grain, PAL_GRAIN));
    }
}

struct Ink {
    gs::Bitmap b;
    explicit Ink(int w, int h) : b(w, h) {}
    void r(int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); }
    void p(int x, int y, int c) { b.set(x, y, c); }
    void el(float cx, float cy, float rx, float ry, int c) { b.ellipse(cx, cy, rx, ry, c); }
    gs::Mipped up(gs::VDP& vdp) const { return gs::uploadMipped(vdp, b); }
};

gs::Mipped hand(gs::VDP& vdp, int pose) {
    Ink k(24, 40);
    k.el(12, 7, 5, 5, 4);
    k.r(9, 6, 6, 3, 3);
    k.p(10, 7, 1);
    k.p(14, 7, 1);
    k.r(8, 12, 8, 12, 6);
    k.r(9, 14, 6, 4, 2);
    k.r(6, 13, 3, 8, 5);
    k.r(15, 13, 3, 8, 5);
    if (pose == 0) {
        k.r(8, 24, 4, 12, 7);
        k.r(13, 24, 4, 12, 7);
        k.r(7, 35, 5, 3, 1);
        k.r(13, 35, 5, 3, 1);
    } else if (pose == 1) {
        k.r(7, 24, 4, 13, 7);
        k.r(14, 23, 4, 10, 7);
        k.r(6, 36, 5, 3, 1);
        k.r(15, 32, 5, 3, 1);
    } else if (pose == 2) {
        k.r(14, 24, 4, 13, 7);
        k.r(7, 23, 4, 10, 7);
        k.r(14, 36, 5, 3, 1);
        k.r(6, 32, 5, 3, 1);
    } else if (pose == 3) {
        k.r(5, 20, 14, 8, 6);
        k.r(4, 22, 4, 6, 5);
        k.r(16, 22, 4, 6, 5);
        k.r(6, 28, 5, 4, 7);
        k.r(13, 28, 5, 4, 7);
    } else {
        k.r(9, 24, 3, 10, 7);
        k.r(13, 22, 3, 8, 7);
        k.r(8, 33, 4, 3, 1);
        k.r(14, 29, 4, 3, 1);
        k.r(4, 14, 4, 3, 5);
    }
    return k.up(vdp);
}

gs::Mipped pouchPic(gs::VDP& vdp, int flap) {
    Ink k(18, 16);
    k.el(9, 10, 7, 5, 6);
    k.r(4, 8, 10, 4, 4);
    k.r(6, 4, 6, 5, 5);
    if (flap) k.r(7, 3, 4, 3, 2);
    k.p(6, 11, 1);
    k.p(12, 11, 1);
    k.r(8, 12, 2, 2, 3);
    return k.up(vdp);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    vdp.setFogColor(rgb4(10, 7, 3));
    pal(vdp, PAL_HUD, {0, rgb4(15, 14, 10), rgb4(8, 6, 3), rgb4(4, 3, 2)});
    pal(vdp, PAL_WOOD, {0, rgb4(6, 3, 1), rgb4(9, 5, 2), rgb4(12, 8, 3), rgb4(4, 2, 1), rgb4(8, 6, 4), rgb4(3, 2, 1)});
    pal(vdp, PAL_GRAIN, {0, rgb4(12, 9, 3), rgb4(14, 12, 5), rgb4(8, 6, 2), rgb4(10, 8, 3), rgb4(6, 4, 1), rgb4(13, 10, 4),
                         rgb4(15, 13, 6)});
    pal(vdp, PAL_POUCH, {0, rgb4(3, 2, 1), rgb4(10, 6, 2), rgb4(14, 10, 3), rgb4(8, 4, 1), rgb4(12, 7, 2), rgb4(6, 3, 1)});
    pal(vdp, PAL_HAND, {0, rgb4(2, 2, 2), rgb4(14, 11, 6), rgb4(12, 8, 5), rgb4(15, 13, 9), rgb4(4, 4, 5), rgb4(7, 8, 11),
                        rgb4(3, 3, 4)});
    pal(vdp, PAL_IRON, {0, rgb4(3, 3, 4), rgb4(8, 8, 9), rgb4(12, 12, 13), rgb4(5, 5, 6), rgb4(10, 9, 7), rgb4(2, 2, 3)});
    pal(vdp, PAL_DOOR, {0, rgb4(4, 2, 1), rgb4(9, 4, 2), rgb4(13, 6, 2), rgb4(6, 3, 1), rgb4(11, 8, 3), rgb4(2, 1, 1)});
    pal(vdp, PAL_ALERT, {0, rgb4(12, 2, 1), rgb4(15, 6, 2), rgb4(8, 1, 1), rgb4(15, 12, 4)});
    pal(vdp, PAL_LOFT, {0, rgb4(7, 5, 3), rgb4(11, 8, 5), rgb4(14, 12, 8), rgb4(5, 4, 3), rgb4(9, 7, 4), rgb4(3, 2, 2)});
    pal(vdp, PAL_DUST, {0, rgb4(14, 12, 7), rgb4(10, 8, 4), rgb4(15, 14, 9), rgb4(8, 6, 3)});

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);
    paintLoft(vdp, tiles);

    art.stand = hand(vdp, 0);
    art.runA = hand(vdp, 1);
    art.runB = hand(vdp, 2);
    art.duck = hand(vdp, 3);
    art.leap = hand(vdp, 4);
    art.pouch[0] = pouchPic(vdp, 0);
    art.pouch[1] = pouchPic(vdp, 1);

    {
        Ink k(22, 18);
        k.el(11, 11, 9, 6, 3);
        k.r(4, 6, 14, 6, 2);
        k.r(6, 4, 10, 4, 4);
        k.r(8, 8, 6, 3, 1);
        art.sack = k.up(vdp);
    }
    {
        Ink k(32, 72);
        k.r(6, 8, 20, 58, 5);
        k.r(4, 14, 24, 48, 4);
        k.el(16, 10, 12, 8, 3);
        k.r(10, 22, 12, 6, 2);
        k.r(10, 40, 12, 4, 1);
        k.r(14, 62, 4, 8, 6);
        art.silo = k.up(vdp);
    }
    {
        Ink k(16, 48);
        k.r(6, 0, 4, 48, 4);
        k.r(4, 4, 8, 4, 2);
        k.r(4, 20, 8, 4, 2);
        k.r(4, 36, 8, 4, 2);
        art.chute = k.up(vdp);
    }
    {
        Ink k(28, 20);
        k.r(2, 8, 24, 6, 3);
        k.r(0, 6, 6, 10, 5);
        k.r(22, 6, 6, 10, 5);
        k.r(10, 2, 8, 16, 2);
        art.blade = k.up(vdp);
    }
    {
        Ink k(36, 56);
        k.r(2, 2, 32, 52, 4);
        k.r(6, 6, 24, 44, 3);
        for (int y = 10; y < 48; y += 8) k.r(8, y, 20, 3, 2);
        k.r(16, 24, 4, 8, 1);
        art.slab = k.up(vdp);
    }
    {
        Ink k(56, 78);
        k.r(4, 18, 48, 58, 5);
        k.r(0, 16, 56, 8, 3);
        k.r(18, 0, 8, 18, 4);
        k.r(20, 28, 16, 18, 1);
        k.r(10, 56, 12, 18, 2);
        k.r(34, 56, 12, 18, 2);
        art.hatch = k.up(vdp);
    }
    {
        Ink k(20, 28);
        k.r(2, 4, 16, 22, 4);
        k.r(4, 8, 12, 14, 2);
        k.r(8, 0, 4, 6, 3);
        art.bin = k.up(vdp);
    }
    {
        Ink k(10, 36);
        k.r(4, 8, 2, 26, 4);
        k.el(5, 6, 4, 4, 3);
        k.r(2, 32, 6, 3, 1);
        art.lamp = k.up(vdp);
    }
    for (int i = 0; i < 2; i++) {
        Ink k(8, 10);
        k.el(4, 6, 3, 3 + i, 2);
        k.el(4, 5, 2, 2, 3);
        art.flame[i] = k.up(vdp);
    }
    {
        Ink k(8, 8);
        k.r(0, 0, 8, 8, 4);
        k.r(1, 2, 6, 4, 1);
        art.hole = k.up(vdp);
    }
    {
        Ink k(8, 8);
        k.el(2, 3, 1.2f, 1.f, 2);
        k.el(5, 5, 1.4f, 1.f, 3);
        k.p(3, 6, 1);
        art.grain = k.up(vdp);
    }
    {
        Ink k(10, 16);
        k.r(2, 0, 6, 14, 5);
        k.r(0, 10, 10, 4, 3);
        art.lip = k.up(vdp);
    }
    {
        Ink k(20, 8);
        k.el(10, 4, 8, 2, 1);
        art.shadow = k.up(vdp);
    }
}

}  // namespace granarypouc
