#include "game/art.h"

#include <string>

namespace subpass {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap subArt(bool crew) {
    Bitmap b(96, 40);
    b.ellipse(50, 22, 36, 11, 2);
    b.ellipse(48, 20, 34, 9, 1);
    b.rect(36, 8, 16, 12, 2);
    b.rect(38, 6, 12, 10, 3);
    b.rect(42, 2, 3, 8, 6);
    b.ellipse(62, 18, 5, 4, 4);
    b.ellipse(62, 18, 2, 2, 8);
    b.rect(14, 18, 8, 5, 6);
    b.rect(10, 16, 4, 3, 7);
    b.rect(10, 22, 4, 3, 7);
    if (crew) {
        b.rect(30, 16, 18, 3, 9);
        b.rect(34, 12, 6, 4, 9);
    }
    b.outline(5, false);
    return b;
}

Bitmap rockArt() {
    Bitmap b(48, 40);
    b.poly({{4, 36}, {10, 14}, {22, 6}, {34, 16}, {44, 34}, {8, 38}}, 2);
    b.poly({{12, 32}, {18, 16}, {28, 12}, {36, 28}}, 1);
    b.poly({{20, 30}, {26, 20}, {32, 32}}, 3);
    b.outline(5, false);
    return b;
}

Bitmap mineArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 10, 10, 2);
    b.ellipse(14, 14, 6, 6, 1);
    b.ellipse(12, 12, 2, 2, 8);
    b.rect(15, 2, 2, 6, 3);
    b.rect(15, 24, 2, 6, 3);
    b.rect(2, 15, 6, 2, 3);
    b.rect(24, 15, 6, 2, 3);
    b.rect(5, 5, 4, 2, 3);
    b.rect(23, 5, 4, 2, 3);
    b.rect(5, 25, 4, 2, 3);
    b.rect(23, 25, 4, 2, 3);
    b.outline(5, false);
    return b;
}

Bitmap kelpArt() {
    Bitmap b(16, 48);
    b.line(8, 46, 6, 30, 1, 3);
    b.line(6, 30, 10, 16, 2, 3);
    b.line(10, 16, 5, 2, 1, 2);
    b.ellipse(5, 4, 4, 3, 3);
    return b;
}

Bitmap gateArt() {
    Bitmap b(28, 96);
    b.rect(4, 8, 16, 84, 2);
    b.rect(8, 12, 8, 76, 1);
    b.rect(6, 4, 14, 8, 3);
    b.ellipse(13, 20, 3, 3, 4);
    b.outline(5, false);
    return b;
}

Bitmap bubbleArt() {
    Bitmap b(12, 12);
    b.ellipse(6, 6, 4, 4, 1);
    b.ellipse(5, 5, 1, 1, 8);
    return b;
}

Bitmap fishArt() {
    Bitmap b(28, 14);
    b.ellipse(14, 7, 9, 5, 1);
    b.poly({{22, 7}, {27, 2}, {27, 12}}, 2);
    b.ellipse(8, 6, 1, 1, 5);
    b.outline(5, false);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(14, 15, 15);
    const uint16_t shadow = gs::rgb4(1, 2, 3);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 12, 14), gs::rgb4(15, 12, 4), gs::rgb4(15, 5, 3), gs::rgb4(4, 14, 8),
                          gs::rgb4(6, 10, 15), shadow});
    setPal(vdp, PAL_SUB, {0, gs::rgb4(6, 13, 12), gs::rgb4(3, 8, 9), gs::rgb4(8, 14, 13), gs::rgb4(15, 14, 6),
                          gs::rgb4(1, 2, 3), gs::rgb4(11, 12, 12), gs::rgb4(4, 6, 7), gs::rgb4(15, 15, 14),
                          gs::rgb4(12, 4, 3)});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(12, 7, 4), gs::rgb4(8, 4, 3), gs::rgb4(14, 9, 5), gs::rgb4(15, 13, 8),
                           gs::rgb4(1, 1, 2), gs::rgb4(10, 8, 7), gs::rgb4(6, 5, 4), gs::rgb4(15, 12, 8),
                           gs::rgb4(15, 6, 2)});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(6, 6, 7), gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 8), 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_MINE, {0, gs::rgb4(5, 6, 6), gs::rgb4(3, 4, 4), gs::rgb4(14, 3, 2), 0, gs::rgb4(1, 1, 2), 0, 0,
                           gs::rgb4(12, 13, 12)});
    setPal(vdp, PAL_KELP, {0, gs::rgb4(2, 8, 4), gs::rgb4(3, 11, 5), gs::rgb4(6, 13, 6)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(10, 11, 8), gs::rgb4(6, 7, 5), gs::rgb4(14, 13, 6), gs::rgb4(15, 14, 4),
                           gs::rgb4(1, 2, 2)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(10, 14, 15), 0, 0, 0, 0, 0, 0, gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_FISH, {0, gs::rgb4(12, 10, 4), gs::rgb4(14, 8, 3), 0, 0, gs::rgb4(1, 1, 2)});

    art.sub = gs::uploadMipped(vdp, subArt(false));
    art.crew = gs::uploadMipped(vdp, subArt(true));
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.mine = gs::uploadMipped(vdp, mineArt());
    art.kelp = gs::uploadMipped(vdp, kelpArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.bubble = gs::uploadMipped(vdp, bubbleArt());
    art.fish = gs::uploadMipped(vdp, fishArt());
    gs::TextStyle big{3, 1, 0, 0, 1};
    art.banner = gs::uploadMipped(vdp, gs::textBitmap("SUBPASS", big));
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(1, 3, 6));
}

}  // namespace subpass
