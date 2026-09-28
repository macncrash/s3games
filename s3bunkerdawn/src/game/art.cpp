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

void paintYard(gs::Bitmap& b) {
    b.rect(0, 0, 320, 88, 2);
    b.rect(0, 0, 320, 6, 4);
    b.rect(0, 70, 320, 18, 3);
    for (int y = 10; y < 78; y += 5)
        for (int x = 4; x < 316; x += 7) {
            uint32_t h = hash2(x, y + 40);
            if ((h % 17) == 0) b.set(x, y, (h & 1) ? 5 : 6);
            if ((h % 41) == 0) b.set(x + 1, y + 1, 7);
        }
    for (int i = 0; i < 9; i++) {
        int x = 8 + i * 36;
        b.rect(x, 18, 22, 5, 8);
        b.rect(x + 2, 22, 18, 3, 9);
    }
    b.rect(132, 8, 56, 10, 10);
    b.rect(140, 4, 8, 8, 11);
    b.rect(172, 4, 8, 8, 11);
    b.line(0, 6, 320, 6, 1, 1.f);
}

gs::Bitmap bunkerArt() {
    gs::Bitmap b(168, 96);
    b.rect(18, 28, 132, 62, 2);
    b.rect(18, 28, 132, 8, 4);
    b.rect(14, 84, 140, 8, 3);
    b.poly({{8, 36}, {28, 18}, {140, 18}, {160, 36}}, 1);
    b.rect(28, 18, 112, 10, 5);
    b.rect(62, 48, 44, 28, 6);
    b.rect(66, 52, 36, 8, 8);
    b.rect(70, 64, 10, 12, 7);
    b.rect(88, 64, 10, 12, 7);
    b.rect(46, 40, 14, 10, 9);
    b.rect(108, 40, 14, 10, 9);
    b.rect(24, 90, 10, 6, 10);
    b.rect(134, 90, 10, 6, 10);
    for (int i = 0; i < 6; i++) b.rect(30 + i * 20, 30, 2, 48, 11);
    b.outline(12, false);
    return b;
}

gs::Bitmap bagArt() {
    gs::Bitmap b(44, 18);
    b.ellipse(12, 10, 11, 7, 2);
    b.ellipse(24, 9, 12, 8, 1);
    b.ellipse(34, 11, 9, 6, 3);
    b.line(8, 8, 16, 12, 4, 1.f);
    b.line(20, 6, 28, 11, 4, 1.f);
    b.outline(5, false);
    return b;
}

