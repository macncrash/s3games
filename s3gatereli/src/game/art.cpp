#include "game/art.h"

#include <cmath>
#include <string>

namespace gate {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap towerArt() {
    Bitmap b(52, 224);
    b.rect(0, 0, 52, 224, 2);
    b.rect(4, 0, 40, 224, 1);
    b.rect(40, 0, 12, 224, 3);
    for (int i = 0; i < 5; i++) {
        b.rect(4 + i * 8, 0, 6, 14, 1);
        b.rect(8 + i * 8, 0, 2, 14, 3);
        b.rect(4 + i * 8, 14, 6, 4, 3);
    }
    for (int y = 28; y < 220; y += 12) b.rect(6, y, 32, 2, 3);
    for (int x = 10; x < 40; x += 14) b.rect(x, 22, 2, 200, 3);
    b.rect(18, 48, 8, 28, 8);
    b.rect(20, 50, 4, 22, 5);
    b.rect(16, 96, 14, 8, 6);
    b.rect(18, 88, 4, 10, 6);
    b.rect(8, 150, 18, 10, 4);
    b.rect(6, 188, 28, 22, 4);
    b.rect(6, 200, 22, 8, 3);
    b.outline(8, false);
    return b;
}

Bitmap sentryArt(bool strike) {
    Bitmap b(48, 84);
    b.ellipse(24, 16, 11, 12, 1);
    b.ellipse(24, 18, 8, 8, 2);
    b.rect(18, 24, 12, 6, 5);
    b.poly({{6, 32}, {42, 32}, {46, 58}, {2, 58}}, 3);
    b.poly({{10, 30}, {38, 30}, {36, 54}, {12, 54}}, 2);
    b.rect(14, 50, 20, 5, 7);
    b.rect(16, 60, 7, 16, 3);
    b.rect(26, 60, 7, 16, 3);
    b.rect(15, 74, 9, 6, 8);
    b.rect(25, 74, 9, 6, 8);
    if (!strike) {
        b.rect(36, 8, 3, 62, 6);
        b.poly({{34, 4}, {42, 4}, {38, 14}}, 1);
    } else {
        b.rect(22, 2, 4, 40, 6);
        b.poly({{20, 0}, {28, 0}, {24, 8}}, 1);
        b.rect(18, 36, 12, 4, 5);
    }
    b.rect(20, 12, 8, 3, 1);
    b.outline(8, false);
    return b;
}

Bitmap runnerArt(int frame) {
    Bitmap b(40, 74);
    b.poly({{20, 2}, {6, 20}, {34, 20}}, 2);
    b.poly({{20, 6}, {10, 18}, {30, 18}}, 1);
    b.ellipse(20, 22, 7, 6, 4);
    b.set(17, 21, 8);
    b.set(23, 21, 8);
    b.rect(12, 26, 16, 22, 1);
    b.rect(14, 28, 12, 16, 2);
    b.rect(10, 28, 5, 14, 3);
    int step = frame ? 4 : 0;
    b.rect(13, 48 + (frame ? 0 : 2), 6, 18 - step, 3);
    b.rect(22, 48 + step, 6, 18 - step, 3);
    b.rect(12, 64, 8, 5, 8);
    b.rect(21, 64 - step, 8, 5, 8);
    b.line(30, 32, 38, 18 + step, 5, 2.0f);
    b.poly({{36, 12 + step}, {40, 16 + step}, {34, 22 + step}}, 5);
    b.outline(8, false);
    return b;
}

Bitmap shieldArt() {
    Bitmap b(52, 76);
    b.ellipse(26, 40, 20, 24, 6);
    b.ellipse(26, 40, 16, 20, 3);
    b.ellipse(26, 40, 5, 5, 7);
    b.ellipse(26, 14, 9, 8, 5);
    b.rect(20, 18, 12, 6, 4);
    b.set(23, 14, 8);
    b.set(29, 14, 8);
    b.rect(16, 62, 7, 10, 3);
    b.rect(29, 62, 7, 10, 3);
    b.rect(15, 70, 9, 4, 8);
    b.rect(28, 70, 9, 4, 8);
    b.outline(8, false);
    return b;
}

Bitmap ramArt() {
    Bitmap b(88, 72);
    b.ellipse(44, 30, 30, 22, 1);
    b.ellipse(44, 30, 22, 16, 2);
    b.ellipse(44, 30, 8, 8, 8);
    b.rect(16, 26, 56, 6, 3);
    b.ellipse(44, 30, 14, 6, 3);
    b.ellipse(18, 54, 7, 7, 5);
    b.ellipse(70, 54, 7, 7, 5);
    b.rect(14, 58, 8, 10, 2);
    b.rect(66, 58, 8, 10, 2);
    b.rect(30, 48, 28, 6, 4);
    b.outline(8, false);
    return b;
}

Bitmap reliefArt(bool bell) {
    Bitmap b(44, 80);
    b.ellipse(22, 14, 10, 11, 4);
    b.ellipse(22, 16, 7, 7, 5);
    b.poly({{8, 28}, {36, 28}, {40, 56}, {4, 56}}, 3);
    b.poly({{12, 28}, {32, 28}, {30, 52}, {14, 52}}, 1);
    b.rect(14, 56, 6, 16, 3);
    b.rect(24, 56, 6, 16, 3);
    b.rect(13, 70, 8, 5, 2);
    b.rect(23, 70, 8, 5, 2);
    if (bell) {
        b.rect(30, 6, 3, 40, 2);
        b.poly({{28, 40}, {36, 40}, {38, 52}, {26, 52}}, 7);
        b.ellipse(32, 50, 4, 3, 2);
    } else {
        b.rect(32, 8, 3, 58, 2);
        b.poly({{30, 4}, {38, 4}, {34, 12}}, 7);
    }
    b.outline(8, false);
    return b;
}

Bitmap spearArt() {
    Bitmap b(14, 52);
    b.rect(6, 10, 3, 40, 6);
    b.poly({{3, 10}, {11, 10}, {7, 0}}, 1);
    return b;
}

Bitmap boltArt() {
    Bitmap b(10, 18);
    b.poly({{5, 0}, {9, 8}, {5, 17}, {1, 8}}, 5);
    b.poly({{5, 3}, {7, 8}, {5, 13}, {3, 8}}, 1);
    return b;
}

Bitmap flameArt(int frame) {
    Bitmap b(16, 24);
    float lift = frame ? 2.0f : 0.0f;
    b.ellipse(8, 16 - lift, 6, 8, 2);
    b.ellipse(8, 14 - lift, 4, 6, 1);
    b.ellipse(8, 18, 3, 3, 3);
    return b;
}

Bitmap postArt() {
    Bitmap b(18, 52);
    b.rect(7, 14, 4, 36, 3);
    b.rect(4, 10, 10, 6, 6);
    b.rect(5, 46, 8, 4, 8);
    b.outline(8, false);
    return b;
}

Bitmap bellArt() {
    Bitmap b(32, 32);
    b.poly({{10, 6}, {22, 6}, {28, 22}, {4, 22}}, 1);
    b.poly({{12, 8}, {20, 8}, {24, 20}, {8, 20}}, 2);
    b.rect(8, 22, 16, 3, 1);
    b.rect(15, 2, 2, 6, 2);
    b.ellipse(16, 18, 2, 3, 8);
    b.outline(8, false);
    return b;
}

Bitmap ropeArt() {
    Bitmap b(6, 72);
    for (int y = 0; y < 72; y++) {
        int x = 2 + ((y / 4) & 1);
        b.set(x, y, 4);
        b.set(x + 1, y, 2);
    }
    return b;
}

Bitmap moonArt() {
    Bitmap b(36, 36);
    for (int y = 0; y < 36; y++) {
        for (int x = 0; x < 36; x++) {
            float dx = x - 15.0f, dy = y - 18.0f;
            float dx2 = x - 23.0f, dy2 = y - 16.0f;
            float d = std::sqrt(dx * dx + dy * dy);
            float d2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
            if (d < 13.0f && d2 > 11.0f) b.set(x, y, d < 7.0f ? 1 : 2);
        }
    }
    return b;
}

Bitmap starArt() {
    Bitmap b(5, 5);
    b.set(2, 0, 2);
    b.set(2, 4, 2);
    b.set(0, 2, 2);
    b.set(4, 2, 2);
    b.set(2, 2, 1);
    return b;
}

Bitmap puffArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 10, 8, 2);
    b.ellipse(12, 12, 6, 5, 1);
    b.ellipse(16, 16, 3, 2, 4);
    return b;
}

