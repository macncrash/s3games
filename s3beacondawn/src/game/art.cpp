#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace bdawn {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 2246822519u + uint32_t(y) * 3266489917u;
    h = (h ^ (h >> 15)) * 668265263u;
    return h ^ (h >> 16);
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

void paintCliff(gs::Bitmap& b) {
    b.rect(0, 40, 320, 48, 2);
    b.poly({{0, 40}, {70, 18}, {150, 28}, {220, 10}, {320, 36}, {320, 88}, {0, 88}}, 3);
    b.rect(0, 70, 320, 18, 4);
    for (int y = 22; y < 84; y += 4)
        for (int x = 2; x < 318; x += 6) {
            uint32_t h = hash2(x, y);
            if ((h % 13) == 0) b.set(x, y, (h & 1) ? 5 : 6);
        }
    b.rect(0, 0, 320, 16, 8);
    for (int x = 0; x < 320; x += 9) {
        int w = 4 + int(hash2(x, 3) % 5);
        b.rect(x, 8, w, 6, 9);
    }
    b.line(0, 16, 320, 16, 1, 1.f);
}

gs::Bitmap towerArt() {
    gs::Bitmap b(72, 120);
    b.poly({{8, 118}, {18, 28}, {54, 28}, {64, 118}}, 2);
    b.rect(16, 22, 40, 12, 3);
    b.rect(22, 8, 28, 16, 4);
    b.rect(30, 0, 12, 10, 5);
    b.rect(33, 0, 6, 6, 6);
    b.rect(26, 48, 20, 16, 7);
    b.rect(30, 52, 12, 8, 8);
    b.rect(20, 78, 32, 8, 1);
    for (int i = 0; i < 5; i++) b.rect(22, 36 + i * 16, 6, 6, 9);
    b.outline(10, false);
    return b;
}

gs::Bitmap bowlArt() {
    gs::Bitmap b(28, 18);
    b.poly({{2, 6}, {8, 16}, {20, 16}, {26, 6}}, 1);
    b.rect(1, 3, 26, 5, 2);
    b.ellipse(14, 5, 8, 2, 3);
    b.rect(12, 0, 4, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap flameArt(int step) {
    gs::Bitmap b(18, 30);
    float lean = step ? 3.f : -2.f;
    b.ellipse(9 + lean, 18, 5, 9, 1);
    b.ellipse(9 + lean * 0.3f, 12, 3.2f, 7, 2);
    b.ellipse(9, 8, 1.8f, 4, 3);
    return b;
}

gs::Bitmap keeperArt(int step) {
    gs::Bitmap b(28, 44);
    b.ellipse(14, 8, 6, 5, 3);
    b.rect(9, 4, 10, 3, 4);
    b.rect(8, 14, 12, 14, 1);
    b.rect(8, 14, 3, 12, 2);
    b.rect(18, 16, 8, 3, 5);
    int lx = step ? 7 : 11;
    int rx = step ? 15 : 12;
    b.rect(lx, 28, 4, 11, 1);
    b.rect(rx, 28, 4, 11, 2);
    b.rect(lx - 1, 38, 6, 3, 6);
    b.rect(rx - 1, 38, 6, 3, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 16);
    b.rect(4, 2, 4, 6, 1);
    b.ellipse(6, 10, 4, 4, 2);
    b.set(6, 0, 3);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(26, 26);
    b.ellipse(13, 13, 11, 11, 1);
    b.ellipse(17, 11, 8, 8, 0);
    b.ellipse(9, 15, 2, 2, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 7, 7, 1);
    b.ellipse(14, 14, 3, 3, 2);
    for (int i = 0; i < 8; i++) {
        float a = i * 0.785f;
        int x = int(14 + std::cos(a) * 11);
        int y = int(14 + std::sin(a) * 11);
        b.rect(x - 1, y - 1, 2, 2, 3);
    }
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 1, 2);
    b.rect(0, 2, 5, 1, 1);
    b.set(2, 3, 2);
    b.set(2, 4, 1);
    return b;
}

gs::Bitmap dropArt() {
    gs::Bitmap b(5, 8);
    b.set(2, 0, 1);
    b.ellipse(2, 4, 2, 3, 2);
    return b;
}

void inkPal(gs::VDP& vdp, int pal, int r, int g, int b) {
    setPal(vdp, pal, {0, gs::rgb4(r, g, b), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    inkPal(vdp, PAL_HUD, 14, 13, 10);
    setPal(vdp, PAL_CLIFF,
           {0, gs::rgb4(2, 2, 2), gs::rgb4(3, 3, 4), gs::rgb4(4, 4, 5), gs::rgb4(3, 3, 2), gs::rgb4(5, 5, 6),
            gs::rgb4(6, 6, 5), gs::rgb4(2, 2, 3), gs::rgb4(1, 2, 5), gs::rgb4(2, 4, 7), gs::rgb4(1, 1, 2), 0, 0,
            0, 0, 0});
    setPal(vdp, PAL_TOWER,
           {0, gs::rgb4(5, 5, 4), gs::rgb4(6, 6, 5), gs::rgb4(8, 8, 7), gs::rgb4(4, 4, 5), gs::rgb4(10, 8, 3),
            gs::rgb4(15, 10, 2), gs::rgb4(2, 2, 3), gs::rgb4(12, 10, 4), gs::rgb4(7, 6, 4), gs::rgb4(1, 1, 1), 0,
            0, 0, 0, 0});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(3, 3, 6), gs::rgb4(2, 2, 4), gs::rgb4(11, 8, 5), gs::rgb4(4, 3, 2), gs::rgb4(14, 11, 3),
            gs::rgb4(2, 1, 1), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BOWL,
           {0, gs::rgb4(5, 4, 3), gs::rgb4(8, 7, 5), gs::rgb4(3, 2, 1), gs::rgb4(10, 6, 2), gs::rgb4(1, 1, 1), 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FIRE,
           {0, gs::rgb4(13, 3, 1), gs::rgb4(15, 9, 1), gs::rgb4(15, 14, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_MOON, {0, gs::rgb4(12, 12, 14), gs::rgb4(7, 7, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    inkPal(vdp, PAL_STAR, 15, 15, 13);
    inkPal(vdp, PAL_SEA, 4, 8, 12);
    inkPal(vdp, PAL_GOLD, 15, 12, 3);
    inkPal(vdp, PAL_ALERT, 15, 4, 2);
    inkPal(vdp, PAL_OK, 6, 14, 7);
    setPal(vdp, PAL_RAIN, {0, gs::rgb4(6, 8, 12), gs::rgb4(10, 12, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::Bitmap cliff(320, 88);
    paintCliff(cliff);
    gs::bitmapToPlane(tiles, vdp.B, 0, 17, cliff, PAL_CLIFF);
    vdp.B.enabled = true;
    vdp.A.enabled = false;

    art.tower = gs::uploadMipped(vdp, towerArt());
    art.bowl = gs::uploadMipped(vdp, bowlArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.drop = gs::uploadMipped(vdp, dropArt());
}

}  // namespace bdawn
