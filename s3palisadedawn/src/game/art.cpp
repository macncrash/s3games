#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <string>

namespace pdawn {
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

void paintBank(gs::Bitmap& b) {
    b.rect(0, 0, 320, 96, 2);
    b.rect(0, 0, 320, 10, 4);
    b.rect(0, 62, 320, 8, 5);
    b.rect(0, 70, 320, 26, 3);
    for (int x = 0; x < 320; x += 8) {
        b.line(float(x), 70.f, float(x + 4), 96.f, 6, 1.f);
        if ((hash2(x, 3) % 5) == 0) b.rect(x, 78, 3, 8, 7);
    }
    for (int i = 0; i < 18; i++) {
        int x = 6 + i * 18;
        b.poly({{float(x), 58.f}, {float(x + 4), 46.f}, {float(x + 8), 58.f}}, 8);
        b.rect(x + 2, 56, 4, 10, 9);
    }
    b.rect(0, 86, 320, 10, 1);
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(28, 72);
    b.poly({{14.f, 2.f}, {6.f, 18.f}, {22.f, 18.f}}, 3);
    b.rect(10, 16, 8, 48, 1);
    b.rect(11, 18, 2, 44, 4);
    b.rect(8, 28, 12, 3, 5);
    b.rect(7, 40, 14, 3, 2);
    b.rect(6, 58, 16, 6, 6);
    b.rect(4, 62, 20, 4, 7);
    b.ellipse(14, 14, 5, 3, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap flameArt(int frame) {
    gs::Bitmap b(16, 28);
    int lean = frame ? 2 : -1;
    b.poly({{8.f + lean, 1.f}, {3.f, 16.f}, {8.f, 26.f}, {13.f, 16.f}}, 1);
    b.poly({{8.f + lean * 0.4f, 6.f}, {5.f, 16.f}, {8.f, 22.f}, {11.f, 16.f}}, 2);
    b.ellipse(8, 16, 2, 4, 3);
    return b;
}

gs::Bitmap sentryArt(int step) {
    gs::Bitmap b(24, 40);
    b.ellipse(12, 6, 4, 4, 3);
    b.rect(9, 10, 6, 4, 4);
    b.rect(8, 14, 8, 12, 1);
    b.rect(7, 16, 3, 8, 2);
    b.rect(14, 16, 3, 8, 2);
    int leg = step ? 3 : 0;
    b.rect(8, 26, 3, 10 + leg, 5);
    b.rect(13, 26, 3, 13 - leg, 5);
    b.line(16, 8, 22, 2, 6, 1.4f);
    b.rect(20, 1, 3, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap flaskArt() {
    gs::Bitmap b(12, 16);
    b.rect(4, 1, 4, 3, 2);
    b.poly({{3.f, 5.f}, {9.f, 5.f}, {10.f, 14.f}, {2.f, 14.f}}, 1);
    b.rect(4, 8, 4, 4, 3);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 8, 1);
    b.ellipse(13, 8, 6, 6, 0);
    b.ellipse(7, 12, 1.2f, 1.2f, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(26, 26);
    b.ellipse(13, 13, 6, 6, 1);
    b.ellipse(13, 13, 3, 3, 2);
    for (int i = 0; i < 8; i++) {
        float a = i * 0.785f;
        b.line(13 + std::cos(a) * 7, 13 + std::sin(a) * 7, 13 + std::cos(a) * 11, 13 + std::sin(a) * 11, 3, 1.2f);
    }
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(7, 7);
    b.set(3, 0, 1);
    b.set(3, 1, 1);
    b.rect(1, 3, 5, 1, 1);
    b.set(3, 4, 1);
    b.set(3, 5, 1);
    b.set(3, 6, 1);
    b.set(3, 3, 2);
    return b;
}

gs::Bitmap gustArt() {
    gs::Bitmap b(16, 8);
    b.line(1, 2, 14, 2, 1, 1.f);
    b.line(3, 4, 12, 4, 2, 1.f);
    b.line(5, 6, 15, 6, 1, 1.f);
    return b;
}

void inkPal(gs::VDP& vdp, int pal, int r, int g, int b) {
    setPal(vdp, pal, {0, gs::rgb4(r, g, b), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    inkPal(vdp, PAL_HUD, 14, 13, 10);
    setPal(vdp, PAL_GROUND,
           {0, gs::rgb4(1, 1, 1), gs::rgb4(2, 2, 1), gs::rgb4(3, 2, 1), gs::rgb4(4, 4, 3), gs::rgb4(5, 4, 2),
            gs::rgb4(4, 3, 2), gs::rgb4(6, 5, 3), gs::rgb4(5, 4, 2), gs::rgb4(6, 5, 3), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_TIMBER,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 1), gs::rgb4(8, 6, 3), gs::rgb4(10, 8, 4), gs::rgb4(3, 2, 1),
            gs::rgb4(5, 4, 2), gs::rgb4(2, 2, 1), gs::rgb4(7, 5, 2), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(3, 4, 6), gs::rgb4(2, 3, 4), gs::rgb4(12, 8, 5), gs::rgb4(8, 5, 3), gs::rgb4(2, 2, 1),
            gs::rgb4(9, 9, 8), gs::rgb4(14, 12, 6), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BASKET,
           {0, gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(11, 7, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FIRE,
           {0, gs::rgb4(13, 3, 1), gs::rgb4(15, 9, 1), gs::rgb4(15, 14, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_MOON, {0, gs::rgb4(13, 13, 11), gs::rgb4(7, 7, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    inkPal(vdp, PAL_STAR, 15, 15, 13);
    setPal(vdp, PAL_OIL, {0, gs::rgb4(4, 5, 2), gs::rgb4(7, 7, 4), gs::rgb4(12, 10, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    inkPal(vdp, PAL_GOLD, 15, 12, 3);
    inkPal(vdp, PAL_ALERT, 15, 3, 2);
    inkPal(vdp, PAL_OK, 6, 14, 5);
    setPal(vdp, PAL_WIND, {0, gs::rgb4(9, 11, 12), gs::rgb4(6, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::Bitmap bank(320, 96);
    paintBank(bank);
    gs::bitmapToPlane(tiles, vdp.B, 0, 16, bank, PAL_GROUND);
    vdp.B.enabled = true;
    vdp.A.enabled = false;

    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.sentry[0] = gs::uploadMipped(vdp, sentryArt(0));
    art.sentry[1] = gs::uploadMipped(vdp, sentryArt(1));
    art.flask = gs::uploadMipped(vdp, flaskArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.gust = gs::uploadMipped(vdp, gustArt());
}

}  // namespace pdawn
