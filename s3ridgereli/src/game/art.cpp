#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace reli {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void boots(Bitmap& b, int x, int y, int step, int cloth) {
    int la = step ? 3 : 0;
    int ra = step ? 0 : 3;
    b.rect(float(x), float(y + la), 6, float(12 - la), cloth);
    b.rect(float(x + 9), float(y + ra), 6, float(12 - ra), cloth);
    b.rect(float(x - 1), float(y + 11), 8, 3, 3);
    b.rect(float(x + 8), float(y + 11), 8, 3, 3);
}

Bitmap sentry(int step) {
    Bitmap b(48, 82);
    b.poly({{24, 20}, {9, 30}, {8, 62}, {40, 62}, {39, 30}}, 2);
    b.poly({{24, 26}, {16, 34}, {16, 60}, {27, 60}, {30, 34}}, 3);
    b.ellipse(24, 16, 11, 12, 2);
    b.ellipse(25, 18, 6, 7, 6);
    b.rect(19, 17, 12, 3, 3);
    b.set(21, 18, 1);
    b.set(28, 18, 1);
    b.line(36, 22, 20, 68, 4, 2.2f);
    b.line(20, 68, 14, 78, 1, 1.5f);
    b.rect(14, 42, 20, 4, 7);
    b.rect(22, 40, 5, 8, 4);
    boots(b, 15, 62, step, 3);
    b.outline(5, false);
    return b;
}

Bitmap skirm(int step) {
    Bitmap b(46, 78);
    b.line(8, 4, 14, 70, 4, 2.0f);
    b.poly({{5, 2}, {14, 2}, {11, 12}}, 1);
    b.poly({{20, 18}, {12, 28}, {14, 58}, {36, 58}, {38, 24}}, 2);
    b.ellipse(26, 18, 9, 10, 2);
    b.ellipse(27, 19, 5, 6, 6);
    b.rect(21, 18, 11, 3, 3);
    b.rect(16, 38, 18, 5, 7);
    boots(b, 16, 56, step, 3);
    b.outline(5, false);
    return b;
}

Bitmap porter(int step) {
    Bitmap b(68, 84);
    b.ellipse(16, 42, 14, 24, 2);
    b.ellipse(16, 42, 8, 16, 3);
    b.rect(14, 26, 4, 30, 1);
    b.ellipse(42, 18, 11, 11, 4);
    b.ellipse(42, 20, 6, 6, 6);
    b.rect(36, 18, 12, 3, 3);
    b.poly({{30, 26}, {26, 36}, {30, 64}, {56, 64}, {58, 34}, {48, 24}}, 2);
    b.poly({{34, 32}, {32, 42}, {36, 62}, {46, 62}, {48, 38}}, 3);
    b.rect(32, 46, 20, 5, 7);
    boots(b, 34, 62, step, 3);
    b.outline(5, false);
    return b;
}

Bitmap scout(int step) {
    Bitmap b(48, 76);
    float scarf = step ? 5.f : 0.f;
    b.poly({{18, 20}, {46, 10 + scarf}, {44, 18 + scarf}, {18, 28}}, 7);
    b.poly({{22, 12}, {14, 24}, {16, 52}, {32, 54}, {34, 22}}, 2);
    b.ellipse(22, 15, 8, 9, 2);
    b.ellipse(23, 16, 4, 5, 6);
    b.set(20, 16, 3);
    b.set(26, 16, 3);
    b.line(30, 24, 42, 60, 4, 1.6f);
    b.rect(14, 34, 14, 3, 7);
    boots(b, 14, 52, step, 3);
    b.outline(5, false);
    return b;
}

Bitmap boltArt() {
    Bitmap b(6, 22);
    b.rect(2, 0, 2, 22, 1);
    b.rect(2, 0, 2, 8, 6);
    b.set(1, 6, 7);
    b.set(4, 6, 7);
    return b;
}

Bitmap glintArt() {
    Bitmap b(16, 16);
    b.poly({{8, 0}, {10, 6}, {15, 8}, {10, 10}, {8, 15}, {6, 10}, {1, 8}, {6, 6}}, 1);
    b.ellipse(8, 8, 2, 2, 6);
    return b;
}

