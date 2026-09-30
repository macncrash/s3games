#include "game/art.h"

#include <initializer_list>

namespace maskseven {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t edge) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 2, edge);
    vdp.setColor(pal * 16 + 15, edge);
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
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Image up(gs::VDP& vdp, const gs::Bitmap& b) { return gs::uploadImage(vdp, b); }

void paintHalf(gs::Bitmap& b, int brow) {
    b.ellipse(40, 52, 34.f, 44.f, 4);
    b.ellipse(40, 50, 26.f, 34.f, 5);
    b.ellipse(28, 46, 8.f, 5.f, 2);
    b.ellipse(52, 46, 8.f, 5.f, 2);
    b.poly({{36, 54}, {44, 54}, {40, 64}}, 3);
    b.ellipse(40, 74, 10.f, 4.f, 2);
    b.rect(18, 16, 44, 5, brow);
    b.rect(12, 12, 8, 10, 6);
    b.rect(60, 12, 8, 10, 6);
    b.outline(1, false);
}

void paintBrush(gs::Bitmap& b) {
    b.rect(6, 0, 4, 18, 3);
    b.rect(3, 16, 10, 6, 4);
    b.rect(4, 20, 8, 4, 5);
    b.rect(5, 0, 6, 4, 2);
    b.outline(1, false);
}

void paintFoil(gs::Bitmap& b) {
    b.ellipse(8, 8, 6.f, 5.f, 3);
    b.ellipse(8, 8, 3.f, 2.f, 4);
    b.line(3, 10, 13, 6, 2, 1.2f);
    b.outline(1, false);
}

void paintBar(gs::Bitmap& b) {
    b.rect(0, 2, 48, 6, 2);
    b.rect(16, 3, 16, 4, 3);
    b.outline(1, false);
}

void paintHook(gs::Bitmap& b) {
    b.rect(6, 0, 4, 14, 2);
    b.rect(4, 12, 8, 3, 3);
    b.outline(1, false);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_CLAY, {0, gs::rgb4(2, 1, 1), gs::rgb4(6, 3, 2), gs::rgb4(9, 5, 3), gs::rgb4(12, 8, 5),
                           gs::rgb4(14, 11, 8), gs::rgb4(10, 3, 4), gs::rgb4(4, 2, 2)});
    setPal(vdp, PAL_RIVAL, {0, gs::rgb4(2, 1, 2), gs::rgb4(5, 3, 4), gs::rgb4(8, 4, 5), gs::rgb4(11, 7, 7),
                            gs::rgb4(13, 10, 9), gs::rgb4(6, 2, 3), gs::rgb4(3, 1, 2)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(5, 3, 1), gs::rgb4(10, 7, 2), gs::rgb4(15, 12, 4), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_FOIL, {0, gs::rgb4(6, 4, 1), gs::rgb4(12, 9, 2), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(4, 5, 2), gs::rgb4(8, 10, 3), gs::rgb4(12, 14, 6), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_BRUSH, {0, gs::rgb4(2, 2, 3), gs::rgb4(7, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(11, 8, 5),
                            gs::rgb4(14, 13, 10), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_BENCH, {0, gs::rgb4(4, 3, 2), gs::rgb4(7, 5, 3), gs::rgb4(10, 7, 4)});
    setPal(vdp, PAL_HOOK, {0, gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 9), gs::rgb4(12, 12, 13)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(3, 2, 1), gs::rgb4(8, 6, 2), gs::rgb4(14, 11, 4)});
    textPal(vdp, PAL_INK, gs::rgb4(14, 12, 9), gs::rgb4(3, 2, 1));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 6), gs::rgb4(5, 2, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3), gs::rgb4(4, 1, 1));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 7, 5), gs::rgb4(2, 2, 1));
    textPal(vdp, PAL_WALL, gs::rgb4(11, 9, 8), gs::rgb4(2, 1, 2));
    loadFont(vdp, art);

    gs::Bitmap yours(80, 96);
    paintHalf(yours, 6);
    art.yours = up(vdp, yours);

    gs::Bitmap theirs(80, 96);
    paintHalf(theirs, 5);
    art.theirs = up(vdp, theirs);

    gs::Bitmap brush(16, 28);
    paintBrush(brush);
    art.brush = up(vdp, brush);

    gs::Bitmap foil(16, 16);
    paintFoil(foil);
    art.foil = up(vdp, foil);

    gs::Bitmap bar(48, 10);
    paintBar(bar);
    art.bar = up(vdp, bar);

    gs::Bitmap hook(16, 18);
    paintHook(hook);
    art.hook = up(vdp, hook);
}

}  // namespace maskseven
