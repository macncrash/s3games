#include "game/art.h"

#include <cmath>
#include <string>

namespace tramgrass {
namespace {

constexpr float kPi = 3.14159265f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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

gs::Bitmap banner(const char* s, int scale) {
    gs::TextStyle st{scale, 1, 0, 0, 1};
    return gs::textBitmap(s, st);
}

// Nose toward +Y in source, then rotated so frame i faces i * 45 degrees from east.
gs::Bitmap tramFrame(int frame) {
    gs::Bitmap src(40, 72);
    src.rect(10, 6, 20, 60, 1);
    src.rect(10, 6, 20, 6, 2);
    src.rect(10, 60, 20, 6, 2);
    src.rect(12, 16, 16, 8, 3);
    src.rect(12, 28, 16, 8, 3);
    src.rect(12, 40, 16, 8, 3);
    src.rect(12, 52, 16, 6, 3);
    src.rect(18, 2, 4, 6, 4);
    src.line(20, 2, 20, 0, 5, 1.2f);
    src.rect(8, 22, 3, 6, 6);
    src.rect(29, 22, 3, 6, 6);
    src.rect(8, 46, 3, 6, 6);
    src.rect(29, 46, 3, 6, 6);
    src.rect(14, 8, 12, 3, 7);

    const float ang = frame * (kPi / 4.f);
    const float c = std::cos(ang), s = std::sin(ang);
    gs::Bitmap b(80, 80);
    for (int y = 0; y < src.h; y++) {
        for (int x = 0; x < src.w; x++) {
            int p = src.get(x, y);
            if (!p) continue;
            float lx = float(x) - 20.f;
            float fy = 36.f - float(y);
            float bx = 40.f + lx * s + fy * c;
            float by = 40.f + lx * c - fy * s;
            b.set(int(std::lround(bx)), int(std::lround(by)), p);
            b.set(int(std::lround(bx)) + 1, int(std::lround(by)), p);
        }
    }
    return b;
}

gs::Bitmap shadeArt() {
    gs::Bitmap b(56, 16);
    b.ellipse(28, 8, 24, 5, 1);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(14, 40);
    b.rect(6, 6, 2, 34, 1);
    b.rect(1, 4, 12, 3, 2);
    b.ellipse(7, 3, 2, 2, 3);
    return b;
}

gs::Bitmap tuftArt() {
    gs::Bitmap b(14, 16);
    b.line(2, 15, 3, 2, 1, 1.5f);
    b.line(7, 15, 6, 1, 2, 1.5f);
    b.line(11, 15, 10, 3, 1, 1.5f);
    return b;
}

gs::Bitmap boardArt() {
    gs::Bitmap b(22, 16);
    b.rect(9, 6, 4, 10, 1);
    b.rect(2, 0, 18, 8, 2);
    b.rect(4, 2, 14, 4, 3);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 3, 1);
    b.set(5, 2, 2);
    b.set(5, 7, 2);
    return b;
}

void surfaceColors(gs::VDP& vdp) {
    auto put = [&](int pal, int i, uint16_t c) { vdp.setColor(pal * 16 + i, c); };
    for (int i = 0; i < 16; i++) put(PAL_RAIL, i, gs::rgb4(4, 4, 5));
    put(PAL_RAIL, 0, 0);
    put(PAL_RAIL, 1, gs::rgb4(6, 6, 7));
    put(PAL_RAIL, 2, gs::rgb4(4, 4, 5));
    put(PAL_RAIL, 3, gs::rgb4(8, 8, 9));
    put(PAL_RAIL, 4, gs::rgb4(5, 5, 6));
    put(PAL_RAIL, 5, gs::rgb4(3, 3, 4));
    put(PAL_RAIL, 6, gs::rgb4(2, 2, 3));
    put(PAL_RAIL, 7, gs::rgb4(5, 5, 6));
    put(PAL_RAIL, 8, gs::rgb4(7, 7, 8));
    put(PAL_RAIL, 9, gs::rgb4(2, 2, 3));
    put(PAL_RAIL, 10, gs::rgb4(4, 4, 5));
    put(PAL_RAIL, 14, gs::rgb4(13, 12, 4));
    put(PAL_RAIL, 15, gs::rgb4(9, 9, 10));

    const uint16_t greens[] = {
        0,
        gs::rgb4(2, 8, 2),
        gs::rgb4(1, 6, 1),
        gs::rgb4(3, 10, 3),
        gs::rgb4(2, 7, 2),
        gs::rgb4(1, 5, 2),
        gs::rgb4(2, 9, 3),
        gs::rgb4(3, 11, 3),
        gs::rgb4(4, 12, 4),
        gs::rgb4(2, 8, 2),
        gs::rgb4(1, 7, 2),
        gs::rgb4(1, 6, 4),
        gs::rgb4(2, 7, 5),
        gs::rgb4(3, 9, 6),
        gs::rgb4(8, 13, 4),
        gs::rgb4(5, 13, 4),
    };
    for (int i = 0; i < 16; i++) put(PAL_FIELD, i, greens[i]);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 12));
    textPal(vdp, PAL_WIN, gs::rgb4(6, 15, 7));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 3));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 12, 4));
    loadFont(vdp, art);

    setPal(vdp, PAL_TRAM,
           {0, gs::rgb4(14, 13, 10), gs::rgb4(12, 2, 2), gs::rgb4(6, 10, 13), gs::rgb4(3, 3, 4), gs::rgb4(9, 9, 10),
            gs::rgb4(2, 2, 2), gs::rgb4(15, 14, 5)});
    setPal(vdp, PAL_POLE, {0, gs::rgb4(4, 4, 5), gs::rgb4(6, 6, 7), gs::rgb4(14, 13, 6)});
    setPal(vdp, PAL_WIRE, {0, gs::rgb4(3, 8, 3), gs::rgb4(5, 12, 4)});
    setPal(vdp, PAL_SPARK, {0, gs::rgb4(8, 8, 8), gs::rgb4(14, 14, 10)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(3, 3, 4), gs::rgb4(12, 3, 2), gs::rgb4(15, 14, 8)});
    surfaceColors(vdp);

    for (int i = 0; i < 8; i++) art.tram[i] = gs::uploadMipped(vdp, tramFrame(i));
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.board = gs::uploadMipped(vdp, boardArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.title = gs::uploadMipped(vdp, banner("TRAM GRASS", 3));
    art.fullStop = gs::uploadMipped(vdp, banner("FULL STOP", 2));
    art.onGrass = gs::uploadMipped(vdp, banner("ON THE GRASS", 2));
    art.past = gs::uploadMipped(vdp, banner("ROLLED PAST", 2));
    art.leftMeadow = gs::uploadMipped(vdp, banner("LEFT THE MEADOW", 2));
    art.onRails = gs::uploadMipped(vdp, banner("STOPPED ON RAILS", 2));
    art.wheels = gs::uploadMipped(vdp, banner("WHEELS OFF GRASS", 2));
    art.clocked = gs::uploadMipped(vdp, banner("CLOCK RAN OUT", 2));
    art.failed = gs::uploadMipped(vdp, banner("LEG FAILED", 2));
    art.paused = gs::uploadMipped(vdp, banner("PAUSED", 2));
}

}  // namespace tramgrass
