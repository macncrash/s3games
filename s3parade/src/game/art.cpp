#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace parade {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float TAU = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void star(Bitmap& b, float cx, float cy, float r, int c) {
    std::vector<Pt> p;
    p.reserve(10);
    for (int i = 0; i < 10; i++) {
        float a = -1.5708f + i * TAU / 10.f;
        float rr = (i & 1) ? r * 0.42f : r;
        p.push_back({cx + std::cos(a) * rr, cy + std::sin(a) * rr});
    }
    b.poly(p, c);
}

Bitmap majorArt(int step) {
    Bitmap b(36, 46);
    b.poly({{18, 3}, {26, 0}, {29, 8}, {20, 9}}, 8);
    b.poly({{20, 2}, {27, 1}, {24, 8}}, 9);
    b.ellipse(18, 10, 10, 5, 4);
    b.rect(8, 8, 20, 5, 4);
    b.rect(8, 12, 20, 3, 3);
    b.ellipse(18, 18, 6, 6, 5);
    b.set(16, 17, 7);
    b.set(21, 17, 7);
    b.rect(17, 19, 2, 2, 11);
    b.poly({{7, 38}, {9, 22}, {27, 22}, {29, 38}}, 1);
    b.rect(16, 22, 4, 14, 3);
    b.set(12, 25, 10);
    b.set(12, 29, 10);
    b.set(12, 33, 10);
    b.rect(5, 24, 5, 9, 1);
    b.rect(26, 24, 5, 9, 1);
    b.ellipse(6, 33, 2, 2, 6);
    b.line(30, 26, 34, 10, 12, 2.2f);
    b.ellipse(34, 8, 3, 3, 13);
    int shift = step ? 2 : 0;
    b.rect(10 - shift, 36, 5, 7, 6);
    b.rect(20 + shift, 36, 5, 7, 6);
    b.rect(9 - shift, 42, 6, 3, 14);
    b.rect(20 + shift, 42, 6, 3, 14);
    b.outline(7, false);
    return b;
}

Bitmap horseArt(int frame, int pale) {
    Bitmap b(72, 40);
    int body = pale ? 3 : 1;
    int shade = pale ? 1 : 2;
    b.poly({{14, 16}, {4, 10}, {6, 24}, {16, 22}}, 4);
    b.ellipse(34, 18, 20, 10, body);
    b.ellipse(30, 16, 12, 6, pale ? 6 : 14);
    b.ellipse(50, 18, 9, 8, body);
    b.poly({{46, 14}, {58, 6}, {62, 12}, {52, 20}}, shade);
    b.ellipse(62, 11, 8, 5, body);
    b.ellipse(67, 12, 3, 2, 13);
    b.rect(58, 9, 5, 2, 6);
    b.set(63, 10, 7);
    b.poly({{56, 7}, {58, 1}, {63, 7}}, shade);
    b.poly({{56, 5}, {48, 0}, {54, 8}, {60, 6}}, 10);
    b.ellipse(50, 2, 2, 2, 11);
    b.rect(26, 14, 16, 9, 8);
    b.rect(28, 16, 12, 4, 9);
    auto leg = [&](float x0, float y0, float x1, float y1) {
        b.line(x0, y0, x1, y1, shade, 2.4f);
        b.rect(x1 - 2, y1 - 1, 5, 3, 5);
    };
    if (frame == 0) {
        leg(24, 26, 16, 36);
        leg(32, 26, 30, 38);
        leg(44, 26, 50, 35);
        leg(52, 24, 60, 36);
    } else {
        leg(22, 26, 26, 38);
        leg(34, 26, 28, 34);
        leg(42, 26, 46, 38);
        leg(54, 24, 62, 37);
    }
    b.outline(15, false);
    return b;
}

Bitmap wagonArt(int kind) {
    Bitmap b(112, 52);
    b.ellipse(26, 42, 9, 9, 4);
    b.ellipse(26, 42, 4, 4, 5);
    b.ellipse(86, 42, 9, 9, 4);
    b.ellipse(86, 42, 4, 4, 5);
    b.rect(12, 32, 88, 8, 11);
    b.rect(8, 29, 96, 5, 3);
    for (int i = 0; i < 8; i++) {
        int c = (i % 3 == 0) ? 8 : (i % 3 == 1) ? 6 : 7;
        float x = 14.f + i * 11.f;
        b.poly({{x, 33}, {x + 8, 33}, {x + 4, 44}}, c);
    }
    b.rect(18, 14, 76, 16, 1);
    b.rect(18, 14, 76, 4, 2);
    b.rect(22, 20, 10, 7, 14);
    b.rect(80, 20, 10, 7, 14);
    if (kind == 0) {
        star(b, 56, 10, 10, 12);
        b.ellipse(56, 10, 2, 2, 7);
    } else {
        b.ellipse(46, 10, 7, 6, 9);
        b.ellipse(66, 10, 7, 6, 9);
        b.ellipse(56, 8, 8, 7, 12);
        b.ellipse(56, 8, 3, 3, 7);
        b.rect(54, 14, 4, 4, 10);
    }
    b.outline(15, false);
    return b;
}

