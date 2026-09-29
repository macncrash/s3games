#include "art.h"

#include <initializer_list>

namespace mailvanslip {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(2, 1, 1));
}

gs::Bitmap vanArt() {
    gs::Bitmap b(72, 32);
    b.rect(4, 6, 64, 20, 1);
    b.rect(6, 8, 38, 16, 2);
    b.rect(46, 9, 18, 14, 3);
    b.rect(49, 11, 5, 5, 4);
    b.rect(56, 11, 5, 5, 4);
    b.rect(10, 11, 22, 4, 5);
    b.rect(12, 16, 14, 5, 6);
    b.rect(2, 3, 10, 7, 7);
    b.rect(2, 22, 10, 7, 7);
    b.rect(56, 3, 12, 7, 7);
    b.rect(56, 22, 12, 7, 7);
    b.rect(4, 4, 6, 5, 8);
    b.rect(4, 23, 6, 5, 8);
    b.rect(58, 4, 8, 5, 8);
    b.rect(58, 23, 8, 5, 8);
    b.rect(42, 4, 3, 24, 5);
    b.outline(9, false);
    return b;
}

gs::Bitmap pierArt() {
    gs::Bitmap b(48, 20);
    b.rect(0, 0, 48, 20, 1);
    for (int x = 2; x < 46; x += 8) b.rect(x, 2, 5, 16, 2);
    b.rect(0, 0, 48, 3, 3);
    b.rect(0, 17, 48, 3, 4);
    return b;
}

gs::Bitmap waterArt() {
    gs::Bitmap b(32, 16);
    b.rect(0, 0, 32, 16, 1);
    for (int y = 2; y < 16; y += 5) {
        for (int x = (y & 1) ? 4 : 0; x < 32; x += 10) b.rect(x, y, 6, 2, 2);
    }
    return b;
}

gs::Bitmap quayArt() {
    gs::Bitmap b(40, 24);
    b.rect(0, 8, 40, 16, 1);
    b.poly({{0.f, 8.f}, {8.f, 2.f}, {20.f, 7.f}, {32.f, 1.f}, {40.f, 8.f}}, 2);
    for (int x = 4; x < 36; x += 8) b.rect(x, 12, 3, 8, 3);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(20, 36);
    b.rect(6, 10, 8, 24, 1);
    b.rect(4, 4, 12, 8, 2);
    b.rect(8, 0, 4, 6, 3);
    b.rect(3, 30, 14, 5, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 28);
    b.rect(5, 8, 2, 16, 1);
    b.rect(2, 2, 8, 8, 2);
    b.rect(4, 4, 4, 4, 3);
    b.rect(3, 23, 6, 3, 4);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(16, 14);
    b.ellipse(8, 8, 6, 5, 1);
    b.rect(5, 2, 6, 3, 2);
    b.line(3, 7, 13, 7, 3, 1);
    b.outline(4, false);
    return b;
}

gs::Bitmap buoyArt() {
    gs::Bitmap b(14, 18);
    b.ellipse(7, 8, 6, 6, 1);
    b.rect(6, 2, 2, 4, 2);
    b.rect(5, 13, 4, 4, 3);
    b.outline(4, false);
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
    const uint16_t ink = gs::rgb4(2, 1, 1);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 13), gs::rgb4(8, 8, 7), ink});
    setPal(vdp, PAL_VAN,
           {0, gs::rgb4(14, 13, 10), gs::rgb4(12, 9, 6), gs::rgb4(7, 10, 13), gs::rgb4(5, 12, 14),
            gs::rgb4(13, 2, 2), gs::rgb4(15, 14, 7), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 1), gs::rgb4(2, 1, 1), ink});
    setPal(vdp, PAL_QUAY, {0, gs::rgb4(7, 7, 6), gs::rgb4(5, 9, 4), gs::rgb4(4, 4, 4), ink});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 6, 10), gs::rgb4(5, 10, 13), gs::rgb4(1, 3, 6), ink});
    setPal(vdp, PAL_PIER, {0, gs::rgb4(8, 7, 6), gs::rgb4(11, 10, 8), gs::rgb4(14, 12, 6), gs::rgb4(5, 4, 3), ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(13, 12, 9), gs::rgb4(12, 2, 2), gs::rgb4(6, 6, 6), gs::rgb4(15, 13, 4),
                           gs::rgb4(2, 1, 1), ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(15, 15, 13), gs::rgb4(1, 4, 2), ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(15, 13, 5), gs::rgb4(4, 1, 1), ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 12), ink});
    setPal(vdp, PAL_SACK, {0, gs::rgb4(12, 8, 3), gs::rgb4(8, 4, 2), gs::rgb4(14, 12, 6), gs::rgb4(3, 2, 1), ink});

    vdp.setFogColor(gs::rgb4(6, 8, 10));
    loadFont(vdp, art);
    art.van = gs::uploadMipped(vdp, vanArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.quay = gs::uploadMipped(vdp, quayArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.title = words(vdp, "MAILVAN SLIP", 2, 1, 2);
    art.berthed = words(vdp, "BERTHED", 3, 1, 3);
    art.missed = words(vdp, "MISSED", 3, 1, 2);
    art.tide = words(vdp, "TIDE", 3, 1, 2);
}

}  // namespace mailvanslip
