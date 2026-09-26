#include "game/art.h"

#include <initializer_list>
#include <string>

namespace well {
namespace {

void pal(gs::VDP& v, int p, std::initializer_list<uint16_t> cs) {
    int i = 1;
    v.setColor(p * 16, 0);
    for (uint16_t c : cs) {
        if (i < 16) v.setColor(p * 16 + i, c);
        ++i;
    }
    while (i < 16) v.setColor(p * 16 + i++, 0);
}

void textPal(gs::VDP& v, int p, uint16_t ink, uint16_t shade) {
    for (int i = 0; i < 16; ++i) v.setColor(p * 16 + i, 0);
    v.setColor(p * 16 + 1, ink);
    v.setColor(p * 16 + 15, shade);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

void box(gs::Bitmap& b, int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); }

gs::Bitmap yardArt() {
    gs::Bitmap b(320, 224);
    box(b, 0, 58, 320, 152, 10);
    for (int y = 66; y < 206; y += 8) box(b, 0, y, 320, 1, 3);
    box(b, 0, 46, 320, 16, 7);
    box(b, 0, 46, 320, 3, 6);
    box(b, 0, 59, 320, 2, 8);
    for (int x = 4; x < 320; x += 18) {
        box(b, x, 32, 11, 16, 6);
        box(b, x, 32, 11, 3, 14);
        box(b, x + 11, 40, 7, 6, 0);
    }
    box(b, 0, 50, 92, 158, 6);
    box(b, 0, 50, 6, 158, 8);
    box(b, 86, 50, 6, 158, 7);
    const int feet[3] = {88, 132, 176};
    for (int i = 0; i < 3; ++i) {
        int y = feet[i];
        b.ellipse(48, float(y - 16), 26, 16, 9);
        box(b, 22, y - 16, 52, 22, 9);
        box(b, 26, y - 8, 44, 14, 2);
        box(b, 22, y - 18, 52, 3, 7);
        int path = i == 0 ? 1 : 2;
        box(b, 78, y - 7, 156, 12, path);
        box(b, 78, y + 4, 156, 2, 3);
        box(b, 96, y - 2, 120, 1, 1);
        box(b, 8, y - 28, 8, 34, 8);
    }
    for (int y = 78; y < 198; y += 16)
        for (int x = 236; x < 312; x += 18) box(b, x, y, 16, 14, ((x + y) / 16) & 1 ? 6 : 7);
    b.ellipse(274, 168, 28, 10, 14);
    for (int i = 0; i < 8; ++i) {
        int x = 104 + i * 16;
        box(b, x, 74, 2, 5, 4);
        box(b, x + 4, 158, 2, 4, 5);
        box(b, x + 1, 112, 2, 3, 11);
    }
    box(b, 156, 46, 2, 10, 15);
    box(b, 214, 46, 2, 10, 15);
    b.ellipse(157, 58, 3, 3, 13);
    b.ellipse(215, 58, 3, 3, 13);
    box(b, 0, 208, 320, 16, 3);
    box(b, 0, 208, 320, 3, 7);
    return b;
}

gs::Bitmap wellArt() {
    gs::Bitmap b(72, 150);
    b.poly({{10.f, 28.f}, {36.f, 6.f}, {62.f, 28.f}}, 8);
    box(b, 8, 24, 56, 6, 9);
    box(b, 14, 26, 44, 3, 8);
    box(b, 12, 30, 6, 78, 8);
    box(b, 14, 30, 2, 78, 9);
    box(b, 54, 30, 6, 78, 8);
    box(b, 56, 30, 2, 78, 9);
    box(b, 16, 36, 40, 4, 11);
    box(b, 34, 40, 2, 36, 10);
    b.ellipse(36, 96, 24, 12, 2);
    b.ellipse(36, 96, 22, 10, 1);
    b.ellipse(36, 98, 14, 7, 3);
    b.ellipse(36, 99, 10, 5, 6);
    b.ellipse(32, 97, 4, 2, 7);
    for (int i = 0; i < 4; ++i) box(b, 14, 108 + i * 6, 44, 5, i & 1 ? 2 : 3);
    box(b, 8, 132, 56, 10, 2);
    box(b, 6, 140, 60, 6, 3);
    box(b, 18, 136, 8, 3, 5);
    box(b, 44, 118, 6, 3, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap rubbleArt() {
    gs::Bitmap b(84, 52);
    b.ellipse(42, 30, 34, 12, 3);
    b.ellipse(24, 28, 14, 8, 2);
    b.ellipse(58, 26, 16, 9, 2);
    b.ellipse(40, 24, 10, 6, 1);
    box(b, 16, 18, 18, 8, 8);
    box(b, 46, 14, 8, 16, 8);
    box(b, 30, 16, 14, 5, 9);
    b.ellipse(22, 22, 5, 3, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap crackArt() {
    gs::Bitmap b(36, 48);
    b.line(8, 4, 16, 18, 13, 1.2f);
    b.line(16, 18, 10, 40, 13, 1.2f);
    b.line(16, 18, 28, 26, 13, 1.1f);
    b.line(12, 28, 22, 34, 4, 1.f);
    return b;
}

gs::Bitmap bucketArt() {
    gs::Bitmap b(16, 14);
    box(b, 3, 3, 10, 8, 12);
    box(b, 4, 4, 8, 3, 9);
    box(b, 2, 2, 12, 2, 11);
    b.line(3, 2, 8, 0, 10, 1.f);
    b.line(13, 2, 8, 0, 10, 1.f);
    b.outline(15, false);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(40, 52);
    box(b, 4, 4, 32, 44, 1);
    box(b, 6, 6, 28, 40, 3);
    for (int i = 0; i < 4; ++i) box(b, 8 + i * 7, 8, 3, 36, 4);
    box(b, 4, 8, 32, 4, 5);
    box(b, 4, 22, 32, 3, 5);
    box(b, 4, 36, 32, 4, 5);
    for (int y = 10; y < 40; y += 10) box(b, 6, y, 2, 2, 6);
    for (int y = 12; y < 42; y += 8) box(b, 2, y, 3, 2, 7);
    box(b, 8, 2, 24, 4, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap keeperArt(int frame) {
    gs::Bitmap b(26, 36);
    box(b, 6, 3, 12, 5, 5);
    box(b, 8, 4, 3, 2, 10);
    box(b, 4, 7, 15, 2, 5);
    box(b, 6, 9, 10, 8, 1);
    box(b, 6, 13, 10, 4, 2);
    box(b, 8, 11, 2, 2, 6);
    box(b, 7, 17, 12, 11, 3);
    box(b, 7, 17, 4, 11, 4);
    box(b, 7, 25, 12, 2, 7);
    int leg = frame ? 2 : 0;
    box(b, 8, 28, 4, 6, 8);
    box(b, 14 + leg, 28, 4, 6, 8);
    box(b, 1, 18 - frame * 2, 8, 4, 3);
    box(b, 1, 19 - frame * 2, 3, 3, 9);
    b.outline(15, false);
    return b;
}

gs::Bitmap runnerArt(int frame) {
    gs::Bitmap b(24, 36);
    int bob = frame ? -1 : 0;
    box(b, 12, 2 + bob, 8, 4, 9);
    box(b, 12, 5 + bob, 8, 7, 1);
    box(b, 12, 9 + bob, 8, 3, 2);
    box(b, 17, 7 + bob, 2, 2, 9);
    box(b, 11, 12 + bob, 10, 9, 3);
    box(b, 11, 12 + bob, 3, 9, 4);
    box(b, 16, 14 + bob, 7, 7, 6);
    box(b, 17, 15 + bob, 5, 3, 7);
    if (frame) {
        box(b, 16, 21, 4, 11, 5);
        box(b, 10, 22, 4, 8, 8);
    } else {
        box(b, 11, 21, 4, 11, 5);
        box(b, 16, 22, 4, 9, 8);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap clubArt(int frame) {
    gs::Bitmap b(30, 40);
    box(b, 12, 4, 9, 4, 9);
    box(b, 12, 7, 9, 8, 1);
    box(b, 12, 12, 9, 3, 2);
    box(b, 18, 9, 2, 2, 9);
    box(b, 10, 15, 13, 12, 3);
    box(b, 10, 15, 4, 12, 4);
    box(b, 12, 24, 3, 2, 7);
    if (frame) {
        box(b, 18, 16, 10, 3, 10);
        box(b, 26, 15, 3, 5, 11);
        box(b, 16, 27, 4, 10, 5);
        box(b, 10, 28, 4, 8, 8);
    } else {
        box(b, 20, 2, 3, 16, 10);
        box(b, 18, 1, 7, 4, 11);
        box(b, 11, 27, 4, 10, 5);
        box(b, 17, 28, 4, 8, 8);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap ramArt(int frame) {
    gs::Bitmap b(52, 32);
    int bob = frame ? 1 : 0;
    box(b, 6, 10 + bob, 30, 11, 1);
    box(b, 8, 12 + bob, 26, 4, 2);
    box(b, 14, 10 + bob, 2, 11, 7);
    box(b, 24, 10 + bob, 2, 11, 7);
    box(b, 32, 8 + bob, 14, 15, 3);
    box(b, 42, 11 + bob, 6, 9, 4);
    box(b, 44, 14 + bob, 3, 3, 8);
    box(b, 10, 21, 4, 7, 5);
    box(b, 18 + frame * 2, 21, 4, 7, 5);
    box(b, 26, 21, 4, 6, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 5, 1);
    b.ellipse(6, 7, 2, 2, 3);
    b.ellipse(11, 8, 2, 2, 2);
    return b;
}

gs::Bitmap shockArt() {
    gs::Bitmap b(18, 18);
    b.line(9, 1, 9, 16, 3, 1.4f);
    b.line(1, 9, 16, 9, 3, 1.4f);
    b.line(3, 3, 14, 14, 2, 1.2f);
    b.line(14, 3, 3, 14, 2, 1.2f);
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(8, 4);
    box(b, 0, 0, 8, 4, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(18, 6);
    b.ellipse(9, 3, 8, 2, 1);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 9, 9, 1);
    b.ellipse(14, 10, 6, 6, 2);
    b.ellipse(8, 8, 2, 2, 3);
    b.ellipse(12, 14, 1, 1, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(7, 7);
    box(b, 3, 0, 1, 7, 4);
    box(b, 0, 3, 7, 1, 4);
    box(b, 2, 2, 3, 3, 1);
    return b;
}

gs::Bitmap chainArt() {
    gs::Bitmap b(6, 8);
    box(b, 1, 1, 4, 6, 4);
    box(b, 2, 2, 2, 4, 0);
    return b;
}

gs::Bitmap winchArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7, 7, 4);
    b.ellipse(9, 9, 3, 3, 5);
    box(b, 8, 1, 2, 16, 6);
    box(b, 1, 8, 16, 2, 6);
    b.outline(15, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& a) {
    textPal(vdp, PAL_TEXT, gs::rgb4(14, 13, 10), gs::rgb4(2, 2, 3));
    textPal(vdp, PAL_GOLD, gs::rgb4(14, 11, 4), gs::rgb4(3, 2, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(14, 4, 3), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_GOOD, gs::rgb4(8, 14, 6), gs::rgb4(1, 3, 1));
    pal(vdp, PAL_STONE,
        {gs::rgb4(12, 11, 9), gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 4), gs::rgb4(3, 3, 3), gs::rgb4(4, 7, 3),
         gs::rgb4(3, 7, 11), gs::rgb4(7, 12, 14), gs::rgb4(7, 5, 2), gs::rgb4(10, 7, 3), gs::rgb4(9, 8, 5),
         gs::rgb4(5, 5, 6), gs::rgb4(8, 6, 3), gs::rgb4(2, 2, 2), gs::rgb4(13, 12, 10), gs::rgb4(1, 1, 1)});
    pal(vdp, PAL_WOOD,
        {gs::rgb4(9, 6, 3), gs::rgb4(12, 8, 4), gs::rgb4(5, 3, 1), gs::rgb4(7, 7, 8), gs::rgb4(3, 3, 4),
         gs::rgb4(12, 10, 6), gs::rgb4(11, 11, 12), gs::rgb4(2, 1, 1), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0),
         gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1)});
    pal(vdp, PAL_KEEPER,
        {gs::rgb4(13, 9, 6), gs::rgb4(9, 6, 4), gs::rgb4(3, 5, 8), gs::rgb4(2, 3, 5), gs::rgb4(4, 4, 6),
         gs::rgb4(3, 2, 1), gs::rgb4(8, 6, 3), gs::rgb4(2, 2, 2), gs::rgb4(12, 9, 6), gs::rgb4(12, 9, 3),
         gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2)});
    pal(vdp, PAL_RAIDER,
        {gs::rgb4(12, 8, 5), gs::rgb4(8, 5, 3), gs::rgb4(8, 3, 2), gs::rgb4(5, 2, 1), gs::rgb4(3, 3, 4),
         gs::rgb4(10, 8, 4), gs::rgb4(6, 5, 2), gs::rgb4(2, 2, 2), gs::rgb4(2, 1, 1), gs::rgb4(7, 5, 2),
         gs::rgb4(4, 3, 1), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1)});
    pal(vdp, PAL_RAM,
        {gs::rgb4(7, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4), gs::rgb4(5, 4, 3),
         gs::rgb4(8, 7, 4), gs::rgb4(3, 2, 2), gs::rgb4(12, 3, 2), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0),
         gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1)});
    pal(vdp, PAL_YARD,
        {gs::rgb4(10, 8, 5), gs::rgb4(8, 6, 4), gs::rgb4(5, 4, 3), gs::rgb4(3, 6, 3), gs::rgb4(2, 4, 2),
         gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 5), gs::rgb4(11, 10, 8), gs::rgb4(2, 2, 3), gs::rgb4(7, 5, 4),
         gs::rgb4(3, 5, 2), gs::rgb4(8, 3, 2), gs::rgb4(14, 11, 4), gs::rgb4(12, 11, 9), gs::rgb4(3, 3, 4)});
    pal(vdp, PAL_FX,
        {gs::rgb4(11, 10, 8), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 12), gs::rgb4(6, 6, 6), gs::rgb4(0, 0, 0),
         gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0),
         gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(2, 2, 2)});
    pal(vdp, PAL_SKY,
        {gs::rgb4(14, 13, 9), gs::rgb4(10, 9, 6), gs::rgb4(8, 7, 5), gs::rgb4(15, 15, 13), gs::rgb4(6, 6, 7),
         gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0),
         gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(2, 2, 3)});

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, a);
    a.well = gs::uploadMipped(vdp, wellArt());
    a.rubble = gs::uploadMipped(vdp, rubbleArt());
    a.crack = gs::uploadMipped(vdp, crackArt());
    a.bucket = gs::uploadMipped(vdp, bucketArt());
    a.gate = gs::uploadMipped(vdp, gateArt());
    a.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    a.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    a.runner[0] = gs::uploadMipped(vdp, runnerArt(0));
    a.runner[1] = gs::uploadMipped(vdp, runnerArt(1));
    a.club[0] = gs::uploadMipped(vdp, clubArt(0));
    a.club[1] = gs::uploadMipped(vdp, clubArt(1));
    a.ram[0] = gs::uploadMipped(vdp, ramArt(0));
    a.ram[1] = gs::uploadMipped(vdp, ramArt(1));
    a.puff = gs::uploadMipped(vdp, puffArt());
    a.shock = gs::uploadMipped(vdp, shockArt());
    a.bar = gs::uploadMipped(vdp, barArt());
    a.shadow = gs::uploadMipped(vdp, shadowArt());
    a.moon = gs::uploadMipped(vdp, moonArt());
    a.star = gs::uploadMipped(vdp, starArt());
    a.chain = gs::uploadMipped(vdp, chainArt());
    a.winch = gs::uploadMipped(vdp, winchArt());

    vdp.A.enabled = false;
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, yardArt(), PAL_YARD);
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(2, 2, 4));
}

}  // namespace well
