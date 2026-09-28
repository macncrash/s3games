#include "game/art.h"

namespace plow {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

// Side-view municipal plow. Blade is the right edge; that tip is the level line.
gs::Bitmap plowArt() {
    gs::Bitmap b(128, 56);
    b.ellipse(28, 42, 12, 12, 4);
    b.ellipse(78, 42, 12, 12, 4);
    b.ellipse(28, 42, 6, 6, 5);
    b.ellipse(78, 42, 6, 6, 5);
    b.rect(18, 22, 78, 16, 2);                  // chassis
    b.poly({{96, 18}, {120, 10}, {122, 40}, {96, 38}}, 3);  // blade
    b.line(96, 16, 122, 8, 6, 2.0f);
    b.rect(40, 8, 36, 18, 1);                   // cab
    b.rect(46, 11, 16, 10, 7);                  // glass
    b.rect(22, 26, 10, 6, 8);                   // lamp
    b.rect(16, 28, 14, 4, 5);                   // hitch
    b.poly({{100, 22}, {116, 16}, {116, 34}, {100, 34}}, 6);
    b.outline(9, false);
    return b;
}

gs::Bitmap deckArt() {
    gs::Bitmap b(140, 64);
    b.rect(6, 10, 128, 28, 2);
    for (int i = 0; i < 8; i++) b.rect(8, 12 + i * 3, 124, 1, 3);
    b.rect(4, 8, 8, 34, 4);  // end face — the lip
    b.rect(0, 4, 4, 44, 5);
    for (int i = 0; i < 4; i++) b.rect(24 + i * 28, 38, 6, 22, 1);
    b.rect(18, 36, 110, 4, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(14, 80);
    b.rect(5, 8, 4, 70, 1);
    for (int y = 10; y < 70; y += 10) b.rect(4, y, 6, 5, 2);
    b.rect(1, 0, 12, 8, 3);
    return b;
}

gs::Bitmap pineArt() {
    gs::Bitmap b(40, 72);
    b.rect(17, 48, 6, 22, 1);
    b.poly({{20, 4}, {38, 34}, {2, 34}}, 2);
    b.poly({{20, 18}, {36, 50}, {4, 50}}, 3);
    b.poly({{20, 32}, {32, 58}, {8, 58}}, 4);
    return b;
}

gs::Bitmap barnArt() {
    gs::Bitmap b(90, 48);
    b.rect(8, 18, 74, 28, 1);
    b.poly({{4, 20}, {45, 2}, {86, 20}}, 2);
    b.rect(38, 28, 14, 18, 3);
    b.rect(16, 26, 10, 8, 4);
    b.rect(62, 26, 10, 8, 4);
    return b;
}

gs::Bitmap snowArt() {
    gs::Bitmap b(72, 22);
    b.rect(0, 6, 72, 16, 1);
    b.rect(0, 16, 72, 6, 2);
    b.poly({{0, 8}, {18, 2}, {36, 8}, {54, 1}, {72, 8}, {72, 10}, {0, 10}}, 3);
    return b;
}

gs::Bitmap sprayArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 4, 1);
    b.ellipse(6, 7, 3, 2, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(3, 4, 6), gs::rgb4(15, 12, 3), gs::rgb4(15, 4, 2)});
    setPal(vdp, PAL_PLOW, {0, gs::rgb4(14, 9, 2), gs::rgb4(12, 7, 1), gs::rgb4(11, 12, 13), gs::rgb4(2, 2, 3),
                           gs::rgb4(5, 5, 6), gs::rgb4(14, 15, 15), gs::rgb4(6, 10, 13), gs::rgb4(15, 13, 4),
                           gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_DECK, {0, gs::rgb4(5, 3, 2), gs::rgb4(9, 6, 3), gs::rgb4(12, 8, 4), gs::rgb4(7, 4, 2),
                           gs::rgb4(15, 14, 10), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(13, 14, 15), gs::rgb4(10, 12, 14), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(4, 2, 1), gs::rgb4(1, 4, 2), gs::rgb4(2, 7, 3), gs::rgb4(3, 9, 4)});
    setPal(vdp, PAL_BARN, {0, gs::rgb4(10, 3, 2), gs::rgb4(7, 2, 1), gs::rgb4(3, 2, 1), gs::rgb4(8, 12, 14)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(2, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(13, 3, 2)});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(14, 15, 15), gs::rgb4(15, 15, 15)});
    art.plow = gs::uploadMipped(vdp, plowArt());
    art.deck = gs::uploadMipped(vdp, deckArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.barn = gs::uploadMipped(vdp, barnArt());
    art.snow = gs::uploadMipped(vdp, snowArt());
    art.spray = gs::uploadMipped(vdp, sprayArt());
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(12, 13, 15));
}

}  // namespace plow