Bitmap dustArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 13, 10, 3);
    b.ellipse(15, 15, 8, 6, 2);
    b.ellipse(14, 14, 3, 2, 1);
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++)
            if (b.get(x, y) && ((x * 5 + y * 3) & 3) == 0 && std::hypot(x - 16.0, y - 16.0) > 7) b.set(x, y, 0);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(64, 16);
    b.ellipse(32, 8, 28, 5, 1);
    return b;
}

Bitmap bellArt() {
    Bitmap b(32, 36);
    b.poly({{9, 4}, {23, 4}, {28, 26}, {4, 26}}, 2);
    b.poly({{12, 6}, {20, 6}, {23, 24}, {9, 24}}, 1);
    b.rect(6, 26, 20, 4, 3);
    b.rect(14, 2, 4, 6, 4);
    b.ellipse(16, 30, 2, 2, 7);
    b.outline(5, false);
    return b;
}

Bitmap yokeArt() {
    Bitmap b(52, 78);
    b.rect(4, 10, 7, 64, 2);
    b.rect(41, 10, 7, 64, 3);
    b.rect(4, 6, 44, 9, 2);
    b.rect(8, 8, 36, 3, 1);
    b.rect(22, 14, 8, 6, 4);
    b.outline(5, false);
    return b;
}

Bitmap ropeArt() {
    Bitmap b(8, 52);
    for (int y = 0; y < 52; y++) {
        int x = 3 + int(std::sin(y * 0.5f) * 1.4f);
        b.set(x, y, 6);
        if (x + 1 < 8) b.set(x + 1, y, 7);
    }
    return b;
}

Bitmap padArt() {
    Bitmap b(52, 18);
    b.ellipse(26, 9, 24, 7, 3);
    b.ellipse(26, 9, 14, 4, 1);
    b.ellipse(26, 9, 4, 2, 6);
    return b;
}

Bitmap cairnArt() {
    Bitmap b(36, 44);
    b.ellipse(18, 34, 15, 7, 3);
    b.ellipse(18, 26, 11, 7, 2);
    b.ellipse(18, 18, 7, 6, 1);
    b.ellipse(18, 12, 4, 4, 6);
    b.outline(5, false);
    return b;
}

Bitmap bannerArt() {
    Bitmap b(28, 64);
    b.rect(6, 2, 4, 60, 4);
    b.rect(10, 8, 14, 18, 2);
    b.rect(12, 10, 10, 14, 7);
    b.poly({{14, 12}, {20, 17}, {14, 22}}, 1);
    b.outline(5, false);
    return b;
}

Bitmap lanternArt() {
    Bitmap b(14, 20);
    b.rect(6, 0, 2, 4, 3);
    b.rect(3, 4, 8, 12, 6);
    b.rect(5, 6, 4, 8, 1);
    b.rect(4, 16, 6, 3, 3);
    return b;
}

Bitmap peakLeft() {
    Bitmap b(96, 58);
    b.poly({{0, 57}, {0, 30}, {18, 34}, {36, 16}, {52, 28}, {70, 8}, {96, 26}, {96, 57}}, 2);
    b.poly({{0, 57}, {8, 40}, {32, 28}, {58, 22}, {96, 36}, {96, 57}}, 3);
    b.poly({{64, 16}, {70, 8}, {78, 18}}, 1);
    return b;
}

Bitmap peakRight() {
    Bitmap b(110, 50);
    b.poly({{0, 49}, {0, 28}, {22, 18}, {40, 30}, {62, 10}, {84, 24}, {110, 16}, {110, 49}}, 2);
    b.poly({{0, 49}, {16, 32}, {48, 24}, {80, 28}, {110, 22}, {110, 49}}, 3);
    b.poly({{56, 16}, {62, 10}, {70, 18}}, 1);
    return b;
}

