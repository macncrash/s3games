#include "game/art.h"

#include <cstdint>
#include <vector>

namespace harbor {
namespace {

void setPal(gs::VDP& vdp, int pal, const std::vector<uint16_t>& cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void px(gs::Bitmap& b, int x, int y, int c) { b.set(x, y, c); }

void box(gs::Bitmap& b, int x, int y, int w, int h, int c) {
    for (int yy = y; yy < y + h; yy++)
        for (int xx = x; xx < x + w; xx++) b.set(xx, yy, c);
}

gs::Bitmap keeperFrame(int step) {
    gs::Bitmap b(28, 32);
    box(b, 4, 22, 20, 6, 3);
    b.ellipse(14, 24, 10, 3, 2);
    box(b, 6, 18, 16, 5, 4);
    int lean = step ? 1 : -1;
    box(b, 10, 8, 8, 10, 1);
    b.ellipse(14, 6, 5, 4, 6);
    box(b, 8, 4, 4, 2, 7);
    box(b, 11 + lean, 16, 3, 6, 5);
    px(b, 12, 11, 8);
    px(b, 16, 11, 8);
    return b;
}

gs::Bitmap cutterFrame(int step) {
    gs::Bitmap b(40, 22);
    b.poly({{2, 14}, {8, 16}, {32, 16}, {38, 12}, {30, 10}, {8, 10}}, 1);
    box(b, 12, 8, 12, 4, 2);
    box(b, 18, 2, 2, 8, 3);
    b.poly({{20, 3}, {30, 8}, {20, 9}}, step ? 4 : 5);
    box(b, 6, 12, 4, 2, 6);
    return b;
}

gs::Bitmap bargeBmp() {
    gs::Bitmap b(48, 20);
    box(b, 4, 10, 40, 6, 1);
    b.poly({{2, 14}, {6, 16}, {42, 16}, {46, 12}, {42, 10}, {6, 10}}, 2);
    box(b, 10, 4, 10, 6, 3);
    box(b, 24, 6, 12, 4, 4);
    box(b, 14, 6, 3, 2, 5);
    return b;
}

gs::Bitmap launchFrame(int step) {
    gs::Bitmap b(36, 18);
    b.poly({{2, 10}, {8, 14}, {30, 14}, {34, 8}, {24, 6}, {8, 6}}, 1);
    box(b, 14, 3, 8, 4, 2);
    box(b, 28, 9, 4, 2, step ? 4 : 3);
    px(b, 16, 5, 5);
    return b;
}

gs::Bitmap chainBmp() {
    gs::Bitmap b(64, 8);
    for (int x = 0; x < 64; x++) {
        int y = (x / 4) & 1 ? 2 : 4;
        b.set(x, y, 1);
        b.set(x, y + 1, 2);
    }
    return b;
}

gs::Bitmap bellBmp() {
    gs::Bitmap b(16, 18);
    b.ellipse(8, 10, 6, 6, 1);
    b.ellipse(8, 10, 3, 3, 2);
    box(b, 7, 1, 2, 4, 3);
    px(b, 8, 14, 4);
    return b;
}

gs::Bitmap ropeBmp() {
    gs::Bitmap b(4, 20);
    for (int y = 0; y < 20; y++) b.set(1 + (y & 1), y, 1);
    return b;
}

gs::Bitmap lightBmp() {
    gs::Bitmap b(14, 28);
    box(b, 5, 8, 4, 18, 1);
    box(b, 3, 6, 8, 4, 2);
    box(b, 4, 2, 6, 4, 3);
    px(b, 6, 3, 4);
    px(b, 7, 3, 4);
    return b;
}

gs::Bitmap buoyBmp() {
    gs::Bitmap b(10, 16);
    b.ellipse(5, 8, 4, 4, 1);
    box(b, 4, 2, 2, 4, 2);
    box(b, 4, 12, 2, 3, 3);
    return b;
}

gs::Bitmap gullBmp() {
    gs::Bitmap b(12, 6);
    b.line(0, 3, 5, 1, 1, 1);
    b.line(5, 1, 11, 4, 1, 1);
    return b;
}

gs::Bitmap splashBmp() {
    gs::Bitmap b(16, 12);
    b.ellipse(8, 8, 6, 3, 1);
    b.line(8, 8, 4, 2, 2, 1);
    b.line(8, 8, 12, 1, 2, 1);
    return b;
}

gs::Bitmap shadowBmp() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7, 2, 1);
    return b;
}

void stoneTile(uint8_t* px) {
    for (int i = 0; i < 64; i++) px[i] = ((i * 3 + (i / 8) * 5) % 7 == 0) ? 2 : 1;
    px[0] = px[7] = px[56] = px[63] = 3;
}

void capTile(uint8_t* px) {
    for (int i = 0; i < 64; i++) px[i] = 4;
    for (int x = 0; x < 8; x++) px[x] = 5;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px64[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px64[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px64[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px64);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t shade = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(10, 11, 12), gs::rgb4(6, 7, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_AMBER,
           {0, gs::rgb4(15, 12, 4), gs::rgb4(12, 8, 2), gs::rgb4(7, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 5, 3), gs::rgb4(9, 2, 2), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 8, 4), gs::rgb4(1, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 2, 1)});
    setPal(vdp, PAL_KEEPER,
           {0, gs::rgb4(12, 11, 8), gs::rgb4(3, 5, 8), gs::rgb4(1, 2, 4), gs::rgb4(8, 6, 3), gs::rgb4(6, 4, 2),
            gs::rgb4(14, 11, 7), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_CUTTER,
           {0, gs::rgb4(13, 13, 12), gs::rgb4(4, 6, 8), gs::rgb4(8, 2, 2), gs::rgb4(14, 14, 13), gs::rgb4(9, 9, 8),
            gs::rgb4(2, 3, 4), 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_BARGE,
           {0, gs::rgb4(8, 6, 4), gs::rgb4(5, 4, 3), gs::rgb4(10, 8, 5), gs::rgb4(6, 3, 2), gs::rgb4(12, 10, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_LAUNCH,
           {0, gs::rgb4(2, 5, 4), gs::rgb4(10, 12, 8), gs::rgb4(14, 12, 4), gs::rgb4(12, 4, 2), gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 4), gs::rgb4(3, 3, 3), gs::rgb4(9, 8, 7), gs::rgb4(12, 11, 9),
            gs::rgb4(5, 6, 5), 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(15, 14, 8), gs::rgb4(11, 8, 3), gs::rgb4(6, 5, 3), gs::rgb4(14, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_NIGHT,
           {0, gs::rgb4(14, 14, 12), gs::rgb4(8, 8, 10), gs::rgb4(4, 4, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_WATER,
           {0, gs::rgb4(2, 5, 4), gs::rgb4(1, 3, 3), gs::rgb4(3, 6, 5), gs::rgb4(6, 6, 5), gs::rgb4(8, 8, 6),
            gs::rgb4(2, 4, 3), gs::rgb4(1, 2, 2), gs::rgb4(4, 5, 4), gs::rgb4(3, 4, 3), gs::rgb4(1, 2, 3),
            gs::rgb4(1, 4, 6), gs::rgb4(2, 6, 8), gs::rgb4(8, 12, 13), gs::rgb4(4, 7, 8), shade});
    setPal(vdp, PAL_LIGHT,
           {0, gs::rgb4(9, 9, 8), gs::rgb4(12, 4, 3), gs::rgb4(14, 12, 6), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_FX, {0, gs::rgb4(12, 14, 15), gs::rgb4(8, 10, 12), gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});

    art.keeper[0] = gs::uploadMipped(vdp, keeperFrame(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperFrame(1));
    art.cutter[0] = gs::uploadMipped(vdp, cutterFrame(0));
    art.cutter[1] = gs::uploadMipped(vdp, cutterFrame(1));
    art.barge = gs::uploadMipped(vdp, bargeBmp());
    art.launch[0] = gs::uploadMipped(vdp, launchFrame(0));
    art.launch[1] = gs::uploadMipped(vdp, launchFrame(1));
    art.chain = gs::uploadMipped(vdp, chainBmp());
    art.bell = gs::uploadMipped(vdp, bellBmp());
    art.rope = gs::uploadMipped(vdp, ropeBmp());
    art.light = gs::uploadMipped(vdp, lightBmp());
    art.buoy = gs::uploadMipped(vdp, buoyBmp());
    art.gull = gs::uploadMipped(vdp, gullBmp());
    art.splash = gs::uploadMipped(vdp, splashBmp());
    art.shadow = gs::uploadMipped(vdp, shadowBmp());

    gs::TileAlloc tiles(vdp, 1);
    uint8_t st[64], cp[64];
    stoneTile(st);
    capTile(cp);
    art.stone = tiles.shared(st);
    art.cap = tiles.shared(cp);
    loadFont(vdp, tiles, art);

    vdp.A.resize(64, 32);
    vdp.A.clear();
    for (int y = 11; y < 28; y++) {
        for (int x = 0; x < 5; x++) vdp.A.set(x, y, gs::entry(art.stone, PAL_STONE));
        for (int x = 35; x < 40; x++) vdp.A.set(x, y, gs::entry(art.stone, PAL_STONE));
        vdp.A.set(4, y, gs::entry(art.cap, PAL_STONE));
        vdp.A.set(35, y, gs::entry(art.cap, PAL_STONE, 1, 0));
    }
    for (int x = 0; x < 40; x++) vdp.A.set(x, 27, gs::entry(art.cap, PAL_STONE));
}

}  // namespace harbor
