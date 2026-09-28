#include "game/art.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace scullboom {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(2, 2, 3));
}

Spr finish(gs::VDP& vdp, const gs::Bitmap& b, float ax, float ay, float ppm) {
    int x0 = b.w, y0 = b.h, x1 = -1, y1 = -1;
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++)
            if (b.get(x, y)) {
                x0 = std::min(x0, x);
                y0 = std::min(y0, y);
                x1 = std::max(x1, x);
                y1 = std::max(y1, y);
            }
    Spr s;
    if (x1 < x0) {
        gs::Bitmap dot(2, 2);
        dot.set(0, 0, 1);
        s.img = gs::uploadMipped(vdp, dot);
        s.ppm = ppm;
        return s;
    }
    gs::Bitmap cropped(x1 - x0 + 1, y1 - y0 + 1);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) cropped.set(x - x0, y - y0, b.get(x, y));
    s.img = gs::uploadMipped(vdp, cropped);
    s.ax = ax - float(x0);
    s.ay = ay - float(y0);
    s.ppm = ppm;
    return s;
}

Spr drawShell(gs::VDP& vdp) {
    const float ppm = 14.f;
    gs::Bitmap b(140, 36);
    b.poly({{8, 20}, {18, 16}, {118, 15}, {132, 20}, {124, 24}, {16, 25}}, 1);
    b.poly({{20, 16}, {112, 15}, {110, 18}, {24, 19}}, 2);
    b.rect(62, 12, 10, 6, 3);
    b.rect(64, 8, 3, 8, 4);
    b.line(48, 18, 92, 18, 5, 1.4f);
    b.ellipse(78, 22, 5, 2, 6);
    b.outline(7, false);
    return finish(vdp, b, 70.f, 22.f, ppm);
}

Spr drawRower(gs::VDP& vdp) {
    gs::Bitmap b(28, 32);
    b.ellipse(14, 7, 4, 4, 1);
    b.rect(12, 11, 5, 8, 2);
    b.line(14, 14, 6, 20, 2, 2.f);
    b.line(14, 16, 22, 12, 2, 2.f);
    b.line(13, 19, 9, 28, 3, 2.f);
    b.line(16, 19, 20, 28, 3, 2.f);
    b.outline(4, false);
    return finish(vdp, b, 14.f, 28.f, 18.f);
}

Spr drawOar(gs::VDP& vdp, bool drive) {
    gs::Bitmap b(72, 28);
    if (drive) {
        b.line(8, 20, 58, 8, 1, 2.2f);
        b.poly({{54, 4}, {70, 6}, {66, 14}, {50, 12}}, 2);
        b.rect(6, 18, 6, 4, 3);
    } else {
        b.line(10, 8, 60, 18, 1, 2.2f);
        b.poly({{56, 14}, {70, 16}, {64, 24}, {50, 20}}, 2);
        b.rect(8, 6, 6, 4, 3);
    }
    b.outline(4, false);
    return finish(vdp, b, 12.f, drive ? 20.f : 10.f, 12.f);
}

Spr drawDrive(gs::VDP& vdp) {
    const float ppm = 12.f;
    int w = int(std::lround(kDriveHalf * 2.0 * ppm));
    int h = int(std::lround(kDriveH * ppm));
    gs::Bitmap b(w, h);
    auto log = [&](float cy, int body, int end) {
        float ry = h * 0.18f;
        b.ellipse(ry + 2, cy, ry, ry, end);
        b.ellipse(w - ry - 2, cy, ry, ry, end);
        b.rect(ry + 2, cy - ry, w - 2.f * (ry + 2), ry * 2.f, body);
        b.ellipse(ry + 2, cy, ry * 0.35f, ry * 0.35f, 4);
        b.ellipse(w - ry - 2, cy, ry * 0.35f, ry * 0.35f, 4);
    };
    log(h * 0.72f, 1, 2);
    log(h * 0.42f, 3, 2);
    for (int i = 0; i < 3; i++) b.rect(w * (0.28f + 0.18f * i), h * 0.22f, 2.f, h * 0.58f, 5);
    b.rect(w * 0.42f, h * 0.34f, w * 0.16f, h * 0.2f, 6);
    b.outline(7, false);
    return finish(vdp, b, w * 0.5f, h * 0.5f, ppm);
}

Spr drawPost(gs::VDP& vdp) {
    gs::Bitmap b(18, 72);
    b.rect(6, 4, 6, 64, 1);
    b.rect(8, 6, 2, 58, 2);
    b.rect(3, 2, 12, 5, 3);
    b.rect(4, 60, 10, 6, 4);
    b.rect(2, 66, 14, 4, 5);
    b.outline(6, false);
    return finish(vdp, b, 9.f, float(b.h - 2), float(b.h) / float(kPostTop));
}

Spr drawReed(gs::VDP& vdp) {
    gs::Bitmap b(16, 28);
    b.line(8, 26, 5, 6, 1, 1.5f);
    b.line(8, 24, 12, 4, 2, 1.5f);
    b.line(8, 20, 3, 10, 1, 1.2f);
    b.ellipse(5, 6, 2, 3, 3);
    b.ellipse(12, 4, 2, 3, 3);
    return finish(vdp, b, 8.f, 26.f, 14.f);
}

