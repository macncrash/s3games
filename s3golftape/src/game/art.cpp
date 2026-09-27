#include "game/art.h"

#include <initializer_list>

namespace golftape {
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

gs::Image phrase(gs::VDP& vdp, const char* s, int scale, int ink, int edge) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, ink, edge, 0, 1}));
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

gs::Bitmap courseArt() {
    gs::Bitmap b(320, 96);
    b.rect(0, 58, 320, 38, 2);
    b.rect(0, 52, 320, 8, 1);
    b.rect(8, 46, 36, 10, 4);
    b.rect(14, 40, 18, 8, 3);
    b.rect(70, 48, 28, 14, 9);
    b.ellipse(150, 54, 22, 8, 3);
    b.rect(188, 50, 22, 12, 4);
    b.ellipse(236, 50, 46, 14, 5);
    b.ellipse(236, 50, 40, 10, 6);
    b.ellipse(262, 50, 3.2f, 2.4f, 7);
    b.rect(248, 36, 2, 16, 8);
    b.ellipse(40, 28, 16, 22, 3);
    b.ellipse(40, 16, 22, 12, 1);
    b.ellipse(292, 30, 14, 20, 3);
    b.ellipse(292, 18, 18, 10, 1);
    return b;
}

gs::Bitmap golferArt(int frame) {
    gs::Bitmap b(36, 48);
    b.rect(8, 40, 6, 4, 5);
    b.rect(18, 40, 6, 4, 5);
    b.line(12, 28, 10, 40, 4, 3.0f);
    b.line(20, 28, 22, 40, 4, 3.0f);
    b.rect(8, 22, 16, 10, 4);
    b.rect(10, 14, 12, 10, 3);
    b.ellipse(16, 8, 6.0f, 5.5f, 2);
    b.ellipse(16, 5, 6.2f, 3.0f, 7);
    if (frame == 0) {
        b.line(18, 18, 32, 28, 6, 1.6f);
        b.line(30, 26, 34, 34, 6, 1.4f);
    } else {
        b.line(14, 16, 4, 8, 6, 1.6f);
        b.line(6, 8, 2, 4, 6, 1.4f);
    }
    b.rect(20, 16, 3, 3, 2);
    return b;
}

gs::Bitmap ballArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 5.5f, 5.5f, 1);
    b.ellipse(5, 5, 2.0f, 1.4f, 2);
    b.line(7, 2, 7, 12, 3, 0.8f);
    return b;
}

gs::Bitmap flagArt(int frame) {
    gs::Bitmap b(22, 28);
    b.rect(4, 4, 2, 22, 1);
    int lean = frame ? 2 : 0;
    b.poly({{6, 4}, {18.f + lean, 8.f}, {6, 14}}, 2);
    return b;
}

gs::Bitmap paperArt() {
    gs::Bitmap b(108, 72);
    b.rect(0, 0, 108, 72, 1);
    b.rect(2, 2, 104, 68, 2);
    b.rect(6, 8, 96, 2, 3);
    return b;
}

gs::Bitmap drawerArt() {
    gs::Bitmap b(300, 28);
    b.rect(0, 0, 300, 28, 1);
    b.rect(4, 4, 90, 20, 2);
    b.rect(104, 4, 90, 20, 2);
    b.rect(204, 4, 90, 20, 2);
    return b;
}

gs::Bitmap slipArt(int ink) {
    gs::Bitmap b(78, 16);
    b.rect(0, 0, 78, 16, 1);
    b.rect(2, 2, 74, 12, ink);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& a) {
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 3));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(4, 2, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(8, 15, 8), gs::rgb4(0, 3, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 6, 5), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 15, 12), gs::rgb4(1, 3, 6));

    setPal(vdp, PAL_PAPER, {0, gs::rgb4(14, 13, 10), gs::rgb4(12, 11, 8), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_WORLD,
           {0, gs::rgb4(6, 12, 4), gs::rgb4(4, 9, 3), gs::rgb4(2, 7, 2), gs::rgb4(12, 10, 6), gs::rgb4(5, 13, 5),
            gs::rgb4(3, 10, 4), gs::rgb4(1, 1, 1), gs::rgb4(14, 14, 12), gs::rgb4(3, 6, 12)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 12, 13), gs::rgb4(8, 8, 9)});
    setPal(vdp, PAL_MAN,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 9, 6), gs::rgb4(14, 14, 15), gs::rgb4(2, 3, 8), gs::rgb4(3, 2, 1),
            gs::rgb4(10, 10, 11), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(15, 15, 14), gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_SLIP, {0, gs::rgb4(15, 14, 9), gs::rgb4(4, 8, 3), gs::rgb4(8, 6, 2), gs::rgb4(3, 5, 10)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(7, 4, 2), gs::rgb4(4, 2, 1)});

    loadFont(vdp, a);
    a.course = gs::uploadImage(vdp, courseArt());
    a.paper = gs::uploadImage(vdp, paperArt());
    a.drawer = gs::uploadImage(vdp, drawerArt());
    a.title = phrase(vdp, "GOLFTAPE", 3, 1, 2);
    a.ball = gs::uploadMipped(vdp, ballArt());
    a.flag[0] = gs::uploadMipped(vdp, flagArt(0));
    a.flag[1] = gs::uploadMipped(vdp, flagArt(1));
    a.golfer[0] = gs::uploadMipped(vdp, golferArt(0));
    a.golfer[1] = gs::uploadMipped(vdp, golferArt(1));
    a.slip[0] = gs::uploadImage(vdp, slipArt(2));
    a.slip[1] = gs::uploadImage(vdp, slipArt(3));
    a.slip[2] = gs::uploadImage(vdp, slipArt(4));
    gs::Bitmap dot(4, 4);
    dot.ellipse(2, 2, 1.6f, 1.6f, 1);
    a.dot = gs::uploadImage(vdp, dot);
    vdp.setFogColor(gs::rgb4(6, 8, 10));
}

}  // namespace golftape
