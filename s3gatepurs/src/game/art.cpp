#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace purs {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

// Shared rear-view indices: 1 highlight, 2 body, 3 shade, 4 iron, 5 lamp,
// 6 glass, 7 stripe, 8 outline, 9 tyre, 10 hub, 11 stack, 12 glint.
Bitmap scoutArt() {
    Bitmap b(48, 42);
    b.poly({{8, 14}, {40, 14}, {46, 34}, {2, 34}}, 2);
    b.poly({{12, 16}, {36, 16}, {40, 28}, {8, 28}}, 1);
    b.rect(14, 8, 20, 8, 3);
    b.rect(18, 4, 12, 6, 4);
    b.rect(16, 18, 16, 7, 6);
    b.rect(18, 19, 5, 3, 12);
    b.rect(4, 30, 40, 5, 4);
    b.rect(6, 31, 36, 2, 7);
    b.ellipse(10, 36, 5, 5, 9);
    b.ellipse(38, 36, 5, 5, 9);
    b.ellipse(10, 36, 2, 2, 10);
    b.ellipse(38, 36, 2, 2, 10);
    b.rect(8, 22, 6, 5, 4);
    b.rect(34, 22, 6, 5, 4);
    b.ellipse(11, 24, 2, 2, 5);
    b.ellipse(37, 24, 2, 2, 5);
    b.rect(22, 30, 4, 3, 11);
    b.outline(8, false);
    return b;
}

Bitmap wagonArt() {
    Bitmap b(52, 60);
    b.rect(8, 6, 36, 28, 2);
    b.rect(10, 8, 32, 6, 1);
    b.poly({{6, 4}, {46, 4}, {42, 10}, {10, 10}}, 3);
    b.rect(14, 16, 24, 12, 6);
    b.rect(16, 18, 8, 4, 12);
    b.rect(6, 34, 40, 16, 2);
    b.rect(8, 36, 36, 4, 1);
    b.rect(4, 46, 44, 5, 4);
    b.rect(6, 47, 40, 2, 7);
    b.ellipse(12, 54, 6, 5, 9);
    b.ellipse(40, 54, 6, 5, 9);
    b.ellipse(12, 54, 2, 2, 10);
    b.ellipse(40, 54, 2, 2, 10);
    b.rect(8, 40, 7, 5, 4);
    b.rect(37, 40, 7, 5, 4);
    b.ellipse(11, 42, 2, 2, 5);
    b.ellipse(40, 42, 2, 2, 5);
    b.rect(24, 48, 4, 4, 11);
    b.outline(8, false);
    return b;
}

Bitmap heavyArt() {
    Bitmap b(68, 48);
    b.poly({{6, 18}, {62, 18}, {64, 38}, {4, 38}}, 2);
    b.poly({{12, 20}, {56, 20}, {54, 30}, {14, 30}}, 1);
    b.rect(18, 8, 32, 12, 3);
    b.rect(28, 4, 12, 6, 4);
    b.rect(22, 12, 24, 6, 6);
    b.rect(24, 13, 6, 3, 12);
    b.rect(4, 34, 60, 6, 4);
    b.rect(8, 35, 52, 2, 7);
    b.ellipse(12, 42, 6, 5, 9);
    b.ellipse(56, 42, 6, 5, 9);
    b.ellipse(12, 42, 2, 2, 10);
    b.ellipse(56, 42, 2, 2, 10);
    b.rect(10, 26, 8, 6, 4);
    b.rect(50, 26, 8, 6, 4);
    b.ellipse(14, 29, 2, 2, 5);
    b.ellipse(54, 29, 2, 2, 5);
    b.rect(20, 6, 4, 8, 11);
    b.rect(44, 6, 4, 8, 11);
    b.rect(30, 36, 8, 3, 3);
    b.outline(8, false);
    return b;
}