Spr drawWillow(gs::VDP& vdp) {
    gs::Bitmap b(40, 48);
    b.rect(18, 22, 5, 24, 1);
    b.ellipse(20, 16, 14, 12, 2);
    b.ellipse(14, 20, 8, 6, 3);
    b.ellipse(26, 18, 7, 5, 3);
    b.line(12, 18, 8, 36, 2, 1.4f);
    b.line(28, 16, 32, 38, 2, 1.4f);
    b.line(20, 12, 18, 34, 3, 1.2f);
    b.outline(4, false);
    return finish(vdp, b, 20.f, 46.f, 10.f);
}

Spr drawSign(gs::VDP& vdp) {
    gs::Bitmap b(36, 28);
    b.rect(16, 12, 3, 14, 1);
    b.rect(4, 3, 28, 12, 2);
    b.rect(7, 6, 8, 2, 3);
    b.rect(7, 10, 14, 2, 3);
    b.outline(4, false);
    return finish(vdp, b, 18.f, 26.f, 12.f);
}

Spr drawChev(gs::VDP& vdp) {
    gs::Bitmap b(16, 12);
    b.poly({{8, 1}, {14, 10}, {8, 7}, {2, 10}}, 1);
    return finish(vdp, b, 8.f, 11.f, 10.f);
}

Spr drawSplash(gs::VDP& vdp) {
    gs::Bitmap b(20, 14);
    b.ellipse(10, 10, 8, 3, 1);
    b.ellipse(6, 7, 2, 3, 2);
    b.ellipse(14, 6, 2, 3, 2);
    b.ellipse(10, 4, 2, 2, 3);
    return finish(vdp, b, 10.f, 12.f, 10.f);
}

void paintWater(gs::Bitmap& b, int frame) {
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            int wave = (x + frame * 4 + y / 2) % 9;
            int c = y < 2 ? 4 : (wave == 0 ? 3 : ((x * 3 + y + frame) % 7 == 0 ? 2 : 1));
            b.set(x, y, c);
        }
}

void paintGrass(gs::Bitmap& b) {
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            int c = 1;
            if (((x * 5 + y * 3) % 11) == 0) c = 2;
            if (y < 2 && ((x + y) % 3) == 0) c = 3;
            b.set(x, y, c);
        }
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 15, 8));

    setPal(vdp, PAL_SHELL, {0, gs::rgb4(14, 13, 11), gs::rgb4(10, 8, 6), gs::rgb4(12, 3, 3), gs::rgb4(6, 4, 3),
                            gs::rgb4(8, 10, 12), gs::rgb4(4, 6, 8), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_DRIVE, {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(14, 10, 5), gs::rgb4(6, 4, 2),
                            gs::rgb4(10, 7, 3), gs::rgb4(15, 13, 3), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_BOOM, {0, gs::rgb4(9, 6, 3), gs::rgb4(12, 9, 5), gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2),
                           gs::rgb4(7, 8, 6), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 6, 10), gs::rgb4(3, 8, 12), gs::rgb4(6, 11, 14), gs::rgb4(10, 13, 15),
                            gs::rgb4(1, 4, 7)});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(4, 8, 3), gs::rgb4(6, 11, 4), gs::rgb4(9, 12, 6), gs::rgb4(3, 5, 2)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(5, 3, 2), gs::rgb4(3, 8, 3), gs::rgb4(5, 11, 4), gs::rgb4(2, 4, 2)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(6, 4, 2), gs::rgb4(14, 12, 8), gs::rgb4(3, 3, 4), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_OAR, {0, gs::rgb4(13, 10, 6), gs::rgb4(9, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(2, 1, 1)});

    art.shell = drawShell(vdp);
    art.rower = drawRower(vdp);
    art.oar[0] = drawOar(vdp, false);
    art.oar[1] = drawOar(vdp, true);
    art.drive = drawDrive(vdp);
    art.post = drawPost(vdp);
    art.reed = drawReed(vdp);
    art.willow = drawWillow(vdp);
    art.sign = drawSign(vdp);
    art.chev = drawChev(vdp);
    art.splash = drawSplash(vdp);

    gs::Bitmap plank(8, 8);
    plank.rect(0, 0, 8, 8, 1);
    plank.rect(0, 3, 8, 1, 2);
    plank.rect(0, 6, 8, 1, 3);
    art.plank = gs::uploadMipped(vdp, plank);

    gs::Bitmap water(32, 20);
    paintWater(water, 0);
    art.water[0] = gs::uploadMipped(vdp, water);
    paintWater(water, 1);
    art.water[1] = gs::uploadMipped(vdp, water);
    gs::Bitmap grass(32, 20);
    paintGrass(grass);
    art.grass = gs::uploadMipped(vdp, grass);
    loadFont(vdp, art);
}

}  // namespace scullboom