Bitmap bannerArt() {
    Bitmap b(18, 40);
    b.poly({{2, 2}, {16, 2}, {14, 34}, {4, 36}}, 1);
    b.poly({{5, 6}, {13, 6}, {12, 28}, {6, 30}}, 2);
    b.rect(8, 0, 2, 4, 3);
    b.rect(7, 12, 4, 8, 7);
    b.outline(8, false);
    return b;
}

Bitmap grateArt() {
    Bitmap b(120, 168);
    for (int x = 4; x < 116; x += 12) b.rect(x, 0, 3, 168, 5);
    for (int y = 8; y < 164; y += 16) b.rect(0, y, 120, 3, 5);
    b.rect(0, 0, 120, 4, 3);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(36, 12);
    b.ellipse(18, 6, 16, 4, 3);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(14, 13, 11);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(6, 6, 8), gs::rgb4(12, 10, 4), gs::rgb4(14, 4, 3), gs::rgb4(6, 12, 6),
                          gs::rgb4(8, 10, 13), gs::rgb4(3, 3, 4), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(9, 9, 8), gs::rgb4(6, 6, 6), gs::rgb4(3, 3, 4), gs::rgb4(3, 5, 3),
                            gs::rgb4(5, 5, 6), gs::rgb4(8, 5, 3), gs::rgb4(10, 8, 4), shadow, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SENTRY, {0, gs::rgb4(11, 12, 13), gs::rgb4(5, 6, 8), gs::rgb4(3, 4, 6), gs::rgb4(8, 9, 11),
                             gs::rgb4(12, 9, 7), gs::rgb4(8, 6, 3), gs::rgb4(12, 10, 4), shadow, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RAIDER, {0, gs::rgb4(14, 3, 3), gs::rgb4(8, 2, 2), gs::rgb4(6, 4, 3), gs::rgb4(12, 8, 6),
                             gs::rgb4(10, 10, 11), gs::rgb4(8, 6, 3), gs::rgb4(12, 11, 8), shadow, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RAM, {0, gs::rgb4(10, 7, 4), gs::rgb4(6, 4, 2), gs::rgb4(8, 8, 9), gs::rgb4(9, 7, 4),
                          gs::rgb4(5, 3, 2), gs::rgb4(12, 9, 7), gs::rgb4(4, 3, 3), shadow, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 13, 6), gs::rgb4(14, 7, 2), gs::rgb4(10, 3, 1), gs::rgb4(15, 15, 12),
                         gs::rgb4(12, 12, 13), gs::rgb4(6, 5, 4), gs::rgb4(9, 6, 3), shadow, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(8, 14, 7), gs::rgb4(10, 8, 3), gs::rgb4(4, 6, 10), gs::rgb4(13, 12, 9),
                           gs::rgb4(12, 9, 7), gs::rgb4(6, 8, 12), gs::rgb4(14, 12, 6), shadow, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(13, 14, 15), gs::rgb4(8, 9, 12), gs::rgb4(6, 4, 3), gs::rgb4(14, 8, 3),
                            gs::rgb4(12, 3, 3), gs::rgb4(7, 7, 8), gs::rgb4(10, 8, 5), shadow, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t field[16] = {
        0,
        gs::rgb4(2, 3, 2), gs::rgb4(1, 2, 1), gs::rgb4(3, 3, 2),
        gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 2),
        gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4),
        gs::rgb4(6, 6, 7), gs::rgb4(4, 4, 5), gs::rgb4(4, 4, 4),
        gs::rgb4(2, 3, 5), gs::rgb4(1, 2, 4), gs::rgb4(6, 7, 8),
        gs::rgb4(12, 10, 5), gs::rgb4(7, 7, 8),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FIELD * 16 + i, field[i]);

    loadFont(vdp, art);
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.sentry[0] = gs::uploadMipped(vdp, sentryArt(false));
    art.sentry[1] = gs::uploadMipped(vdp, sentryArt(true));
    art.runner[0] = gs::uploadMipped(vdp, runnerArt(0));
    art.runner[1] = gs::uploadMipped(vdp, runnerArt(1));
    art.shield = gs::uploadMipped(vdp, shieldArt());
    art.ram = gs::uploadMipped(vdp, ramArt());
    art.relief[0] = gs::uploadMipped(vdp, reliefArt(true));
    art.relief[1] = gs::uploadMipped(vdp, reliefArt(false));
    art.spear = gs::uploadMipped(vdp, spearArt());
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.post = gs::uploadMipped(vdp, postArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.grate = gs::uploadMipped(vdp, grateArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.setFogColor(gs::rgb4(1, 2, 4));
}

}  // namespace gate
