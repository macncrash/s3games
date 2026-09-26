#include "game/art.h"

#include <initializer_list>

namespace hooptape {
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

// Indices match PAL_YOU: 1 outline, 2 skin, 3 hair, 4 jersey, 5 white, 6 shorts, 7 shoes, 8 band, 9 gold.
gs::Bitmap playerArt() {
    gs::Bitmap b(36, 58);
    b.rect(6, 52, 9, 4, 7);
    b.rect(21, 52, 9, 4, 7);
    b.rect(7, 48, 7, 5, 5);
    b.rect(22, 48, 7, 5, 5);
    b.line(11, 38, 10, 50, 2, 3.2f);
    b.line(23, 38, 25, 50, 2, 3.2f);
    b.rect(8, 32, 18, 8, 6);
    b.rect(15, 32, 3, 8, 9);
    b.rect(9, 18, 16, 15, 4);
    b.rect(9, 18, 16, 3, 8);
    b.rect(14, 23, 6, 2, 9);
    b.rect(14, 25, 2, 6, 9);
    b.rect(18, 25, 2, 6, 9);
    b.rect(14, 29, 6, 2, 9);
    b.ellipse(18, 12, 6.2f, 6.0f, 2);
    b.ellipse(18, 8.4f, 6.4f, 3.4f, 3);
    b.rect(12, 10, 12, 2, 8);
    b.rect(22, 12, 2, 2, 1);
    b.line(22, 22, 33, 14, 2, 2.8f);
    b.ellipse(33.2f, 13.2f, 2.1f, 1.8f, 2);
    b.line(12, 24, 5, 33, 2, 2.5f);
    b.outline(1, false);
    return b;
}

gs::Bitmap ballArt(int frame) {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 9.4f, 9.4f, 1);
    b.ellipse(8, 8, 3.0f, 2.1f, 2);
    if (frame == 0) {
        b.line(11, 2, 11, 20, 3, 1.05f);
        b.line(3, 7, 19, 15, 3, 1.0f);
        b.line(3, 15, 19, 7, 3, 1.0f);
    } else {
        b.line(2, 11, 20, 11, 3, 1.05f);
        b.line(6, 3, 8, 19, 3, 1.0f);
        b.line(16, 3, 14, 19, 3, 1.0f);
    }
    b.outline(4, false);
    return b;
}

gs::Bitmap rimArt() {
    gs::Bitmap b(44, 14);
    b.ellipse(18, 7, 15.5f, 5.2f, 1);
    b.ellipse(18, 8, 15.2f, 3.6f, 2);
    b.rect(32, 5, 11, 4, 3);
    b.rect(34, 6, 7, 2, 4);
    return b;
}

gs::Bitmap netArt(int sway) {
    gs::Bitmap b(32, 30);
    float lean = sway ? 2.4f : -0.4f;
    for (int i = 0; i < 7; i++) {
        float x0 = 3.f + float(i) * 4.0f;
        float x1 = 8.f + float(i) * 2.3f + lean;
        b.line(x0, 1, x1, 28, (i & 1) ? 2 : 1, 1.05f);
    }
    for (int y = 4; y < 28; y += 5) {
        float shrink = float(y) * 0.22f;
        b.line(3.f + shrink, float(y), 29.f - shrink + lean, float(y + sway), 3, 1.0f);
    }
    return b;
}

gs::Bitmap boardArt() {
    gs::Bitmap b(50, 36);
    b.rect(1, 1, 48, 34, 1);
    b.rect(1, 1, 48, 3, 2);
    b.rect(1, 32, 48, 3, 2);
    b.rect(1, 1, 3, 34, 2);
    b.rect(46, 1, 3, 34, 2);
    b.rect(16, 12, 16, 12, 3);
    b.rect(21, 16, 6, 5, 1);
    b.rect(34, 6, 10, 10, 4);
    b.rect(36, 8, 6, 6, 5);
    for (int i = 0; i < 4; i++) b.rect(6.f + float(i) * 11.f, 4, 2, 2, 6);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(8, 12);
    b.rect(2, 0, 5, 12, 5);
    b.rect(3, 0, 2, 12, 4);
    return b;
}

gs::Bitmap armArt() {
    gs::Bitmap b(26, 6);
    b.rect(0, 1, 26, 4, 3);
    b.rect(0, 1, 26, 1, 4);
    return b;
}

gs::Bitmap baseArt() {
    gs::Bitmap b(18, 8);
    b.rect(1, 2, 16, 5, 5);
    b.rect(4, 0, 10, 3, 3);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6.6f, 6.6f, 1);
    b.ellipse(8, 8, 2.6f, 2.6f, 2);
    return b;
}

gs::Bitmap boxArt() {
    gs::Bitmap b(16, 16);
    b.rect(1, 1, 14, 14, 1);
    b.rect(4, 4, 8, 8, 2);
    return b;
}

gs::Bitmap blotArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 9.f, 3.f, 1);
    return b;
}

gs::Bitmap windowArt() {
    gs::Bitmap b(28, 18);
    b.rect(0, 0, 28, 18, 2);
    b.rect(2, 2, 11, 6, 1);
    b.rect(15, 2, 11, 6, 1);
    b.rect(2, 10, 11, 6, 3);
    b.rect(15, 10, 11, 6, 3);
    return b;
}

gs::Bitmap keyArt() {
    gs::Bitmap b(12, 10);
    b.rect(0, 0, 12, 10, 1);
    return b;
}

