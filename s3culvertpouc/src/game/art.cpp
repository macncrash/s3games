#include "game/art.h"

#include <initializer_list>
#include <string>

namespace culvertpouc {
namespace {

void pal(gs::VDP& vdp, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(p * 16 + i++, c);
    }
    while (i < 15) vdp.setColor(p * 16 + i++, 0);
    if (i == 15) vdp.setColor(p * 16 + 15, gs::rgb4(1, 1, 2));
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

void paintBarrel(gs::VDP& vdp, gs::TileAlloc& tiles) {
    int sky = makeTile(tiles, vdp, {"00000000", "00010000", "00000000", "00000010", "00000000", "00100000",
                                    "00000000", "00000000"});
    int asphalt = makeTile(tiles, vdp, {"22222222", "22322232", "22242222", "22222222", "32222223", "22222222",
                                        "22232222", "11111111"});
    int course = makeTile(tiles, vdp, {"33333333", "31111311", "31111311", "33333333", "13111313", "13111313",
                                       "33333333", "11311311"});
    int damp = makeTile(tiles, vdp, {"44444444", "42444424", "44424444", "44444444", "24444442", "44442444",
                                     "44444424", "22222222"});
    vdp.A.clear();
    vdp.B.clear();
    for (int cx = 0; cx < 64; cx++) {
        for (int cy = 0; cy < 6; cy++) vdp.B.set(cx, cy, gs::entry(sky, PAL_ROAD));
        vdp.A.set(cx, 6, gs::entry(asphalt, PAL_ROAD));
        vdp.A.set(cx, 7, gs::entry(asphalt, PAL_ROAD));
        for (int cy = 10; cy < 20; cy++) vdp.B.set(cx, cy, gs::entry(course, PAL_STONE, (cx + cy) & 1, 0));
        for (int cy = 22; cy < 28; cy++) vdp.B.set(cx, cy, gs::entry(damp, PAL_STONE));
    }
}

struct Ink {
    gs::Bitmap b;
    explicit Ink(int w, int h) : b(w, h) {}
    void r(int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); }
    void p(int x, int y, int c) { b.set(x, y, c); }
    void el(float cx, float cy, float rx, float ry, int c) { b.ellipse(cx, cy, rx, ry, c); }
    void ln(float x0, float y0, float x1, float y1, int c, float t = 1.f) { b.line(x0, y0, x1, y1, c, t); }
};

gs::Mipped up(gs::VDP& vdp, const Ink& ink) { return gs::uploadMipped(vdp, ink.b); }

void body(Ink& k, int coat, int boot, bool crouch, bool leap) {
    k.el(12, crouch ? 8 : 6, 4.2f, 4.2f, 3);
    k.r(10, crouch ? 6 : 4, 3, 2, 2);
    k.r(8, crouch ? 11 : 10, 8, crouch ? 10 : 16, coat);
    k.r(7, crouch ? 12 : 12, 3, 8, 4);
    if (!crouch) {
        k.r(9, 26, 3, leap ? 8 : 12, boot);
        k.r(14, 26, 3, leap ? 6 : 12, boot);
        if (leap) k.r(15, 22, 7, 3, boot);
    } else {
        k.r(6, 20, 12, 4, boot);
    }
    k.p(14, crouch ? 7 : 5, 1);
    k.r(16, crouch ? 10 : 14, 4, 3, 5);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    vdp.setFogColor(gs::rgb4(2, 3, 4));
    pal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 14, 12), gs::rgb4(8, 8, 6), gs::rgb4(4, 4, 5)});
    pal(vdp, PAL_STONE, {gs::rgb4(0, 0, 0), gs::rgb4(10, 10, 11), gs::rgb4(6, 6, 7), gs::rgb4(4, 4, 5),
                         gs::rgb4(3, 5, 5), gs::rgb4(8, 8, 6), gs::rgb4(12, 12, 10)});
    pal(vdp, PAL_FLOW, {gs::rgb4(0, 0, 0), gs::rgb4(6, 12, 11), gs::rgb4(2, 6, 7), gs::rgb4(3, 9, 8),
                        gs::rgb4(8, 14, 12), gs::rgb4(1, 3, 4), gs::rgb4(10, 13, 8)});
    pal(vdp, PAL_POUCH, {gs::rgb4(0, 0, 0), gs::rgb4(12, 8, 3), gs::rgb4(7, 4, 1), gs::rgb4(14, 11, 5),
                         gs::rgb4(4, 2, 1), gs::rgb4(15, 13, 8)});
    pal(vdp, PAL_PLAYER, {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 10), gs::rgb4(9, 6, 4), gs::rgb4(12, 9, 6),
                          gs::rgb4(3, 4, 8), gs::rgb4(15, 13, 4), gs::rgb4(2, 2, 4)});
    pal(vdp, PAL_GATE, {gs::rgb4(0, 0, 0), gs::rgb4(12, 8, 5), gs::rgb4(6, 4, 3), gs::rgb4(9, 3, 2),
                        gs::rgb4(14, 12, 8), gs::rgb4(3, 3, 3)});
    pal(vdp, PAL_ROAD, {gs::rgb4(1, 2, 4), gs::rgb4(13, 12, 10), gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 6),
                        gs::rgb4(10, 8, 4), gs::rgb4(14, 14, 12), gs::rgb4(3, 5, 3)});
    pal(vdp, PAL_ALERT, {gs::rgb4(0, 0, 0), gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2), gs::rgb4(15, 12, 6)});
    pal(vdp, PAL_LAMP, {gs::rgb4(0, 0, 0), gs::rgb4(15, 14, 6), gs::rgb4(12, 8, 2), gs::rgb4(15, 15, 12),
                        gs::rgb4(6, 6, 7)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    paintBarrel(vdp, tiles);

    {
        Ink k(24, 40);
        body(k, 4, 6, false, false);
        k.r(17, 16, 5, 4, 5);
        art.stand = up(vdp, k);
    }
    {
        Ink k(24, 40);
        body(k, 4, 6, false, false);
        k.r(8, 28, 3, 10, 6);
        k.r(15, 24, 3, 12, 6);
        art.runA = up(vdp, k);
    }
    {
        Ink k(24, 40);
        body(k, 4, 6, false, false);
        k.r(9, 24, 3, 12, 6);
        k.r(15, 28, 3, 10, 6);
        art.runB = up(vdp, k);
    }
    {
        Ink k(24, 28);
        body(k, 4, 6, true, false);
        art.duck = up(vdp, k);
    }
    {
        Ink k(28, 36);
        body(k, 4, 6, false, true);
        art.leap = up(vdp, k);
    }
    for (int i = 0; i < 2; i++) {
        Ink k(18, 16);
        k.r(2, 4, 14, 10, 1);
        k.r(3, 5, 12, 8, 2);
        k.r(4, 2 + i, 10, 4, 3);
        k.ln(4, 6, 14, 6, 4, 1);
        k.r(7, 7, 4, 3, 5);
        art.pouch[i] = up(vdp, k);
    }
    {
        Ink k(48, 72);
        k.r(0, 8, 8, 56, 1);
        k.r(40, 8, 8, 56, 1);
        k.r(4, 0, 40, 12, 2);
        k.el(24, 28, 16, 18, 0);
        k.r(6, 8, 4, 48, 3);
        k.r(38, 8, 4, 48, 6);
        for (int y = 16; y < 64; y += 12) k.r(8, y, 32, 2, 5);
        art.ring = up(vdp, k);
    }
    {
        Ink k(56, 14);
        k.r(0, 2, 56, 10, 1);
        k.r(0, 4, 56, 3, 2);
        for (int x = 4; x < 52; x += 8) k.r(x, 2, 2, 10, 5);
        art.soffit = up(vdp, k);
    }
    {
        Ink k(20, 64);
        k.r(2, 0, 16, 64, 2);
        for (int y = 2; y < 62; y += 6) {
            k.r(3, y, 14, 3, 1);
            k.r(4, y, 12, 1, 4);
        }
        k.r(0, 0, 20, 4, 5);
        art.gate = up(vdp, k);
    }
    {
        Ink k(16, 20);
        k.r(4, 0, 8, 16, 1);
        k.r(2, 14, 12, 6, 3);
        art.lip = up(vdp, k);
    }
    {
        Ink k(32, 18);
        k.r(0, 2, 32, 14, 2);
        k.r(0, 4, 32, 4, 1);
        k.ln(2, 8, 14, 6, 4, 1);
        k.ln(16, 10, 30, 7, 3, 1);
        k.r(0, 12, 32, 6, 5);
        art.water = up(vdp, k);
    }
    {
        Ink k(14, 22);
        k.ln(7, 20, 4, 2, 6, 1.4f);
        k.ln(7, 20, 10, 4, 1, 1.2f);
        k.ln(7, 14, 2, 8, 6, 1);
        art.weed = up(vdp, k);
    }
    {
        Ink k(80, 16);
        k.r(0, 4, 80, 8, 2);
        k.r(0, 6, 80, 2, 3);
        for (int x = 0; x < 80; x += 16) k.r(x, 2, 8, 2, 4);
        k.r(0, 12, 80, 3, 1);
        art.deck = up(vdp, k);
    }
    {
        Ink k(36, 16);
        k.r(2, 6, 28, 7, 2);
        k.r(22, 3, 10, 6, 3);
        k.r(6, 12, 5, 4, 5);
        k.r(22, 12, 5, 4, 5);
        k.r(24, 5, 4, 2, 1);
        art.truck = up(vdp, k);
    }
    {
        Ink k(10, 36);
        k.r(4, 6, 2, 28, 4);
        k.r(2, 0, 6, 8, 1);
        k.el(5, 3, 2.2f, 2.2f, 3);
        art.lamp = up(vdp, k);
    }
    for (int i = 0; i < 2; i++) {
        Ink k(8, 8);
        k.el(4, 4, 3.f - i, 3.f, 1);
        k.p(4, 3, 3);
        art.flame[i] = up(vdp, k);
    }
    {
        Ink k(12, 28);
        k.r(5, 4, 2, 22, 1);
        k.r(2, 2, 8, 6, 3);
        art.mark = up(vdp, k);
    }
    {
        Ink k(20, 6);
        k.el(10, 3, 9, 2, 1);
        art.shadow = up(vdp, k);
    }
}

}  // namespace culvertpouc
