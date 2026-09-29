#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace rickslip {
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

// Top-down rickshaw, nose toward +Y in world after frame 2 (north).
gs::Bitmap shawFrame(int frame) {
    gs::Bitmap src(48, 72);
    src.ellipse(24, 40, 13, 15, 1);
    src.ellipse(24, 38, 9, 10, 2);
    src.rect(17, 44, 14, 8, 3);
    src.ellipse(24, 26, 4, 5, 4);
    src.rect(22, 20, 4, 7, 5);
    src.line(15, 34, 7, 16, 6, 1.7f);
    src.line(33, 34, 41, 16, 6, 1.7f);
    src.ellipse(9, 14, 4, 5, 4);
    src.rect(7, 8, 4, 6, 5);
    src.ellipse(15, 54, 5, 6, 7);
    src.ellipse(33, 54, 5, 6, 7);
    src.ellipse(15, 54, 2, 3, 6);
    src.ellipse(33, 54, 2, 3, 6);
    src.rect(20, 16, 8, 3, 8);
    src.line(24, 16, 24, 6, 6, 1.2f);

    const float ang = frame * (kPi / 4.f);
    const float c = std::cos(ang), s = std::sin(ang);
    gs::Bitmap b(72, 72);
    for (int y = 0; y < src.h; y++) {
        for (int x = 0; x < src.w; x++) {
            int p = src.get(x, y);
            if (!p) continue;
            float lx = float(x) - 24.f;
            float fy = 36.f - float(y);
            float bx = 36.f + lx * s + fy * c;
            float by = 36.f + lx * c - fy * s;
            b.set(int(std::lround(bx)), int(std::lround(by)), p);
            b.set(int(std::lround(bx)) + 1, int(std::lround(by)), p);
        }
    }
    return b;
}

gs::Bitmap pileArt() {
    gs::Bitmap b(14, 28);
    b.rect(5, 2, 4, 24, 1);
    b.rect(3, 0, 8, 4, 2);
    b.rect(4, 22, 6, 4, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 32);
    b.rect(5, 10, 2, 20, 1);
    b.ellipse(6, 6, 5, 5, 2);
    b.rect(2, 28, 8, 3, 3);
    return b;
}

gs::Bitmap cleatArt() {
    gs::Bitmap b(16, 10);
    b.rect(2, 4, 12, 3, 1);
    b.rect(1, 2, 3, 6, 2);
    b.rect(12, 2, 3, 6, 2);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(40, 28);
    b.rect(4, 8, 32, 18, 1);
    b.poly({{2, 10}, {20, 2}, {38, 10}}, 2);
    b.rect(16, 16, 8, 10, 3);
    b.rect(8, 14, 6, 5, 4);
    return b;
}

gs::Bitmap tuftArt() {
    gs::Bitmap b(12, 14);
    b.line(2, 13, 4, 2, 1, 1.4f);
    b.line(6, 13, 6, 1, 2, 1.4f);
    b.line(10, 13, 8, 3, 1, 1.4f);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(16, 22);
    b.rect(2, 2, 2, 20, 1);
    b.poly({{4, 3}, {15, 7}, {4, 11}}, 2);
    return b;
}

gs::Bitmap gullArt(int flap) {
    gs::Bitmap b(18, 10);
    if (flap) {
        b.line(1, 8, 9, 3, 1, 1.2f);
        b.line(17, 8, 9, 3, 1, 1.2f);
    } else {
        b.line(1, 4, 9, 5, 1, 1.2f);
        b.line(17, 4, 9, 5, 1, 1.2f);
    }
    b.set(9, 5, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 3, 1);
    return b;
}

gs::Bitmap dashArt() {
    gs::Bitmap b(10, 4);
    b.rect(0, 1, 10, 2, 1);
    return b;
}

gs::Bitmap pinArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

gs::Bitmap endMarkArt() {
    gs::Bitmap b(28, 18);
    b.rect(2, 2, 24, 14, 1);
    b.rect(6, 6, 16, 6, 2);
    return b;
}

