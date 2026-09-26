#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace curlbell {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 2, 3));
}

void stonePal(gs::VDP& vdp, int pal, uint16_t edge, uint16_t body, uint16_t lite, uint16_t grip) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, edge);
    vdp.setColor(pal * 16 + 2, body);
    vdp.setColor(pal * 16 + 3, lite);
    vdp.setColor(pal * 16 + 4, grip);
}

uint32_t mix(int x, int y) {
    uint32_t h = uint32_t(x) * 2246822519u ^ uint32_t(y) * 3266489917u;
    h ^= h >> 16;
    h *= 0x7feb352du;
    return h ^ (h >> 15);
}

int icePixel(int x, int y) {
    const int left = iceX();
    const int right = left + iceW();
    if (x < left || x >= right) {
        int edge = x < left ? left - x : x - (right - 1);
        bool rail = (y % 52) < 3 || y > gs::SCREEN_H - 6;
        bool post = (x % 18) < 2;
        if (edge <= 2 || rail || post) return 11;
        return ((x / 5) & 1) ? 10 : 9;
    }
    float wx = worldX(x);
    float wy = worldY(y);
    float ax = std::fabs(wx);
    if (ax > kSide + 0.02f) {
        bool plank = ((y / 5) & 1) == 0;
        return ax > kSide + kBoard - 0.12f ? 15 : (plank ? 10 : 9);
    }
    if (std::fabs(ax - kSide) <= 0.09f) return 4;
    if (std::fabs(wy - kHog) <= 0.13f) return 5;
    if (std::fabs(wy - kBack) <= 0.09f) return 4;
    float hd = std::hypot(wx, wy - kTee);
    if (hd <= 0.30f) return 6;
    if (hd <= 0.48f) return 4;
    const float rings[3] = {kHouse, kHouse * (2.f / 3.f), kHouse / 3.f};
    const int ringCol[3] = {7, 8, 7};
    for (int i = 0; i < 3; i++)
        if (std::fabs(hd - rings[i]) <= 0.16f) return ringCol[i];
    if (std::fabs(wy - kTee) <= 0.08f && ax > 0.55f && ax < kHouse) return 4;
    if (ax <= 0.07f && wy >= kHack - 0.15f && wy <= kTee - kHouse) return 4;
    if (std::fabs(wy - kHack) <= 0.22f && std::fabs(ax - 0.95f) <= 0.28f) return 14;
    uint32_t h = mix(x, y);
    if (hd < kHouse / 3.f) return (h & 31u) == 0u ? 2 : 13;
    if (hd < kHouse) return (h & 29u) == 0u ? 2 : 12;
    if ((h & 23u) == 0u) return 2;
    if ((h & 41u) == 0u) return 3;
    return 1;
}

gs::Bitmap sheetArt() {
    gs::Bitmap b(gs::SCREEN_W, gs::SCREEN_H);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) b.set(x, y, icePixel(x, y));
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(8.6f, 8.4f, 7.4f, 7.2f, 1);
    b.ellipse(8.6f, 8.2f, 6.0f, 5.8f, 2);
    b.ellipse(6.2f, 6.2f, 2.1f, 1.5f, 3);
    b.rect(12.4f, 6.5f, 4.2f, 3.4f, 4);
    b.rect(13.2f, 7.2f, 2.4f, 2.0f, 3);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(20, 24);
    b.rect(9, 0, 2, 4, 4);
    b.rect(3, 2, 14, 2, 4);
    b.ellipse(10.f, 8.f, 4.6f, 3.1f, 1);
    b.ellipse(10.f, 8.f, 3.1f, 2.0f, 2);
    b.ellipse(10.f, 14.2f, 8.2f, 6.6f, 1);
    b.ellipse(10.f, 13.6f, 6.3f, 5.0f, 2);
    b.ellipse(7.4f, 12.2f, 1.8f, 2.2f, 3);
    b.rect(2, 18, 16, 3, 1);
    b.rect(3, 18, 14, 1, 3);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(5, 9);
    b.rect(2, 0, 1, 4, 1);
    b.ellipse(2.5f, 6.2f, 2.1f, 2.2f, 1);
    return b;
}

