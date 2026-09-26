#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace curlmark {
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

bool speck(int x, int y, unsigned mask) {
    uint32_t h = uint32_t(x) * 2246822519u ^ uint32_t(y) * 3266489917u;
    h ^= h >> 16;
    return (h & mask) == 0;
}

int icePixel(int x, int y) {
    float wx = worldX(x);
    float wy = worldY(y);
    float ax = std::fabs(wx);
    if (ax > kSide + 0.05f) {
        bool edge = ax < kSide + 0.2f;
        bool plank = ((y / 7) & 1) == 0;
        return edge ? 11 : (plank ? 10 : 9);
    }
    if (std::fabs(ax - kSide) <= 0.09f) return 4;
    if (std::fabs(wy - kHog) <= 0.11f) return 5;
    if (std::fabs(wy - kBack) <= 0.1f) return 4;
    float hd = std::hypot(wx, wy - kTee);
    if (hd <= 0.34f) return 6;
    const float rings[3] = {kHouse, kHouse * (2.f / 3.f), kHouse / 3.f};
    for (int i = 0; i < 3; i++)
        if (std::fabs(hd - rings[i]) <= 0.16f) return i == 2 ? 8 : 7;
    if (std::fabs(wy - kTee) <= 0.08f && ax < kHouse) return 4;
    if (ax <= 0.07f && wy >= kReleaseY - 0.2f && wy <= kHog) return 4;
    if (ax <= 0.07f && wy >= kTee + kHouse && wy <= kBack) return 4;
    if (std::fabs(wy - kReleaseY) <= 0.09f && ax > 0.4f && ax < 1.25f) return 14;
    if (std::fabs(ax - 1.15f) <= 0.08f && wy < kReleaseY && wy > kReleaseY - 0.7f) return 14;
    if (hd < kHouse) {
        if (speck(x, y, 15u)) return 2;
        if (speck(x + 3, y + 1, 31u)) return 3;
        return 12;
    }
    if (speck(x, y, 19u)) return 2;
    if (speck(x + 5, y + 2, 47u)) return 13;
    if (speck(x + 1, y + 4, 61u)) return 3;
    return 1;
}

gs::Bitmap rinkArt() {
    gs::Bitmap b(rinkW(), rinkH());
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) b.set(x, y, icePixel(x, y));
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(8.5f, 8.5f, 8.0f, 8.0f, 1);
    b.ellipse(8.5f, 8.5f, 6.7f, 6.7f, 2);
    b.ellipse(6.4f, 6.2f, 2.3f, 1.7f, 3);
    b.ellipse(12.6f, 8.6f, 2.1f, 3.3f, 4);
    b.ellipse(12.2f, 8.6f, 0.8f, 1.6f, 3);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(15, 15);
    b.ellipse(7.f, 7.f, 6.4f, 6.4f, 1);
    b.ellipse(7.f, 7.f, 4.8f, 4.8f, 2);
    b.ellipse(5.4f, 5.2f, 1.6f, 1.2f, 3);
    b.rect(6.4f, 2.2f, 1.3f, 9.6f, 1);
    b.rect(2.2f, 6.4f, 9.6f, 1.3f, 1);
    return b;
}

gs::Bitmap broomArt() {
    gs::Bitmap b(13, 22);
    b.rect(5, 0, 3, 14, 1);
    b.rect(6, 0, 1, 13, 2);
    b.ellipse(6.5f, 16.6f, 5.6f, 4.4f, 3);
    b.rect(2, 15, 9, 2, 4);
    b.line(3, 18, 10, 18, 4, 1.2f);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(7, 7);
    b.ellipse(3.f, 3.f, 3.f, 2.6f, 1);
    b.ellipse(2.4f, 2.4f, 1.2f, 1.f, 2);
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
    b.ellipse(6.5f, 2.6f, 6.2f, 2.3f, 1);
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_ICE,
           {0, gs::rgb4(10, 13, 15), gs::rgb4(13, 15, 15), gs::rgb4(7, 11, 13), gs::rgb4(1, 3, 8), gs::rgb4(12, 2, 3),
            gs::rgb4(13, 1, 2), gs::rgb4(2, 5, 12), gs::rgb4(11, 2, 3), gs::rgb4(4, 2, 1), gs::rgb4(6, 4, 2),
            gs::rgb4(8, 6, 3), gs::rgb4(8, 12, 14), gs::rgb4(12, 14, 15), gs::rgb4(3, 3, 6)});
    stonePal(vdp, PAL_RED, gs::rgb4(6, 1, 1), gs::rgb4(13, 2, 2), gs::rgb4(15, 9, 7), gs::rgb4(14, 13, 11));
    stonePal(vdp, PAL_YEL, gs::rgb4(7, 5, 1), gs::rgb4(14, 11, 2), gs::rgb4(15, 15, 8), gs::rgb4(8, 4, 1));
    stonePal(vdp, PAL_GHOST, gs::rgb4(6, 3, 4), gs::rgb4(12, 7, 8), gs::rgb4(15, 12, 12), gs::rgb4(8, 6, 6));
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(8, 5, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 10)});
    textPal(vdp, PAL_INK, gs::rgb4(14, 15, 15), gs::rgb4(1, 2, 4));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 12, 4), gs::rgb4(3, 2, 1));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 8), gs::rgb4(1, 3, 2));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 14, 6), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_AIM, gs::rgb4(15, 13, 6), gs::rgb4(2, 2, 1));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 10, 12), gs::rgb4(1, 2, 3));
    setPal(vdp, PAL_BROOM,
           {0, gs::rgb4(5, 3, 1), gs::rgb4(10, 7, 3), gs::rgb4(12, 9, 4), gs::rgb4(7, 5, 2)});
    setPal(vdp, PAL_PUFF, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 14, 15)});

    loadFont(vdp, art);
    art.rink = gs::uploadImage(vdp, rinkArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.mark = gs::uploadImage(vdp, markArt());
    art.broom = gs::uploadImage(vdp, broomArt());
    art.puff = gs::uploadImage(vdp, puffArt());
    art.dot = gs::uploadImage(vdp, dotArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
    art.logo = gs::uploadImage(vdp, gs::textBitmap("CURLMARK", {2, 1, 2, 0, 1}));
    art.fin = gs::uploadImage(vdp, gs::textBitmap("FINISHED", {2, 1, 2, 0, 1}));
    art.open = gs::uploadImage(vdp, gs::textBitmap("OPEN", {2, 1, 2, 0, 1}));
}

}  // namespace curlmark
