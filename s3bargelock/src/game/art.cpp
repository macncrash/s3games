#include "art.h"

#include <cmath>
#include <initializer_list>

namespace bargelock {
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

gs::Bitmap hullArt() {
    gs::Bitmap b(96, 36);
    b.poly({{4.f, 18.f}, {18.f, 4.f}, {78.f, 4.f}, {92.f, 14.f}, {92.f, 22.f}, {78.f, 32.f}, {18.f, 32.f}}, 1);
    b.poly({{20.f, 8.f}, {74.f, 8.f}, {86.f, 16.f}, {86.f, 20.f}, {74.f, 28.f}, {20.f, 28.f}}, 2);
    b.rect(28, 10, 28, 16, 3);
    b.rect(32, 13, 8, 6, 6);
    b.rect(44, 13, 8, 6, 6);
    b.rect(60, 12, 10, 12, 4);
    b.ellipse(66, 18, 3, 3, 5);
    b.rect(8, 15, 8, 6, 7);
    b.rect(14, 16, 10, 4, 8);
    for (int x = 22; x < 72; x += 10) b.rect(x, 6, 2, 3, 9);
    b.outline(10, false);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(20, 64);
    b.rect(2, 0, 16, 64, 1);
    for (int y = 0; y < 64; y += 8) b.rect(4, y, 12, 6, (y / 8) & 1 ? 2 : 3);
    b.rect(0, 0, 3, 64, 4);
    b.rect(17, 0, 3, 64, 4);
    for (int y = 8; y < 58; y += 14) b.rect(7, y, 6, 3, 5);
    b.rect(6, 0, 8, 4, 4);
    b.outline(6, false);
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(40, 24);
    b.rect(0, 0, 40, 24, 1);
    for (int row = 0; row < 24; row += 8) {
        int off = ((row / 8) & 1) ? 10 : 0;
        for (int x = -20; x < 40; x += 20) b.rect(x + off, row + 1, 18, 6, ((x + row) & 16) ? 2 : 3);
        b.rect(0, row, 40, 1, 4);
    }
    return b;
}

gs::Bitmap grassArt() {
    gs::Bitmap b(48, 20);
    b.rect(0, 6, 48, 14, 1);
    b.poly({{0.f, 10.f}, {8.f, 3.f}, {18.f, 9.f}, {30.f, 2.f}, {42.f, 8.f}, {48.f, 4.f}, {48.f, 20.f}, {0.f, 20.f}}, 2);
    for (int x = 3; x < 46; x += 7) b.rect(x, 5, 2, 5, 3);
    return b;
}

gs::Bitmap houseArt() {
    gs::Bitmap b(52, 40);
    b.rect(8, 18, 36, 20, 1);
    b.poly({{4.f, 18.f}, {26.f, 4.f}, {48.f, 18.f}}, 2);
    b.rect(22, 24, 8, 14, 3);
    b.rect(12, 22, 7, 6, 4);
    b.rect(33, 22, 7, 6, 4);
    b.rect(24, 6, 4, 8, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 36);
    b.rect(12, 22, 4, 12, 3);
    b.ellipse(14, 14, 11, 11, 1);
    b.ellipse(10, 12, 5, 5, 2);
    b.outline(4, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 28);
    b.rect(4, 8, 2, 20, 1);
    b.rect(2, 2, 6, 8, 2);
    b.rect(3, 4, 4, 4, 3);
    return b;
}

gs::Bitmap foamArt() {
    gs::Bitmap b(18, 8);
    b.ellipse(9, 4, 7, 3, 1);
    b.ellipse(5, 4, 2, 1.2f, 2);
    b.ellipse(13, 3, 2, 1.2f, 2);
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(16, 8);
    b.rect(0, 2, 16, 4, 1);
    b.rect(0, 1, 16, 1, 2);
    b.rect(2, 3, 3, 2, 3);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 11, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(12, 8, 4), gs::rgb4(6, 4, 2), gs::rgb4(14, 13, 10), gs::rgb4(3, 5, 7), gs::rgb4(15, 12, 4),
            gs::rgb4(8, 13, 15), gs::rgb4(10, 2, 2), gs::rgb4(14, 12, 8), gs::rgb4(4, 3, 2), gs::rgb4(1, 1, 2), ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 6), gs::rgb4(11, 10, 9), gs::rgb4(3, 3, 4), ink});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(10, 7, 3), gs::rgb4(7, 4, 2), gs::rgb4(13, 10, 5), gs::rgb4(4, 4, 5),
                           gs::rgb4(14, 13, 8), gs::rgb4(2, 2, 3), ink});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(3, 8, 3), gs::rgb4(6, 11, 4), gs::rgb4(2, 5, 2), gs::rgb4(8, 6, 3), ink});
    setPal(vdp, PAL_HOUSE, {0, gs::rgb4(13, 12, 9), gs::rgb4(9, 3, 2), gs::rgb4(4, 3, 2), gs::rgb4(6, 12, 14),
                            gs::rgb4(6, 6, 6), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 6, 8), gs::rgb4(4, 9, 11), gs::rgb4(8, 13, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(9, 13, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(15, 15, 13), gs::rgb4(1, 4, 2), ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(15, 13, 6), gs::rgb4(4, 1, 1), ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 5), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 12), ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(5, 5, 6), gs::rgb4(15, 12, 3), gs::rgb4(12, 10, 8), gs::rgb4(2, 6, 2), ink});

    vdp.setFogColor(gs::rgb4(6, 9, 11));
    loadFont(vdp, art);
    art.hull = gs::uploadMipped(vdp, hullArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.grass = gs::uploadMipped(vdp, grassArt());
    art.house = gs::uploadMipped(vdp, houseArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.title = words(vdp, "BARGE LOCK", 3, 1, 2);
    art.clear = words(vdp, "CLEAR", 3, 1, 3);
    art.fail = words(vdp, "SCRAPED", 3, 1, 2);
}

}  // namespace bargelock
