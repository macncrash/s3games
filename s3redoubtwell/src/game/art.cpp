#include "art.h"

#include <cstdint>
#include <initializer_list>
#include <string>

namespace rwell {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void inkPal(gs::VDP& vdp, int pal, int r, int g, int b) {
    setPal(vdp, pal,
           {0, gs::rgb4(r, g, b), gs::rgb4(r / 3, g / 3, b / 3), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0});
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

void paintWork(gs::Bitmap& b) {
    b.rect(0, 0, 320, 40, 2);
    b.rect(0, 36, 320, 6, 4);
    b.rect(0, 42, 320, 52, 6);
    b.rect(18, 48, 284, 40, 5);
    b.rect(0, 88, 320, 22, 8);
    b.rect(0, 86, 320, 3, 9);
    b.rect(0, 108, 320, 4, 3);
    for (int y = 112; y < 224; y++) {
        int shade = 10 + ((y / 18) & 1);
        b.rect(0, y, 320, 1, shade);
    }
    for (int i = 0; i < LANES; i++) {
        int x = int(LANE_X[i]);
        b.rect(x - 16, 88, 32, 22, 7);
        b.rect(x - 18, 86, 4, 24, 9);
        b.rect(x + 14, 86, 4, 24, 9);
        b.line(float(x), 112, float(x), 220, 12, 1.5f);
    }
    for (int y = 118; y < 218; y += 5)
        for (int x = 8; x < 312; x += 7) {
            uint32_t h = hash2(x, y);
            if ((h % 17) == 0) b.set(x, y, (h & 1) ? 13 : 14);
        }
    b.rect(8, 70, 10, 16, 9);
    b.rect(302, 70, 10, 16, 9);
    b.ellipse(24, 22, 10, 6, 15);
    b.ellipse(40, 20, 14, 7, 15);
    b.rect(132, 14, 56, 3, 1);
}

gs::Bitmap figure(int coat, int step, bool pack, bool lean) {
    gs::Bitmap b(34, 48);
    b.ellipse(16, 9, 7, 6, 4);
    b.rect(11, 8, 11, 3, 5);
    b.rect(9, 16, 14, 15, coat);
    b.rect(9, 16, 4, 15, coat == 1 ? 2 : 3);
    b.rect(12, 18, 4, 3, 6);
    if (pack) b.ellipse(24, 24, 5, 6, 8);
    if (lean) b.rect(18, 20, 12, 2, 7);
    else b.rect(20, 22, 10, 2, 7);
    int lx = step ? 8 : 12;
    int rx = step ? 18 : 15;
    b.rect(lx, 31, 5, 11, 1);
    b.rect(rx, 31, 5, 11, 2);
    b.rect(lx - 1, 41, 7, 3, 9);
    b.rect(rx - 1, 41, 7, 3, 9);
    b.outline(3, false);
    return b;
}

gs::Bitmap sentryArt(int step) {
    gs::Bitmap b(40, 46);
    b.ellipse(14, 9, 7, 6, 4);
    b.rect(9, 8, 12, 3, 5);
    b.rect(7, 15, 14, 15, 1);
    b.rect(7, 15, 4, 15, 2);
    b.rect(10, 17, 4, 3, 6);
    b.rect(18, 20, 16, 3, 8);
    b.rect(32, 18, 4, 5, 7);
    b.rect(9, 30, 5, 11, 1);
    b.rect(16 + step, 30, 5, 11, 2);
    b.rect(8, 40, 7, 3, 9);
    b.rect(16, 40, 7, 3, 9);
    b.outline(3, false);
    return b;
}

gs::Bitmap wellArt() {
    gs::Bitmap b(72, 56);
    b.ellipse(36, 34, 30, 16, 3);
    b.ellipse(36, 30, 26, 13, 1);
    b.ellipse(36, 28, 20, 10, 2);
    b.ellipse(36, 26, 14, 7, 6);
    b.ellipse(36, 24, 9, 4, 5);
    b.rect(10, 16, 6, 16, 4);
    b.rect(56, 16, 6, 16, 4);
    b.rect(10, 12, 52, 5, 8);
    b.rect(14, 8, 4, 6, 7);
    b.outline(9, false);
    return b;
}

gs::Bitmap crackArt() {
    gs::Bitmap b(28, 22);
    b.line(4, 2, 12, 10, 1, 2.f);
    b.line(12, 10, 8, 20, 1, 2.f);
    b.line(12, 10, 22, 16, 2, 1.5f);
    return b;
}

gs::Bitmap shotArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 2, 1);
    b.set(4, 4, 3);
    return b;
}

