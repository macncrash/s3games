#include "game/art.h"

#include <cmath>
#include <cstdint>

namespace blitz {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
}

gs::Bitmap shipArt(int bank) {
    gs::Bitmap b(80, 64);
    const float lean = float(bank - 1) * 6.f;
    b.poly({{40.f + lean * 0.2f, 16}, {10, 30 - lean}, {8, 40 - lean}, {34, 36}, {38, 26}}, 1);
    b.poly({{40.f + lean * 0.2f, 16}, {70, 30 + lean}, {72, 40 + lean}, {46, 36}, {42, 26}}, 1);
    b.poly({{40.f + lean * 0.15f, 6}, {52, 22}, {49, 48}, {40, 58}, {31, 48}, {28, 22}}, 2);
    b.poly({{40.f + lean * 0.1f, 12}, {45, 28}, {40, 46}, {35, 28}}, 9);
    b.ellipse(24, 42, 7, 9, 3);
    b.ellipse(56, 42, 7, 9, 3);
    b.ellipse(24, 42, 4, 5, 5);
    b.ellipse(56, 42, 4, 5, 5);
    b.ellipse(24, 42, 2.f, 2.4f, 6);
    b.ellipse(56, 42, 2.f, 2.4f, 6);
    b.poly({{40, 14}, {47, 24}, {40, 32}, {33, 24}}, 4);
    b.poly({{40, 17}, {44, 24}, {40, 28}, {36, 24}}, 6);
    b.rect(9, 34, 4, 3, 7);
    b.rect(67, 34, 4, 3, 8);
    b.rect(18, 36, 2, 10, 3);
    b.rect(60, 36, 2, 10, 3);
    b.outline(3, false);
    return b;
}

gs::Bitmap turretArt() {
    gs::Bitmap b(40, 52);
    b.poly({{6, 38}, {34, 38}, {30, 50}, {10, 50}}, 2);
    b.rect(10, 18, 20, 22, 1);
    b.rect(12, 20, 16, 6, 3);
    b.ellipse(20, 32, 6, 6, 3);
    b.ellipse(20, 32, 3, 3, 4);
    b.rect(18, 4, 4, 16, 3);
    b.rect(8, 44, 5, 6, 2);
    b.rect(27, 44, 5, 6, 2);
    b.rect(4, 24, 4, 3, 5);
    b.rect(32, 24, 4, 3, 5);
    b.outline(2, false);
    return b;
}

gs::Bitmap sparArt() {
    gs::Bitmap b(64, 18);
    b.rect(0, 0, 64, 18, 3);
    for (int x = 0; x < 64; x += 8) b.rect(float(x), 3, 8, 12, ((x / 8) & 1) ? 1 : 2);
    b.rect(0, 0, 64, 3, 3);
    b.rect(0, 15, 64, 3, 3);
    return b;
}

gs::Bitmap blockArt() {
    gs::Bitmap b(32, 56);
    b.rect(1, 1, 30, 54, 1);
    b.rect(1, 1, 30, 6, 3);
    b.rect(1, 49, 30, 6, 2);
    for (int y = 12; y < 46; y += 8) b.rect(4, float(y), 24, 2, 2);
    b.rect(11, 22, 10, 10, 4);
    b.rect(14, 25, 4, 4, 5);
    b.outline(2, false);
    return b;
}

gs::Bitmap cellArt() {
    gs::Bitmap b(28, 28);
    b.poly({{14, 1}, {26, 14}, {14, 27}, {2, 14}}, 1);
    b.poly({{14, 6}, {21, 14}, {14, 22}, {7, 14}}, 3);
    b.ellipse(14, 14, 3.2f, 3.2f, 2);
    return b;
}

gs::Bitmap boltArt() {
    gs::Bitmap b(8, 20);
    b.poly({{4, 0}, {7, 13}, {4, 10}, {1, 13}}, 2);
    b.poly({{4, 2}, {6, 11}, {4, 9}, {2, 11}}, 1);
    b.rect(3, 12, 2, 7, 3);
    return b;
}

gs::Bitmap boomArt() {
    gs::Bitmap b(40, 40);
    b.ellipse(20, 20, 18, 15, 4);
    b.ellipse(20, 20, 11, 9, 3);
    b.ellipse(17, 17, 5, 5, 2);
    b.ellipse(15, 15, 2, 2.2f, 1);
    return b;
}

gs::Bitmap wallArt() {
    gs::Bitmap b(24, 80);
    b.rect(0, 0, 24, 80, 2);
    b.rect(0, 0, 5, 80, 3);
    for (int y = 4; y < 76; y += 12) {
        b.rect(8, float(y), 12, 2, 1);
        b.rect(16, float(y + 4), 4, 3, 4);
    }
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2.f, 2.f, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(48, 16);
    b.ellipse(24, 8, 20, 5, 1);
    return b;
}

