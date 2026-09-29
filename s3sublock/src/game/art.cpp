#include "art.h"

#include <initializer_list>

namespace sublock {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 2, 3));
}

gs::Bitmap subArt() {
    gs::Bitmap b(88, 28);
    b.poly({{6.f, 14.f}, {18.f, 6.f}, {62.f, 6.f}, {78.f, 11.f}, {84.f, 14.f}, {78.f, 17.f}, {62.f, 22.f}, {18.f, 22.f}}, 1);
    b.poly({{22.f, 9.f}, {58.f, 9.f}, {70.f, 14.f}, {58.f, 19.f}, {22.f, 19.f}}, 2);
    b.ellipse(36, 13, 7, 4, 3);
    b.ellipse(34, 12, 3, 2, 6);
    b.rect(48, 10, 8, 6, 4);
    b.rect(8, 12, 8, 4, 5);
    b.rect(14, 13, 8, 2, 7);
    b.rect(60, 4, 3, 4, 8);
    b.ellipse(62, 4, 2, 2, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap propArt() {
    gs::Bitmap b(14, 14);
    b.rect(6, 2, 2, 10, 1);
    b.rect(2, 6, 10, 2, 1);
    b.ellipse(7, 7, 2, 2, 2);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(16, 56);
    b.rect(1, 0, 14, 56, 1);
    for (int y = 0; y < 56; y += 8) b.rect(3, y + 1, 10, 5, (y / 8) & 1 ? 2 : 3);
    b.rect(0, 0, 2, 56, 4);
    b.rect(14, 0, 2, 56, 4);
    for (int y = 6; y < 50; y += 12) b.ellipse(8, float(y), 2.2f, 2.2f, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap rockArt() {
    gs::Bitmap b(40, 22);
    b.rect(0, 0, 40, 22, 1);
    b.poly({{0.f, 8.f}, {8.f, 2.f}, {18.f, 7.f}, {28.f, 1.f}, {40.f, 6.f}, {40.f, 22.f}, {0.f, 22.f}}, 2);
    for (int x = 2; x < 38; x += 9) b.rect(x, 12, 5, 4, 3);
    b.rect(0, 0, 40, 2, 4);
    return b;
}

gs::Bitmap kelpArt() {
    gs::Bitmap b(12, 36);
    b.poly({{6.f, 34.f}, {3.f, 22.f}, {7.f, 14.f}, {2.f, 4.f}, {6.f, 0.f}, {9.f, 8.f}, {5.f, 18.f}, {9.f, 28.f}}, 1);
    b.poly({{7.f, 32.f}, {10.f, 18.f}, {6.f, 10.f}, {11.f, 2.f}}, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 22);
    b.rect(4, 8, 2, 14, 1);
    b.ellipse(5, 5, 4, 4, 2);
    b.ellipse(5, 5, 2, 2, 3);
    return b;
}

gs::Bitmap bubArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.ellipse(3, 3, 1, 1, 2);
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
    const uint16_t ink = gs::rgb4(1, 2, 3);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 15, 15), gs::rgb4(6, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SUB,
           {0, gs::rgb4(12, 13, 10), gs::rgb4(5, 7, 6), gs::rgb4(8, 14, 15), gs::rgb4(3, 4, 5), gs::rgb4(14, 8, 3),
            gs::rgb4(15, 15, 14), gs::rgb4(9, 10, 8), gs::rgb4(4, 8, 6), gs::rgb4(1, 2, 2), ink});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(8, 4, 4), gs::rgb4(4, 2, 3), gs::rgb4(10, 6, 6), gs::rgb4(3, 2, 3), gs::rgb4(12, 8, 4),
            gs::rgb4(10, 8, 8), gs::rgb4(6, 3, 3), gs::rgb4(5, 2, 2), gs::rgb4(2, 1, 2), ink});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(4, 5, 6), gs::rgb4(6, 7, 7), gs::rgb4(3, 4, 5), gs::rgb4(2, 3, 4), ink});
    setPal(vdp, PAL_GATE,
           {0, gs::rgb4(11, 9, 4), gs::rgb4(7, 6, 3), gs::rgb4(14, 12, 6), gs::rgb4(3, 4, 5), gs::rgb4(15, 14, 8),
            gs::rgb4(2, 2, 3), ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(5, 6, 6), gs::rgb4(14, 13, 6), gs::rgb4(15, 15, 12), ink});
    setPal(vdp, PAL_BUB, {0, gs::rgb4(10, 14, 15), gs::rgb4(15, 15, 15), ink});
    setPal(vdp, PAL_KELP, {0, gs::rgb4(2, 8, 4), gs::rgb4(4, 11, 5), ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 10), gs::rgb4(3, 8, 5), gs::rgb4(14, 15, 14), ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 4), gs::rgb4(8, 2, 2), gs::rgb4(15, 12, 8), ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 5), gs::rgb4(8, 6, 2), gs::rgb4(15, 15, 12), ink});
    setPal(vdp, PAL_DEEP, {0, gs::rgb4(2, 5, 8), gs::rgb4(3, 7, 10), ink});
    vdp.setFogColor(gs::rgb4(1, 3, 6));

    art.sub = gs::uploadMipped(vdp, subArt());
    art.prop = gs::uploadMipped(vdp, propArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.kelp = gs::uploadMipped(vdp, kelpArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.bub = gs::uploadMipped(vdp, bubArt());
    art.title = words(vdp, "SUB LOCK", 3, 1, 2);
    art.clear = words(vdp, "CLEAR", 3, 1, 2);
    art.fail = words(vdp, "SCRAPE", 3, 1, 2);
    loadFont(vdp, art);
}

}  // namespace sublock
