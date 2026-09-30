#include "game/art.h"

#include <cmath>
#include <string>

namespace kilo {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void wheelDisc(Bitmap& b, float cx, float cy, float r, int tire, int spoke, int hub) {
    b.ellipse(cx, cy, r, r, tire);
    b.ellipse(cx, cy, r - 3.2f, r - 3.2f, 0);
    b.line(cx - r + 2, cy, cx + r - 2, cy, spoke, 1.2f);
    b.line(cx, cy - r + 2, cx, cy + r - 2, spoke, 1.2f);
    b.ellipse(cx, cy, 3.0f, 3.0f, hub);
}

Bitmap busBody(Bitmap& b, float y0, float bodyH, int squat) {
    float roof = y0;
    float floor = y0 + bodyH;
    b.rect(8, roof + 4, 80, bodyH - 6, 2);
    b.rect(10, roof, 72, 6, 3);
    b.rect(86, roof + 8, 8, bodyH - 14, 2);
    b.rect(4, roof + 10, 8, bodyH - 16, 4);
    b.rect(14, roof + 8, 16, 10, 8);
    b.rect(34, roof + 8, 14, 10, 8);
    b.rect(52, roof + 8, 14, 10, 8);
    b.rect(70, roof + 8, 12, 10, 8);
    b.rect(16, roof + bodyH * 0.55f, 18, bodyH * 0.32f, 5);
    b.rect(18, roof + bodyH * 0.58f, 6, bodyH * 0.22f, 9);
    b.line(8, floor - 2, 88, floor - 2, 1, 2.0f);
    b.rect(78, roof + 12, 3, 8, squat ? 6 : 7);
    (void)floor;
    return b;
}

Bitmap rideArt(int phase) {
    Bitmap b(104, 56);
    float bob = phase ? 1.f : 0.f;
    busBody(b, 4 + bob, 36, 0);
    wheelDisc(b, 24, 44, 10, 1, 5, 6);
    wheelDisc(b, 78, 44, 10, 1, 5, 6);
    if (phase) {
        b.line(24, 36, 24, 52, 4, 1.1f);
        b.line(78, 36, 78, 52, 4, 1.1f);
    }
    b.outline(1, false);
    return b;
}

Bitmap duckArt() {
    Bitmap b(104, 42);
    busBody(b, 2, 24, 1);
    wheelDisc(b, 24, 32, 9, 1, 5, 6);
    wheelDisc(b, 78, 32, 9, 1, 5, 6);
    b.outline(1, false);
    return b;
}

Bitmap spillArt() {
    Bitmap b(104, 48);
    b.rect(10, 18, 78, 16, 2);
    b.rect(12, 16, 60, 5, 3);
    b.rect(20, 20, 12, 8, 8);
    b.rect(38, 20, 12, 8, 8);
    wheelDisc(b, 22, 36, 10, 1, 5, 6);
    wheelDisc(b, 86, 14, 9, 1, 5, 6);
    b.outline(1, false);
    return b;
}

Bitmap looseWheel(int phase) {
    Bitmap b(36, 36);
    wheelDisc(b, 18, 18, 15, 1, phase ? 4 : 5, 6);
    b.ellipse(18, 18, 15, 15, 2);
    b.ellipse(18, 18, 12, 12, 0);
    b.line(18 - 10, 18, 18 + 10, 18, phase ? 5 : 4, 1.3f);
    b.line(18, 8, 18, 28, phase ? 4 : 5, 1.3f);
    b.ellipse(18, 18, 3, 3, 6);
    b.outline(1, false);
    return b;
}

Bitmap hangWheel() {
    Bitmap b(34, 40);
    b.line(17, 1, 17, 10, 3, 2.0f);
    b.rect(14, 0, 6, 4, 2);
    wheelDisc(b, 17, 24, 13, 1, 5, 6);
    b.outline(1, false);
    return b;
}

Bitmap crewArt() {
    Bitmap b(48, 30);
    b.rect(6, 8, 34, 12, 2);
    b.rect(8, 6, 26, 4, 3);
    b.rect(10, 10, 6, 5, 4);
    wheelDisc(b, 14, 22, 6, 1, 5, 6);
    wheelDisc(b, 34, 22, 6, 1, 5, 6);
    b.outline(1, false);
    return b;
}

Bitmap clockFace(int hand) {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 2);
    b.ellipse(14, 14, 10, 10, 1);
    b.ellipse(14, 14, 9, 9, 3);
    float a = hand * 0.785398f - 1.5708f;
    float x = 14 + std::cos(a) * 7.f;
    float y = 14 + std::sin(a) * 7.f;
    b.line(14, 14, x, y, 4, 1.6f);
    b.ellipse(14, 14, 1.6f, 1.6f, 5);
    b.outline(2, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 88);
    b.rect(6, 8, 4, 78, 1);
    b.rect(1, 2, 14, 8, 2);
    b.rect(3, 12, 10, 6, 4);
    return b;
}

