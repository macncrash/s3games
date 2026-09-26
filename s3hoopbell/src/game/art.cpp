#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace hoopbell {
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

gs::Image phrase(gs::VDP& vdp, const char* s, int scale) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, 1, 2, 0, 1}));
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

gs::Bitmap playerArt() {
    gs::Bitmap b(36, 58);
    b.rect(6, 52, 9, 4, 7);
    b.rect(21, 52, 9, 4, 7);
    b.line(11, 40, 10, 52, 3, 3.4f);
    b.line(22, 40, 25, 52, 3, 3.4f);
    b.rect(8, 33, 17, 9, 6);
    b.rect(15, 33, 2, 9, 5);
    b.rect(11, 19, 14, 16, 4);
    b.rect(11, 19, 4, 16, 5);
    b.rect(18, 23, 3, 8, 8);
    b.ellipse(18, 12, 6.4f, 6.2f, 2);
    b.ellipse(18, 9.2f, 6.6f, 3.6f, 9);
    b.rect(22, 12, 2, 2, 1);
    b.line(23, 22, 33, 6, 2, 3.1f);
    b.ellipse(33.5f, 5.5f, 2.2f, 2.0f, 2);
    b.line(12, 24, 5, 34, 2, 2.7f);
    b.outline(1, false);
    return b;
}

gs::Bitmap ballArt(int frame) {
    gs::Bitmap b(26, 26);
    b.ellipse(13, 13, 11.2f, 11.2f, 1);
    b.ellipse(10, 9, 3.6f, 2.6f, 2);
    b.line(13, 3, 13, 23, 3, 1.15f);
    if (frame == 0) {
        b.line(4, 8, 22, 18, 3, 1.1f);
        b.line(4, 18, 22, 8, 3, 1.1f);
    } else {
        b.line(3, 13, 23, 13, 3, 1.1f);
        b.line(6, 5, 8, 21, 3, 1.05f);
        b.line(20, 5, 18, 21, 3, 1.05f);
    }
    b.outline(4, false);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(32, 40);
    b.rect(14, 0, 4, 7, 4);
    b.rect(9, 5, 14, 3, 3);
    b.ellipse(16, 20, 12.2f, 11.4f, 2);
    b.ellipse(16, 18.5f, 8.2f, 7.6f, 1);
    b.ellipse(16, 22, 5.2f, 4.4f, 3);
    b.ellipse(16, 30.2f, 11.2f, 3.1f, 5);
    b.ellipse(16, 22.5f, 3.6f, 2.8f, 3);
    b.line(10, 14, 12, 24, 6, 1.3f);
    b.outline(4, false);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(8, 14);
    b.line(4, 0, 4, 7, 1, 1.2f);
    b.ellipse(4, 10, 2.6f, 2.6f, 2);
    return b;
}

gs::Bitmap rimArt() {
    gs::Bitmap b(46, 16);
    b.ellipse(20, 8, 16.5f, 5.8f, 1);
    b.ellipse(20, 9.2f, 16.5f, 4.4f, 2);
    b.rect(34, 6, 11, 4, 3);
    b.rect(36, 7, 7, 2, 1);
    return b;
}

gs::Bitmap netArt(int sway) {
    gs::Bitmap b(34, 32);
    float lean = sway ? 2.2f : -0.6f;
    for (int i = 0; i < 7; i++) {
        float x0 = 4.f + float(i) * 4.2f;
        float x1 = 9.f + float(i) * 2.5f + lean;
        b.line(x0, 1, x1, 30, (i & 1) ? 7 : 6, 1.05f);
    }
    for (int y = 5; y < 30; y += 5) {
        float shrink = float(y) * 0.22f;
        b.line(4.f + shrink, float(y), 30.f - shrink + lean, float(y + sway), 7, 1.0f);
    }
    return b;
}

gs::Bitmap boardArt() {
    gs::Bitmap b(52, 38);
    b.rect(1, 1, 50, 36, 1);
    b.rect(1, 1, 50, 3, 2);
    b.rect(1, 34, 50, 3, 2);
    b.rect(1, 1, 3, 36, 2);
    b.rect(48, 1, 3, 36, 2);
    b.rect(16, 8, 20, 16, 3);
    b.rect(23, 13, 6, 6, 4);
    b.rect(25, 15, 2, 2, 1);
    for (int i = 0; i < 4; i++) b.rect(6.f + float(i) * 12.f, 4, 2, 2, 5);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(10, 16);
    b.rect(2, 0, 6, 16, 5);
    b.rect(3, 0, 2, 16, 4);
    return b;
}

gs::Bitmap armArt() {
    gs::Bitmap b(28, 6);
    b.rect(0, 1, 28, 4, 5);
    b.rect(0, 1, 28, 1, 4);
    return b;
}

gs::Bitmap glassArt() {
    const int n = 64;
    gs::Bitmap b(n, n);
    const float c = (n - 1) * 0.5f;
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            float dx = x + 0.5f - c;
            float dy = y + 0.5f - c;
            float r = std::hypot(dx, dy);
            int col = 0;
            if (r <= 6.2f) col = 5;
            else if (r <= 8.4f) col = 6;
            else if (r <= 20.f) col = 1;
            else if (r <= 22.4f) col = 3;
            else if (r <= 25.6f) col = 2;
            else if (r <= 27.4f) col = 4;
            b.set(x, y, col);
        }
    }
    return b;
}

