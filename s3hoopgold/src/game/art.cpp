#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace hoopgold {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 0, 0);
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

gs::Image phrase(gs::VDP& vdp, const char* s, int scale) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, 1, 2, 0, 1}));
}

gs::Bitmap ballArt(int spin) {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 8.6f, 8.6f, 1);
    b.ellipse(8, 7.5f, 3.1f, 2.2f, 2);
    b.line(10, 2, 10, 18, 3, 1.3f);
    if (spin == 0) {
        b.line(3, 6, 17, 14, 3, 1.2f);
        b.line(3, 14, 17, 6, 3, 1.2f);
    } else {
        b.line(3, 10, 17, 10, 3, 1.3f);
        b.line(6, 3, 5, 17, 3, 1.1f);
        b.line(14, 3, 15, 17, 3, 1.1f);
    }
    b.ellipse(10, 10, 8.6f, 8.6f, 1);
    b.ellipse(8, 7.5f, 3.1f, 2.2f, 2);
    if (spin == 0) {
        b.line(10, 2, 10, 18, 3, 1.2f);
        b.line(3, 6, 17, 14, 3, 1.15f);
        b.line(3, 14, 17, 6, 3, 1.15f);
    } else {
        b.line(10, 2, 10, 18, 3, 1.2f);
        b.line(3, 10, 17, 10, 3, 1.2f);
    }
    b.outline(4, false);
    return b;
}

gs::Bitmap playerArt(bool shoot) {
    gs::Bitmap b(36, 54);
    b.ellipse(16, 9, 6.2f, 6.0f, 1);
    b.ellipse(16, 6.5f, 6.4f, 3.6f, 5);
    b.rect(11, 5, 11, 3, 8);
    b.set(13, 9, 7);
    b.set(18, 9, 7);
    b.rect(10, 16, 13, 14, 3);
    b.rect(10, 16, 4, 14, 4);
    b.rect(14, 20, 2, 7, 6);
    b.rect(17, 20, 2, 7, 6);
    b.rect(14, 20, 5, 2, 6);
    b.rect(14, 23, 5, 2, 6);
    b.rect(9, 30, 15, 8, 10);
    b.rect(16, 30, 2, 8, 4);
    if (shoot) {
        b.line(22, 18, 33, 11, 1, 3.4f);
        b.ellipse(33, 10, 2.3f, 2.2f, 1);
        b.line(11, 20, 5, 28, 2, 3.0f);
    } else {
        b.line(22, 20, 30, 30, 1, 3.2f);
        b.ellipse(30, 31, 2.2f, 2.1f, 1);
        b.line(11, 20, 6, 27, 2, 3.0f);
    }
    b.rect(11, 38, 5, 11, 2);
    b.rect(19, 38, 5, 11, 2);
    b.rect(9, 47, 8, 4, 9);
    b.rect(18, 47, 8, 4, 9);
    b.rect(9, 47, 8, 1, 6);
    b.rect(18, 47, 8, 1, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap rimArt() {
    gs::Bitmap b(52, 16);
    b.ellipse(26, 8, 24, 6.4f, 3);
    b.ellipse(26, 8, 20, 4.6f, 2);
    b.ellipse(26, 7.2f, 16, 3.2f, 1);
    b.ellipse(26, 8, 12.5f, 2.1f, 0);
    b.ellipse(18, 6, 3.2f, 1.3f, 4);
    return b;
}

gs::Bitmap netArt(int sway) {
    gs::Bitmap b(36, 32);
    float lean = sway ? 3.5f : 0.f;
    for (int i = 0; i < 7; i++) {
        float x0 = 4.f + i * 4.6f;
        float x1 = 8.f + i * 3.1f + lean;
        b.line(x0, 1, x1, 30, 1, 1.15f);
    }
    for (int i = 0; i < 7; i++) {
        float x0 = 32.f - i * 4.6f;
        float x1 = 28.f - i * 3.1f + lean;
        b.line(x0, 1, x1, 30, 2, 1.05f);
    }
    for (int y = 4; y < 30; y += 5) {
        float t = (y - 4) / 26.f;
        float left = 4.f + t * (8.f + lean - 4.f);
        float right = 32.f + t * (28.f + lean - 32.f);
        b.line(left, float(y), right, float(y) + (sway ? 1.f : 0.f), 1, 1.05f);
    }
    return b;
}

gs::Bitmap boardArt() {
    gs::Bitmap b(48, 40);
    b.rect(1, 1, 46, 38, 2);
    b.rect(3, 3, 42, 34, 1);
    b.rect(14, 6, 20, 16, 3);
    b.rect(16, 8, 16, 12, 1);
    b.rect(22, 22, 4, 10, 3);
    b.rect(1, 1, 46, 3, 5);
    b.rect(1, 36, 46, 3, 4);
    b.outline(4, false);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(12, 72);
    b.rect(2, 0, 8, 72, 1);
    b.rect(3, 0, 3, 72, 2);
    b.rect(8, 0, 2, 72, 3);
    for (int y = 6; y < 70; y += 12) b.rect(2, y, 8, 2, 4);
    return b;
}

gs::Bitmap chipArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6.2f, 6.2f, 5);
    b.ellipse(7, 7, 5.0f, 5.0f, 2);
    b.ellipse(5.2f, 5.0f, 1.8f, 1.3f, 1);
    return b;
}

