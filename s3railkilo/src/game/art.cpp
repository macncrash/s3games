#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace railkilo {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void flange(Bitmap& b, float cx, float cy) {
    b.ellipse(cx, cy, 11, 11, 1);
    b.ellipse(cx, cy, 8.2f, 8.2f, 0);
    b.ellipse(cx, cy, 7.2f, 7.2f, 4);
    b.ellipse(cx, cy, 3.1f, 3.1f, 5);
    b.rect(cx - 13, cy - 1.4f, 26, 2.6f, 2);
}

Bitmap speeder(int phase, bool low) {
    Bitmap b(78, low ? 40 : 56);
    float base = low ? 28.f : 44.f;
    flange(b, 18, base);
    flange(b, 58, base);
    if (phase) {
        b.line(18, base - 7, 18, base + 7, 6, 1.1f);
        b.line(58, base - 7, 58, base + 7, 6, 1.1f);
    } else {
        b.line(12, base, 24, base, 6, 1.1f);
        b.line(52, base, 64, base, 6, 1.1f);
    }
    b.rect(8, base - 8, 62, 6, 3);
    b.rect(10, base - 10, 58, 3, 7);
    float roof = low ? 8.f : 6.f;
    float cabTop = low ? 16.f : 14.f;
    b.rect(30, roof, 28, cabTop, 3);
    b.rect(33, roof + 3, 16, low ? 6.f : 10.f, 8);
    b.rect(50, roof + 4, 5, 5, 9);
    b.rect(22, base - 14, 8, 6, 6);
    b.rect(66, base - 12, 6, 4, 9);
    b.outline(1, false);
    return b;
}

Bitmap spillArt() {
    Bitmap b(80, 36);
    flange(b, 22, 24);
    flange(b, 60, 16);
    b.line(16, 22, 66, 12, 3, 3.0f);
    b.rect(34, 8, 22, 12, 3);
    b.rect(37, 10, 10, 6, 8);
    b.outline(1, false);
    return b;
}

Bitmap scrapWheel(int phase) {
    Bitmap b(34, 34);
    b.ellipse(17, 17, 14, 14, 1);
    b.ellipse(17, 17, 11, 11, 0);
    b.ellipse(17, 17, 10, 10, 3);
    float a = phase ? 0.9f : 0.2f;
    b.line(17 - 9, 17 - a, 17 + 9, 17 + a, 5, 1.4f);
    b.line(17 - a, 17 - 9, 17 + a, 17 + 9, 6, 1.4f);
    b.ellipse(17, 17, 3.2f, 3.2f, 2);
    b.rect(4, 30, 26, 3, 4);
    b.outline(1, false);
    return b;
}

Bitmap gantryArt() {
    Bitmap b(40, 72);
    b.rect(30, 0, 5, 72, 2);
    b.rect(8, 6, 26, 4, 3);
    b.line(18, 10, 18, 22, 4, 1.6f);
    b.ellipse(18, 36, 13, 13, 1);
    b.ellipse(18, 36, 9.5f, 9.5f, 0);
    b.ellipse(18, 36, 8.4f, 8.4f, 5);
    b.line(10, 36, 26, 36, 6, 1.2f);
    b.line(18, 28, 18, 44, 6, 1.2f);
    b.ellipse(18, 36, 3, 3, 7);
    b.rect(32, 58, 4, 6, 8);
    b.outline(1, false);
    return b;
}

Bitmap crewArt() {
    Bitmap b(46, 28);
    b.ellipse(10, 20, 7, 7, 1);
    b.ellipse(34, 20, 7, 7, 1);
    b.ellipse(10, 20, 3.4f, 3.4f, 0);
    b.ellipse(34, 20, 3.4f, 3.4f, 0);
    b.rect(6, 12, 34, 5, 2);
    b.rect(18, 4, 16, 10, 3);
    b.rect(21, 6, 8, 5, 4);
    b.outline(1, false);
    return b;
}

Bitmap clockFace(int hand) {
    Bitmap b(26, 26);
    b.ellipse(13, 13, 12, 12, 2);
    b.ellipse(13, 13, 10, 10, 1);
    float ang = hand * 1.0472f - 1.5708f;
    float c = std::cos(ang), s = std::sin(ang);
    b.line(13, 13, 13 + c * 7.f, 13 + s * 7.f, 4, 1.4f);
    b.line(13, 13, 13 + c * 4.f, 13 - s * 3.f, 3, 1.2f);
    b.ellipse(13, 13, 1.6f, 1.6f, 5);
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 64);
    b.rect(7, 10, 3, 54, 1);
    b.rect(2, 8, 12, 10, 2);
    b.rect(4, 10, 8, 6, 3);
    return b;
}

