#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace ridge {
namespace {

using gs::Bitmap;

// Figure inks: 1 light, 2 main, 3 shade, 4 metal, 5 outline, 6 skin, 7 trim.
void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void legs(Bitmap& b, int x, int y, int step, int c) {
    int a = step ? 0 : 3;
    int d = step ? 3 : 0;
    b.rect(float(x), float(y + a), 7, float(13 - a), c);
    b.rect(float(x + 10), float(y + d), 7, float(13 - d), c);
    b.rect(float(x - 1), float(y + 12), 9, 3, 3);
    b.rect(float(x + 9), float(y + 12), 9, 3, 3);
}

Bitmap keeper(int step) {
    Bitmap b(48, 80);
    b.line(31, 3, 29, 70, 4, 2.2f);
    b.poly({{29, 1}, {35, 1}, {32, 8}}, 1);
    b.poly({{24, 14}, {14, 22}, {8, 68}, {40, 68}, {34, 20}}, 2);
    b.poly({{24, 18}, {18, 24}, {14, 66}, {28, 66}, {30, 24}}, 3);
    b.ellipse(24, 16, 8, 9, 2);
    b.ellipse(23, 15, 5, 6, 3);
    b.rect(12, 40, 22, 4, 6);
    b.rect(21, 38, 6, 7, 7);
    b.rect(16, 44, 5, 8, 3);
    legs(b, 14, 62, step, 3);
    b.outline(5, false);
    return b;
}

Bitmap raider(int step) {
    Bitmap b(46, 76);
    b.line(11, 4, 16, 68, 4, 2.0f);
    b.poly({{8, 2}, {15, 2}, {13, 11}}, 1);
    b.poly({{22, 6}, {12, 18}, {13, 60}, {34, 60}, {36, 18}}, 2);
    b.ellipse(23, 16, 9, 10, 2);
    b.ellipse(23, 15, 5, 6, 3);
    b.rect(18, 16, 10, 3, 6);
    b.set(20, 17, 1);
    b.set(26, 17, 1);
    b.rect(14, 36, 18, 4, 7);
    legs(b, 14, 56, step, 3);
    b.outline(5, false);
    return b;
}

Bitmap brute(int step) {
    Bitmap b(68, 84);
    b.ellipse(16, 44, 13, 18, 4);
    b.ellipse(16, 44, 8, 12, 2);
    b.rect(14, 34, 4, 18, 1);
    b.poly({{24, 20}, {30, 4}, {34, 18}}, 6);
    b.poly({{42, 18}, {46, 4}, {52, 20}}, 6);
    b.ellipse(38, 22, 12, 11, 2);
    b.ellipse(38, 22, 7, 7, 3);
    b.rect(32, 22, 12, 3, 6);
    b.set(34, 23, 1);
    b.set(41, 23, 1);
    b.poly({{22, 28}, {18, 34}, {22, 70}, {56, 70}, {58, 30}, {50, 24}}, 2);
    b.poly({{28, 32}, {26, 40}, {30, 68}, {44, 68}, {46, 36}}, 3);
    b.rect(24, 46, 28, 5, 7);
    b.line(50, 8, 54, 66, 4, 2.4f);
    b.rect(48, 6, 10, 5, 3);
    legs(b, 26, 66, step, 3);
    b.outline(5, false);
    return b;
}

Bitmap runner(int step) {
    Bitmap b(44, 74);
    float scarf = step ? 4.f : 0.f;
    b.poly({{18, 20}, {40, 24 + scarf}, {38, 32 + scarf}, {16, 28}}, 7);
    b.poly({{22, 10}, {14, 22}, {16, 52}, {32, 54}, {34, 20}}, 2);
    b.ellipse(22, 16, 8, 9, 2);
    b.ellipse(22, 16, 5, 5, 6);
    b.set(20, 16, 3);
    b.set(25, 16, 3);
    b.line(30, 18, 40, 48, 4, 1.8f);
    b.rect(12, 34, 16, 3, 7);
    legs(b, 13, 52, step, 3);
    b.outline(5, false);
    return b;
}

Bitmap boltArt() {
    Bitmap b(8, 20);
    b.rect(3, 0, 2, 20, 2);
    b.rect(3, 0, 2, 7, 1);
    b.set(2, 4, 5);
    b.set(5, 4, 5);
    return b;
}

Bitmap dustArt() {
    Bitmap b(36, 36);
    b.ellipse(18, 18, 14, 12, 4);
    b.ellipse(16, 16, 9, 8, 3);
    b.ellipse(15, 15, 4, 3, 1);
    for (int y = 0; y < 36; y++)
        for (int x = 0; x < 36; x++)
            if (b.get(x, y) && ((x * 3 + y) & 3) == 0 && std::hypot(x - 18.0f, y - 18.0f) > 8) b.set(x, y, 0);
    return b;
}

Bitmap flashArt() {
    Bitmap b(16, 16);
    b.poly({{8, 1}, {10, 6}, {15, 8}, {10, 10}, {8, 15}, {6, 10}, {1, 8}, {6, 6}}, 1);
    b.ellipse(8, 8, 3, 3, 2);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(64, 16);
    b.ellipse(32, 8, 28, 6, 1);
    return b;
}

Bitmap sunArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 12, 12, 7);
    b.ellipse(16, 16, 8, 8, 6);
    b.ellipse(13, 13, 3, 3, 1);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(72, 28);
    b.ellipse(22, 16, 16, 8, 2);
    b.ellipse(40, 14, 18, 10, 1);
    b.ellipse(56, 16, 12, 7, 2);
    return b;
}

