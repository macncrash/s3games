#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace bwell {
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
    b.rect(0, 0, 320, 224, 2);
    b.rect(0, 0, 320, 28, 4);
    b.rect(0, 26, 320, 4, 1);
    b.rect(0, 196, 320, 28, 11);
    b.rect(0, 196, 320, 3, 1);
    b.rect(0, 28, 18, 168, 4);
    b.rect(302, 28, 18, 168, 4);
    b.rect(16, 28, 4, 168, 1);
    b.rect(300, 28, 4, 168, 1);
    for (int y = 36; y < 190; y += 4)
        for (int x = 24; x < 296; x += 6) {
            uint32_t h = hash2(x, y);
            if ((h % 19) == 0) b.set(x, y, (h & 1) ? 3 : 6);
            if ((h % 47) == 0) b.set(x + 1, y, 9);
        }
    for (int i = 0; i < 7; i++) {
        int x = 36 + i * 40;
        b.rect(x, 8, 22, 6, 5);
        b.rect(x + 4, 4, 6, 4, 12);
    }
    b.rect(28, 14, 70, 3, 13);
    b.rect(210, 12, 64, 3, 13);
    b.line(40, 0, 40, 28, 12, 2.f);
    b.ellipse(40, 18, 5, 4, 8);
    b.ellipse(40, 18, 2, 2, 15);
    b.rect(250, 40, 40, 22, 1);
    b.rect(254, 44, 32, 10, 12);
    b.rect(266, 36, 3, 8, 12);
    b.ellipse(48, 168, 18, 8, 10);
    b.ellipse(70, 178, 14, 7, 7);
    b.ellipse(250, 172, 16, 8, 6);
    b.rect(86, 48, 3, 40, 12);
    b.rect(230, 56, 3, 36, 12);
    b.ellipse(WELL_X, WELL_Y + 8, 46, 28, 11);
    b.ellipse(WELL_X, WELL_Y + 6, 34, 20, 9);
    gs::Bitmap tag = gs::textBitmap("W-1", {2, 5, 0, 0, 1});
    b.blit(tag, 22, 40);
}

gs::Bitmap person(int kind, int step) {
    const bool ram = kind == 2;
    const int W = ram ? 48 : 36;
    const int H = ram ? 58 : 50;
    gs::Bitmap b(W, H);
    const float cx = W * 0.5f;
    b.ellipse(cx, 12, ram ? 11.f : 8.f, ram ? 9.f : 7.f, 4);
    b.rect(cx - 7, 11, ram ? 16.f : 14.f, 4, 5);
    b.rect(cx - (ram ? 12.f : 8.f), 20, ram ? 24.f : 16.f, ram ? 22.f : 16.f, 1);
    b.rect(cx - (ram ? 12.f : 8.f), 20, 5, ram ? 22.f : 16.f, 2);
    b.rect(cx - 3, 22, 5, 4, 3);
    if (kind == 1) b.ellipse(cx + 8, 30, 6, 5, 8);
    if (ram) {
        b.rect(4, 24, 8, 18, 6);
        b.rect(W - 12, 24, 8, 18, 6);
    }
    b.rect(6, 30, W - 12, 3, 6);
    int lx = step ? 8 : 12;
    int rx = step ? W - 14 : W - 18;
    b.rect(lx, 38, 6, 12, 1);
    b.rect(rx, 38, 6, 12, 2);
    b.rect(lx, 48, 7, 4, 7);
    b.rect(rx, 48, 7, 4, 7);
    b.outline(9, false);
    return b;
}

gs::Bitmap gunner(int step) {
    gs::Bitmap b(40, 48);
    b.ellipse(16, 11, 8, 7, 4);
    b.rect(10, 10, 13, 3, 5);
    b.rect(8, 18, 16, 16, 1);
    b.rect(8, 18, 5, 16, 2);
    b.rect(12, 20, 5, 4, 3);
    b.rect(20, 22, 16, 3, 6);
    b.rect(32, 20, 4, 5, 8);
    b.rect(10, 34, 6, 10, 1);
    b.rect(18 + step * 2, 34, 6, 10, 2);
    b.rect(9, 42, 8, 4, 7);
    b.rect(18, 42, 8, 4, 7);
    b.outline(9, false);
    return b;
}

gs::Bitmap wellArt() {
    gs::Bitmap b(96, 78);
    b.ellipse(48, 46, 40, 24, 4);
    b.ellipse(48, 42, 36, 20, 1);
    b.ellipse(48, 40, 30, 16, 2);
    b.ellipse(48, 38, 22, 12, 7);
    b.ellipse(48, 36, 16, 8, 5);
    b.ellipse(44, 34, 6, 3, 6);
    for (int i = 0; i < 8; i++) {
        float a = i * 0.78f;
        int x = int(48 + std::cos(a) * 28);
        int y = int(40 + std::sin(a) * 16);
        b.rect(x, y, 3, 5, 3);
    }
    b.rect(46, 8, 4, 30, 9);
    b.ellipse(48, 8, 16, 5, 9);
    b.rect(32, 6, 3, 8, 10);
    b.rect(61, 6, 3, 8, 10);
    b.rect(40, 18, 6, 8, 10);
    b.rect(42, 26, 2, 10, 9);
    b.ellipse(48, 44, 3, 2, 8);
    b.outline(11, false);
    return b;
}

