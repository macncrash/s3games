#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace granarydawn {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 2246822519u + uint32_t(y) * 3266489917u;
    h = (h ^ (h >> 13)) * 16777619u;
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

void paintYard(gs::Bitmap& b) {
    b.rect(0, 0, 320, 88, 2);
    b.rect(0, 0, 320, 28, 4);
    for (int x = 0; x < 320; x += 8) b.rect(x, 8, 6, 18, (x / 8) & 1 ? 5 : 6);
    for (int i = 0; i < 5; i++) {
        int x = 18 + i * 62;
        b.rect(x, 14, 22, 12, 8);
        b.rect(x + 4, 18, 6, 6, 9);
        b.rect(x + 12, 18, 6, 6, 10);
    }
    b.rect(0, 36, 320, 8, 3);
    b.rect(0, 62, 320, 26, 7);
    for (int y = 66; y < 86; y += 4)
        for (int x = 2; x < 318; x += 6) {
            uint32_t h = hash2(x, y);
            if ((h % 5) == 0) b.set(x, y, (h & 1) ? 11 : 12);
        }
    b.line(0, 36, 320, 36, 1, 1.f);
    b.line(0, 62, 320, 62, 1, 1.f);
}

gs::Bitmap barnArt() {
    gs::Bitmap b(120, 96);
    b.poly({{8, 40}, {60, 8}, {112, 40}}, 1);
    b.rect(14, 38, 92, 50, 2);
    b.rect(14, 38, 92, 6, 3);
    for (int i = 0; i < 8; i++) b.rect(18, 46 + i * 5, 84, 2, i & 1 ? 4 : 5);
    b.rect(46, 58, 28, 28, 6);
    b.rect(50, 62, 8, 10, 7);
    b.rect(62, 62, 8, 10, 7);
    b.rect(54, 14, 12, 16, 8);
    b.rect(57, 8, 6, 8, 9);
    b.rect(22, 48, 14, 10, 10);
    b.rect(84, 48, 14, 10, 10);
    b.outline(11, false);
    return b;
}

gs::Bitmap siloArt() {
    gs::Bitmap b(36, 88);
    b.rect(6, 16, 24, 64, 1);
    b.ellipse(18, 16, 12, 8, 2);
    b.ellipse(18, 16, 6, 4, 3);
    b.rect(4, 76, 28, 8, 4);
    for (int i = 0; i < 5; i++) b.rect(8, 24 + i * 10, 20, 2, 5);
    b.rect(14, 40, 8, 12, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(28, 20);
    b.ellipse(14, 12, 12, 7, 1);
    b.ellipse(14, 9, 8, 4, 2);
    b.line(8, 8, 20, 8, 3, 1.f);
    b.line(10, 6, 14, 10, 4, 1.f);
    b.outline(5, false);
    return b;
}

gs::Bitmap cupArt() {
    gs::Bitmap b(22, 18);
    b.rect(3, 4, 16, 10, 1);
    b.rect(1, 3, 20, 3, 2);
    b.rect(5, 13, 12, 3, 3);
    b.ellipse(11, 8, 3, 2, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap flameArt(int step) {
    gs::Bitmap b(18, 30);
    float lean = step ? 2.f : -1.5f;
    b.ellipse(9 + lean, 18, 5, 9, 1);
    b.ellipse(9 + lean * 0.4f, 12, 3.4f, 7, 2);
    b.ellipse(9, 8, 2, 4, 3);
    b.rect(8, 22, 3, 6, 4);
    return b;
}

gs::Bitmap keeperArt(int step) {
    gs::Bitmap b(32, 46);
    b.ellipse(16, 8, 6, 6, 3);
    b.rect(11, 6, 10, 3, 4);
    b.rect(10, 15, 12, 14, 1);
    b.rect(10, 15, 4, 14, 2);
    b.rect(18, 18, 8, 3, 5);
    int lx = step ? 8 : 12;
    int rx = step ? 18 : 14;
    b.rect(lx, 29, 4, 12, 1);
    b.rect(rx, 29, 4, 12, 2);
    b.rect(lx - 1, 39, 6, 3, 6);
    b.rect(rx - 1, 39, 6, 3, 6);
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
    b.ellipse(14, 14, 3, 3, 2);
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

gs::Bitmap chaffArt() {
    gs::Bitmap b(16, 10);
    b.ellipse(5, 5, 4, 2, 1);
    b.ellipse(11, 4, 3, 2.2f, 2);
    b.set(8, 7, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    inkPal(vdp, PAL_HUD, 14, 13, 10);
    setPal(vdp, PAL_YARD,
           {0, gs::rgb4(2, 1, 1), gs::rgb4(4, 3, 2), gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(7, 5, 3),
            gs::rgb4(5, 3, 2), gs::rgb4(5, 4, 2), gs::rgb4(3, 2, 1), gs::rgb4(8, 7, 3), gs::rgb4(2, 2, 3),
            gs::rgb4(8, 6, 2), gs::rgb4(6, 5, 2), gs::rgb4(1, 1, 1), 0, 0});
    setPal(vdp, PAL_BARN,
           {0, gs::rgb4(6, 3, 1), gs::rgb4(8, 5, 2), gs::rgb4(10, 6, 2), gs::rgb4(7, 4, 2), gs::rgb4(5, 3, 1),
            gs::rgb4(2, 1, 1), gs::rgb4(12, 10, 4), gs::rgb4(4, 4, 5), gs::rgb4(9, 8, 6), gs::rgb4(3, 4, 5),
            gs::rgb4(1, 1, 1), 0, 0, 0, 0});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(3, 4, 6), gs::rgb4(2, 3, 5), gs::rgb4(11, 8, 5), gs::rgb4(4, 3, 2), gs::rgb4(9, 7, 3),
            gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_CUP,
           {0, gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(3, 3, 3), gs::rgb4(12, 8, 2), gs::rgb4(1, 1, 1), 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FIRE,
           {0, gs::rgb4(13, 3, 1), gs::rgb4(15, 9, 1), gs::rgb4(15, 14, 5), gs::rgb4(7, 3, 1), 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0});
    setPal(vdp, PAL_MOON, {0, gs::rgb4(13, 13, 11), gs::rgb4(8, 8, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    inkPal(vdp, PAL_STAR, 15, 15, 12);
    setPal(vdp, PAL_SACK,
           {0, gs::rgb4(7, 6, 3), gs::rgb4(9, 8, 4), gs::rgb4(4, 3, 1), gs::rgb4(12, 10, 5), gs::rgb4(2, 1, 1), 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0});
    inkPal(vdp, PAL_GOLD, 15, 12, 4);
    inkPal(vdp, PAL_ALERT, 15, 4, 2);
    inkPal(vdp, PAL_OK, 7, 14, 5);
    setPal(vdp, PAL_CHAFF,
           {0, gs::rgb4(10, 8, 4), gs::rgb4(12, 10, 6), gs::rgb4(8, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::Bitmap yard(320, 88);
    paintYard(yard);
    gs::bitmapToPlane(tiles, vdp.B, 0, 17, yard, PAL_YARD);
    vdp.B.enabled = true;
    vdp.A.enabled = false;

    art.barn = gs::uploadMipped(vdp, barnArt());
    art.silo = gs::uploadMipped(vdp, siloArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.cup = gs::uploadMipped(vdp, cupArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.chaff = gs::uploadMipped(vdp, chaffArt());
}

}  // namespace granarydawn