Bitmap postArt() {
    Bitmap b(28, 72);
    b.rect(8, 16, 12, 54, 2);
    b.rect(10, 18, 4, 50, 1);
    b.rect(4, 10, 20, 10, 3);
    b.rect(6, 8, 16, 4, 2);
    b.rect(11, 40, 6, 8, 4);
    b.outline(5, false);
    return b;
}

Bitmap cairnArt() {
    Bitmap b(40, 48);
    b.ellipse(20, 36, 16, 8, 3);
    b.ellipse(20, 28, 12, 8, 2);
    b.ellipse(20, 20, 8, 7, 1);
    b.ellipse(20, 13, 5, 5, 6);
    b.outline(5, false);
    return b;
}

Bitmap bannerArt() {
    Bitmap b(40, 68);
    b.rect(8, 4, 3, 62, 4);
    b.rect(11, 10, 22, 16, 2);
    b.rect(13, 12, 18, 12, 3);
    b.poly({{18, 14}, {28, 18}, {18, 22}}, 6);
    b.rect(10, 26, 4, 3, 7);
    b.outline(5, false);
    return b;
}

Bitmap mouthArt() {
    Bitmap b(128, 8);
    for (int x = 4; x < 124; x++) {
        if ((x / 10) % 2 == 0) {
            b.set(x, 3, 6);
            b.set(x, 4, 1);
        }
    }
    return b;
}

Bitmap mountSharp() {
    Bitmap b(80, 52);
    b.poly({{0, 51}, {0, 34}, {16, 26}, {30, 38}, {48, 6}, {64, 28}, {80, 18}, {80, 51}}, 2);
    b.poly({{0, 51}, {10, 40}, {28, 34}, {44, 18}, {62, 36}, {80, 28}, {80, 51}}, 3);
    b.poly({{42, 14}, {48, 6}, {56, 16}}, 4);
    return b;
}