Bitmap youArt() {
    Bitmap b(50, 54);
    b.poly({{10, 18}, {40, 18}, {46, 40}, {4, 40}}, 2);
    b.poly({{14, 20}, {36, 20}, {38, 30}, {12, 30}}, 1);
    b.rect(16, 8, 18, 12, 3);
    b.rect(18, 4, 14, 6, 4);
    b.rect(20, 5, 10, 2, 7);
    b.ellipse(25, 6, 2, 2, 5);
    b.rect(16, 22, 18, 8, 6);
    b.rect(18, 23, 6, 3, 12);
    b.rect(6, 36, 38, 6, 4);
    b.rect(8, 37, 34, 2, 7);
    b.ellipse(12, 46, 6, 5, 9);
    b.ellipse(38, 46, 6, 5, 9);
    b.ellipse(12, 46, 2, 2, 10);
    b.ellipse(38, 46, 2, 2, 10);
    b.rect(8, 30, 7, 5, 4);
    b.rect(35, 30, 7, 5, 4);
    b.ellipse(11, 32, 2, 2, 5);
    b.ellipse(38, 32, 2, 2, 5);
    b.rect(23, 40, 4, 4, 11);
    b.outline(8, false);
    return b;
}

Bitmap towerArt() {
    Bitmap b(36, 96);
    b.rect(6, 16, 24, 80, 2);
    b.rect(8, 18, 8, 76, 1);
    b.rect(22, 18, 6, 76, 3);
    for (int i = 0; i < 4; i++) {
        b.rect(4 + i * 8, 4, 6, 14, 2);
        b.rect(4 + i * 8, 4, 6, 3, 1);
    }
    b.rect(2, 16, 32, 4, 4);
    for (int y = 28; y < 90; y += 14) b.rect(10, y, 8, 10, 6);
    b.rect(12, 32, 4, 4, 12);
    b.rect(14, 86, 8, 6, 5);
    b.outline(8, false);
    return b;
}

Bitmap grateArt() {
    Bitmap b(72, 48);
    b.rect(0, 0, 72, 4, 4);
    b.rect(0, 40, 72, 4, 4);
    for (int x = 4; x < 70; x += 8) b.rect(x, 4, 3, 36, 3);
    for (int y = 12; y < 40; y += 10) b.rect(0, y, 72, 2, 4);
    for (int x = 6; x < 70; x += 8) b.poly({{x, 44}, {x + 6, 44}, {x + 3, 48}}, 3);
    return b;
}

Bitmap beamArt() {
    Bitmap b(80, 14);
    b.rect(0, 2, 80, 10, 2);
    b.rect(0, 2, 80, 3, 1);
    b.rect(0, 9, 80, 3, 3);
    for (int x = 6; x < 76; x += 10) b.rect(x, 5, 2, 4, 4);
    b.outline(8, false);
    return b;
}

Bitmap bannerArt() {
    Bitmap b(28, 22);
    b.rect(4, 0, 4, 6, 4);
    b.rect(20, 0, 4, 6, 4);
    b.poly({{2, 4}, {26, 4}, {24, 16}, {14, 20}, {4, 16}}, 5);
    b.rect(8, 8, 12, 3, 7);
    b.outline(8, false);
    return b;
}

Bitmap flameArt(int frame) {
    Bitmap b(14, 22);
    float lift = frame ? 2.f : 0.f;
    b.ellipse(7, 14 - lift, 5, 7, 3);
    b.ellipse(7, 12 - lift, 3, 5, 2);
    b.ellipse(7, 10 - lift, 2, 3, 1);
    return b;
}

Bitmap lampArt() {
    Bitmap b(10, 8);
    b.ellipse(5, 4, 4, 3, 2);
    b.ellipse(5, 4, 2, 2, 1);
    return b;
}

Bitmap shotArt() {
    Bitmap b(6, 16);
    b.rect(2, 2, 2, 12, 2);
    b.rect(2, 0, 2, 4, 1);
    b.rect(1, 8, 4, 2, 3);
    return b;
}

