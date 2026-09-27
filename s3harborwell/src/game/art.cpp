#include "game/art.h"

#include <cstdint>
#include <vector>

namespace well {
namespace {

void setPal(gs::VDP& vdp, int pal, const std::vector<uint16_t>& cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void box(gs::Bitmap& b, int x, int y, int w, int h, int c) {
    for (int yy = y; yy < y + h; yy++)
        for (int xx = x; xx < x + w; xx++) b.set(xx, yy, c);
}

gs::Bitmap wellBmp() {
    gs::Bitmap b(36, 48);
    b.ellipse(18, 10, 14, 5, 2);
    b.ellipse(18, 10, 8, 3, 4);
    box(b, 6, 12, 24, 28, 1);
    box(b, 8, 14, 4, 24, 3);
    box(b, 24, 14, 4, 24, 5);
    for (int y = 16; y < 38; y += 6) box(b, 8, y, 20, 2, 6);
    b.ellipse(18, 40, 14, 4, 2);
    return b;
}

gs::Bitmap roofBmp() {
    gs::Bitmap b(40, 16);
    b.poly({{2, 14}, {20, 2}, {38, 14}}, 1);
    b.poly({{8, 14}, {20, 6}, {32, 14}}, 2);
    box(b, 18, 1, 4, 4, 3);
    return b;
}

gs::Bitmap keeperFrame(int step) {
    gs::Bitmap b(24, 32);
    int lean = step ? 2 : 0;
    box(b, 8, 14, 8, 10, 1);
    b.ellipse(12, 8, 5, 5, 2);
    box(b, 7, 3, 10, 3, 3);
    box(b, 4 + lean, 16, 4, 3, 4);
    box(b, 16, 16, 4, 3, 4);
    box(b, 9, 24, 3, 7, 5);
    box(b, 13, 24, 3, 7, 5);
    b.set(10, 9, 6);
    b.set(14, 9, 6);
    return b;
}

gs::Bitmap beamBmp() {
    gs::Bitmap b(28, 8);
    box(b, 0, 2, 28, 4, 1);
    box(b, 0, 2, 3, 4, 2);
    box(b, 25, 2, 3, 4, 2);
    return b;
}

gs::Bitmap sprayBmp() {
    gs::Bitmap b(28, 20);
    b.ellipse(14, 14, 12, 5, 1);
    b.line(6, 12, 4, 3, 2, 1);
    b.line(14, 12, 14, 2, 2, 1);
    b.line(22, 12, 24, 4, 3, 1);
    b.ellipse(14, 16, 8, 2, 3);
    return b;
}

gs::Bitmap gullBmp() {
    gs::Bitmap b(14, 6);
    b.line(0, 3, 6, 1, 1, 1);
    b.line(6, 1, 13, 4, 1, 1);
    return b;
}

gs::Bitmap lampBmp() {
    gs::Bitmap b(10, 26);
    box(b, 4, 8, 2, 16, 1);
    box(b, 2, 4, 6, 5, 2);
    b.set(4, 6, 3);
    b.set(5, 6, 3);
    return b;
}

gs::Bitmap crackBmp() {
    gs::Bitmap b(10, 16);
    b.line(2, 1, 6, 6, 1, 1);
    b.line(6, 6, 3, 14, 1, 1);
    b.line(6, 6, 9, 10, 1, 1);
    return b;
}

gs::Bitmap shadowBmp() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7, 2, 1);
    return b;
}

void quayTile(uint8_t* px) {
    for (int i = 0; i < 64; i++) px[i] = ((i + (i / 8) * 3) % 5 == 0) ? 2 : 1;
}

void lipTile(uint8_t* px) {
    for (int i = 0; i < 64; i++) px[i] = 3;
    for (int x = 0; x < 8; x++) px[x] = 4;
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
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 9, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 4), gs::rgb4(10, 7, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 7), gs::rgb4(2, 7, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_KEEPER,
           {0, gs::rgb4(4, 7, 10), gs::rgb4(14, 11, 8), gs::rgb4(6, 4, 2), gs::rgb4(10, 8, 5), gs::rgb4(3, 3, 4),
            gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(8, 8, 7), gs::rgb4(11, 11, 10), gs::rgb4(5, 5, 5), gs::rgb4(2, 5, 7), gs::rgb4(6, 6, 6),
            gs::rgb4(4, 4, 4), 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(13, 10, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_SEA, {0, gs::rgb4(3, 8, 10), gs::rgb4(6, 12, 13), gs::rgb4(1, 4, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(12, 14, 15), gs::rgb4(8, 12, 14), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(12, 12, 11), gs::rgb4(14, 10, 4), gs::rgb4(15, 15, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_WATER,
           {0, gs::rgb4(1, 4, 6), gs::rgb4(2, 6, 8), gs::rgb4(6, 6, 5), gs::rgb4(8, 8, 6), gs::rgb4(1, 3, 4),
            gs::rgb4(2, 5, 6), gs::rgb4(1, 2, 3), gs::rgb4(3, 7, 8), gs::rgb4(4, 8, 9), gs::rgb4(1, 5, 7),
            gs::rgb4(2, 7, 9), gs::rgb4(8, 12, 13), gs::rgb4(3, 5, 6), gs::rgb4(4, 6, 7), shade});

    art.well = gs::uploadMipped(vdp, wellBmp());
    art.roof = gs::uploadMipped(vdp, roofBmp());
    art.keeper[0] = gs::uploadMipped(vdp, keeperFrame(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperFrame(1));
    art.beam = gs::uploadMipped(vdp, beamBmp());
    art.spray = gs::uploadMipped(vdp, sprayBmp());
    art.gull = gs::uploadMipped(vdp, gullBmp());
    art.lamp = gs::uploadMipped(vdp, lampBmp());
    art.crack = gs::uploadMipped(vdp, crackBmp());
    art.shadow = gs::uploadMipped(vdp, shadowBmp());

    gs::TileAlloc tiles(vdp, 1);
    uint8_t q[64], lip[64];
    quayTile(q);
    lipTile(lip);
    art.quay = tiles.shared(q);
    art.lip = tiles.shared(lip);
    loadFont(vdp, tiles, art);

    vdp.A.resize(64, 32);
    vdp.A.clear();
    vdp.B.clear();
    for (int y = 18; y < 28; y++)
        for (int x = 0; x < 40; x++) vdp.A.set(x, y, gs::entry(art.quay, PAL_STONE));
    for (int x = 0; x < 40; x++) vdp.A.set(x, 18, gs::entry(art.lip, PAL_STONE));
}

}  // namespace well
