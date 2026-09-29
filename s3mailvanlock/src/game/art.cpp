#include "art.h"

#include <cmath>
#include <initializer_list>

namespace mailvanlock {
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
    gs::Bitmap b(88, 40);
    b.rect(6, 6, 76, 28, 1);
    b.rect(8, 8, 50, 24, 2);
    b.rect(58, 10, 20, 20, 3);
    b.rect(62, 13, 6, 6, 4);
    b.rect(70, 13, 6, 6, 4);
    b.rect(14, 12, 28, 6, 5);
    b.rect(16, 20, 18, 8, 6);
    b.rect(4, 2, 10, 8, 7);
    b.rect(4, 30, 10, 8, 7);
    b.rect(68, 2, 12, 8, 7);
    b.rect(68, 30, 12, 8, 7);
    b.rect(6, 3, 6, 6, 8);
    b.rect(6, 31, 6, 6, 8);
    b.rect(70, 3, 8, 6, 8);
    b.rect(70, 31, 8, 6, 8);
    b.rect(46, 4, 4, 32, 5);
    b.outline(9, false);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(18, 72);
    b.rect(1, 0, 16, 72, 1);
    for (int y = 0; y < 72; y += 8) b.rect(3, y, 12, 7, (y / 8) & 1 ? 2 : 3);
    b.rect(0, 0, 2, 72, 4);
    b.rect(16, 0, 2, 72, 4);
    b.rect(6, 30, 6, 12, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap roadArt() {
    gs::Bitmap b(40, 16);
    b.rect(0, 0, 40, 16, 1);
    for (int x = 2; x < 38; x += 10) b.rect(x, 2, 6, 4, 2);
    for (int x = 6; x < 40; x += 10) b.rect(x, 9, 6, 4, 3);
    b.rect(0, 7, 40, 2, 4);
    return b;
}

gs::Bitmap bankArt() {
    gs::Bitmap b(40, 18);
    b.rect(0, 0, 40, 18, 1);
    b.poly({{0.f, 8.f}, {10.f, 2.f}, {22.f, 7.f}, {34.f, 1.f}, {40.f, 6.f}, {40.f, 18.f}, {0.f, 18.f}}, 2);
    for (int x = 4; x < 38; x += 8) b.rect(x, 4, 2, 6, 3);
    return b;
}

gs::Bitmap officeArt() {
    gs::Bitmap b(56, 44);
    b.rect(6, 16, 44, 26, 1);
    b.poly({{2.f, 16.f}, {28.f, 3.f}, {54.f, 16.f}}, 2);
    b.rect(24, 26, 10, 16, 3);
    b.rect(10, 22, 8, 7, 4);
    b.rect(38, 22, 8, 7, 4);
    b.rect(24, 6, 6, 8, 5);
    b.rect(18, 18, 20, 4, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap boxArt() {
    gs::Bitmap b(16, 28);
    b.rect(3, 8, 10, 16, 1);
    b.rect(4, 10, 8, 5, 2);
    b.rect(5, 4, 6, 5, 3);
    b.rect(7, 24, 2, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 32);
    b.rect(5, 10, 2, 20, 1);
    b.rect(2, 2, 8, 9, 2);
    b.rect(4, 4, 4, 5, 3);
    b.rect(3, 28, 6, 3, 4);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(26, 34);
    b.rect(11, 20, 4, 12, 3);
    b.ellipse(13, 13, 11, 10, 1);
    b.ellipse(9, 11, 5, 4, 2);
    b.outline(4, false);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(18, 16);
    b.ellipse(9, 9, 7, 6, 1);
    b.rect(6, 2, 6, 4, 2);
    b.line(4, 8, 14, 8, 3, 1);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(9, 8, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_VAN,
           {0, gs::rgb4(14, 13, 10), gs::rgb4(12, 10, 7), gs::rgb4(8, 10, 12), gs::rgb4(6, 12, 14),
            gs::rgb4(12, 2, 2), gs::rgb4(15, 14, 8), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 1), gs::rgb4(2, 1, 1), ink});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(5, 5, 5), gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 4), gs::rgb4(13, 11, 3), ink});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(3, 3, 4), gs::rgb4(15, 12, 2), gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 9),
                           gs::rgb4(14, 4, 3), gs::rgb4(1, 1, 1), ink});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(3, 7, 3), gs::rgb4(6, 11, 4), gs::rgb4(2, 5, 2), gs::rgb4(8, 6, 3), ink});
    setPal(vdp, PAL_OFFICE,
           {0, gs::rgb4(13, 12, 9), gs::rgb4(9, 3, 2), gs::rgb4(4, 3, 3), gs::rgb4(7, 13, 15), gs::rgb4(6, 6, 6),
            gs::rgb4(15, 13, 4), gs::rgb4(2, 1, 1), ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(4, 8, 12), gs::rgb4(15, 14, 8), gs::rgb4(12, 2, 2), gs::rgb4(5, 5, 5),
                           gs::rgb4(1, 1, 2), ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(15, 15, 13), gs::rgb4(1, 4, 2), ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(15, 13, 6), gs::rgb4(4, 1, 1), ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 5), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 12), ink});
    setPal(vdp, PAL_MAIL, {0, gs::rgb4(12, 8, 3), gs::rgb4(8, 4, 2), gs::rgb4(14, 12, 6), gs::rgb4(3, 2, 1), ink});

    vdp.setFogColor(gs::rgb4(8, 9, 10));
    loadFont(vdp, art);
    art.van = gs::uploadMipped(vdp, vanArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.road = gs::uploadMipped(vdp, roadArt());
    art.bank = gs::uploadMipped(vdp, bankArt());
    art.office = gs::uploadMipped(vdp, officeArt());
    art.box = gs::uploadMipped(vdp, boxArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.title = words(vdp, "MAILVAN LOCK", 2, 1, 2);
    art.clear = words(vdp, "CLEAR", 3, 1, 3);
    art.fail = words(vdp, "SCRAPED", 3, 1, 2);
}

}  // namespace mailvanlock