gs::Bitmap crackArt() {
    gs::Bitmap b(36, 28);
    b.line(8, 4, 16, 14, 1, 1.4f);
    b.line(16, 14, 10, 26, 1, 1.3f);
    b.line(16, 14, 28, 20, 1, 1.2f);
    return b;
}

gs::Bitmap shotArt() {
    gs::Bitmap b(10, 4);
    b.rect(0, 1, 10, 2, 3);
    b.rect(6, 0, 4, 4, 1);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 2);
    b.ellipse(6, 6, 2.5f, 2.5f, 3);
    return b;
}

void inkPal(gs::VDP& vdp, int pal, int r, int g, int b) {
    setPal(vdp, pal, {0, gs::rgb4(r, g, b), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    inkPal(vdp, PAL_HUD, 14, 14, 12);
    setPal(vdp, PAL_YARD,
           {0, gs::rgb4(1, 1, 1), gs::rgb4(3, 3, 2), gs::rgb4(4, 4, 3), gs::rgb4(5, 5, 4), gs::rgb4(8, 8, 6),
            gs::rgb4(6, 5, 3), gs::rgb4(9, 8, 4), gs::rgb4(15, 13, 5), gs::rgb4(2, 2, 2), gs::rgb4(3, 5, 2),
            gs::rgb4(2, 2, 1), gs::rgb4(6, 6, 6), gs::rgb4(8, 4, 2), 0, gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_WELL,
           {0, gs::rgb4(4, 4, 4), gs::rgb4(6, 6, 5), gs::rgb4(9, 9, 8), gs::rgb4(3, 3, 3), gs::rgb4(2, 5, 7),
            gs::rgb4(5, 9, 11), gs::rgb4(7, 6, 5), gs::rgb4(3, 6, 3), gs::rgb4(8, 6, 3), gs::rgb4(5, 5, 6),
            gs::rgb4(1, 1, 1), 0, 0, 0, 0});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(2, 4, 2), gs::rgb4(1, 2, 1), gs::rgb4(12, 8, 5), gs::rgb4(4, 5, 3), gs::rgb4(2, 3, 2),
            gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 1), gs::rgb4(13, 11, 4), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FOE,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1), gs::rgb4(12, 8, 5), gs::rgb4(5, 4, 2), gs::rgb4(2, 2, 1),
            gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 1), gs::rgb4(8, 6, 2), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SAP,
           {0, gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), gs::rgb4(12, 8, 5), gs::rgb4(4, 4, 5), gs::rgb4(1, 1, 2),
            gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 1), gs::rgb4(9, 7, 2), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_RAM,
           {0, gs::rgb4(5, 2, 2), gs::rgb4(3, 1, 1), gs::rgb4(12, 8, 5), gs::rgb4(4, 3, 3), gs::rgb4(2, 1, 1),
            gs::rgb4(6, 6, 5), gs::rgb4(2, 2, 1), gs::rgb4(8, 4, 2), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 13, 4), gs::rgb4(14, 7, 2), gs::rgb4(15, 15, 12), gs::rgb4(6, 5, 4), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0});
    inkPal(vdp, PAL_AMBER, 15, 11, 3);
    inkPal(vdp, PAL_ALERT, 15, 4, 3);
    inkPal(vdp, PAL_OK, 6, 14, 6);

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::Bitmap yard(320, 224);
    paintYard(yard);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, yard, PAL_YARD);
    vdp.A.enabled = false;
    vdp.B.enabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) vdp.road[y].on = false;

    art.gunner[0] = gs::uploadMipped(vdp, gunner(0));
    art.gunner[1] = gs::uploadMipped(vdp, gunner(1));
    art.raider[0] = gs::uploadMipped(vdp, person(0, 0));
    art.raider[1] = gs::uploadMipped(vdp, person(0, 1));
    art.sapper[0] = gs::uploadMipped(vdp, person(1, 0));
    art.sapper[1] = gs::uploadMipped(vdp, person(1, 1));
    art.rammer[0] = gs::uploadMipped(vdp, person(2, 0));
    art.rammer[1] = gs::uploadMipped(vdp, person(2, 1));
    art.well = gs::uploadMipped(vdp, wellArt());
    art.crack = gs::uploadMipped(vdp, crackArt());
    art.shot = gs::uploadMipped(vdp, shotArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
}

}  // namespace bwell