Bitmap drumArt(int frame) {
    Bitmap b(40, 40);
    b.ellipse(20, 21, 16, 16, 6);
    b.ellipse(20, 21, 13, 13, 1);
    b.ellipse(20, 21, 11, 11, 3);
    b.ellipse(16, 17, 4, 3, 8);
    for (int i = 0; i < 6; i++) {
        float a = i * TAU / 6.f + (frame ? 0.3f : 0.f);
        b.line(20 + std::cos(a) * 12, 21 + std::sin(a) * 12, 20 + std::cos(a + 0.5f) * 5, 21 + std::sin(a + 0.5f) * 5, 5, 1.2f);
    }
    b.ellipse(20, 21, 4, 4, 7);
    b.ellipse(20, 21, 2, 2, 6);
    if (frame == 0) {
        b.line(4, 34, 16, 20, 10, 1.6f);
        b.ellipse(4, 34, 2, 2, 11);
    } else {
        b.line(34, 32, 24, 18, 10, 1.6f);
        b.ellipse(34, 32, 2, 2, 11);
    }
    b.outline(15, false);
    return b;
}

Bitmap batonArt(int frame) {
    Bitmap b(40, 22);
    if (frame == 0) {
        b.line(6, 16, 34, 5, 2, 2.4f);
        b.line(6, 16, 34, 5, 1, 1.2f);
        b.ellipse(6, 16, 4, 4, 3);
        b.ellipse(34, 5, 4, 4, 3);
        b.ellipse(34, 5, 2, 2, 5);
        b.set(33, 4, 6);
    } else {
        b.line(6, 5, 34, 16, 2, 2.4f);
        b.line(6, 5, 34, 16, 1, 1.2f);
        b.ellipse(6, 5, 4, 4, 3);
        b.ellipse(34, 16, 4, 4, 3);
        b.ellipse(6, 5, 2, 2, 5);
        b.set(7, 4, 6);
    }
    return b;
}

Bitmap squareArt() {
    Bitmap b(48, 48);
    b.rect(0, 0, 48, 48, 3);
    b.rect(3, 3, 42, 42, 1);
    b.rect(6, 6, 36, 36, 2);
    b.rect(9, 9, 30, 30, 4);
    b.rect(12, 12, 24, 24, 5);
    b.rect(15, 15, 18, 18, 6);
    star(b, 24, 24, 8, 7);
    return b;
}

Bitmap ringArt() {
    Bitmap b(64, 64);
    for (int i = 0; i < 80; i++) {
        if ((i % 16) < 3) continue;
        float a = i * TAU / 80.f;
        int x = int(std::lround(32 + std::cos(a) * 27));
        int y = int(std::lround(32 + std::sin(a) * 27));
        b.set(x, y, 2);
        b.set(x, y + 1, 1);
        int x2 = int(std::lround(32 + std::cos(a) * 24));
        int y2 = int(std::lround(32 + std::sin(a) * 24));
        b.set(x2, y2, 7);
    }
    return b;
}

Bitmap personArt(int shirt) {
    Bitmap b(20, 28);
    b.rect(6, 1, 8, 4, 7);
    b.ellipse(10, 8, 5, 5, 1);
    b.ellipse(10, 8, 4, 3, 2);
    b.set(8, 8, 9);
    b.set(12, 8, 9);
    b.set(10, 10, 12);
    b.rect(5, 13, 10, 9, shirt);
    b.rect(3, 14, 3, 6, shirt);
    b.rect(14, 14, 3, 6, shirt);
    b.rect(6, 21, 3, 5, 8);
    b.rect(11, 21, 3, 5, 8);
    b.set(8, 16, 11);
    b.poly({{15, 14}, {19, 12}, {18, 18}}, 10);
    b.outline(9, false);
    return b;
}

Bitmap pennantArt() {
    Bitmap b(26, 22);
    b.line(2, 2, 24, 4, 5, 1.3f);
    b.poly({{4, 4}, {12, 5}, {6, 16}}, 1);
    b.poly({{12, 5}, {20, 6}, {14, 17}}, 2);
    b.poly({{6, 4}, {10, 5}, {7, 10}}, 4);
    return b;
}