gs::Bitmap tapeArt() {
    gs::Bitmap b(78, 46);
    b.rect(0, 0, 78, 46, 3);
    b.rect(0, 0, 78, 11, 4);
    b.rect(0, 0, 3, 46, 2);
    for (int y = 14; y < 44; y += 4) b.rect(6, y, 2, 1, 2);
    gs::Bitmap head = gs::textBitmap("TAPE", {1, 1, 0, 0, 1});
    b.blit(head, 26, 2);
    const char* row[] = {"SWISH 2", "BANK  3", "FREE  1"};
    for (int i = 0; i < 3; i++) {
        gs::Bitmap t = gs::textBitmap(row[i], {1, 1, 0, 0, 0});
        b.blit(t, 12, 14 + i * 10);
    }
    return b;
}

gs::Bitmap drawerArt() {
    gs::Bitmap b(220, 26);
    b.rect(0, 0, 220, 26, 3);
    b.rect(0, 0, 220, 4, 2);
    b.rect(0, 22, 220, 4, 4);
    b.rect(0, 0, 3, 26, 4);
    b.rect(217, 0, 3, 26, 4);
    gs::Bitmap word = gs::textBitmap("DRAWER", {1, 1, 0, 0, 0});
    b.blit(word, 8, 9);
    return b;
}

gs::Bitmap slotArt() {
    gs::Bitmap b(40, 14);
    b.rect(0, 0, 40, 14, 5);
    b.rect(2, 2, 36, 10, 1);
    return b;
}

gs::Bitmap slipArt(const char* label) {
    gs::Bitmap b(58, 16);
    b.rect(0, 0, 58, 16, 3);
    b.rect(0, 0, 58, 3, 5);
    b.rect(0, 14, 58, 2, 4);
    gs::Bitmap t = gs::textBitmap(label, {1, 1, 0, 0, 0});
    int x = t.w < 54 ? (58 - t.w) / 2 : 2;
    b.blit(t, x, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_INK, gs::rgb4(14, 13, 11), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(4, 2, 0));
    textPal(vdp, PAL_GOOD, gs::rgb4(7, 15, 8), gs::rgb4(0, 3, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 6, 4), gs::rgb4(4, 0, 0));
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 8, 5), gs::rgb4(3, 2, 1), gs::rgb4(2, 3, 8), gs::rgb4(14, 14, 13),
            gs::rgb4(13, 5, 1), gs::rgb4(2, 2, 2), gs::rgb4(13, 13, 12), gs::rgb4(13, 10, 2)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(14, 6, 1), gs::rgb4(15, 11, 4), gs::rgb4(2, 1, 1), gs::rgb4(6, 2, 0)});
    setPal(vdp, PAL_IRON,
           {0, gs::rgb4(13, 5, 1), gs::rgb4(8, 3, 1), gs::rgb4(7, 7, 8), gs::rgb4(13, 13, 14), gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_BOARD,
           {0, gs::rgb4(12, 13, 14), gs::rgb4(4, 4, 5), gs::rgb4(12, 2, 2), gs::rgb4(2, 4, 12), gs::rgb4(6, 8, 14),
            gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_NET, {0, gs::rgb4(14, 14, 13), gs::rgb4(9, 9, 10), gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_PAPER,
           {0, gs::rgb4(3, 2, 1), gs::rgb4(7, 5, 3), gs::rgb4(14, 12, 8), gs::rgb4(9, 7, 5), gs::rgb4(12, 3, 2),
            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(2, 1, 1), gs::rgb4(11, 7, 3), gs::rgb4(8, 5, 2), gs::rgb4(4, 2, 1), gs::rgb4(1, 1, 2),
            gs::rgb4(12, 9, 3)});
    setPal(vdp, PAL_MARKW, {0, gs::rgb4(14, 14, 13), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_MARKB, {0, gs::rgb4(3, 6, 13), gs::rgb4(1, 1, 3)});
    setPal(vdp, PAL_MARKG, {0, gs::rgb4(4, 13, 6), gs::rgb4(0, 3, 1)});
    setPal(vdp, PAL_MARKR, {0, gs::rgb4(13, 4, 3), gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(11, 13, 15), gs::rgb4(3, 3, 4), gs::rgb4(6, 8, 12)});

    loadFont(vdp, art);
    art.player = gs::uploadMipped(vdp, playerArt());
    art.ball[0] = gs::uploadMipped(vdp, ballArt(0));
    art.ball[1] = gs::uploadMipped(vdp, ballArt(1));
    art.rim = gs::uploadImage(vdp, rimArt());
    art.net[0] = gs::uploadImage(vdp, netArt(0));
    art.net[1] = gs::uploadImage(vdp, netArt(1));
    art.board = gs::uploadImage(vdp, boardArt());
    art.pole = gs::uploadImage(vdp, poleArt());
    art.arm = gs::uploadImage(vdp, armArt());
    art.base = gs::uploadImage(vdp, baseArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    art.box = gs::uploadImage(vdp, boxArt());
    art.blot = gs::uploadImage(vdp, blotArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
    art.window = gs::uploadImage(vdp, windowArt());
    art.key = gs::uploadImage(vdp, keyArt());
    art.tape = gs::uploadImage(vdp, tapeArt());
    art.drawer = gs::uploadImage(vdp, drawerArt());
    art.slot = gs::uploadImage(vdp, slotArt());
    art.logo = phrase(vdp, "HOOPTAPE", 2, 1, 2);
    art.matchW = phrase(vdp, "MATCH", 2, 1, 2);
    art.openW = phrase(vdp, "OPEN", 2, 1, 2);
    const char* slips[] = {"SWISH 2", "BANK 3", "FREE 1"};
    for (int i = 0; i < kTapeN; i++) art.slip[i] = gs::uploadImage(vdp, slipArt(slips[i]));
}

}  // namespace hooptape