gs::Bitmap muzzleArt() {
    gs::Bitmap b(14, 10);
    b.ellipse(6, 5, 5, 4, 2);
    b.ellipse(6, 5, 2, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_WORK,
           {0, gs::rgb4(12, 11, 8), gs::rgb4(6, 9, 13), gs::rgb4(4, 6, 8), gs::rgb4(8, 10, 12), gs::rgb4(7, 6, 4),
            gs::rgb4(5, 5, 3), gs::rgb4(9, 8, 5), gs::rgb4(6, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(3, 5, 2),
            gs::rgb4(4, 6, 3), gs::rgb4(10, 9, 5), gs::rgb4(8, 7, 4), gs::rgb4(2, 3, 1), gs::rgb4(14, 14, 13)});
    setPal(vdp, PAL_WELL,
           {0, gs::rgb4(5, 5, 5), gs::rgb4(7, 7, 6), gs::rgb4(3, 3, 3), gs::rgb4(8, 8, 7), gs::rgb4(2, 5, 8),
            gs::rgb4(4, 8, 11), gs::rgb4(6, 5, 3), gs::rgb4(9, 7, 4), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(3, 5, 3), gs::rgb4(1, 3, 2), gs::rgb4(0, 0, 0), gs::rgb4(11, 8, 5), gs::rgb4(3, 3, 2),
            gs::rgb4(13, 10, 7), gs::rgb4(4, 4, 3), gs::rgb4(12, 10, 3), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1), 0, 0, 0,
            0, 0, 0});
    setPal(vdp, PAL_FOE,
           {0, gs::rgb4(6, 3, 2), gs::rgb4(3, 1, 1), gs::rgb4(0, 0, 0), gs::rgb4(11, 8, 5), gs::rgb4(4, 2, 1),
            gs::rgb4(13, 10, 7), gs::rgb4(5, 5, 4), gs::rgb4(8, 6, 2), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1), 0, 0, 0, 0,
            0, 0});
    setPal(vdp, PAL_SAP,
           {0, gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2), gs::rgb4(11, 8, 5), gs::rgb4(3, 3, 4),
            gs::rgb4(13, 10, 7), gs::rgb4(6, 6, 5), gs::rgb4(8, 7, 3), gs::rgb4(5, 4, 2), gs::rgb4(1, 1, 1), 0, 0, 0, 0,
            0, 0});
    setPal(vdp, PAL_RUN,
           {0, gs::rgb4(5, 4, 2), gs::rgb4(3, 2, 1), gs::rgb4(2, 1, 0), gs::rgb4(11, 8, 5), gs::rgb4(4, 3, 2),
            gs::rgb4(13, 10, 7), gs::rgb4(8, 6, 2), gs::rgb4(10, 8, 3), gs::rgb4(3, 3, 2), gs::rgb4(1, 1, 1), 0, 0, 0, 0,
            0, 0});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 14, 6), gs::rgb4(14, 8, 2), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    inkPal(vdp, PAL_HUD, 14, 13, 10);
    inkPal(vdp, PAL_AMBER, 15, 11, 3);
    inkPal(vdp, PAL_ALERT, 15, 4, 3);
    inkPal(vdp, PAL_OK, 6, 14, 6);

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::Bitmap work(320, 224);
    paintWork(work);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, work, PAL_WORK);
    vdp.A.enabled = false;
    vdp.B.enabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) vdp.road[y].on = false;

    art.sentry[0] = gs::uploadMipped(vdp, sentryArt(0));
    art.sentry[1] = gs::uploadMipped(vdp, sentryArt(1));
    art.musket[0] = gs::uploadMipped(vdp, figure(1, 0, false, true));
    art.musket[1] = gs::uploadMipped(vdp, figure(1, 1, false, true));
    art.sapper[0] = gs::uploadMipped(vdp, figure(1, 0, true, false));
    art.sapper[1] = gs::uploadMipped(vdp, figure(1, 1, true, false));
    art.runner[0] = gs::uploadMipped(vdp, figure(2, 0, false, true));
    art.runner[1] = gs::uploadMipped(vdp, figure(2, 1, false, true));
    art.well = gs::uploadMipped(vdp, wellArt());
    art.crack = gs::uploadMipped(vdp, crackArt());
    art.shot = gs::uploadMipped(vdp, shotArt());
    art.muzzle = gs::uploadMipped(vdp, muzzleArt());
}

}  // namespace rwell