Bitmap skyline() {
    Bitmap b(512, 80);
    for (int i = 0; i < 18; ++i) {
        int h = 22 + ((i * 47) % 52);
        int x = i * 28 + 2;
        int c = 1 + (i % 4);
        b.rect(float(x), float(80 - h), 22, float(h), c);
        for (int wy = 80 - h + 6; wy < 74; wy += 8)
            for (int wx = x + 3; wx < x + 18; wx += 6) b.rect(float(wx), float(wy), 2, 3, 5);
    }
    b.rect(0, 74, 512, 6, 6);
    return b;
}

Bitmap asphalt() {
    Bitmap b(128, 48);
    b.rect(0, 0, 128, 48, 1);
    b.rect(0, 0, 128, 3, 2);
    for (int x = 4; x < 128; x += 16) b.rect(float(x), 22, 8, 3, 3);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 15, 15), gs::rgb4(6, 8, 10), gs::rgb4(2, 3, 4), gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(8, 2, 1), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_BUS, {0, gs::rgb4(1, 2, 3), gs::rgb4(14, 12, 3), gs::rgb4(15, 14, 6), gs::rgb4(3, 5, 8),
                          gs::rgb4(6, 7, 8), gs::rgb4(12, 4, 3), gs::rgb4(15, 10, 2), gs::rgb4(10, 13, 15),
                          gs::rgb4(2, 3, 4)});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(2, 1, 3), gs::rgb4(8, 3, 5), gs::rgb4(12, 6, 6), gs::rgb4(14, 12, 8),
                           gs::rgb4(10, 10, 11), gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(1, 1, 2), gs::rgb4(6, 5, 4), gs::rgb4(10, 8, 5), gs::rgb4(14, 12, 8),
                            gs::rgb4(12, 12, 13), gs::rgb4(15, 10, 2)});
    setPal(vdp, PAL_HANG, {0, gs::rgb4(2, 1, 1), gs::rgb4(8, 3, 3), gs::rgb4(12, 6, 4), gs::rgb4(14, 10, 6),
                           gs::rgb4(10, 10, 11), gs::rgb4(15, 13, 8)});
    setPal(vdp, PAL_CITY, {0, gs::rgb4(3, 4, 7), gs::rgb4(4, 5, 8), gs::rgb4(5, 6, 9), gs::rgb4(6, 7, 10),
                           gs::rgb4(12, 13, 8), gs::rgb4(7, 8, 6)});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6), gs::rgb4(13, 11, 3)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(14, 12, 8), gs::rgb4(3, 2, 2), gs::rgb4(12, 9, 5), gs::rgb4(15, 4, 3),
                            gs::rgb4(8, 6, 3)});
    setPal(vdp, PAL_TITLE, {0, gs::rgb4(15, 14, 8), gs::rgb4(12, 8, 2), gs::rgb4(4, 7, 10)});

    art.ride[0] = gs::uploadMipped(vdp, rideArt(0));
    art.ride[1] = gs::uploadMipped(vdp, rideArt(1));
    art.duck = gs::uploadMipped(vdp, duckArt());
    art.spill = gs::uploadMipped(vdp, spillArt());
    art.wheel[0] = gs::uploadMipped(vdp, looseWheel(0));
    art.wheel[1] = gs::uploadMipped(vdp, looseWheel(1));
    art.hang = gs::uploadMipped(vdp, hangWheel());
    art.crew = gs::uploadMipped(vdp, crewArt());
    for (int i = 0; i < 8; i++) art.clock[i] = gs::uploadMipped(vdp, clockFace(i));
    art.post = gs::uploadMipped(vdp, postArt());

    gs::TextStyle title{3, 1, 2, 0, 1};
    gs::TextStyle sub{1, 2, 0, 0, 1};
    art.title = gs::uploadImage(vdp, gs::textBitmap("S3 BUSKILO", title));
    art.sub = gs::uploadImage(vdp, gs::textBitmap("FINISH THE KILOMETER", sub));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::bitmapToPlane(tiles, vdp.B, 0, 8, skyline(), PAL_CITY);
    for (int x = 0; x < 64; x += 16) gs::bitmapToPlane(tiles, vdp.A, x, 21, asphalt(), PAL_ROAD);
    vdp.A.enabled = true;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
}

}  // namespace kilo
