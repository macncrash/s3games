#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace curlseven {
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

void stonePal(gs::VDP& vdp, int pal, uint16_t edge, uint16_t body, uint16_t lite, uint16_t grip) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, edge);
    vdp.setColor(pal * 16 + 2, body);
    vdp.setColor(pal * 16 + 3, lite);
    vdp.setColor(pal * 16 + 4, grip);
}

uint32_t mix(int x, int y) {
    uint32_t h = uint32_t(x) * 0x9e3779b1u ^ uint32_t(y) * 0x85ebca6bu;
    h ^= h >> 16;
    h *= 0xc2b2ae35u;
    return h ^ (h >> 13);
}

int icePixel(int x, int y) {
    float wx = worldX(x);
    float wy = worldY(y);
    float ax = std::fabs(wx);
    if (ax > kSide + 0.1f || wy > kBack + 0.14f) {
        bool edge = ax < kSide + 0.28f || wy < kBack + 0.32f;
        bool plank = ((y / 6) & 1) == 0;
        return edge ? 11 : (plank ? 10 : 9);
    }
    if (std::fabs(ax - kSide) <= 0.09f) return 4;
    if (std::fabs(wy - kHog) <= 0.11f) return 5;
    if (std::fabs(wy - kBack) <= 0.09f) return 4;
    if (wy > kRelease - 0.55f && wy < kRelease + 0.12f && ax > 0.82f && ax < 1.38f) return 14;

    float hd = buttonDist(wx, wy);
    float r4 = kHouse / 3.f;
    float r8 = kHouse * (2.f / 3.f);
    if (hd <= 0.32f) return 6;
    if (std::fabs(hd - r4) <= 0.11f) return 7;
    if (std::fabs(hd - r8) <= 0.10f) return 15;
    if (std::fabs(hd - kHouse) <= 0.12f) return 8;
    if (std::fabs(wy - kTee) <= 0.07f && ax < kHouse && hd > 0.45f) return 15;
    if (ax <= 0.065f && wy > kHog && wy < kBack && hd > kHouse + 0.05f) return 4;
    if (ax <= 0.065f && wy > kRelease - 0.2f && wy < kHog) return 4;

    int base = 1;
    if (hd < r4) base = 12;
    else if (hd < r8) base = 13;
    uint32_t h = mix(x, y);
    if ((h & 31u) == 0) return base == 1 ? 2 : 3;
    if ((h & 53u) == 0) return 3;
    return base;
}

gs::Bitmap rinkArt() {
    gs::Bitmap b(kRinkW, gs::SCREEN_H);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) b.set(x, y, icePixel(x, y));
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(9.5f, 9.5f, 9.0f, 9.0f, 1);
    b.ellipse(9.5f, 9.5f, 7.4f, 7.4f, 2);
    b.ellipse(6.6f, 6.4f, 2.6f, 1.8f, 3);
    b.ellipse(13.4f, 9.6f, 2.3f, 3.4f, 4);
    b.ellipse(12.8f, 9.6f, 0.9f, 1.5f, 3);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(9, 9);
    b.ellipse(4.f, 4.f, 4.f, 4.f, 1);
    b.ellipse(4.f, 4.f, 2.7f, 2.7f, 2);
    b.ellipse(3.1f, 3.f, 1.1f, 0.8f, 3);
    return b;
}

gs::Bitmap broomArt() {
    gs::Bitmap b(14, 24);
    b.rect(6, 0, 2, 15, 1);
    b.rect(7, 0, 1, 14, 2);
    b.ellipse(7.f, 18.f, 6.f, 4.2f, 3);
    b.rect(2, 16, 10, 2, 4);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(3.5f, 3.5f, 3.4f, 2.8f, 1);
    b.ellipse(2.6f, 2.8f, 1.3f, 1.f, 2);
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
    gs::Bitmap b(16, 7);
    b.ellipse(7.5f, 3.f, 7.f, 2.6f, 1);
    return b;
}

