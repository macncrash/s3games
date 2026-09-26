#include "game/art.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <initializer_list>

namespace dartseven {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t edge) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 2, edge);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

bool grain(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u ^ uint32_t(y) * 668265263u;
    h ^= h >> 13;
    return (h & 31u) == 0;
}

bool wireAt(float dx, float dy, float r) {
    const float rings[] = {kBullIn, kBullOut, kTripIn, kTripOut, kDoubIn, kDoubOut};
    for (float rr : rings)
        if (std::fabs(r - rr) <= 0.7f) return true;
    if (r <= kBullOut || r > kDoubOut) return false;
    float deg = std::atan2(dx, -dy) * (180.f / 3.14159265f);
    if (deg < 0.f) deg += 360.f;
    float e = std::fmod(deg + 9.f, 18.f);
    float ded = std::min(e, 18.f - e);
    return ded * 0.017453292f * r <= 0.72f;
}

int boardPixel(int x, int y) {
    float dx = (x + 0.5f) - kBmpC;
    float dy = (y + 0.5f) - kBmpC;
    float r = std::hypot(dx, dy);
    if (r > kRim) return 0;
    if (r > kRim - 2.2f) return 12;
    bool spec = grain(x, y);
    if (r > kWoodIn) {
        // A red notch outside the 20, so the fast bed is easy to find.
        if (r < kWoodIn + 3.2f && sectorAt(dx, dy) == kFastSeg && std::fabs(dx) < 4.5f) return 6;
        return spec ? 14 : 13;
    }
    for (int i = 0; i < 4; i++) {
        float a = 0.78539816f + float(i) * 1.5707963f;
        float sx = std::cos(a) * (kRim - 3.4f);
        float sy = std::sin(a) * (kRim - 3.4f);
        if (std::hypot(dx - sx, dy - sy) < 2.1f) return spec ? 15 : 12;
    }
    if (r <= kDoubOut + 0.8f && wireAt(dx, dy, r)) return 11;
    if (r <= kBullIn) return 9;
    if (r <= kBullOut) return spec ? 8 : 10;
    int s = sectorAt(dx, dy);
    bool dark = (s & 1) == 0;
    bool ring = (r > kTripIn && r <= kTripOut) || r > kDoubIn;
    if (ring) return dark ? (spec ? 6 : 5) : (spec ? 8 : 7);
    return dark ? (spec ? 2 : 1) : (spec ? 4 : 3);
}

gs::Bitmap boardArt() {
    gs::Bitmap b(kBoard, kBoard);
    for (int y = 0; y < kBoard; y++)
        for (int x = 0; x < kBoard; x++) b.set(x, y, boardPixel(x, y));
    for (int s = 0; s < 20; s++) {
        char buf[4];
        std::snprintf(buf, sizeof buf, "%d", kSeg[s]);
        gs::Bitmap t = gs::textBitmap(buf, {1, 15, 12, 0, 0});
        float ang = float(s) * (3.14159265f / 10.f);
        float nx = kBmpC + std::sin(ang) * kNumR;
        float ny = kBmpC - std::cos(ang) * kNumR;
        b.blit(t, int(std::lround(nx - t.w * 0.5f)), int(std::lround(ny - t.h * 0.5f)));
    }
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(16, 118);
    b.rect(2, 0, 12, 118, 13);
    b.rect(4, 2, 8, 114, 1);
    b.rect(6, 0, 4, 118, 12);
    for (int i = 0; i < kRace; i++) {
        float cy = 11.f + float(i) * kLampStep;
        b.ellipse(8.f, cy, 5.2f, 5.2f, 1);
        b.ellipse(8.f, cy, 3.1f, 3.1f, 2);
    }
    return b;
}

gs::Bitmap dartArt() {
    gs::Bitmap b(15, 24);
    b.line(7, 1, 1, 8, 1, 1.8f);
    b.line(7, 1, 13, 8, 1, 1.8f);
    b.line(7, 2, 3, 9, 1, 1.2f);
    b.line(7, 2, 11, 9, 1, 1.2f);
    b.line(7, 8, 7, 13, 4, 1.5f);
    b.ellipse(7.f, 16.2f, 2.6f, 3.4f, 2);
    b.line(6, 14, 8, 14, 3, 1.f);
    b.line(7, 19, 7, 23, 3, 1.3f);
    return b;
}