gs::Bitmap skipArt() {
    gs::Bitmap b(16, 22);
    b.ellipse(8.f, 4.2f, 3.3f, 3.3f, 5);
    b.ellipse(8.f, 4.6f, 2.3f, 2.1f, 3);
    b.rect(5, 8, 6, 7, 2);
    b.rect(3, 9, 2, 5, 2);
    b.rect(11, 9, 2, 5, 1);
    b.rect(5, 15, 2, 5, 4);
    b.rect(9, 15, 2, 5, 4);
    b.rect(4, 20, 3, 2, 6);
    b.rect(9, 20, 3, 2, 6);
    return b;
}

gs::Bitmap broomArt() {
    gs::Bitmap b(12, 22);
    b.rect(5, 0, 2, 14, 1);
    b.rect(6, 0, 1, 13, 2);
    b.ellipse(6.f, 16.8f, 5.2f, 4.2f, 3);
    b.rect(2, 15, 8, 2, 4);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(7, 7);
    b.ellipse(3.f, 3.f, 3.f, 2.5f, 1);
    b.ellipse(2.3f, 2.4f, 1.2f, 1.f, 2);
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

gs::Bitmap shadowArt() {
    gs::Bitmap b(14, 6);
    b.ellipse(6.5f, 2.6f, 6.2f, 2.2f, 1);
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
    setPal(vdp, PAL_ICE,
           {gs::rgb4(1, 1, 2), gs::rgb4(8, 12, 14), gs::rgb4(12, 15, 15), gs::rgb4(5, 9, 12), gs::rgb4(1, 2, 5),
            gs::rgb4(13, 2, 2), gs::rgb4(14, 11, 3), gs::rgb4(2, 4, 12), gs::rgb4(12, 2, 3), gs::rgb4(3, 2, 1),
            gs::rgb4(5, 3, 2), gs::rgb4(8, 6, 4), gs::rgb4(11, 14, 15), gs::rgb4(14, 13, 12), gs::rgb4(2, 2, 3),
            gs::rgb4(7, 5, 3)});
    stonePal(vdp, PAL_RED, gs::rgb4(6, 1, 1), gs::rgb4(13, 2, 2), gs::rgb4(15, 8, 6), gs::rgb4(14, 13, 10));
    stonePal(vdp, PAL_YEL, gs::rgb4(6, 4, 1), gs::rgb4(14, 11, 2), gs::rgb4(15, 15, 8), gs::rgb4(5, 3, 1));
    stonePal(vdp, PAL_GHOST, gs::rgb4(4, 6, 8), gs::rgb4(8, 11, 13), gs::rgb4(13, 15, 15), gs::rgb4(6, 8, 9));
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(6, 4, 1), gs::rgb4(13, 10, 3), gs::rgb4(15, 14, 8), gs::rgb4(3, 3, 4)});
    textPal(vdp, PAL_INK, gs::rgb4(14, 15, 15), gs::rgb4(1, 2, 4));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 12, 4), gs::rgb4(3, 2, 1));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 8), gs::rgb4(1, 3, 2));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 14, 6), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_AIM, gs::rgb4(15, 13, 6), gs::rgb4(2, 2, 1));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 10, 12), gs::rgb4(1, 2, 3));
    setPal(vdp, PAL_BROOM, {0, gs::rgb4(5, 3, 1), gs::rgb4(10, 7, 3), gs::rgb4(12, 10, 4), gs::rgb4(7, 5, 2)});
    setPal(vdp, PAL_PUFF, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 14, 15)});
    setPal(vdp, PAL_SKIP,
           {0, gs::rgb4(2, 1, 1), gs::rgb4(12, 2, 2), gs::rgb4(14, 10, 7), gs::rgb4(2, 3, 6), gs::rgb4(3, 2, 1),
            gs::rgb4(1, 1, 1)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::bitmapToPlane(tiles, vdp.A, 0, 0, sheetArt(), PAL_ICE);
    vdp.A.scroll(0, 0);
    vdp.A.enabled = true;
    vdp.B.enabled = false;

    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.bell = gs::uploadImage(vdp, bellArt());
    art.clapper = gs::uploadImage(vdp, clapperArt());
    art.skip = gs::uploadImage(vdp, skipArt());
    art.broom = gs::uploadImage(vdp, broomArt());
    art.puff = gs::uploadImage(vdp, puffArt());
    art.dot = gs::uploadImage(vdp, dotArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
}

}  // namespace curlbell
