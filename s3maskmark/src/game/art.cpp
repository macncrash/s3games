#include "game/art.h"

#include <initializer_list>

namespace maskmark {
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
    b.ellipse(40, 52, 32.f, 42.f, 3);
    b.ellipse(40, 46, 26.f, 34.f, 4);
    b.ellipse(28, 46, 8.f, 5.f, 2);
    b.ellipse(52, 46, 8.f, 5.f, 2);
    b.ellipse(28, 46, 3.2f, 1.6f, 1);
    b.ellipse(52, 46, 3.2f, 1.6f, 1);
    b.rect(38, 52, 4, 12, 5);
    b.ellipse(40, 74, 10.f, 4.f, 2);
    b.ellipse(40, 74, 6.f, 1.6f, 1);
    b.rect(22, 18, 36, 4, 6);
    b.outline(1, false);
}

void paintChisel(gs::Bitmap& b) {
    b.rect(5, 0, 4, 14, 3);
    b.poly({{3, 12}, {11, 12}, {7, 20}}, 4);
    b.rect(4, 0, 6, 3, 2);
    b.outline(1, false);
}

void paintStroke(gs::Bitmap& b) {
    b.rect(0, 2, 28, 4, 2);
    b.rect(2, 3, 24, 2, 3);
}

void paintStamp(gs::Bitmap& b) {
    b.rect(2, 2, 28, 28, 3);
    b.rect(6, 6, 20, 20, 2);
    b.rect(10, 10, 4, 12, 4);
    b.rect(16, 10, 6, 4, 4);
    b.rect(16, 16, 4, 6, 4);
    b.outline(1, false);
}

void paintLamp(gs::Bitmap& b) {
    b.ellipse(12, 10, 8.f, 6.f, 3);
    b.rect(10, 14, 4, 8, 2);
    b.rect(6, 21, 12, 3, 4);
    b.outline(1, false);
}

void paintPot(gs::Bitmap& b) {
    b.ellipse(12, 16, 10.f, 8.f, 3);
    b.ellipse(12, 10, 7.f, 3.f, 2);
    b.ellipse(12, 10, 4.f, 1.6f, 4);
    b.rect(10, 4, 4, 6, 5);
    b.outline(1, false);
}

void paintHands(gs::Bitmap& b) {
    b.ellipse(10, 14, 8.f, 6.f, 3);
    b.rect(4, 16, 14, 8, 3);
    b.rect(6, 8, 3, 8, 4);
    b.rect(10, 6, 3, 10, 4);
    b.rect(14, 9, 3, 8, 4);
    b.outline(1, false);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_INK, gs::rgb4(14, 13, 11), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(4, 2, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 14, 12), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 7, 6), gs::rgb4(2, 1, 1));
    setPal(vdp, PAL_CLAY, {0, gs::rgb4(3, 2, 2), gs::rgb4(6, 4, 3), gs::rgb4(12, 9, 7), gs::rgb4(15, 13, 10),
                           gs::rgb4(8, 6, 5), gs::rgb4(9, 7, 5), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(2, 1, 0), gs::rgb4(5, 3, 1), gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3)});
    setPal(vdp, PAL_TOOL, {0, gs::rgb4(1, 1, 1), gs::rgb4(4, 4, 5), gs::rgb4(10, 10, 11), gs::rgb4(14, 13, 8),
                           gs::rgb4(8, 8, 9)});
    setPal(vdp, PAL_SEAL, {0, gs::rgb4(2, 0, 0), gs::rgb4(8, 1, 1), gs::rgb4(12, 2, 2), gs::rgb4(15, 8, 4),
                           gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_SMEAR, {0, gs::rgb4(3, 0, 0), gs::rgb4(10, 2, 1), gs::rgb4(14, 4, 2), gs::rgb4(15, 8, 6)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(3, 2, 1), gs::rgb4(10, 7, 4), gs::rgb4(14, 10, 7), gs::rgb4(8, 5, 3)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(3, 2, 0), gs::rgb4(8, 5, 1), gs::rgb4(15, 12, 4), gs::rgb4(15, 14, 8),
                           gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_CUT, {0, gs::rgb4(4, 3, 0), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_BENCH, {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(0, 0, 0)});
    setPal(vdp, PAL_WALL, {0, gs::rgb4(4, 3, 3)});

    loadFont(vdp, art);
    gs::Bitmap mask(80, 104);
    paintMask(mask);
    art.mask = up(vdp, mask);
    gs::Bitmap chisel(14, 22);
    paintChisel(chisel);
    art.chisel = up(vdp, chisel);
    gs::Bitmap stroke(28, 8);
    paintStroke(stroke);
    art.stroke = up(vdp, stroke);
    gs::Bitmap stamp(32, 32);
    paintStamp(stamp);
    art.stamp = up(vdp, stamp);
    gs::Bitmap lamp(24, 26);
    paintLamp(lamp);
    art.lamp = up(vdp, lamp);
    gs::Bitmap pot(24, 28);
    paintPot(pot);
    art.pot = up(vdp, pot);
    gs::Bitmap hands(24, 28);
    paintHands(hands);
    art.hands = up(vdp, hands);
}

}  // namespace maskmark
