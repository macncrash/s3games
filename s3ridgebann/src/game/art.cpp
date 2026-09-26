#include "game/art.h"

#include <initializer_list>
#include <string>

namespace rbann {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void legs(Bitmap& b, int x, int y, int step) {
    int liftL = step ? 0 : 4;
    int liftR = step ? 4 : 0;
    b.rect(float(x), float(y + liftL), 7, float(14 - liftL), 3);
    b.rect(float(x + 12), float(y + liftR), 7, float(14 - liftR), 3);
    b.rect(float(x - 1), float(y + 12 - liftL / 2), 9, 4, 4);
    b.rect(float(x + 11), float(y + 12 - liftR / 2), 9, 4, 4);
}

Bitmap runner(int step) {
    Bitmap b(48, 78);
    b.poly({{24, 3}, {9, 16}, {13, 28}, {35, 28}, {39, 16}}, 2);
    b.ellipse(24, 20, 7, 8, 6);
    b.rect(17, 16, 14, 4, 2);
    b.rect(20, 21, 3, 2, 5);
    b.rect(27, 21, 3, 2, 5);
    b.poly({{24, 26}, {8, 36}, {6, 58}, {42, 58}, {40, 36}}, 1);
    b.poly({{24, 30}, {17, 36}, {16, 56}, {26, 56}, {25, 36}}, 2);
    b.rect(18, 42, 12, 3, 8);
    b.poly({{28, 32}, {44, 40}, {40, 46}, {26, 38}}, 7);
    b.rect(5, 36, 6, 16, 1);
    b.rect(37, 36, 6, 16, 1);
    b.rect(4, 50, 7, 5, 6);
    b.rect(37, 50, 7, 5, 6);
    legs(b, 14, 56, step);
    b.outline(5, false);
    return b;
}

Bitmap plantPose() {
    Bitmap b(52, 80);
    b.poly({{26, 4}, {12, 16}, {16, 28}, {36, 28}, {40, 16}}, 2);
    b.ellipse(26, 20, 7, 8, 6);
    b.rect(19, 16, 14, 4, 2);
    b.poly({{26, 26}, {12, 36}, {10, 58}, {42, 58}, {40, 36}}, 1);
    b.poly({{26, 30}, {19, 36}, {18, 56}, {28, 56}, {27, 36}}, 2);
    b.rect(20, 42, 12, 3, 8);
    b.rect(4, 16, 6, 18, 1);
    b.rect(42, 16, 6, 18, 1);
    b.rect(2, 12, 8, 6, 6);
    b.rect(42, 12, 8, 6, 6);
    b.rect(16, 56, 7, 14, 3);
    b.rect(29, 56, 7, 14, 3);
    b.rect(14, 68, 10, 4, 4);
    b.rect(27, 68, 10, 4, 4);
    b.outline(5, false);
    return b;
}

Bitmap sentinel(int step) {
    Bitmap b(52, 76);
    b.ellipse(24, 12, 8, 6, 8);
    b.rect(16, 16, 16, 8, 8);
    b.ellipse(24, 22, 6, 6, 6);
    b.rect(19, 21, 3, 2, 5);
    b.rect(27, 21, 3, 2, 5);
    b.poly({{24, 26}, {10, 34}, {8, 56}, {40, 56}, {38, 34}}, 1);
    b.poly({{24, 30}, {18, 36}, {17, 54}, {27, 54}, {26, 36}}, 2);
    b.ellipse(10, 42, 7, 8, 2);
    b.ellipse(10, 42, 4, 5, 7);
    b.line(40, 18, 48, 8, 3, 2);
    b.rect(46, 4, 4, 6, 7);
    b.rect(6, 36, 5, 14, 1);
    legs(b, 14, 54, step);
    b.outline(5, false);
    return b;
}

Bitmap bannerCloth(int frame) {
    Bitmap b(48, 74);
    b.rect(7, 2, 4, 68, 4);
    b.rect(5, 1, 8, 5, 3);
    b.ellipse(9, 3, 2, 2, 1);
    int dy = frame ? 4 : 0;
    b.poly({{11, 8}, {42, 12 + dy}, {38, 36 + dy}, {11, 30}}, 2);
    b.poly({{13, 12}, {34, 15 + dy}, {32, 26 + dy}, {13, 23}}, 1);
    b.poly({{20, 15}, {30, 18 + dy}, {24, 25 + dy}}, 7);
    b.poly({{11, 30}, {38, 36 + dy}, {34, 44 + dy}, {11, 38}}, 6);
    b.line(11, 30, 38, 36 + dy, 3, 1);
    b.outline(5, false);
    return b;
}

Bitmap staffArt() {
    Bitmap b(40, 90);
    b.rect(17, 12, 6, 70, 7);
    b.rect(18, 14, 2, 64, 1);
    b.rect(6, 10, 28, 6, 2);
    b.rect(8, 7, 24, 4, 3);
    b.ellipse(20, 12, 4, 4, 6);
    b.ellipse(20, 12, 2, 2, 0);
    b.rect(10, 78, 20, 6, 3);
    b.rect(8, 82, 24, 4, 2);
    b.outline(5, false);
    return b;
}

Bitmap cairnArt() {
    Bitmap b(56, 46);
    b.rect(4, 34, 48, 8, 3);
    b.rect(8, 26, 40, 9, 2);
    b.rect(14, 18, 30, 9, 1);
    b.rect(18, 10, 20, 9, 6);
    b.rect(22, 3, 12, 8, 3);
    b.rect(6, 36, 8, 3, 4);
    b.rect(40, 28, 6, 3, 4);
    b.outline(5, false);
    return b;
}

Bitmap stoneArt() {
    Bitmap b(36, 28);
    b.poly({{4, 24}, {10, 8}, {26, 6}, {32, 18}, {22, 26}}, 2);
    b.poly({{12, 16}, {18, 10}, {26, 14}, {20, 22}}, 1);
    b.rect(8, 20, 10, 3, 4);
    b.outline(5, false);
    return b;
}

Bitmap dustArt() {
    Bitmap b(28, 20);
    b.ellipse(14, 11, 12, 6, 2);
    b.ellipse(12, 10, 7, 4, 1);
    b.ellipse(16, 12, 3, 2, 3);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(64, 16);
    b.ellipse(32, 8, 26, 5, 1);
    return b;
}

Bitmap sunArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 8, 8, 1);
    b.ellipse(16, 16, 5, 5, 4);
    b.rect(15, 1, 2, 5, 2);
    b.rect(15, 26, 2, 5, 2);
    b.rect(1, 15, 5, 2, 2);
    b.rect(26, 15, 5, 2, 2);
    b.line(5, 5, 9, 9, 2, 1);
    b.line(27, 5, 23, 9, 2, 1);
    b.line(5, 27, 9, 23, 2, 1);
    b.line(27, 27, 23, 23, 2, 1);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(80, 28);
    b.ellipse(24, 16, 16, 8, 2);
    b.ellipse(44, 13, 18, 9, 1);
    b.ellipse(62, 17, 12, 6, 2);
    return b;
}