Bitmap yard() {
    Bitmap b(512, 72);
    for (int i = 0; i < 16; i++) {
        int h = 16 + ((i * 37) % 40);
        int x = i * 32;
        b.poly({{float(x), 72}, {float(x + 16), float(72 - h)}, {float(x + 32), 72}}, 1 + (i % 3));
    }
    for (int i = 0; i < 8; i++) {
        int x = 20 + i * 64;
        b.line(float(x), 20, float(x), 70, 4, 1.4f);
        b.line(float(x), 24, float(x + 48), 24, 5, 1.0f);
    }
    return b;
}

Bitmap ballast() {
    Bitmap b(128, 40);
    b.rect(0, 0, 128, 40, 1);
    b.rect(0, 0, 128, 6, 2);
    b.rect(54, 8, 20, 4, 3);
    for (int x = 0; x < 128; x += 16) b.rect(float(x + 2), 14, 10, 4, 4);
    for (int y = 22; y < 40; y += 5)
        for (int x = (y & 1) ? 4 : 0; x < 128; x += 9) b.set(x, y, 5);
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 15, 13), gs::rgb4(5, 7, 8), gs::rgb4(2, 3, 4), gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 7, 3), gs::rgb4(8, 2, 1), gs::rgb4(15, 14, 9)});
    setPal(vdp, PAL_SPEEDER,
           {0, gs::rgb4(1, 2, 3), gs::rgb4(8, 5, 3), gs::rgb4(12, 8, 4), gs::rgb4(4, 5, 6), gs::rgb4(14, 13, 10),
            gs::rgb4(9, 10, 11), gs::rgb4(6, 7, 8), gs::rgb4(10, 14, 15), gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(2, 1, 2), gs::rgb4(12, 3, 3), gs::rgb4(14, 7, 4), gs::rgb4(15, 12, 8)});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(1, 1, 2), gs::rgb4(9, 6, 3), gs::rgb4(5, 5, 6), gs::rgb4(7, 6, 4),
                            gs::rgb4(13, 12, 10), gs::rgb4(15, 11, 4)});
    setPal(vdp, PAL_GANTRY, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 7), gs::rgb4(9, 8, 6), gs::rgb4(4, 4, 5),
                             gs::rgb4(12, 11, 8), gs::rgb4(14, 13, 11), gs::rgb4(15, 8, 2), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_YARD, {0, gs::rgb4(2, 4, 5), gs::rgb4(3, 5, 6), gs::rgb4(4, 6, 7), gs::rgb4(5, 5, 4),
                           gs::rgb4(8, 8, 5)});
    setPal(vdp, PAL_RAIL, {0, gs::rgb4(3, 3, 3), gs::rgb4(5, 5, 4), gs::rgb4(12, 11, 8), gs::rgb4(6, 5, 4),
                           gs::rgb4(4, 4, 3)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(14, 12, 8), gs::rgb4(3, 2, 2), gs::rgb4(10, 8, 5), gs::rgb4(15, 5, 3),
                            gs::rgb4(8, 6, 3)});
    setPal(vdp, PAL_TITLE, {0, gs::rgb4(15, 14, 11), gs::rgb4(14, 8, 3), gs::rgb4(5, 8, 9)});

    art.ride[0] = gs::uploadMipped(vdp, speeder(0, false));
    art.ride[1] = gs::uploadMipped(vdp, speeder(1, false));
    art.duck = gs::uploadMipped(vdp, speeder(0, true));
    art.spill = gs::uploadMipped(vdp, spillArt());
    art.scrap[0] = gs::uploadMipped(vdp, scrapWheel(0));
    art.scrap[1] = gs::uploadMipped(vdp, scrapWheel(1));
    art.gantry = gs::uploadMipped(vdp, gantryArt());
    art.crew = gs::uploadMipped(vdp, crewArt());
    for (int i = 0; i < 6; i++) art.clock[i] = gs::uploadMipped(vdp, clockFace(i));
    art.post = gs::uploadMipped(vdp, postArt());

    gs::TextStyle title{3, 1, 2, 0, 1};
    gs::TextStyle sub{1, 2, 0, 0, 1};
    art.title = gs::uploadImage(vdp, gs::textBitmap("S3 RAILKILO", title));
    art.sub = gs::uploadImage(vdp, gs::textBitmap("TAKE THE RAIL  BEAT THE CREW", sub));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::bitmapToPlane(tiles, vdp.B, 0, 12, yard(), PAL_YARD);
    for (int x = 0; x < 64; x += 16) gs::bitmapToPlane(tiles, vdp.A, x, 21, ballast(), PAL_RAIL);
    vdp.A.enabled = true;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
}

}  // namespace railkilo
