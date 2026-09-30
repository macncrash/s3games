#include "game/art.h"

#include <initializer_list>

namespace masktape {
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

void paintMask(gs::Bitmap& b) {
    b.ellipse(48, 58, 40.f, 46.f, 4);
    b.ellipse(48, 56, 30.f, 34.f, 5);
    b.ellipse(34, 50, 9.f, 6.f, 2);
    b.ellipse(62, 50, 9.f, 6.f, 2);
    b.poly({{42, 60}, {54, 60}, {48, 72}}, 3);
    b.ellipse(48, 84, 14.f, 5.f, 2);
    b.rect(16, 18, 64, 7, 6);
    b.rect(10, 10, 12, 16, 7);
    b.rect(74, 10, 12, 16, 7);
    b.rect(40, 8, 16, 8, 3);
    b.outline(1, false);
}

void paintBrush(gs::Bitmap& b) {
    b.rect(6, 0, 4, 16, 3);
    b.rect(3, 14, 10, 7, 4);
    b.rect(4, 19, 8, 5, 5);
    b.rect(5, 0, 6, 4, 2);
    b.outline(1, false);
}

void paintRail(gs::Bitmap& b) {
    b.rect(0, 3, 180, 8, 2);
    b.rect(2, 5, 26, 4, 3);
    b.rect(62, 5, 26, 4, 4);
    b.rect(122, 5, 26, 4, 5);
    b.outline(1, false);
}

void paintPaper(gs::Bitmap& b) {
    b.rect(1, 1, 46, 70, 3);
    b.rect(4, 6, 40, 4, 4);
    b.rect(4, 16, 34, 3, 2);
    b.rect(4, 24, 30, 3, 2);
    b.rect(4, 32, 36, 3, 2);
    b.outline(1, false);
}

void paintDrawer(gs::Bitmap& b) {
    b.rect(0, 0, 168, 28, 3);
    b.rect(4, 4, 48, 18, 2);
    b.rect(58, 4, 48, 18, 2);
    b.rect(112, 4, 48, 18, 2);
    b.rect(78, 22, 12, 4, 4);
    b.outline(1, false);
}

void paintSlip(gs::Bitmap& b) {
    b.rect(1, 1, 22, 12, 3);
    b.rect(3, 3, 18, 3, 4);
    b.outline(1, false);
}

void paintHook(gs::Bitmap& b) {
    b.rect(6, 0, 4, 16, 2);
    b.rect(3, 14, 10, 3, 3);
    b.outline(1, false);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_CLAY, {0, gs::rgb4(3, 2, 2), gs::rgb4(7, 4, 3), gs::rgb4(11, 6, 4), gs::rgb4(14, 10, 7),
                           gs::rgb4(15, 13, 9), gs::rgb4(12, 4, 5), gs::rgb4(5, 3, 3)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(5, 3, 1), gs::rgb4(11, 8, 2), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 9)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_BRUSH, {0, gs::rgb4(2, 2, 3), gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(12, 9, 5),
                            gs::rgb4(15, 14, 9), gs::rgb4(9, 8, 6)});
    setPal(vdp, PAL_SLIP, {0, gs::rgb4(6, 5, 2), gs::rgb4(12, 10, 4), gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(4, 3, 2), gs::rgb4(8, 6, 3), gs::rgb4(13, 11, 7), gs::rgb4(10, 8, 4)});
    setPal(vdp, PAL_DRAWER, {0, gs::rgb4(2, 1, 1), gs::rgb4(5, 3, 2), gs::rgb4(8, 5, 3), gs::rgb4(11, 8, 4)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(4, 3, 1), gs::rgb4(10, 8, 2), gs::rgb4(15, 12, 4), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_WAX, {0, gs::rgb4(5, 2, 2), gs::rgb4(10, 4, 3), gs::rgb4(14, 8, 4), gs::rgb4(15, 12, 8)});
    setPal(vdp, PAL_HOOK, {0, gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 9), gs::rgb4(12, 12, 13)});
    setPal(vdp, PAL_DECOY, {0, gs::rgb4(3, 1, 1), gs::rgb4(8, 3, 2), gs::rgb4(12, 5, 3), gs::rgb4(6, 4, 3)});
    textPal(vdp, PAL_INK, gs::rgb4(14, 12, 9), gs::rgb4(3, 2, 1));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 6), gs::rgb4(5, 2, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3), gs::rgb4(4, 1, 1));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 7, 5), gs::rgb4(2, 2, 1));
    textPal(vdp, PAL_WALL, gs::rgb4(11, 9, 8), gs::rgb4(2, 1, 2));
    loadFont(vdp, art);

    gs::Bitmap mask(96, 104);
    paintMask(mask);
    art.mask = up(vdp, mask);

    gs::Bitmap brush(16, 26);
    paintBrush(brush);
    art.brush = up(vdp, brush);

    gs::Bitmap rail(180, 14);
    paintRail(rail);
    art.rail = up(vdp, rail);

    gs::Bitmap paper(48, 72);
    paintPaper(paper);
    art.paper = up(vdp, paper);

    gs::Bitmap drawer(168, 28);
    paintDrawer(drawer);
    art.drawer = up(vdp, drawer);

    gs::Bitmap slip(24, 14);
    paintSlip(slip);
    art.slip = up(vdp, slip);

    gs::Bitmap hook(16, 20);
    paintHook(hook);
    art.hook = up(vdp, hook);
}

}  // namespace masktape