Bitmap boltArt() {
    Bitmap b(8, 14);
    b.poly({{4, 0}, {7, 6}, {4, 13}, {1, 6}}, 1);
    b.poly({{4, 3}, {6, 6}, {4, 10}, {2, 6}}, 2);
    return b;
}

Bitmap puffArt() {
    Bitmap b(16, 14);
    b.ellipse(8, 8, 6, 4, 3);
    b.ellipse(6, 6, 3, 3, 2);
    b.ellipse(11, 7, 2, 2, 1);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(10, 10);
    b.poly({{5, 0}, {6, 4}, {10, 5}, {6, 6}, {5, 10}, {4, 6}, {0, 5}, {4, 4}}, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(36, 10);
    b.ellipse(18, 5, 16, 4, 1);
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 48);
    b.rect(6, 10, 4, 34, 2);
    b.rect(5, 42, 6, 4, 3);
    b.rect(3, 8, 10, 4, 4);
    b.ellipse(8, 6, 3, 3, 5);
    b.outline(8, false);
    return b;
}

Bitmap treeArt() {
    Bitmap b(36, 58);
    b.rect(16, 36, 4, 20, 4);
    b.poly({{18, 4}, {34, 40}, {2, 40}}, 2);
    b.poly({{18, 10}, {28, 36}, {8, 36}}, 1);
    b.poly({{18, 16}, {24, 32}, {12, 32}}, 3);
    b.outline(8, false);
    return b;
}

Bitmap moonArt() {
    Bitmap b(32, 32);
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            float dx = x - 14.f, dy = y - 16.f;
            float dx2 = x - 20.f, dy2 = y - 14.f;
            if (dx * dx + dy * dy < 12.f * 12.f && dx2 * dx2 + dy2 * dy2 > 10.f * 10.f) b.set(x, y, 1);
        }
    }
    b.set(10, 12, 2);
    b.set(12, 18, 2);
    b.set(8, 20, 3);
    return b;
}