Bitmap confettiArt() {
    Bitmap b(12, 12);
    b.rect(1, 2, 3, 2, 1);
    b.rect(6, 1, 2, 3, 2);
    b.rect(8, 5, 3, 2, 3);
    b.rect(2, 6, 3, 2, 4);
    b.rect(6, 8, 2, 3, 5);
    b.rect(4, 4, 2, 2, 6);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(18, 8);
    b.ellipse(9, 4, 8, 3, 1);
    return b;
}

Bitmap chevronArt() {
    Bitmap b(20, 16);
    b.poly({{2, 12}, {10, 2}, {18, 12}, {10, 7}}, 1);
    b.poly({{6, 13}, {10, 7}, {14, 13}, {10, 10}}, 2);
    return b;
}

Bitmap puffArt() {
    Bitmap b(22, 22);
    b.ellipse(11, 12, 8, 7, 1);
    b.ellipse(8, 9, 4, 3, 4);
    b.ellipse(14, 10, 3, 3, 2);
    b.ellipse(11, 13, 2, 2, 3);
    return b;
}

void fill8(uint8_t* px, int c) {
    for (int i = 0; i < 64; i++) px[i] = uint8_t(c);
}

void cobble(uint8_t* px, int variant, bool stripe) {
    fill8(px, 2);
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int c = 2;
            if (x == 0 || y == 0) c = 4;
            else if (((x + y * 3 + variant) & 3) == 0) c = 1;
            else if (((x * 2 + y + variant * 5) & 5) == 0) c = 3;
            if (stripe && y < 2) c = (x & 1) ? 13 : 11;
            if (((x * 5 + y * 7 + variant) % 17) == 0) c = 12;
            px[y * 8 + x] = uint8_t(c);
        }
    }
}

void sidewalk(uint8_t* px) {
    fill8(px, 5);
    for (int y = 0; y < 8; y++) {
        px[y * 8 + 7] = 6;
        if ((y & 3) == 0)
            for (int x = 0; x < 8; x++) px[y * 8 + x] = 6;
    }
}

void curb(uint8_t* px, bool left) {
    fill8(px, 5);
    for (int y = 0; y < 8; y++) {
        int edge = left ? 6 : 1;
        px[y * 8 + edge] = 11;
        px[y * 8 + (left ? 7 : 0)] = 6;
        if (y == 0 || y == 7) px[y * 8 + (left ? 5 : 2)] = 6;
    }
}

void building(uint8_t* px, int variant) {
    fill8(px, 7);
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++)
            if (((x + variant) & 3) == 0 && (y & 1)) px[y * 8 + x] = 8;
    int wx = 2 + (variant & 1);
    int lit = (variant % 3) != 0;
    for (int y = 1; y <= 4; y++)
        for (int x = wx; x <= wx + 2; x++) px[y * 8 + x] = uint8_t(lit ? 9 : 10);
    px[5 * 8 + wx] = 8;
}

void standTile(uint8_t* px, int variant) {
    fill8(px, 14);
    for (int y = 0; y < 8; y++) {
        px[y * 8 + 0] = 11;
        px[y * 8 + 7] = 11;
        if (y < 2) px[y * 8 + (variant & 7)] = 12;
    }
    px[3 * 8 + 3] = 11;
    px[3 * 8 + 4] = 12;
    px[4 * 8 + 3] = 12;
    px[4 * 8 + 4] = 11;
}

void plaza(uint8_t* px, int variant) {
    int base = (variant & 1) ? 11 : 12;
    fill8(px, base);
    for (int i = 0; i < 8; i++) {
        px[i] = 1;
        px[7 * 8 + i] = 3;
        px[i * 8] = 3;
        px[i * 8 + 7] = 1;
    }
}

void carpet(uint8_t* px) {
    fill8(px, 13);
    for (int y = 0; y < 8; y++) {
        px[y * 8 + 0] = 11;
        px[y * 8 + 7] = 11;
        if ((y & 3) == 0) {
            px[y * 8 + 3] = 12;
            px[y * 8 + 4] = 12;
        }
    }
}

void skyTile(uint8_t* px) {
    fill8(px, 15);
    px[6 * 8 + 2] = 12;
    px[2 * 8 + 6] = 12;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64];
        fill8(px, 6);
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    int sx = x + 2;
                    int sy = y + 1;
                    if (sy + 1 < 8 && sx + 1 < 8) px[(sy + 1) * 8 + sx] = 15;
                    px[sy * 8 + (x + 1)] = 1;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
        art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

