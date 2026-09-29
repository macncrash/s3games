#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace rdawn {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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

void inkPal(gs::VDP& vdp, int pal, int r, int g, int b) {
    setPal(vdp, pal, {0, gs::rgb4(r, g, b), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
}

gs::Bitmap redoubtArt() {
    gs::Bitmap b(200, 78);
    b.poly({{0, 70}, {28, 28}, {172, 28}, {200, 70}}, 2);
    b.poly({{18, 70}, {40, 34}, {160, 34}, {182, 70}}, 3);
    b.rect(36, 30, 128, 8, 4);
    for (int i = 0; i < 9; i++) {
        b.rect(34 + i * 15, 18, 6, 16, 5);
        b.rect(33 + i * 15, 16, 8, 4, 6);
    }
    b.rect(92, 42, 16, 22, 7);
    b.rect(96, 46, 8, 6, 8);
    b.rect(8, 62, 184, 10, 1);
    b.line(0, 28, 28, 70, 9, 1.4f);
    b.line(200, 28, 172, 70, 9, 1.4f);
    b.outline(10, false);
    return b;
}

gs::Bitmap ditchArt() {
    gs::Bitmap b(220, 18);
    b.ellipse(110, 10, 104, 7, 1);
    b.ellipse(110, 9, 88, 4, 2);
    b.ellipse(70, 10, 10, 3, 3);
    b.ellipse(150, 11, 12, 3, 3);
    return b;
}

gs::Bitmap brazierArt() {
    gs::Bitmap b(28, 24);
    b.poly({{4, 8}, {24, 8}, {20, 16}, {8, 16}}, 1);
    b.rect(6, 6, 16, 4, 2);
    b.rect(12, 16, 4, 6, 3);
    b.rect(7, 20, 14, 3, 4);
    b.ellipse(14, 9, 5, 2, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap flameArt(int step) {
    gs::Bitmap b(18, 30);
    float lean = step ? 3.f : -2.f;
    b.ellipse(9 + lean, 18, 5.5f, 9, 1);
    b.ellipse(9 + lean * 0.35f, 12, 3.4f, 7, 2);
    b.ellipse(9, 8, 1.8f, 4, 3);
    b.set(9, 4, 3);
    return b;
}

gs::Bitmap watchArt(int step) {
    gs::Bitmap b(32, 46);
    b.ellipse(15, 8, 6, 5, 3);
    b.rect(9, 6, 12, 3, 4);
    b.rect(8, 14, 14, 13, 1);
    b.rect(8, 14, 5, 13, 2);
    b.rect(18, 18, 9, 3, 5);
    b.ellipse(26, 18, 3, 3, 6);
    int lx = step ? 7 : 11;
    int rx = step ? 17 : 13;
    b.rect(lx, 27, 5, 12, 1);
    b.rect(rx, 27, 5, 12, 2);
    b.rect(lx - 1, 38, 7, 4, 7);
    b.rect(rx - 1, 38, 7, 4, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap barrelArt() {
    gs::Bitmap b(22, 26);
    b.ellipse(11, 8, 8, 4, 2);
    b.rect(3, 8, 16, 12, 1);
    b.ellipse(11, 20, 8, 4, 3);
    b.line(3, 12, 19, 12, 4, 1.f);
    b.line(3, 16, 19, 16, 4, 1.f);
    b.rect(9, 2, 4, 6, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(36, 28);
    b.rect(4, 2, 2, 24, 1);
    b.poly({{6, 3}, {32, 8}, {28, 14}, {6, 12}}, 2);
    b.poly({{10, 5}, {22, 8}, {18, 12}, {8, 10}}, 3);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(26, 26);
    b.ellipse(13, 13, 11, 11, 1);
    b.ellipse(17, 11, 8, 8, 0);
    b.ellipse(9, 15, 1.6f, 1.6f, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 7, 7, 1);
    b.ellipse(14, 14, 3.5f, 3.5f, 2);
    for (int i = 0; i < 8; i++) {
        float a = i * 0.785f;
        int x = int(14 + std::cos(a) * 11);
        int y = int(14 + std::sin(a) * 11);
        b.rect(x - 1, y - 1, 3, 3, 3);
    }
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(7, 7);
    b.set(3, 0, 1);
    b.set(3, 1, 1);
    b.set(3, 2, 2);
    b.rect(1, 3, 5, 1, 1);
    b.set(3, 4, 2);
    b.set(3, 5, 1);
    b.set(3, 6, 1);
    return b;
}

gs::Bitmap dropArt() {
    gs::Bitmap b(5, 8);
    b.ellipse(2, 4, 1.6f, 2.4f, 1);
    b.set(2, 1, 2);
    b.set(2, 6, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    inkPal(vdp, PAL_HUD, 13, 12, 9);
    setPal(vdp, PAL_EARTH,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(3, 5, 3), gs::rgb4(4, 6, 3), gs::rgb4(8, 7, 3), gs::rgb4(5, 4, 2),
            gs::rgb4(9, 8, 4), gs::rgb4(2, 3, 2), gs::rgb4(7, 6, 3), gs::rgb4(10, 8, 4), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_WALL,
           {0, gs::rgb4(5, 4, 3), gs::rgb4(8, 6, 4), gs::rgb4(6, 5, 3), gs::rgb4(10, 8, 5), gs::rgb4(4, 3, 2),
            gs::rgb4(7, 5, 3), gs::rgb4(3, 2, 2), gs::rgb4(12, 10, 6), gs::rgb4(9, 7, 4), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(4, 5, 7), gs::rgb4(3, 3, 5), gs::rgb4(12, 9, 6), gs::rgb4(6, 4, 3), gs::rgb4(14, 12, 8),
            gs::rgb4(13, 8, 2), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_IRON,
           {0, gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 2), gs::rgb4(14, 8, 2),
            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FIRE,
           {0, gs::rgb4(14, 4, 1), gs::rgb4(15, 10, 2), gs::rgb4(15, 14, 6), gs::rgb4(6, 3, 1)});
    setPal(vdp, PAL_MOON, {0, gs::rgb4(12, 12, 10), gs::rgb4(8, 8, 7)});
    setPal(vdp, PAL_STAR, {0, gs::rgb4(14, 14, 12), gs::rgb4(10, 10, 8)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(4, 3, 2), gs::rgb4(12, 3, 2), gs::rgb4(15, 12, 4)});
    inkPal(vdp, PAL_GOLD, 15, 12, 3);
    inkPal(vdp, PAL_ALERT, 15, 3, 2);
    inkPal(vdp, PAL_OK, 6, 14, 5);
    setPal(vdp, PAL_RAIN, {0, gs::rgb4(8, 10, 13), gs::rgb4(13, 14, 15)});
    setPal(vdp, PAL_OIL,
           {0, gs::rgb4(6, 4, 1), gs::rgb4(9, 6, 2), gs::rgb4(4, 3, 1), gs::rgb4(12, 9, 3), gs::rgb4(2, 2, 1),
            gs::rgb4(1, 1, 1)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.redoubt = gs::uploadMipped(vdp, redoubtArt());
    art.ditch = gs::uploadMipped(vdp, ditchArt());
    art.brazier = gs::uploadMipped(vdp, brazierArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.watch[0] = gs::uploadMipped(vdp, watchArt(0));
    art.watch[1] = gs::uploadMipped(vdp, watchArt(1));
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.drop = gs::uploadMipped(vdp, dropArt());
}

}  // namespace rdawn