gs::Bitmap crossArt() {
    gs::Bitmap b(15, 15);
    b.rect(1, 1, 4, 1, 1);
    b.rect(1, 1, 1, 4, 1);
    b.rect(10, 1, 4, 1, 1);
    b.rect(13, 1, 1, 4, 1);
    b.rect(1, 13, 4, 1, 1);
    b.rect(1, 10, 1, 4, 1);
    b.rect(10, 13, 4, 1, 1);
    b.rect(13, 10, 1, 4, 1);
    b.set(7, 7, 1);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(3, 3);
    b.set(1, 0, 1);
    b.set(0, 1, 1);
    b.set(1, 1, 1);
    b.set(2, 1, 1);
    b.set(1, 2, 1);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(9, 9);
    b.ellipse(4.f, 4.f, 4.1f, 4.1f, 2);
    b.ellipse(4.f, 4.f, 2.5f, 2.5f, 1);
    b.set(3, 3, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(11, 5);
    b.ellipse(5.f, 2.f, 5.f, 2.f, 1);
    return b;
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

void flightPal(gs::VDP& vdp, int pal, uint16_t flight) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, flight);
    vdp.setColor(pal * 16 + 2, gs::rgb4(12, 10, 5));
    vdp.setColor(pal * 16 + 3, gs::rgb4(14, 14, 13));
    vdp.setColor(pal * 16 + 4, gs::rgb4(3, 2, 2));
}

void pipPal(gs::VDP& vdp, int pal, uint16_t bulb, uint16_t cup) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, bulb);
    vdp.setColor(pal * 16 + 2, cup);
    vdp.setColor(pal * 16 + 3, gs::rgb4(15, 15, 13));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_BOARD,
           {0, gs::rgb4(2, 1, 1), gs::rgb4(5, 3, 3), gs::rgb4(13, 11, 8), gs::rgb4(15, 14, 11), gs::rgb4(10, 1, 1),
            gs::rgb4(14, 4, 3), gs::rgb4(0, 6, 2), gs::rgb4(3, 11, 4), gs::rgb4(13, 1, 1), gs::rgb4(1, 8, 3),
            gs::rgb4(13, 13, 11), gs::rgb4(9, 7, 4), gs::rgb4(5, 3, 1), gs::rgb4(8, 5, 2), gs::rgb4(15, 14, 12)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 4), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 7), gs::rgb4(0, 2, 1));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_AIM, gs::rgb4(15, 13, 6), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 5), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 8), gs::rgb4(2, 2, 0));
    textPal(vdp, PAL_LOSE, gs::rgb4(15, 5, 3), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_METER, gs::rgb4(15, 14, 8), gs::rgb4(4, 2, 1));
    flightPal(vdp, PAL_YOU, gs::rgb4(14, 15, 13));
    flightPal(vdp, PAL_HOUSE, gs::rgb4(14, 3, 2));
    pipPal(vdp, PAL_PIPY, gs::rgb4(4, 14, 6), gs::rgb4(8, 6, 2));
    pipPal(vdp, PAL_PIPH, gs::rgb4(15, 10, 2), gs::rgb4(8, 6, 2));
    pipPal(vdp, PAL_PIPE, gs::rgb4(3, 2, 2), gs::rgb4(6, 4, 2));

    loadFont(vdp, art);
    art.board = gs::uploadImage(vdp, boardArt());
    art.rail = gs::uploadImage(vdp, railArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("SEVEN", {3, 1, 2, 0, 1}));
    art.win = gs::uploadImage(vdp, gs::textBitmap("FIRST", {3, 1, 2, 0, 1}));
    art.lose = gs::uploadImage(vdp, gs::textBitmap("HOUSE", {3, 1, 2, 0, 1}));
    art.dart = gs::uploadMipped(vdp, dartArt());
    art.cross = gs::uploadImage(vdp, crossArt());
    art.dot = gs::uploadImage(vdp, dotArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
}

}  // namespace dartseven
