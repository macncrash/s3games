#include "art.h"

#include <initializer_list>

namespace metrolock {
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

gs::Bitmap carArt() {
    gs::Bitmap b(72, 28);
    b.rect(2, 3, 68, 22, 1);
    b.rect(4, 5, 64, 8, 2);
    for (int x = 8; x < 64; x += 14) b.rect(x, 6, 10, 6, 3);
    b.rect(6, 16, 60, 6, 4);
    b.rect(2, 20, 6, 6, 5);
    b.rect(64, 20, 6, 6, 5);
    b.rect(30, 1, 12, 3, 6);
    b.rect(34, 0, 4, 2, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap noseArt() {
    gs::Bitmap b(28, 28);
    b.poly({{2.f, 8.f}, {10.f, 3.f}, {26.f, 3.f}, {26.f, 24.f}, {10.f, 24.f}, {2.f, 19.f}}, 1);
    b.rect(12, 6, 12, 7, 2);
    b.rect(14, 7, 8, 5, 3);
    b.rect(8, 16, 14, 5, 4);
    b.ellipse(7, 14, 3, 3, 6);
    b.rect(4, 20, 6, 5, 5);
    b.rect(20, 20, 5, 5, 5);
    b.outline(8, false);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(18, 72);
    b.rect(1, 0, 16, 72, 1);
    for (int y = 2; y < 70; y += 8) b.rect(3, y, 12, 5, (y / 8) & 1 ? 2 : 3);
    b.rect(0, 0, 2, 72, 4);
    b.rect(16, 0, 2, 72, 4);
    for (int y = 10; y < 64; y += 16) b.rect(6, y, 6, 3, 5);
    b.rect(5, 0, 8, 4, 4);
    b.outline(6, false);
    return b;
}

gs::Bitmap concreteArt() {
    gs::Bitmap b(36, 20);
    b.rect(0, 0, 36, 20, 1);
    for (int row = 0; row < 20; row += 10) {
        int off = ((row / 10) & 1) ? 8 : 0;
        for (int x = -18; x < 36; x += 18) b.rect(x + off, row + 1, 16, 8, ((x + row) & 16) ? 2 : 3);
        b.rect(0, row, 36, 1, 4);
    }
    return b;
}

gs::Bitmap tileArt() {
    gs::Bitmap b(32, 16);
    b.rect(0, 0, 32, 16, 1);
    for (int y = 0; y < 16; y += 8)
        for (int x = (y ? 8 : 0); x < 32; x += 16) b.rect(x, y + 1, 14, 6, 2);
    b.rect(0, 0, 32, 1, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 24);
    b.rect(3, 6, 2, 18, 1);
    b.rect(1, 1, 6, 7, 2);
    b.rect(2, 2, 4, 4, 3);
    return b;
}

gs::Bitmap sleeperArt() {
    gs::Bitmap b(28, 8);
    b.rect(0, 2, 28, 4, 1);
    b.rect(2, 1, 4, 6, 2);
    b.rect(22, 1, 4, 6, 2);
    b.rect(12, 3, 4, 2, 3);
    return b;
}

gs::Bitmap signalArt() {
    gs::Bitmap b(12, 28);
    b.rect(5, 10, 2, 18, 1);
    b.rect(2, 1, 8, 12, 2);
    b.ellipse(6, 5, 2.2f, 2.2f, 3);
    b.ellipse(6, 10, 2.2f, 2.2f, 4);
    return b;
}

gs::Bitmap dripArt() {
    gs::Bitmap b(10, 6);
    b.ellipse(5, 3, 4, 2, 1);
    b.ellipse(3, 3, 1.4f, 1.f, 2);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 15, 15), gs::rgb4(7, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CAR,
           {0, gs::rgb4(11, 12, 13), gs::rgb4(4, 7, 10), gs::rgb4(13, 15, 15), gs::rgb4(6, 7, 8), gs::rgb4(2, 2, 3),
            gs::rgb4(14, 10, 3), gs::rgb4(15, 13, 6), gs::rgb4(1, 1, 2), ink});
    setPal(vdp, PAL_CONCRETE, {0, gs::rgb4(7, 7, 8), gs::rgb4(5, 5, 6), gs::rgb4(10, 10, 11), gs::rgb4(3, 3, 4), ink});
    setPal(vdp, PAL_GATE,
           {0, gs::rgb4(5, 8, 10), gs::rgb4(3, 5, 7), gs::rgb4(8, 11, 12), gs::rgb4(2, 3, 4), gs::rgb4(14, 12, 5),
            gs::rgb4(1, 2, 3), ink});
    setPal(vdp, PAL_TILE, {0, gs::rgb4(4, 8, 9), gs::rgb4(7, 11, 10), gs::rgb4(2, 4, 5), ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(4, 4, 5), gs::rgb4(12, 10, 6), gs::rgb4(15, 14, 6), ink});
    setPal(vdp, PAL_RAIL, {0, gs::rgb4(6, 5, 4), gs::rgb4(9, 9, 10), gs::rgb4(14, 11, 4), ink});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(6, 12, 14), gs::rgb4(10, 14, 15), ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(7, 15, 8), gs::rgb4(14, 15, 13), gs::rgb4(1, 4, 2), ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(15, 13, 6), gs::rgb4(4, 1, 1), ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 5), gs::rgb4(8, 6, 2), gs::rgb4(15, 15, 12), ink});
    setPal(vdp, PAL_SIGNAL, {0, gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), gs::rgb4(4, 12, 4), gs::rgb4(14, 3, 2), ink});

    vdp.setFogColor(gs::rgb4(2, 4, 6));
    loadFont(vdp, art);
    art.car = gs::uploadMipped(vdp, carArt());
    art.nose = gs::uploadMipped(vdp, noseArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.concrete = gs::uploadMipped(vdp, concreteArt());
    art.tile = gs::uploadMipped(vdp, tileArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.sleeper = gs::uploadMipped(vdp, sleeperArt());
    art.signal = gs::uploadMipped(vdp, signalArt());
    art.drip = gs::uploadMipped(vdp, dripArt());
    art.title = words(vdp, "METRO LOCK", 3, 1, 2);
    art.clear = words(vdp, "CLEAR", 3, 1, 3);
    art.fail = words(vdp, "SCRAPED", 3, 1, 2);
}

}  // namespace metrolock
