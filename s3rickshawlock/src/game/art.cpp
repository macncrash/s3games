#include "art.h"

#include <initializer_list>

namespace ricklock {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap bodyArt() {
    gs::Bitmap b(72, 22);
    b.poly({{6.f, 11.f}, {16.f, 3.f}, {58.f, 3.f}, {66.f, 8.f}, {66.f, 14.f}, {58.f, 19.f}, {16.f, 19.f}}, 1);
    b.rect(18, 6, 28, 10, 2);
    b.rect(48, 7, 10, 8, 3);
    b.rect(8, 9, 8, 4, 4);
    b.line(20, 5, 54, 5, 5, 1);
    b.outline(6, false);
    return b;
}

gs::Bitmap hoodArt() {
    gs::Bitmap b(40, 28);
    b.poly({{2.f, 22.f}, {6.f, 8.f}, {20.f, 3.f}, {34.f, 8.f}, {38.f, 22.f}}, 1);
    b.poly({{8.f, 20.f}, {12.f, 10.f}, {20.f, 7.f}, {28.f, 10.f}, {32.f, 20.f}}, 2);
    b.rect(18, 4, 4, 6, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 1);
    b.ellipse(9, 9, 5, 5, 2);
    b.ellipse(9, 9, 2, 2, 3);
    b.line(9, 2, 9, 16, 4, 1);
    b.line(2, 9, 16, 9, 4, 1);
    b.line(4, 4, 14, 14, 4, 1);
    b.line(14, 4, 4, 14, 4, 1);
    return b;
}

gs::Bitmap driverArt() {
    gs::Bitmap b(14, 16);
    b.ellipse(7, 5, 4, 4, 1);
    b.rect(4, 9, 6, 6, 2);
    b.rect(2, 10, 2, 4, 3);
    b.rect(10, 10, 2, 4, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap seatArt() {
    gs::Bitmap b(22, 16);
    b.rect(2, 4, 18, 10, 1);
    b.rect(3, 2, 16, 4, 2);
    b.ellipse(8, 9, 3, 3, 3);
    b.rect(14, 7, 4, 5, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(16, 56);
    b.rect(1, 0, 14, 56, 1);
    for (int y = 2; y < 54; y += 8) b.rect(3, y, 10, 5, (y / 8) & 1 ? 2 : 3);
    b.rect(0, 0, 2, 56, 4);
    b.rect(14, 0, 2, 56, 4);
    for (int y = 10; y < 48; y += 16) b.rect(5, y, 6, 3, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(36, 20);
    b.rect(0, 0, 36, 20, 1);
    for (int row = 0; row < 20; row += 7) {
        int off = ((row / 7) & 1) ? 9 : 0;
        for (int x = -18; x < 36; x += 18) b.rect(x + off, row + 1, 16, 5, ((x + row) & 16) ? 2 : 3);
        b.rect(0, row, 36, 1, 4);
    }
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(28, 12);
    b.rect(0, 0, 28, 12, 1);
    for (int x = 0; x < 28; x += 7) b.rect(x, 1, 6, 10, (x / 7) & 1 ? 2 : 3);
    b.rect(0, 0, 28, 1, 4);
    return b;
}

gs::Bitmap grassArt() {
    gs::Bitmap b(40, 16);
    b.rect(0, 4, 40, 12, 1);
    b.poly({{0.f, 8.f}, {8.f, 2.f}, {16.f, 7.f}, {26.f, 2.f}, {36.f, 6.f}, {40.f, 3.f}, {40.f, 16.f}, {0.f, 16.f}}, 2);
    for (int x = 2; x < 38; x += 6) b.rect(x, 4, 2, 4, 3);
    return b;
}

gs::Bitmap houseArt() {
    gs::Bitmap b(44, 36);
    b.rect(6, 16, 32, 18, 1);
    b.poly({{2.f, 16.f}, {22.f, 3.f}, {42.f, 16.f}}, 2);
    b.rect(18, 22, 8, 12, 3);
    b.rect(9, 20, 6, 5, 4);
    b.rect(29, 20, 6, 5, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 26);
    b.rect(4, 8, 2, 18, 1);
    b.rect(2, 2, 6, 8, 2);
    b.rect(3, 4, 4, 4, 3);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(48, 8);
    b.rect(0, 2, 48, 4, 1);
    b.rect(0, 1, 48, 1, 2);
    b.rect(4, 3, 6, 2, 3);
    b.rect(38, 3, 6, 2, 3);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int fill, int edge) {
    gs::TextStyle st{scale, fill, edge, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 10, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(12, 3, 2), gs::rgb4(14, 10, 4), gs::rgb4(6, 8, 10), gs::rgb4(3, 3, 4), gs::rgb4(15, 13, 8),
            gs::rgb4(2, 1, 1), ink});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(2, 2, 3), gs::rgb4(8, 7, 6), gs::rgb4(12, 10, 6), gs::rgb4(5, 5, 6), ink});
    setPal(vdp, PAL_GATE,
           {0, gs::rgb4(9, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(12, 9, 5), gs::rgb4(3, 3, 4), gs::rgb4(14, 12, 7),
            gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 6), gs::rgb4(11, 10, 9), gs::rgb4(3, 3, 4), ink});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2), ink});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(3, 8, 3), gs::rgb4(7, 12, 4), gs::rgb4(2, 5, 2), gs::rgb4(9, 8, 3), ink});
    setPal(vdp, PAL_HOUSE,
           {0, gs::rgb4(13, 11, 8), gs::rgb4(10, 3, 2), gs::rgb4(4, 3, 2), gs::rgb4(6, 12, 14), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(15, 15, 13), gs::rgb4(1, 4, 2), ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(15, 13, 6), gs::rgb4(4, 1, 1), ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 4), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 12), ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(5, 5, 6), gs::rgb4(15, 12, 3), gs::rgb4(14, 14, 10), ink});

    vdp.setFogColor(gs::rgb4(4, 8, 10));
    loadFont(vdp, art);
    art.body = gs::uploadMipped(vdp, bodyArt());
    art.hood = gs::uploadMipped(vdp, hoodArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.driver = gs::uploadMipped(vdp, driverArt());
    art.seat = gs::uploadMipped(vdp, seatArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.grass = gs::uploadMipped(vdp, grassArt());
    art.house = gs::uploadMipped(vdp, houseArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.title = words(vdp, "RICKSHAW LOCK", 2, 1, 2);
    art.clear = words(vdp, "CLEAR", 3, 1, 3);
    art.fail = words(vdp, "SCRAPED", 3, 1, 2);
}

}  // namespace ricklock
