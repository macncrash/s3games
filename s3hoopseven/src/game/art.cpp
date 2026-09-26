#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace hoopseven {
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
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7.6f, 7.6f, 1);
    b.outline(4, false);
    b.ellipse(7.2f, 6.4f, 2.4f, 1.7f, 2);
    if (spin == 0) {
        b.line(9, 2, 9, 16, 3, 1.15f);
        b.line(3, 6, 15, 13, 3, 1.05f);
        b.line(3, 12, 15, 5, 3, 1.05f);
    } else {
        b.line(9, 2, 9, 16, 3, 1.15f);
        b.line(3, 9, 15, 9, 3, 1.15f);
        b.line(6, 3, 5, 15, 3, 1.0f);
    }
    return b;
}

gs::Bitmap playerArt(bool shoot) {
    gs::Bitmap b(34, 50);
    b.ellipse(15, 8, 5.6f, 5.4f, 1);
    b.rect(10, 4, 11, 3, 5);
    b.rect(10, 4, 11, 1, 6);
    b.set(13, 8, 8);
    b.set(17, 8, 8);
    b.rect(9, 14, 13, 13, 3);
    b.rect(9, 14, 3, 13, 4);
    b.rect(12, 17, 8, 2, 6);
    b.rect(18, 17, 2, 8, 6);
    if (shoot) {
        b.line(21, 16, 31, 8, 1, 3.1f);
        b.ellipse(31, 7.5f, 2.1f, 2.0f, 1);
        b.line(10, 18, 4, 26, 2, 2.8f);
    } else {
        b.line(21, 18, 29, 30, 1, 3.0f);
        b.ellipse(29, 31, 2.0f, 1.9f, 1);
        b.line(10, 18, 5, 25, 2, 2.8f);
    }
    b.rect(10, 27, 12, 6, 10);
    b.rect(11, 33, 4, 10, 2);
    b.rect(18, 33, 4, 10, 2);
    b.rect(10, 42, 6, 4, 9);
    b.rect(17, 42, 6, 4, 9);
    b.rect(10, 42, 6, 1, 6);
    b.rect(17, 42, 6, 1, 6);
    b.outline(7, false);
    b.rect(12, 17, 8, 2, 6);
    b.rect(18, 17, 2, 8, 6);
    return b;
}

gs::Bitmap rimArt() {
    gs::Bitmap b(48, 16);
    b.ellipse(24, 8, 22, 6.2f, 1);
    b.ellipse(24, 8, 17, 4.4f, 2);
    b.ellipse(24, 7.4f, 12.5f, 2.8f, 0);
    b.ellipse(24, 11.2f, 8, 1.3f, 3);
    b.ellipse(16, 6.2f, 3.0f, 1.2f, 3);
    return b;
}

gs::Bitmap netArt(int sway) {
    gs::Bitmap b(32, 28);
    float lean = sway ? 3.2f : 0.f;
    for (int i = 0; i < 6; i++) {
        float x0 = 3.f + i * 5.0f;
        float x1 = 7.f + i * 3.2f + lean;
        b.line(x0, 1, x1, 26, 1, 1.1f);
    }
    for (int i = 0; i < 6; i++) {
        float x0 = 29.f - i * 5.0f;
        float x1 = 25.f - i * 3.2f + lean;
        b.line(x0, 1, x1, 26, 2, 1.0f);
    }
    for (int y = 4; y < 26; y += 5) {
        float t = (y - 4) / 22.f;
        float left = 3.f + t * (7.f + lean - 3.f);
        float right = 29.f + t * (25.f + lean - 29.f);
        b.line(left, float(y), right, float(y), 1, 1.0f);
    }
    return b;
}

gs::Bitmap boardArt() {
    gs::Bitmap b(40, 34);
    b.rect(0, 0, 40, 34, 5);
    b.rect(2, 2, 36, 30, 1);
    b.rect(2, 2, 36, 2, 6);
    b.rect(11, 6, 18, 12, 3);
    b.rect(13, 8, 14, 8, 1);
    b.rect(14, 20, 13, 2, 4);
    b.rect(25, 20, 2, 10, 4);
    b.outline(5, false);
    b.rect(14, 20, 13, 2, 4);
    b.rect(25, 20, 2, 10, 4);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(10, 64);
    b.rect(2, 0, 6, 64, 1);
    b.rect(3, 0, 2, 64, 2);
    b.rect(7, 0, 1, 64, 3);
    for (int y = 6; y < 62; y += 10) b.rect(2, y, 6, 1, 4);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 3);
    b.ellipse(9, 9, 4.6f, 4.6f, 1);
    b.ellipse(7.6f, 7.4f, 1.8f, 1.5f, 2);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6.2f, 6.2f, 1);
    b.ellipse(11, 7, 5.0f, 5.0f, 0);
    b.ellipse(6.5f, 6.2f, 1.4f, 1.1f, 2);
    return b;
}

gs::Bitmap fenceArt() {
    gs::Bitmap b(72, 22);
    b.rect(0, 6, 72, 12, 1);
    for (int x = 2; x < 72; x += 6) b.rect(x, 1, 2, 20, 2);
    b.rect(0, 6, 72, 2, 3);
    b.rect(0, 16, 72, 2, 3);
    return b;
}