gs::Bitmap portArt() {
    gs::Bitmap b(72, 72);
    b.ellipse(36, 36, 34, 34, 3);
    b.ellipse(36, 36, 27, 27, 1);
    b.ellipse(36, 36, 16, 16, 0);
    b.ellipse(36, 36, 7, 7, 2);
    b.ellipse(36, 36, 3, 3, 6);
    for (int i = 0; i < 8; ++i) {
        const float a = float(i) * 3.1415926f / 4.f;
        b.line(36 + std::cos(a) * 18, 36 + std::sin(a) * 18, 36 + std::cos(a) * 26, 36 + std::sin(a) * 26, 4, 2.f);
    }
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y) {
            for (int x = 0; x < 5; ++x) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        const int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int spacing) {
    gs::TextStyle st{scale, 1, 3, 15, spacing};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 9, 11), gs::rgb4(3, 3, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SHIP,
           {0, gs::rgb4(11, 13, 15), gs::rgb4(5, 7, 10), gs::rgb4(2, 3, 5), gs::rgb4(14, 10, 3), gs::rgb4(3, 12, 14),
            gs::rgb4(14, 15, 15), gs::rgb4(15, 3, 2), gs::rgb4(4, 15, 7), gs::rgb4(8, 10, 13), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TURRET,
           {0, gs::rgb4(9, 6, 5), gs::rgb4(3, 2, 3), gs::rgb4(5, 5, 7), gs::rgb4(15, 3, 2), gs::rgb4(14, 11, 4), 0, 0, 0, 0,
            0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CELL,
           {0, gs::rgb4(4, 14, 15), gs::rgb4(14, 15, 15), gs::rgb4(2, 7, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SPAR,
           {0, gs::rgb4(15, 12, 2), gs::rgb4(2, 2, 3), gs::rgb4(12, 6, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BLOCK,
           {0, gs::rgb4(6, 7, 9), gs::rgb4(3, 3, 5), gs::rgb4(10, 8, 4), gs::rgb4(14, 4, 2), gs::rgb4(15, 13, 8), 0, 0, 0, 0,
            0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOLT, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 12, 3), gs::rgb4(15, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOOM,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 13, 4), gs::rgb4(15, 7, 1), gs::rgb4(12, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, shadow});
    setPal(vdp, PAL_PORT,
           {0, gs::rgb4(3, 12, 6), gs::rgb4(10, 15, 11), gs::rgb4(2, 4, 4), gs::rgb4(14, 12, 4), 0, gs::rgb4(15, 15, 15), 0, 0,
            0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WALL,
           {0, gs::rgb4(7, 8, 11), gs::rgb4(3, 3, 6), gs::rgb4(1, 1, 3), gs::rgb4(13, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(8, 6, 2), gs::rgb4(4, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 3, 2), gs::rgb4(8, 2, 2), gs::rgb4(4, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 13, 5), gs::rgb4(8, 5, 1), gs::rgb4(10, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(4, 15, 7), gs::rgb4(2, 6, 3), gs::rgb4(1, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 11, 3), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    int rb = PAL_ROAD * 16;
    vdp.setColor(rb + 1, gs::rgb4(2, 2, 4));
    vdp.setColor(rb + 2, gs::rgb4(1, 1, 2));
    vdp.setColor(rb + 3, gs::rgb4(3, 3, 6));
    vdp.setColor(rb + 4, gs::rgb4(12, 9, 1));
    vdp.setColor(rb + 5, gs::rgb4(3, 3, 4));
    vdp.setColor(rb + 6, gs::rgb4(6, 6, 8));
    vdp.setColor(rb + 7, gs::rgb4(4, 4, 6));
    vdp.setColor(rb + 8, gs::rgb4(8, 8, 10));
    vdp.setColor(rb + 9, gs::rgb4(5, 5, 6));
    vdp.setColor(rb + 10, gs::rgb4(7, 7, 8));
    vdp.setColor(rb + 11, gs::rgb4(2, 3, 5));
    vdp.setColor(rb + 12, gs::rgb4(1, 2, 4));
    vdp.setColor(rb + 13, gs::rgb4(4, 5, 7));
    vdp.setColor(rb + 14, gs::rgb4(13, 10, 2));
    vdp.setColor(rb + 15, gs::rgb4(8, 8, 9));

    loadFont(vdp, art);
    for (int i = 0; i < 3; ++i) art.ship[i] = gs::uploadMipped(vdp, shipArt(i));
    art.turret = gs::uploadMipped(vdp, turretArt());
    art.spar = gs::uploadMipped(vdp, sparArt());
    art.block = gs::uploadMipped(vdp, blockArt());
    art.cell = gs::uploadMipped(vdp, cellArt());
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.boom = gs::uploadMipped(vdp, boomArt());
    art.wall = gs::uploadMipped(vdp, wallArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.port = gs::uploadMipped(vdp, portArt());
    art.mark = words(vdp, "S3", 2, 1);
    art.logo = words(vdp, "BLITZ", 4, 1);
    for (int d = 0; d < 10; ++d) {
        char s[2] = {char('0' + d), 0};
        art.digit[d] = words(vdp, s, 3, 1);
    }
}

}  // namespace blitz