Bitmap mountLong() {
    Bitmap b(96, 44);
    b.poly({{0, 43}, {0, 24}, {18, 16}, {34, 28}, {52, 10}, {70, 22}, {96, 14}, {96, 43}}, 2);
    b.poly({{0, 43}, {14, 30}, {36, 22}, {58, 18}, {80, 26}, {96, 20}, {96, 43}}, 3);
    b.poly({{48, 16}, {52, 10}, {58, 16}}, 4);
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
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 15, 15), gs::rgb4(9, 10, 12), gs::rgb4(6, 7, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 5), gs::rgb4(10, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 5, 4), gs::rgb4(8, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 8, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 1)});

    setPal(vdp, PAL_YOU, {0, gs::rgb4(13, 12, 9), gs::rgb4(6, 7, 5), gs::rgb4(3, 4, 3), gs::rgb4(12, 13, 14),
                          gs::rgb4(1, 1, 1), gs::rgb4(8, 6, 4), gs::rgb4(9, 8, 5), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RAID, {0, gs::rgb4(14, 12, 9), gs::rgb4(11, 4, 3), gs::rgb4(6, 2, 2), gs::rgb4(10, 11, 12),
                           gs::rgb4(1, 1, 1), gs::rgb4(13, 9, 7), gs::rgb4(8, 7, 4), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BRUTE, {0, gs::rgb4(12, 12, 13), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4), gs::rgb4(10, 2, 2),
                            gs::rgb4(1, 1, 1), gs::rgb4(12, 10, 8), gs::rgb4(5, 4, 3), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RUN, {0, gs::rgb4(14, 13, 10), gs::rgb4(9, 8, 5), gs::rgb4(5, 4, 3), gs::rgb4(13, 13, 12),
                          gs::rgb4(1, 1, 1), gs::rgb4(13, 10, 8), gs::rgb4(8, 3, 3), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 13), gs::rgb4(15, 12, 4), gs::rgb4(12, 10, 8), gs::rgb4(7, 6, 5),
                         gs::rgb4(15, 8, 3), gs::rgb4(15, 14, 8), gs::rgb4(13, 10, 5), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(13, 12, 10), gs::rgb4(8, 7, 6), gs::rgb4(4, 4, 3), gs::rgb4(6, 7, 4),
                            gs::rgb4(1, 1, 1), gs::rgb4(14, 13, 11), gs::rgb4(9, 8, 6), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MOUNT, {0, gs::rgb4(8, 9, 11), gs::rgb4(4, 5, 7), gs::rgb4(2, 3, 5), gs::rgb4(10, 10, 11), 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(14, 13, 10), gs::rgb4(9, 2, 2), gs::rgb4(4, 1, 1), gs::rgb4(10, 9, 8),
                             gs::rgb4(1, 1, 1), gs::rgb4(12, 10, 4), gs::rgb4(6, 5, 4), 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t field[16] = {
        0,
        gs::rgb4(6, 7, 5),
        gs::rgb4(3, 4, 3),
        gs::rgb4(5, 5, 4),
        gs::rgb4(8, 7, 5),
        gs::rgb4(5, 4, 3),
        gs::rgb4(11, 10, 8),
        gs::rgb4(7, 6, 5),
        gs::rgb4(4, 4, 4),
        gs::rgb4(12, 11, 9),
        gs::rgb4(13, 12, 10),
        gs::rgb4(2, 3, 4),
        gs::rgb4(1, 2, 3),
        gs::rgb4(3, 4, 5),
        gs::rgb4(14, 13, 11),
        gs::rgb4(9, 8, 7),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FIELD * 16 + i, field[i]);

    art.you[0] = gs::uploadMipped(vdp, keeper(0));
    art.you[1] = gs::uploadMipped(vdp, keeper(1));
    art.raider[0] = gs::uploadMipped(vdp, raider(0));
    art.raider[1] = gs::uploadMipped(vdp, raider(1));
    art.brute[0] = gs::uploadMipped(vdp, brute(0));
    art.brute[1] = gs::uploadMipped(vdp, brute(1));
    art.runner[0] = gs::uploadMipped(vdp, runner(0));
    art.runner[1] = gs::uploadMipped(vdp, runner(1));
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.cairn = gs::uploadMipped(vdp, cairnArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.mouth = gs::uploadMipped(vdp, mouthArt());
    art.mount[0] = gs::uploadMipped(vdp, mountSharp());
    art.mount[1] = gs::uploadMipped(vdp, mountLong());
    loadFont(vdp, art);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace ridge