Bitmap moonArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 11, 11, 2);
    b.ellipse(14, 14, 8, 8, 1);
    b.ellipse(11, 11, 3, 3, 3);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(70, 26);
    b.ellipse(20, 15, 16, 8, 2);
    b.ellipse(38, 13, 18, 9, 1);
    b.ellipse(54, 15, 12, 7, 2);
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
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t shade = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(10, 11, 12), gs::rgb4(6, 7, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_AMBER,
           {0, gs::rgb4(15, 12, 5), gs::rgb4(12, 8, 3), gs::rgb4(8, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_RED,
           {0, gs::rgb4(15, 5, 4), gs::rgb4(9, 2, 2), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_GREEN,
           {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 9, 4), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 3, 1)});

    setPal(vdp, PAL_SENTRY,
           {0, gs::rgb4(13, 14, 15), gs::rgb4(4, 6, 8), gs::rgb4(2, 3, 5), gs::rgb4(12, 11, 8), gs::rgb4(1, 1, 2),
            gs::rgb4(12, 8, 6), gs::rgb4(8, 7, 4), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_SKIRM,
           {0, gs::rgb4(14, 12, 9), gs::rgb4(10, 3, 3), gs::rgb4(5, 1, 2), gs::rgb4(9, 10, 11), gs::rgb4(1, 1, 1),
            gs::rgb4(13, 9, 7), gs::rgb4(14, 8, 3), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_PORTER,
           {0, gs::rgb4(12, 12, 13), gs::rgb4(5, 6, 7), gs::rgb4(2, 3, 4), gs::rgb4(11, 9, 6), gs::rgb4(1, 1, 1),
            gs::rgb4(12, 9, 7), gs::rgb4(8, 3, 3), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_SCOUT,
           {0, gs::rgb4(14, 13, 10), gs::rgb4(7, 6, 4), gs::rgb4(3, 3, 2), gs::rgb4(13, 13, 12), gs::rgb4(1, 1, 1),
            gs::rgb4(13, 10, 8), gs::rgb4(6, 8, 10), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 15, 13), gs::rgb4(12, 10, 8), gs::rgb4(7, 6, 5), 0, gs::rgb4(1, 1, 1), gs::rgb4(15, 12, 4),
            gs::rgb4(15, 8, 3), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(15, 13, 6), gs::rgb4(12, 9, 3), gs::rgb4(7, 5, 2), gs::rgb4(10, 8, 5), gs::rgb4(2, 1, 1),
            gs::rgb4(11, 9, 5), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(13, 12, 10), gs::rgb4(8, 7, 6), gs::rgb4(4, 4, 3), gs::rgb4(6, 6, 5), gs::rgb4(1, 1, 1),
            gs::rgb4(11, 9, 6), gs::rgb4(6, 5, 3), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_RELIEF,
           {0, gs::rgb4(13, 15, 12), gs::rgb4(3, 7, 4), gs::rgb4(1, 4, 3), gs::rgb4(12, 12, 8), gs::rgb4(1, 1, 2),
            gs::rgb4(12, 9, 6), gs::rgb4(14, 12, 4), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_MOUNT,
           {0, gs::rgb4(12, 13, 14), gs::rgb4(4, 5, 8), gs::rgb4(2, 3, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});

    const uint16_t field[16] = {
        0,
        gs::rgb4(5, 6, 4),
        gs::rgb4(2, 3, 2),
        gs::rgb4(4, 5, 3),
        gs::rgb4(9, 8, 6),
        gs::rgb4(5, 4, 3),
        gs::rgb4(10, 9, 7),
        gs::rgb4(6, 5, 4),
        gs::rgb4(13, 12, 9),
        gs::rgb4(4, 4, 3),
        gs::rgb4(8, 7, 5),
        gs::rgb4(2, 3, 5),
        gs::rgb4(1, 2, 4),
        gs::rgb4(3, 4, 6),
        gs::rgb4(8, 9, 6),
        gs::rgb4(14, 13, 11),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FIELD * 16 + i, field[i]);

    art.sentry[0] = gs::uploadMipped(vdp, sentry(0));
    art.sentry[1] = gs::uploadMipped(vdp, sentry(1));
    art.skirm[0] = gs::uploadMipped(vdp, skirm(0));
    art.skirm[1] = gs::uploadMipped(vdp, skirm(1));
    art.porter[0] = gs::uploadMipped(vdp, porter(0));
    art.porter[1] = gs::uploadMipped(vdp, porter(1));
    art.scout[0] = gs::uploadMipped(vdp, scout(0));
    art.scout[1] = gs::uploadMipped(vdp, scout(1));
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.glint = gs::uploadMipped(vdp, glintArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.yoke = gs::uploadMipped(vdp, yokeArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.pad = gs::uploadMipped(vdp, padArt());
    art.cairn = gs::uploadMipped(vdp, cairnArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.lantern = gs::uploadMipped(vdp, lanternArt());
    art.peakL = gs::uploadMipped(vdp, peakLeft());
    art.peakR = gs::uploadMipped(vdp, peakRight());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.star = gs::uploadMipped(vdp, starArt());
    loadFont(vdp, art);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace reli