gs::Bitmap skipArt() {
    gs::Bitmap b(24, 40);
    b.rect(16, 3, 2, 24, 4);
    b.ellipse(17.f, 30.f, 6.f, 4.f, 5);
    b.rect(2, 28, 10, 2, 5);
    b.rect(8, 24, 3, 11, 3);
    b.rect(13, 24, 3, 11, 3);
    b.rect(7, 34, 5, 3, 6);
    b.rect(13, 34, 5, 3, 6);
    b.rect(7, 13, 10, 13, 2);
    b.rect(8, 14, 3, 8, 7);
    b.rect(15, 15, 6, 3, 2);
    b.ellipse(12.f, 8.f, 5.f, 5.f, 1);
    b.ellipse(10.2f, 7.2f, 1.8f, 1.1f, 7);
    b.rect(7, 3, 10, 3, 2);
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
        a.font[c - 32] = tiles.shared(px);
    }
}

gs::Image words(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.outline = 2;
    st.spacing = 1;
    return gs::uploadImage(vdp, gs::textBitmap(s, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_ICE,
           {0, gs::rgb4(10, 14, 15), gs::rgb4(13, 15, 15), gs::rgb4(6, 10, 12), gs::rgb4(1, 3, 9), gs::rgb4(13, 2, 2),
            gs::rgb4(14, 1, 2), gs::rgb4(2, 6, 14), gs::rgb4(12, 2, 3), gs::rgb4(4, 2, 1), gs::rgb4(7, 4, 2),
            gs::rgb4(10, 7, 4), gs::rgb4(8, 12, 14), gs::rgb4(13, 14, 15), gs::rgb4(14, 11, 3), gs::rgb4(15, 15, 15)});
    stonePal(vdp, PAL_RED, gs::rgb4(6, 1, 1), gs::rgb4(13, 2, 2), gs::rgb4(15, 10, 8), gs::rgb4(14, 13, 10));
    stonePal(vdp, PAL_YEL, gs::rgb4(7, 5, 1), gs::rgb4(14, 11, 2), gs::rgb4(15, 15, 8), gs::rgb4(8, 4, 1));
    stonePal(vdp, PAL_GHOST, gs::rgb4(4, 5, 7), gs::rgb4(9, 11, 13), gs::rgb4(14, 15, 15), gs::rgb4(6, 7, 9));
    setPal(vdp, PAL_DIM, {0, gs::rgb4(4, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(6, 7, 8)});
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(4, 3, 1));
    textPal(vdp, PAL_INK, gs::rgb4(14, 15, 15), gs::rgb4(1, 2, 4));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 5), gs::rgb4(4, 2, 1));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 8), gs::rgb4(1, 3, 2));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 14, 6), gs::rgb4(4, 3, 0));
    setPal(vdp, PAL_SKIP,
           {0, gs::rgb4(14, 10, 7), gs::rgb4(13, 2, 2), gs::rgb4(2, 3, 6), gs::rgb4(10, 7, 3), gs::rgb4(6, 8, 3),
            gs::rgb4(2, 2, 2), gs::rgb4(15, 13, 10)});
    setPal(vdp, PAL_BROOM, {0, gs::rgb4(5, 3, 1), gs::rgb4(11, 8, 3), gs::rgb4(8, 10, 4), gs::rgb4(4, 6, 2)});
    setPal(vdp, PAL_PUFF, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15)});
    textPal(vdp, PAL_AIM, gs::rgb4(15, 14, 6), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_WHITE, gs::rgb4(15, 15, 15), gs::rgb4(2, 3, 5));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::bitmapToPlane(tiles, vdp.A, kRinkCol, 0, rinkArt(), PAL_ICE);

    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    art.broom = gs::uploadImage(vdp, broomArt());
    art.puff = gs::uploadImage(vdp, puffArt());
    art.dot = gs::uploadImage(vdp, dotArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
    art.skip = gs::uploadImage(vdp, skipArt());
    art.curl = words(vdp, "CURL", 2);
    art.seven = words(vdp, "SEVEN", 2);
    art.big = words(vdp, "7", 4);
    art.shorty = words(vdp, "SHORT", 2);
}

}  // namespace curlseven