void roadColors(gs::VDP& vdp) {
    auto put = [&](int pal, int i, uint16_t c) { vdp.setColor(pal * 16 + i, c); };
    for (int i = 0; i < 16; i++) put(PAL_SLIP, i, gs::rgb4(2, 5, 8));
    put(PAL_SLIP, 1, gs::rgb4(3, 7, 10));
    put(PAL_SLIP, 2, gs::rgb4(4, 8, 11));
    put(PAL_SLIP, 3, gs::rgb4(6, 10, 12));
    put(PAL_SLIP, 4, gs::rgb4(8, 11, 9));
    put(PAL_SLIP, 5, gs::rgb4(2, 4, 6));
    put(PAL_SLIP, 8, gs::rgb4(5, 8, 7));
    put(PAL_SLIP, 14, gs::rgb4(12, 11, 6));
    put(PAL_SLIP, 15, gs::rgb4(9, 8, 5));

    for (int i = 0; i < 16; i++) put(PAL_QUAY, i, gs::rgb4(6, 5, 4));
    put(PAL_QUAY, 1, gs::rgb4(8, 7, 5));
    put(PAL_QUAY, 2, gs::rgb4(5, 4, 3));
    put(PAL_QUAY, 3, gs::rgb4(9, 8, 6));
    put(PAL_QUAY, 4, gs::rgb4(7, 6, 4));
    put(PAL_QUAY, 14, gs::rgb4(13, 11, 4));
    put(PAL_QUAY, 15, gs::rgb4(4, 4, 3));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_SHAW,
           {0, gs::rgb4(12, 3, 2), gs::rgb4(14, 8, 3), gs::rgb4(6, 4, 3), gs::rgb4(13, 10, 7), gs::rgb4(3, 2, 2),
            gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), gs::rgb4(14, 12, 4)});
    setPal(vdp, PAL_PIER, {0, gs::rgb4(8, 7, 5), gs::rgb4(11, 10, 7), gs::rgb4(5, 4, 3), gs::rgb4(4, 6, 4)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(4, 4, 5), gs::rgb4(14, 12, 4), gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(13, 11, 3), gs::rgb4(12, 4, 2)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(10, 9, 7)});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(14, 14, 13), gs::rgb4(8, 6, 3)});
    setPal(vdp, PAL_TIDE, {0, gs::rgb4(6, 10, 12)});
    setPal(vdp, PAL_CANOPY, {0, gs::rgb4(10, 3, 2), gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_SHED, {0, gs::rgb4(7, 5, 4), gs::rgb4(10, 4, 3), gs::rgb4(3, 3, 4), gs::rgb4(12, 11, 8)});
    textPal(vdp, PAL_HUD, gs::rgb4(14, 13, 10));
    textPal(vdp, PAL_WIN, gs::rgb4(6, 14, 7));
    textPal(vdp, PAL_ALERT, gs::rgb4(14, 5, 3));
    textPal(vdp, PAL_BANNER, gs::rgb4(14, 12, 6));
    textPal(vdp, PAL_MAP, gs::rgb4(8, 12, 14));
    roadColors(vdp);
    vdp.setFogColor(gs::rgb4(6, 7, 8));

    for (int i = 0; i < 8; i++) art.shaw[i] = gs::uploadMipped(vdp, shawFrame(i));
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.cleat = gs::uploadMipped(vdp, cleatArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(0));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(1));
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.dash = gs::uploadMipped(vdp, dashArt());
    art.pin = gs::uploadMipped(vdp, pinArt());
    art.endMark = gs::uploadMipped(vdp, endMarkArt());
    art.title = gs::uploadMipped(vdp, banner("RICKSHAW SLIP", 2));
    art.berthed = gs::uploadMipped(vdp, banner("BERTHED", 3));
    art.inSlip = gs::uploadMipped(vdp, banner("IN THE SLIP", 2));
    art.missed = gs::uploadMipped(vdp, banner("MISSED THE END", 2));
    art.tide = gs::uploadMipped(vdp, banner("TIDE TURNED", 2));
    art.scraped = gs::uploadMipped(vdp, banner("SCRAPED THE PIER", 2));
    art.leg = gs::uploadMipped(vdp, banner("LEG FAILED", 2));
    art.paused = gs::uploadMipped(vdp, banner("PAUSED", 2));
    loadFont(vdp, art);
}

}  // namespace rickslip