gs::Bitmap seatArt() {
    gs::Bitmap b(78, 26);
    b.rect(0, 8, 78, 4, 1);
    b.rect(0, 14, 78, 4, 2);
    b.rect(0, 20, 78, 4, 1);
    for (int i = 0; i < 5; i++) {
        float x = 8.f + i * 14.f;
        b.ellipse(x, 6, 3.2f, 3.0f, 3);
        b.ellipse(x, 4.2f, 3.3f, 1.6f, 4);
        b.rect(x - 3, 9, 6, 4, 5);
    }
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7.2f, 2.2f, 1);
    return b;
}

gs::Bitmap solidArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, 1);
    return b;
}

gs::Bitmap bracketArt() {
    gs::Bitmap b(14, 16);
    b.rect(1, 1, 3, 2, 1);
    b.rect(10, 1, 3, 2, 1);
    b.rect(1, 13, 3, 2, 1);
    b.rect(10, 13, 3, 2, 1);
    b.rect(1, 1, 2, 4, 1);
    b.rect(11, 1, 2, 4, 1);
    b.rect(1, 11, 2, 4, 1);
    b.rect(11, 11, 2, 4, 1);
    b.ellipse(7, 8, 1.5f, 1.5f, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t dim = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_INK, {0, ink, gs::rgb4(2, 2, 4), gs::rgb4(8, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, dim});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(14, 10, 7), gs::rgb4(10, 6, 4), gs::rgb4(2, 13, 12), gs::rgb4(1, 7, 8),
                          gs::rgb4(1, 2, 6), gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2), gs::rgb4(1, 1, 2),
                          gs::rgb4(15, 12, 3), gs::rgb4(1, 6, 8), 0, 0, 0, 0, dim});
    setPal(vdp, PAL_LANE, {0, gs::rgb4(13, 9, 6), gs::rgb4(9, 5, 4), gs::rgb4(13, 3, 2), gs::rgb4(8, 1, 1),
                           gs::rgb4(2, 1, 1), gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2), gs::rgb4(1, 1, 2),
                           gs::rgb4(12, 10, 3), gs::rgb4(6, 1, 2), 0, 0, 0, 0, dim});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(14, 6, 1), gs::rgb4(15, 12, 6), gs::rgb4(4, 1, 0), gs::rgb4(2, 1, 0), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, dim});
    setPal(vdp, PAL_RIM, {0, gs::rgb4(14, 5, 1), gs::rgb4(12, 8, 2), gs::rgb4(15, 13, 6), gs::rgb4(6, 2, 0), 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, dim});
    setPal(vdp, PAL_BOARD, {0, gs::rgb4(14, 14, 15), gs::rgb4(11, 11, 13), gs::rgb4(12, 2, 2), gs::rgb4(2, 1, 1),
                            gs::rgb4(5, 5, 7), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, dim});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(7, 7, 9), gs::rgb4(12, 12, 14), gs::rgb4(3, 3, 5), gs::rgb4(5, 5, 7), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, dim});
    setPal(vdp, PAL_NET, {0, gs::rgb4(13, 13, 12), gs::rgb4(8, 8, 8), gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, dim});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 8), gs::rgb4(1, 5, 2), gs::rgb4(13, 15, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, dim});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 5, 4), gs::rgb4(5, 1, 1), gs::rgb4(15, 10, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          dim});
    setPal(vdp, PAL_COURT, {0, gs::rgb4(2, 8, 6), gs::rgb4(1, 5, 4), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            dim});
    setPal(vdp, PAL_METER, {0, gs::rgb4(4, 4, 6), gs::rgb4(8, 15, 7), gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, dim});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 13), gs::rgb4(8, 5, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, dim});
    setPal(vdp, PAL_SEAT, {0, gs::rgb4(8, 5, 3), gs::rgb4(5, 3, 2), gs::rgb4(12, 8, 6), gs::rgb4(2, 1, 1),
                           gs::rgb4(3, 5, 9), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, dim});
    setPal(vdp, PAL_WORD, {0, gs::rgb4(15, 14, 5), gs::rgb4(3, 2, 1), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, dim});
    setPal(vdp, PAL_SHADOW, {0, gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, dim});

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
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.fence = gs::uploadMipped(vdp, fenceArt());
    art.seats = gs::uploadMipped(vdp, seatArt());
    art.solid = gs::uploadImage(vdp, solidArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
    art.bracket = gs::uploadImage(vdp, bracketArt());
    art.hoop = phrase(vdp, "HOOP", 3);
    art.seven = phrase(vdp, "SEVEN", 3);
    art.first = phrase(vdp, "FIRST TO 7", 2);
    art.floor7 = phrase(vdp, "7", 3);
    art.swish = phrase(vdp, "SWISH", 2);
    art.bank = phrase(vdp, "BANK", 2);
    art.count = phrase(vdp, "COUNT", 2);
    art.rimWord = phrase(vdp, "RIM", 2);
    art.shortWord = phrase(vdp, "SHORT", 2);
    art.longWord = phrase(vdp, "LONG", 2);
    art.airWord = phrase(vdp, "AIR", 2);
    art.num[0] = phrase(vdp, "1", 2);
    art.num[1] = phrase(vdp, "2", 2);
    art.num[2] = phrase(vdp, "3", 2);
}

}  // namespace hoopseven