void paintWorld(gs::VDP& vdp, gs::TileAlloc& tiles, const float* rows, int nrows) {
    uint8_t px[64];
    int cob[4], laneCob[4], walk, curbL, curbR, bld[6], stand[4], plaz[2], carp, sky;
    for (int i = 0; i < 4; i++) {
        cobble(px, i, false);
        cob[i] = tiles.shared(px);
        cobble(px, i, true);
        laneCob[i] = tiles.shared(px);
    }
    sidewalk(px);
    walk = tiles.shared(px);
    curb(px, true);
    curbL = tiles.shared(px);
    curb(px, false);
    curbR = tiles.shared(px);
    for (int i = 0; i < 6; i++) {
        building(px, i);
        bld[i] = tiles.shared(px);
    }
    for (int i = 0; i < 4; i++) {
        standTile(px, i);
        stand[i] = tiles.shared(px);
    }
    plaza(px, 0);
    plaz[0] = tiles.shared(px);
    plaza(px, 1);
    plaz[1] = tiles.shared(px);
    carpet(px);
    carp = tiles.shared(px);
    skyTile(px);
    sky = tiles.shared(px);

    bool laneRow[128] = {};
    for (int i = 0; i < nrows; i++) {
        int ty = int(rows[i]) >> 3;
        if (ty >= 0 && ty < 128) laneRow[ty] = true;
    }

    for (int ty = 0; ty < 128; ty++) {
        for (int tx = 0; tx < 40; tx++) {
            int tile;
            bool side = tx < 5 || tx > 34;
            if (ty < 2) tile = sky;
            else if (ty <= 16 && side) tile = stand[(tx + ty) & 3];
            else if (ty <= 16 && tx >= 18 && tx <= 25) tile = carp;
            else if (ty <= 16) tile = plaz[(tx + ty) & 1];
            else if (ty >= 17 && ty <= 19 && tx >= 18 && tx <= 25) tile = carp;
            else if (side && (tx < 3 || tx > 36)) tile = bld[(tx * 3 + ty) % 6];
            else if (tx == 3 || tx == 36) tile = walk;
            else if (tx == 4) tile = curbL;
            else if (tx == 35) tile = curbR;
            else if (laneRow[ty]) tile = laneCob[(tx + ty) & 3];
            else tile = cob[(tx * 2 + ty) & 3];
            vdp.B.set(tx, ty, gs::entry(tile, PAL_STREET));
        }
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art, const float* rows, int nrows) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t bar = gs::rgb4(2, 2, 5);
    auto textPal = [&](int pal, uint16_t ink) {
        setPal(vdp, pal, {0, ink, gs::rgb4(15, 14, 8), gs::rgb4(15, 4, 3), gs::rgb4(8, 14, 6), gs::rgb4(6, 8, 14), bar,
                          gs::rgb4(15, 12, 4), gs::rgb4(4, 4, 6), 0, 0, 0, 0, 0, 0, shadow});
    };
    textPal(PAL_WHITE, gs::rgb4(15, 15, 15));
    textPal(PAL_GOLD, gs::rgb4(15, 13, 4));
    textPal(PAL_RED, gs::rgb4(15, 4, 3));
    textPal(PAL_GREEN, gs::rgb4(8, 15, 6));

    setPal(vdp, PAL_MAJOR, {0, gs::rgb4(2, 3, 8), gs::rgb4(1, 2, 5), gs::rgb4(14, 11, 3), gs::rgb4(13, 2, 2),
                            gs::rgb4(14, 10, 7), gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2), gs::rgb4(15, 15, 15),
                            gs::rgb4(15, 5, 4), gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 6), gs::rgb4(12, 13, 14),
                            gs::rgb4(15, 14, 6), gs::rgb4(2, 2, 2), shadow});
    setPal(vdp, PAL_HORSE, {0, gs::rgb4(10, 6, 3), gs::rgb4(6, 3, 2), gs::rgb4(13, 9, 5), gs::rgb4(2, 1, 1),
                            gs::rgb4(3, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 1), gs::rgb4(13, 2, 2),
                            gs::rgb4(14, 11, 2), gs::rgb4(15, 3, 3), gs::rgb4(15, 13, 4), gs::rgb4(5, 3, 2),
                            gs::rgb4(8, 5, 4), gs::rgb4(14, 11, 8), shadow});
    setPal(vdp, PAL_WAGON, {0, gs::rgb4(13, 2, 3), gs::rgb4(15, 13, 9), gs::rgb4(14, 11, 3), gs::rgb4(3, 2, 2),
                            gs::rgb4(12, 10, 5), gs::rgb4(3, 5, 13), gs::rgb4(15, 15, 15), gs::rgb4(14, 3, 3),
                            gs::rgb4(15, 7, 8), gs::rgb4(3, 10, 4), gs::rgb4(8, 5, 3), gs::rgb4(15, 14, 4),
                            gs::rgb4(4, 2, 2), gs::rgb4(15, 15, 12), shadow});
    setPal(vdp, PAL_DRUM, {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(15, 14, 12), gs::rgb4(12, 11, 9),
                           gs::rgb4(14, 12, 8), gs::rgb4(13, 10, 3), gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 14),
                           gs::rgb4(9, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(14, 14, 12), gs::rgb4(3, 2, 2),
                           gs::rgb4(15, 13, 6), gs::rgb4(15, 6, 4), shadow});
    setPal(vdp, PAL_BATON, {0, gs::rgb4(12, 13, 14), gs::rgb4(6, 7, 8), gs::rgb4(15, 15, 15), gs::rgb4(12, 12, 14),
                            gs::rgb4(14, 12, 4), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SQUARE, {0, gs::rgb4(14, 11, 2), gs::rgb4(15, 14, 7), gs::rgb4(8, 6, 1), gs::rgb4(15, 15, 15),
                             gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(15, 15, 8), gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0,
                             0, shadow});
    setPal(vdp, PAL_CROWD, {0, gs::rgb4(14, 10, 7), gs::rgb4(11, 7, 5), gs::rgb4(13, 2, 2), gs::rgb4(2, 4, 12),
                            gs::rgb4(2, 9, 4), gs::rgb4(3, 2, 1), gs::rgb4(8, 2, 3), gs::rgb4(2, 2, 4), gs::rgb4(1, 1, 1),
                            gs::rgb4(15, 15, 15), gs::rgb4(14, 12, 4), gs::rgb4(15, 8, 7), 0, 0, 0, shadow});
    setPal(vdp, PAL_STREET, {0, gs::rgb4(11, 10, 8), gs::rgb4(8, 7, 6), gs::rgb4(6, 5, 4), gs::rgb4(5, 4, 3),
                             gs::rgb4(12, 11, 10), gs::rgb4(9, 8, 7), gs::rgb4(11, 4, 3), gs::rgb4(8, 2, 2),
                             gs::rgb4(15, 12, 5), gs::rgb4(3, 3, 5), gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 8),
                             gs::rgb4(12, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(2, 3, 8)});
    setPal(vdp, PAL_CONFETTI, {0, gs::rgb4(15, 3, 3), gs::rgb4(15, 13, 3), gs::rgb4(3, 6, 14), gs::rgb4(15, 15, 15),
                               gs::rgb4(4, 13, 5), gs::rgb4(15, 7, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PENNANT, {0, gs::rgb4(14, 2, 2), gs::rgb4(14, 11, 3), gs::rgb4(3, 5, 13), gs::rgb4(15, 15, 15),
                              gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 12), gs::rgb4(15, 8, 2), gs::rgb4(15, 15, 8), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, shadow});

    vdp.setFogColor(gs::rgb4(1, 1, 4));
    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    paintWorld(vdp, tiles, rows, nrows);

    art.major[0] = gs::uploadMipped(vdp, majorArt(0));
    art.major[1] = gs::uploadMipped(vdp, majorArt(1));
    art.horse[0] = gs::uploadMipped(vdp, horseArt(0, 0));
    art.horse[1] = gs::uploadMipped(vdp, horseArt(1, 0));
    art.wagon[0] = gs::uploadMipped(vdp, wagonArt(0));
    art.wagon[1] = gs::uploadMipped(vdp, wagonArt(1));
    art.drum[0] = gs::uploadMipped(vdp, drumArt(0));
    art.drum[1] = gs::uploadMipped(vdp, drumArt(1));
    art.baton[0] = gs::uploadMipped(vdp, batonArt(0));
    art.baton[1] = gs::uploadMipped(vdp, batonArt(1));
    art.square = gs::uploadMipped(vdp, squareArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.person[0] = gs::uploadMipped(vdp, personArt(3));
    art.person[1] = gs::uploadMipped(vdp, personArt(4));
    art.person[2] = gs::uploadMipped(vdp, personArt(5));
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.confetti = gs::uploadMipped(vdp, confettiArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.chevron = gs::uploadMipped(vdp, chevronArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
}

}  // namespace parade