gs::Bitmap yardArt() {
    gs::Bitmap b(210, 64);
    b.rect(8, 18, 78, 36, 5);
    b.rect(8, 14, 82, 8, 7);
    b.rect(86, 26, 36, 28, 5);
    b.rect(86, 22, 40, 6, 7);
    for (int i = 0; i < 3; i++) b.rect(16.f + float(i) * 18.f, 28, 10, 8, 6);
    b.rect(96, 32, 8, 10, 6);
    b.rect(40, 40, 12, 14, 10);
    gs::Bitmap word = gs::textBitmap("YARD", {1, 1, 2, 0, 0});
    b.blit(word, 30, 20);
    b.ellipse(150, 40, 22, 16, 8);
    b.ellipse(168, 36, 16, 14, 9);
    b.rect(156, 44, 4, 16, 4);
    for (int x = 0; x < 210; x += 7) b.rect(float(x), 52, 2, 12, 3);
    b.rect(0, 51, 210, 2, 4);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 8, 8, 3);
    b.ellipse(11, 11, 5.5f, 5.5f, 1);
    b.ellipse(11, 11, 2.4f, 2.4f, 2);
    return b;
}

gs::Bitmap crossArt() {
    gs::Bitmap b(13, 13);
    b.rect(5, 1, 3, 11, 1);
    b.rect(1, 5, 11, 3, 1);
    b.set(6, 6, 0);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3.2f, 3.2f, 1);
    return b;
}

gs::Bitmap blotArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(14, 6);
    b.ellipse(7, 3, 6.2f, 2.2f, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_COURT,
           {0, gs::rgb4(13, 13, 12), gs::rgb4(4, 4, 5), gs::rgb4(8, 7, 6), gs::rgb4(5, 4, 4), gs::rgb4(9, 5, 4),
            gs::rgb4(14, 10, 4), gs::rgb4(5, 3, 3), gs::rgb4(3, 6, 3), gs::rgb4(2, 4, 2), gs::rgb4(6, 4, 3)});
    setPal(vdp, PAL_BOARD,
           {0, gs::rgb4(14, 14, 13), gs::rgb4(6, 7, 8), gs::rgb4(3, 3, 4), gs::rgb4(12, 4, 1), gs::rgb4(8, 8, 8)});
    setPal(vdp, PAL_IRON,
           {0, gs::rgb4(13, 5, 1), gs::rgb4(8, 3, 1), gs::rgb4(6, 6, 7), gs::rgb4(11, 11, 12), gs::rgb4(5, 5, 6),
            gs::rgb4(13, 13, 13), gs::rgb4(7, 7, 8)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(14, 6, 1), gs::rgb4(15, 12, 6), gs::rgb4(4, 1, 1), gs::rgb4(8, 3, 0)});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(1, 1, 1), gs::rgb4(13, 8, 5), gs::rgb4(9, 5, 3), gs::rgb4(14, 11, 2), gs::rgb4(9, 6, 1),
            gs::rgb4(2, 3, 7), gs::rgb4(14, 14, 13), gs::rgb4(12, 2, 2), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(15, 13, 5), gs::rgb4(12, 8, 2), gs::rgb4(5, 3, 1), gs::rgb4(3, 2, 1), gs::rgb4(10, 7, 2),
            gs::rgb4(15, 14, 8)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 13), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 4, 3), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 7), gs::rgb4(0, 2, 1));
    setPal(vdp, PAL_GLASS,
           {0, gs::rgb4(2, 4, 5), gs::rgb4(8, 10, 11), gs::rgb4(13, 5, 1), gs::rgb4(6, 6, 7), gs::rgb4(2, 1, 1),
            gs::rgb4(13, 10, 3)});
    setPal(vdp, PAL_METER, {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 12, 5), gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 10), gs::rgb4(12, 6, 3)});
    setPal(vdp, PAL_ASH, {0, gs::rgb4(6, 6, 6), gs::rgb4(3, 3, 4)});

    loadFont(vdp, art);
    art.player = gs::uploadMipped(vdp, playerArt());
    art.ball[0] = gs::uploadMipped(vdp, ballArt(0));
    art.ball[1] = gs::uploadMipped(vdp, ballArt(1));
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.clapper = gs::uploadImage(vdp, clapperArt());
    art.rim = gs::uploadImage(vdp, rimArt());
    art.net[0] = gs::uploadImage(vdp, netArt(0));
    art.net[1] = gs::uploadImage(vdp, netArt(1));
    art.board = gs::uploadImage(vdp, boardArt());
    art.pole = gs::uploadImage(vdp, poleArt());
    art.arm = gs::uploadImage(vdp, armArt());
    art.glass = gs::uploadImage(vdp, glassArt());
    art.yard = gs::uploadImage(vdp, yardArt());
    art.sun = gs::uploadImage(vdp, sunArt());
    art.cross = gs::uploadImage(vdp, crossArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    art.blot = gs::uploadImage(vdp, blotArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
    art.hoop = phrase(vdp, "HOOP", 3);
    art.bellWord = phrase(vdp, "BELL", 3);
    art.rung = phrase(vdp, "RUNG", 3);
    art.deadW = phrase(vdp, "DEAD", 3);
    art.leaveW = phrase(vdp, "LEAVE", 2);
    art.shortW = phrase(vdp, "SHORT", 2);
    art.hotW = phrase(vdp, "HOT", 2);
    art.ironW = phrase(vdp, "IRON", 2);
    art.missW = phrase(vdp, "MISS", 2);
}

}  // namespace hoopbell
