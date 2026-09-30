#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace wharfdawn {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
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

void inkPal(gs::VDP& vdp, int pal, int r, int g, int b) {
    setPal(vdp, pal, {0, gs::rgb4(r, g, b), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
}

void paintWater(gs::Bitmap& b) {
    for (int y = 0; y < b.h; y++) {
        int band = 2 + (y / 6) % 3;
        b.rect(0, y, b.w, 1, band);
        for (int x = 0; x < b.w; x += 9) {
            uint32_t h = hash2(x, y);
            if ((h % 11) == 0) b.set(x, y, 5);
            if ((h % 23) == 0) b.set(x + 2, y, 6);
        }
    }
    b.rect(0, 0, b.w, 3, 4);
}

gs::Bitmap pierArt() {
    gs::Bitmap b(300, 36);
    b.rect(0, 8, 300, 16, 1);
    for (int x = 0; x < 300; x += 12) b.rect(x, 8, 2, 16, 2);
    b.rect(0, 6, 300, 3, 3);
    b.rect(0, 22, 300, 4, 4);
    for (int i = 0; i < 8; i++) {
        int x = 10 + i * 36;
        b.rect(x, 24, 6, 12, 5);
        b.rect(x + 18, 24, 6, 12, 5);
    }
    b.line(0, 8, 300, 8, 6, 1.f);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(48, 40);
    b.poly({{2, 16}, {24, 4}, {46, 16}}, 1);
    b.rect(6, 16, 36, 20, 2);
    b.rect(18, 22, 12, 14, 3);
    b.rect(10, 20, 6, 6, 4);
    b.rect(32, 20, 6, 6, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 28);
    b.rect(3, 0, 4, 28, 1);
    b.rect(1, 0, 8, 3, 2);
    b.rect(2, 24, 6, 4, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(18, 16);
    b.rect(3, 4, 12, 8, 1);
    b.rect(1, 6, 16, 4, 2);
    b.rect(7, 0, 4, 5, 3);
    b.ellipse(9, 8, 3, 2, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap flameArt(int step) {
    gs::Bitmap b(16, 24);
    float lean = step ? 2.f : -1.5f;
    b.ellipse(8 + lean, 14, 5, 8, 1);
    b.ellipse(8 + lean * 0.3f, 9, 3, 6, 2);
    b.ellipse(8, 6, 1.6f, 3.5f, 3);
    return b;
}

gs::Bitmap keeperArt(int step) {
    gs::Bitmap b(30, 42);
    b.ellipse(15, 8, 6, 5, 3);
    b.poly({{8, 8}, {15, 1}, {22, 8}}, 4);
    b.rect(9, 13, 12, 14, 1);
    b.rect(9, 13, 4, 14, 2);
    b.rect(18, 16, 8, 4, 5);
    int lx = step ? 7 : 11;
    int rx = step ? 16 : 12;
    b.rect(lx, 26, 5, 11, 1);
    b.rect(rx, 26, 5, 11, 2);
    b.rect(lx - 1, 35, 7, 4, 6);
    b.rect(rx - 1, 35, 7, 4, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap boatArt() {
    gs::Bitmap b(56, 22);
    b.poly({{4, 8}, {8, 16}, {48, 16}, {52, 8}}, 1);
    b.rect(10, 6, 28, 6, 2);
    b.rect(22, 2, 3, 8, 3);
    b.poly({{25, 3}, {40, 8}, {25, 12}}, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(18, 8);
    b.line(0, 5, 8, 2, 1, 1.2f);
    b.line(8, 2, 17, 5, 1, 1.2f);
    b.set(8, 3, 2);
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
    b.set(3, 1, 1);
    b.rect(1, 3, 5, 1, 1);
    b.set(3, 5, 1);
    b.set(3, 3, 2);
    return b;
}

gs::Bitmap sprayArt() {
    gs::Bitmap b(14, 10);
    b.ellipse(4, 6, 3, 2, 1);
    b.ellipse(9, 4, 3, 2.4f, 2);
    b.ellipse(7, 7, 2, 1.2f, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    inkPal(vdp, PAL_HUD, 14, 14, 12);
    setPal(vdp, PAL_WATER,
           {0, gs::rgb4(1, 2, 4), gs::rgb4(1, 3, 6), gs::rgb4(2, 4, 7), gs::rgb4(4, 7, 10), gs::rgb4(6, 10, 12),
            gs::rgb4(3, 6, 8), gs::rgb4(8, 11, 12), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_PIER,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 1), gs::rgb4(8, 6, 3), gs::rgb4(3, 2, 1), gs::rgb4(5, 5, 4),
            gs::rgb4(9, 8, 5), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(1, 3, 5), gs::rgb4(1, 2, 3), gs::rgb4(11, 8, 5), gs::rgb4(8, 2, 1), gs::rgb4(10, 9, 4),
            gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 7), gs::rgb4(4, 3, 2), gs::rgb4(12, 8, 2), gs::rgb4(1, 1, 1), 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FIRE,
           {0, gs::rgb4(12, 3, 1), gs::rgb4(15, 8, 1), gs::rgb4(15, 14, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_MOON, {0, gs::rgb4(13, 13, 11), gs::rgb4(8, 8, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    inkPal(vdp, PAL_STAR, 15, 15, 12);
    setPal(vdp, PAL_BOAT,
           {0, gs::rgb4(5, 3, 2), gs::rgb4(8, 7, 5), gs::rgb4(4, 4, 3), gs::rgb4(12, 12, 10), gs::rgb4(1, 1, 1), 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0});
    inkPal(vdp, PAL_GOLD, 15, 12, 4);
    inkPal(vdp, PAL_ALERT, 15, 4, 2);
    inkPal(vdp, PAL_OK, 8, 14, 6);
    setPal(vdp, PAL_SWELL, {0, gs::rgb4(10, 12, 14), gs::rgb4(14, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::Bitmap water(320, 96);
    paintWater(water);
    gs::bitmapToPlane(tiles, vdp.B, 0, 16, water, PAL_WATER);
    vdp.B.enabled = true;
    vdp.A.enabled = false;

    art.pier = gs::uploadMipped(vdp, pierArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.boat = gs::uploadMipped(vdp, boatArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.spray = gs::uploadMipped(vdp, sprayArt());
}

}  // namespace wharfdawn