gs::Bitmap potArt() {
    gs::Bitmap b(26, 22);
    b.rect(4, 6, 18, 12, 1);
    b.rect(2, 4, 22, 4, 2);
    b.rect(6, 16, 14, 4, 3);
    b.rect(10, 2, 6, 4, 4);
    b.ellipse(13, 10, 4, 3, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap flameArt(int step) {
    gs::Bitmap b(20, 32);
    float lean = step ? 2.f : -1.f;
    b.ellipse(10 + lean, 20, 6, 10, 1);
    b.ellipse(10 + lean * 0.4f, 14, 4, 8, 2);
    b.ellipse(10, 10, 2.4f, 5, 3);
    b.rect(9, 24, 3, 6, 4);
    return b;
}

gs::Bitmap manArt(int step) {
    gs::Bitmap b(34, 48);
    b.ellipse(16, 9, 7, 6, 3);
    b.rect(10, 8, 12, 3, 4);
    b.rect(9, 16, 14, 14, 1);
    b.rect(9, 16, 4, 14, 2);
    b.rect(12, 18, 4, 3, 5);
    b.rect(20, 20, 10, 3, 6);
    int lx = step ? 8 : 12;
    int rx = step ? 18 : 14;
    b.rect(lx, 30, 5, 12, 1);
    b.rect(rx, 30, 5, 12, 2);
    b.rect(lx - 1, 40, 7, 4, 7);
    b.rect(rx - 1, 40, 7, 4, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(18, 12, 9, 9, 0);
    b.ellipse(10, 16, 2, 2, 2);
    b.ellipse(14, 20, 1.4f, 1.4f, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(30, 30);
    b.ellipse(15, 15, 8, 8, 1);
    b.ellipse(15, 15, 4, 4, 2);
    for (int i = 0; i < 8; i++) {
        float a = i * 0.785f;
        int x = int(15 + std::cos(a) * 12);
        int y = int(15 + std::sin(a) * 12);
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

gs::Bitmap sparkArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.ellipse(4, 4, 1.4f, 1.4f, 2);
    return b;
}

void inkPal(gs::VDP& vdp, int pal, int r, int g, int b) {
    setPal(vdp, pal, {0, gs::rgb4(r, g, b), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    inkPal(vdp, PAL_HUD, 14, 14, 12);
    setPal(vdp, PAL_YARD,
           {0, gs::rgb4(2, 2, 2), gs::rgb4(3, 3, 2), gs::rgb4(2, 2, 1), gs::rgb4(5, 5, 4), gs::rgb4(4, 4, 3),
            gs::rgb4(6, 5, 3), gs::rgb4(5, 4, 2), gs::rgb4(4, 5, 3), gs::rgb4(3, 4, 2), gs::rgb4(6, 6, 5),
            gs::rgb4(8, 7, 4), gs::rgb4(1, 1, 1), 0, 0, 0});
    setPal(vdp, PAL_BLOCK,
           {0, gs::rgb4(4, 4, 5), gs::rgb4(6, 6, 6), gs::rgb4(3, 3, 3), gs::rgb4(8, 8, 8), gs::rgb4(5, 5, 6),
            gs::rgb4(1, 1, 2), gs::rgb4(12, 9, 3), gs::rgb4(15, 12, 4), gs::rgb4(2, 3, 4), gs::rgb4(7, 6, 4),
            gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 1), 0, 0, 0});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(2, 4, 2), gs::rgb4(1, 3, 1), gs::rgb4(10, 7, 4), gs::rgb4(3, 3, 2), gs::rgb4(12, 8, 5),
            gs::rgb4(8, 7, 3), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_POT,
           {0, gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 8), gs::rgb4(2, 2, 2), gs::rgb4(9, 6, 2), gs::rgb4(3, 2, 1),
            gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FIRE,
           {0, gs::rgb4(12, 3, 1), gs::rgb4(15, 8, 1), gs::rgb4(15, 14, 4), gs::rgb4(6, 3, 1), 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0});
    setPal(vdp, PAL_MOON,
           {0, gs::rgb4(13, 13, 11), gs::rgb4(8, 8, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    inkPal(vdp, PAL_STAR, 15, 15, 12);
    setPal(vdp, PAL_BAG,
           {0, gs::rgb4(5, 5, 3), gs::rgb4(4, 4, 2), gs::rgb4(6, 6, 4), gs::rgb4(3, 3, 2), gs::rgb4(2, 2, 1), 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0});
    inkPal(vdp, PAL_GOLD, 15, 12, 4);
    inkPal(vdp, PAL_ALERT, 15, 4, 2);
    inkPal(vdp, PAL_OK, 8, 14, 6);
    inkPal(vdp, PAL_WIND, 8, 9, 11);

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::Bitmap yard(320, 88);
    paintYard(yard);
    gs::bitmapToPlane(tiles, vdp.B, 0, 17, yard, PAL_YARD);
    vdp.B.enabled = true;
    vdp.A.enabled = false;

    art.bunker = gs::uploadMipped(vdp, bunkerArt());
    art.bag = gs::uploadMipped(vdp, bagArt());
    art.pot = gs::uploadMipped(vdp, potArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.man[0] = gs::uploadMipped(vdp, manArt(0));
    art.man[1] = gs::uploadMipped(vdp, manArt(1));
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
}

}  // namespace bdawn