gs::Bitmap fenceArt() {
    gs::Bitmap b(80, 28);
    b.rect(0, 8, 80, 16, 1);
    for (int x = 2; x < 80; x += 6) {
        b.rect(x, 2, 2, 24, 2);
        b.rect(x, 2, 2, 2, 3);
    }
    b.rect(0, 8, 80, 2, 3);
    b.rect(0, 20, 80, 2, 3);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 6.4f, 6.4f, 1);
    b.ellipse(9, 9, 2.4f, 2.0f, 2);
    for (int i = 0; i < 8; i++) {
        float a = i * 0.785398f;
        float c = 11.f + std::cos(a) * 9.2f;
        float s = 11.f + std::sin(a) * 9.2f;
        b.line(11, 11, c, s, 3, 1.2f);
    }
    b.ellipse(11, 11, 5.2f, 5.2f, 1);
    b.ellipse(9.2f, 9.2f, 1.8f, 1.4f, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(18, 8);
    b.ellipse(9, 4, 8.f, 2.6f, 1);
    return b;
}

gs::Bitmap blotArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3.2f, 3.2f, 1);
    return b;
}

gs::Bitmap bracketArt() {
    gs::Bitmap b(12, 14);
    b.rect(1, 1, 3, 2, 1);
    b.rect(8, 1, 3, 2, 1);
    b.rect(1, 11, 3, 2, 1);
    b.rect(8, 11, 3, 2, 1);
    b.rect(1, 1, 2, 4, 1);
    b.rect(9, 1, 2, 4, 1);
    b.rect(1, 9, 2, 4, 1);
    b.rect(9, 9, 2, 4, 1);
    b.ellipse(6, 7, 1.4f, 1.4f, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 4), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 15, 11), gs::rgb4(15, 12, 3), gs::rgb4(11, 8, 1), gs::rgb4(15, 14, 8),
                           gs::rgb4(5, 3, 0), gs::rgb4(13, 9, 2), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 0)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 15, 13), gs::rgb4(13, 12, 8), gs::rgb4(9, 8, 5), gs::rgb4(15, 14, 12),
                            gs::rgb4(5, 4, 3), gs::rgb4(11, 10, 7), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(14, 6, 1), gs::rgb4(15, 11, 5), gs::rgb4(6, 2, 1), gs::rgb4(3, 1, 0),
                           gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(14, 9, 6), gs::rgb4(10, 6, 4), gs::rgb4(15, 12, 3), gs::rgb4(9, 7, 1),
                          gs::rgb4(3, 2, 3), gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2), gs::rgb4(13, 2, 3),
                          gs::rgb4(12, 9, 2), gs::rgb4(4, 3, 8), 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_BOARD, {0, gs::rgb4(14, 14, 15), gs::rgb4(11, 11, 13), gs::rgb4(12, 3, 2), gs::rgb4(4, 4, 6),
                            gs::rgb4(15, 13, 6), gs::rgb4(8, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(8, 8, 10), gs::rgb4(13, 13, 14), gs::rgb4(4, 4, 6), gs::rgb4(6, 6, 8),
                           gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_NET, {0, gs::rgb4(13, 13, 12), gs::rgb4(8, 8, 7), gs::rgb4(15, 15, 14), gs::rgb4(5, 5, 4), 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 8), gs::rgb4(1, 5, 2), gs::rgb4(12, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, gs::rgb4(0, 3, 1)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 5, 4), gs::rgb4(5, 1, 1), gs::rgb4(15, 10, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_COURT, {0, gs::rgb4(14, 14, 12), gs::rgb4(6, 5, 5), gs::rgb4(9, 8, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_METER, {0, gs::rgb4(5, 5, 7), gs::rgb4(8, 15, 7), gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 3), 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 13), gs::rgb4(12, 8, 2), gs::rgb4(6, 6, 8), 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_FENCE, {0, gs::rgb4(3, 4, 6), gs::rgb4(7, 8, 10), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_WORD, {0, gs::rgb4(15, 14, 6), gs::rgb4(4, 2, 1), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_SHADOW, {0, gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    loadFont(vdp, art);
    art.ball[0] = gs::uploadMipped(vdp, ballArt(0));
    art.ball[1] = gs::uploadMipped(vdp, ballArt(1));
    art.player[0] = gs::uploadMipped(vdp, playerArt(false));
    art.player[1] = gs::uploadMipped(vdp, playerArt(true));
    art.rim = gs::uploadMipped(vdp, rimArt());
    art.net[0] = gs::uploadMipped(vdp, netArt(0));
    art.net[1] = gs::uploadMipped(vdp, netArt(1));
    art.board = gs::uploadMipped(vdp, boardArt());
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.chip = gs::uploadMipped(vdp, chipArt());
    art.fence = gs::uploadMipped(vdp, fenceArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
    art.blot = gs::uploadImage(vdp, blotArt());
    art.bracket = gs::uploadImage(vdp, bracketArt());
    art.hoop = phrase(vdp, "HOOP", 3);
    art.doubled = phrase(vdp, "DOUBLE", 3);
    art.noDouble = phrase(vdp, "NO DOUBLE", 2);
    art.swish = phrase(vdp, "SWISH", 2);
    art.count = phrase(vdp, "COUNT", 2);
    art.bank = phrase(vdp, "BANK", 2);
    art.rimWord = phrase(vdp, "RIM", 2);
    art.shortWord = phrase(vdp, "SHORT", 2);
    art.longWord = phrase(vdp, "LONG", 2);
    art.airWord = phrase(vdp, "AIR", 2);
    art.notWord = phrase(vdp, "NOT DOUBLE", 2);
    art.x2 = phrase(vdp, "X2", 2);
    art.x1 = phrase(vdp, "X1", 2);
}

}  // namespace hoopgold