Bitmap starArt() {
    Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 1, 1);
    b.set(2, 2, 1);
    b.set(2, 3, 1);
    b.set(2, 4, 1);
    b.set(0, 2, 1);
    b.set(1, 2, 1);
    b.set(3, 2, 1);
    b.set(4, 2, 1);
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
    const uint16_t ink = gs::rgb4(14, 14, 12);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_INK, {0, ink, gs::rgb4(10, 10, 9), gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(14, 12, 4), gs::rgb4(15, 14, 8), gs::rgb4(10, 7, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(14, 4, 3), gs::rgb4(15, 8, 4), gs::rgb4(8, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 14, 7), gs::rgb4(12, 15, 10), gs::rgb4(3, 8, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    setPal(vdp, PAL_YOU, {0, gs::rgb4(10, 12, 14), gs::rgb4(5, 7, 10), gs::rgb4(3, 4, 6), gs::rgb4(2, 2, 3), gs::rgb4(6, 14, 15),
                          gs::rgb4(1, 2, 4), gs::rgb4(5, 13, 14), shadow, gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 5),
                          gs::rgb4(9, 12, 14), 0, 0, shadow});
    setPal(vdp, PAL_SCOUT, {0, gs::rgb4(14, 6, 5), gs::rgb4(10, 2, 2), gs::rgb4(6, 1, 1), gs::rgb4(3, 2, 2), gs::rgb4(15, 4, 3),
                            gs::rgb4(2, 1, 2), gs::rgb4(12, 8, 6), shadow, gs::rgb4(2, 2, 2), gs::rgb4(7, 6, 6), gs::rgb4(5, 4, 4),
                            gs::rgb4(8, 6, 6), 0, 0, shadow});
    setPal(vdp, PAL_WAGON, {0, gs::rgb4(12, 10, 6), gs::rgb4(8, 7, 3), gs::rgb4(5, 4, 2), gs::rgb4(3, 3, 2), gs::rgb4(15, 12, 3),
                            gs::rgb4(2, 2, 3), gs::rgb4(10, 8, 3), shadow, gs::rgb4(2, 2, 2), gs::rgb4(7, 7, 6), gs::rgb4(6, 5, 3),
                            gs::rgb4(9, 8, 6), 0, 0, shadow});
    setPal(vdp, PAL_HEAVY, {0, gs::rgb4(8, 10, 6), gs::rgb4(4, 6, 3), gs::rgb4(2, 3, 2), gs::rgb4(2, 2, 2), gs::rgb4(13, 14, 11),
                            gs::rgb4(1, 2, 2), gs::rgb4(9, 8, 4), shadow, gs::rgb4(1, 1, 1), gs::rgb4(6, 6, 5), gs::rgb4(5, 5, 4),
                            gs::rgb4(7, 8, 6), 0, 0, shadow});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(10, 9, 8), gs::rgb4(7, 6, 6), gs::rgb4(4, 4, 4), gs::rgb4(3, 3, 3), gs::rgb4(12, 3, 2),
                            gs::rgb4(2, 2, 3), gs::rgb4(12, 10, 4), shadow, gs::rgb4(5, 5, 5), gs::rgb4(8, 8, 7), gs::rgb4(6, 5, 4),
                            gs::rgb4(9, 9, 8), 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 8), gs::rgb4(14, 8, 3), gs::rgb4(8, 7, 6), gs::rgb4(15, 15, 12), gs::rgb4(6, 5, 4),
                         gs::rgb4(10, 9, 8), 0, shadow, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(13, 13, 10), gs::rgb4(8, 8, 7), gs::rgb4(6, 6, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SHOT, {0, gs::rgb4(14, 15, 15), gs::rgb4(6, 13, 14), gs::rgb4(3, 7, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOLT, {0, gs::rgb4(15, 14, 8), gs::rgb4(14, 7, 2), gs::rgb4(8, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(4, 6, 3), gs::rgb4(2, 4, 2), gs::rgb4(3, 5, 2), gs::rgb4(4, 3, 2), gs::rgb4(14, 12, 6),
                           0, 0, shadow, 0, 0, 0, 0, 0, 0, 0, shadow});

    int r = PAL_ROAD * 16;
    vdp.setColor(r + 0, 0);
    vdp.setColor(r + 1, gs::rgb4(2, 4, 2));
    vdp.setColor(r + 2, gs::rgb4(1, 2, 1));
    vdp.setColor(r + 3, gs::rgb4(3, 4, 2));
    vdp.setColor(r + 4, gs::rgb4(3, 3, 2));
    vdp.setColor(r + 5, gs::rgb4(2, 2, 1));
    vdp.setColor(r + 6, gs::rgb4(2, 2, 3));
    vdp.setColor(r + 7, gs::rgb4(4, 4, 5));
    vdp.setColor(r + 8, gs::rgb4(5, 5, 4));
    vdp.setColor(r + 9, gs::rgb4(1, 1, 2));
    vdp.setColor(r + 10, gs::rgb4(3, 3, 4));
    vdp.setColor(r + 11, gs::rgb4(2, 3, 4));
    vdp.setColor(r + 12, gs::rgb4(3, 4, 5));
    vdp.setColor(r + 13, gs::rgb4(6, 7, 8));
    vdp.setColor(r + 14, gs::rgb4(12, 11, 6));
    vdp.setColor(r + 15, gs::rgb4(6, 6, 7));

    loadFont(vdp, art);
    art.you = gs::uploadMipped(vdp, youArt());
    art.scout = gs::uploadMipped(vdp, scoutArt());
    art.wagon = gs::uploadMipped(vdp, wagonArt());
    art.heavy = gs::uploadMipped(vdp, heavyArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.grate = gs::uploadMipped(vdp, grateArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.shot = gs::uploadMipped(vdp, shotArt());
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.setFogColor(gs::rgb4(2, 2, 4));
}

}  // namespace purs
