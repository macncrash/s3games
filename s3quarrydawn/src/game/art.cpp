#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace qdawn {
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

void inkPal(gs::VDP& vdp, int pal, int r, int g, int b) {
    setPal(vdp, pal, {0, gs::rgb4(r, g, b), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
}

gs::Bitmap faceArt() {
    gs::Bitmap b(320, 150);
    b.rect(0, 18, 320, 132, 2);
    b.poly({{0, 28}, {70, 18}, {150, 34}, {230, 16}, {320, 30}, {320, 48}, {0, 48}}, 3);
    b.rect(0, 48, 320, 10, 4);
    b.rect(18, 58, 284, 28, 5);
    b.rect(36, 86, 248, 26, 6);
    b.rect(58, 112, 204, 22, 1);
    b.rect(0, 134, 320, 16, 7);
    for (int i = 0; i < 14; i++) {
        int x = 8 + i * 22;
        b.line(float(x), 50, float(x + 10), 146, 8, 1.f);
    }
    for (int y = 22; y < 140; y += 6)
        for (int x = 4; x < 316; x += 9) {
            uint32_t h = hash2(x, y);
            if ((h % 11) == 0) b.set(x, y, (h & 1) ? 9 : 10);
        }
    b.rect(148, 58, 20, 76, 11);
    b.outline(12, false);
    return b;
}

gs::Bitmap waterArt() {
    gs::Bitmap b(180, 22);
    b.ellipse(90, 14, 86, 10, 1);
    b.ellipse(90, 14, 60, 6, 2);
    b.line(30, 12, 70, 16, 3, 1.f);
    b.line(100, 10, 150, 15, 3, 1.f);
    return b;
}

gs::Bitmap craneArt() {
    gs::Bitmap b(90, 70);
    b.rect(8, 40, 10, 28, 1);
    b.rect(4, 64, 18, 5, 2);
    b.poly({{12, 42}, {78, 10}, {82, 14}, {16, 46}}, 3);
    b.line(74, 12, 74, 58, 4, 1.f);
    b.rect(68, 56, 12, 8, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap potArt() {
    gs::Bitmap b(22, 18);
    b.rect(3, 5, 16, 9, 1);
    b.rect(1, 3, 20, 4, 2);
    b.rect(5, 13, 12, 3, 3);
    b.rect(9, 1, 4, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap flameArt(int step) {
    gs::Bitmap b(18, 30);
    float lean = step ? 2.f : -1.2f;
    b.ellipse(9 + lean, 18, 5.5f, 9, 1);
    b.ellipse(9 + lean * 0.4f, 13, 3.5f, 7, 2);
    b.ellipse(9, 9, 2.f, 4.5f, 3);
    b.rect(8, 22, 3, 6, 4);
    return b;
}

gs::Bitmap manArt(int step) {
    gs::Bitmap b(30, 44);
    b.rect(9, 2, 12, 4, 4);
    b.ellipse(15, 10, 6, 5, 3);
    b.rect(8, 15, 14, 12, 1);
    b.rect(8, 15, 4, 12, 2);
    b.rect(18, 18, 8, 3, 5);
    int lx = step ? 7 : 11;
    int rx = step ? 16 : 13;
    b.rect(lx, 27, 5, 11, 1);
    b.rect(rx, 27, 5, 11, 2);
    b.rect(lx - 1, 36, 7, 4, 6);
    b.rect(rx - 1, 36, 7, 4, 6);
    b.outline(7, false);
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

gs::Bitmap rungArt() {
    gs::Bitmap b(16, 10);
    b.rect(1, 1, 3, 8, 1);
    b.rect(12, 1, 3, 8, 1);
    b.rect(2, 4, 12, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    inkPal(vdp, PAL_HUD, 14, 13, 11);
    setPal(vdp, PAL_ROCK,
           {0, gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 2), gs::rgb4(6, 5, 4), gs::rgb4(8, 7, 5), gs::rgb4(4, 4, 3),
            gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 2), gs::rgb4(7, 6, 4), gs::rgb4(9, 8, 6), gs::rgb4(4, 3, 2),
            gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 1), 0, 0, 0});
    setPal(vdp, PAL_CUT,
           {0, gs::rgb4(4, 5, 3), gs::rgb4(2, 3, 2), gs::rgb4(6, 7, 4), gs::rgb4(8, 8, 5), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(9, 6, 2), gs::rgb4(6, 4, 1), gs::rgb4(12, 8, 4), gs::rgb4(14, 11, 2), gs::rgb4(3, 3, 3),
            gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_POT,
           {0, gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 9), gs::rgb4(2, 2, 2), gs::rgb4(10, 6, 2), gs::rgb4(1, 1, 1), 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FIRE,
           {0, gs::rgb4(12, 3, 1), gs::rgb4(15, 8, 1), gs::rgb4(15, 14, 4), gs::rgb4(6, 3, 1), 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0});
    setPal(vdp, PAL_MOON, {0, gs::rgb4(13, 13, 11), gs::rgb4(8, 8, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    inkPal(vdp, PAL_STAR, 15, 15, 12);
    setPal(vdp, PAL_CRANE,
           {0, gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 3), gs::rgb4(10, 8, 3), gs::rgb4(5, 5, 4), gs::rgb4(12, 7, 2),
            gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    inkPal(vdp, PAL_GOLD, 15, 12, 4);
    inkPal(vdp, PAL_ALERT, 15, 4, 3);
    inkPal(vdp, PAL_OK, 6, 14, 6);
    setPal(vdp, PAL_WATER,
           {0, gs::rgb4(1, 3, 6), gs::rgb4(2, 6, 9), gs::rgb4(8, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    loadFont(vdp, art);
    art.face = gs::uploadMipped(vdp, faceArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.pot = gs::uploadMipped(vdp, potArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.man[0] = gs::uploadMipped(vdp, manArt(0));
    art.man[1] = gs::uploadMipped(vdp, manArt(1));
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.rung = gs::uploadMipped(vdp, rungArt());
}

}  // namespace qdawn