Bitmap peakNear() {
    Bitmap b(96, 58);
    b.poly({{0, 57}, {0, 36}, {16, 30}, {34, 10}, {52, 28}, {70, 16}, {96, 34}, {96, 57}}, 2);
    b.poly({{0, 57}, {14, 42}, {32, 26}, {50, 34}, {72, 24}, {96, 40}, {96, 57}}, 3);
    b.poly({{28, 18}, {34, 10}, {42, 20}}, 1);
    b.poly({{64, 22}, {70, 16}, {78, 24}}, 1);
    return b;
}

Bitmap peakFar() {
    Bitmap b(110, 46);
    b.poly({{0, 45}, {0, 26}, {22, 18}, {46, 6}, {68, 20}, {90, 12}, {110, 24}, {110, 45}}, 2);
    b.poly({{0, 45}, {18, 30}, {44, 16}, {70, 24}, {92, 18}, {110, 28}, {110, 45}}, 3);
    b.poly({{40, 14}, {46, 6}, {54, 16}}, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(9, 10, 12), gs::rgb4(5, 6, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 4), gs::rgb4(10, 7, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 8), gs::rgb4(3, 8, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           gs::rgb4(1, 3, 1)});

    setPal(vdp, PAL_YOU, {0, gs::rgb4(12, 8, 3), gs::rgb4(7, 4, 2), gs::rgb4(4, 2, 1), gs::rgb4(2, 2, 2), ink,
                          gs::rgb4(13, 9, 6), gs::rgb4(14, 12, 8), gs::rgb4(11, 8, 3), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOE, {0, gs::rgb4(6, 8, 6), gs::rgb4(3, 5, 4), gs::rgb4(8, 6, 3), gs::rgb4(2, 2, 2), ink,
                          gs::rgb4(12, 9, 7), gs::rgb4(10, 11, 12), gs::rgb4(4, 5, 5), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(14, 10, 2), gs::rgb4(11, 2, 2), gs::rgb4(13, 9, 3), gs::rgb4(5, 4, 3), ink,
                             gs::rgb4(14, 12, 8), gs::rgb4(3, 1, 1), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(12, 11, 9), gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 4), gs::rgb4(5, 6, 3), ink,
                            gs::rgb4(10, 10, 8), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 13, 8), gs::rgb4(12, 9, 5), gs::rgb4(7, 6, 4), gs::rgb4(15, 12, 4), 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MOUNT, {0, gs::rgb4(13, 12, 12), gs::rgb4(5, 6, 8), gs::rgb4(3, 3, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, ink});

    const uint16_t field[16] = {
        0,
        gs::rgb4(4, 5, 4),
        gs::rgb4(3, 3, 3),
        gs::rgb4(5, 5, 4),
        gs::rgb4(6, 6, 5),
        gs::rgb4(4, 4, 3),
        gs::rgb4(7, 6, 5),
        gs::rgb4(5, 5, 4),
        gs::rgb4(10, 9, 8),
        gs::rgb4(6, 5, 4),
        gs::rgb4(8, 7, 6),
        gs::rgb4(3, 4, 6),
        gs::rgb4(2, 3, 5),
        gs::rgb4(4, 5, 6),
        gs::rgb4(12, 11, 9),
        gs::rgb4(8, 7, 6),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_FIELD * 16 + i, field[i]);

    art.runner[0] = gs::uploadMipped(vdp, runner(0));
    art.runner[1] = gs::uploadMipped(vdp, runner(1));
    art.plant = gs::uploadMipped(vdp, plantPose());
    art.foe[0] = gs::uploadMipped(vdp, sentinel(0));
    art.foe[1] = gs::uploadMipped(vdp, sentinel(1));
    art.banner[0] = gs::uploadMipped(vdp, bannerCloth(0));
    art.banner[1] = gs::uploadMipped(vdp, bannerCloth(1));
    art.staff = gs::uploadMipped(vdp, staffArt());
    art.cairn = gs::uploadMipped(vdp, cairnArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.peak[0] = gs::uploadMipped(vdp, peakNear());
    art.peak[1] = gs::uploadMipped(vdp, peakFar());
    loadFont(vdp, art);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace rbann
