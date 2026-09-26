#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace curlchime {
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
    vdp.setColor(pal * 16 + 15, edge);
}

uint32_t mix(int x, int y) {
    uint32_t h = uint32_t(x) * 2246822519u ^ uint32_t(y) * 3266489917u;
    h ^= h >> 16;
    h *= 0x7feb352du;
    return h ^ (h >> 15);
}

int scorePixel(int x, int y) {
    if (y == kViewTop - 1 || y == kViewTop + kViewH) return 9;
    bool plank = ((x / 12) & 1) == 0;
    return plank ? 14 : 13;
}

int marginPixel(int x, int y) {
    float dx = float(x) - 278.f;
    float dy = float(y) - 74.f;
    if (dx * dx + dy * dy < 30.f * 30.f) return 10;
    bool plank = ((y / 8) & 1) == 0;
    bool stud = (x % 18) < 2 && (y % 14) < 2;
    if (stud) return 12;
    return plank ? 13 : 14;
}

int sheetPixel(int x, int y) {
    float wx = worldX(x);
    float wy = worldY(y);
    float ax = std::fabs(wx);
    if (ax > kSide) return ((int(wy * 3.f) & 1) == 0) ? 12 : 11;

    float hd = std::hypot(wx, wy - kTee);
    float four = kHouse / 3.f;
    float eight = kHouse * 2.f / 3.f;
    if (std::fabs(wy - kHog) <= 0.09f) return 6;
    if (std::fabs(wy - kBack) <= 0.07f || std::fabs(ax - kSide) <= 0.07f) return 5;
    if (std::fabs(wy - kTee) <= 0.055f && ax > 0.40f && ax < kHouse) return 5;
    if (ax <= 0.05f && wy < kTee - 0.36f && wy > kHog) return 5;
    if (hd > 0.40f && (std::fabs(hd - four) <= 0.06f || std::fabs(hd - eight) <= 0.06f || std::fabs(hd - kHouse) <= 0.07f))
        return 5;
    if (hd <= 0.09f) return 3;
    if (hd <= 0.22f) return 9;
    if (hd <= four) return 2;
    if (hd <= eight) return 8;
    if (hd <= kHouse) return 7;
    if (std::fabs(wy - kHack) <= 0.26f && std::fabs(ax - 0.92f) <= 0.28f) return 15;

    uint32_t h = mix(x, y);
    if ((h & 29u) == 0u) return 3;
    if ((h & 43u) == 0u) return 4;
    return (wy > (kTee + kHog) * 0.5f) ? 1 : 2;
}

int icePixel(int x, int y) {
    if (y < kViewTop || y >= kViewTop + kViewH) return scorePixel(x, y);
    if (x < kIceX || x >= kIceX + kIceW) return marginPixel(x, y);
    return sheetPixel(x, y);
}

gs::Bitmap sheetArt() {
    gs::Bitmap b(gs::SCREEN_W, gs::SCREEN_H);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) b.set(x, y, icePixel(x, y));
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(8.8f, 9.0f, 7.6f, 7.0f, 1);
    b.ellipse(8.6f, 8.6f, 6.0f, 5.5f, 2);
    b.ellipse(6.4f, 6.8f, 2.0f, 1.3f, 3);
    b.set(11, 8, 1);
    b.set(7, 11, 1);
    b.rect(12.2f, 6.6f, 4.4f, 3.2f, 4);
    b.rect(13.0f, 7.3f, 2.2f, 1.6f, 3);
    return b;
}

gs::Bitmap skipArt(bool slide) {
    gs::Bitmap b(slide ? 24 : 18, slide ? 20 : 28);
    if (!slide) {
        b.ellipse(9.f, 4.6f, 5.2f, 3.4f, 5);
        b.rect(4, 5, 10, 2, 5);
        b.ellipse(9.f, 8.6f, 3.1f, 3.0f, 3);
        b.rect(5, 12, 8, 8, 2);
        b.rect(2, 13, 3, 6, 2);
        b.rect(13, 13, 3, 6, 2);
        b.rect(5, 20, 3, 5, 4);
        b.rect(10, 20, 3, 5, 4);
        b.rect(4, 25, 4, 2, 6);
        b.rect(10, 25, 4, 2, 6);
        return b;
    }
    b.ellipse(15.f, 5.2f, 4.6f, 3.2f, 5);
    b.ellipse(15.f, 8.4f, 2.8f, 2.6f, 3);
    b.rect(8, 8, 8, 6, 2);
    b.rect(4, 10, 5, 3, 2);
    b.rect(6, 14, 8, 3, 4);
    b.rect(2, 15, 6, 2, 4);
    b.rect(1, 16, 4, 2, 6);
    b.rect(13, 16, 4, 2, 6);
    return b;
}

gs::Bitmap broomArt() {
    gs::Bitmap b(14, 24);
    b.rect(6, 0, 2, 15, 1);
    b.rect(7, 0, 1, 14, 2);
    b.ellipse(7.f, 18.2f, 6.0f, 4.4f, 3);
    b.rect(2, 16, 10, 2, 4);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(7, 7);
    b.ellipse(3.f, 3.f, 3.f, 2.4f, 1);
    b.ellipse(2.2f, 2.3f, 1.2f, 0.9f, 2);
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
    b.ellipse(6.5f, 2.6f, 6.2f, 2.1f, 1);
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(kDial, kDial);
    float c = (kDial - 1) * 0.5f;
    for (int y = 0; y < kDial; y++) {
        for (int x = 0; x < kDial; x++) {
            float dx = x + 0.5f - c;
            float dy = y + 0.5f - c;
            float r = std::sqrt(dx * dx + dy * dy);
            if (r > 21.5f) continue;
            if (r > 18.6f) {
                b.set(x, y, 1);
                continue;
            }
            b.set(x, y, r < 3.2f ? 5 : 2);
            float ang = std::atan2(dx, -dy);
            if (ang < 0) ang += 6.2831853f;
            float tick = std::fmod(ang + 0.11f, 0.5235988f);
            bool hour = tick < 0.09f || tick > 0.5235988f - 0.09f;
            if (hour && r > 14.2f && r < 18.2f) b.set(x, y, (r > 16.4f && tick < 0.16f && ang < 0.35f) ? 6 : 4);
        }
    }
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(kDial + 8, kDial + 8);
    float c = (b.w - 1) * 0.5f;
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            float dx = x + 0.5f - c;
            float dy = y + 0.5f - c;
            float r = std::sqrt(dx * dx + dy * dy);
            if (r > 23.5f && r < 26.2f) b.set(x, y, (int(std::atan2(dy, dx) * 8.f) & 1) ? 1 : 2);
        }
    }
    return b;
}

gs::Bitmap capArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(3.5f, 3.5f, 3.0f, 3.0f, 1);
    b.ellipse(3.5f, 3.5f, 1.4f, 1.4f, 2);
    return b;
}

void paintHand(gs::Bitmap& b, int kind, int step) {
    float theta = float(step) * 0.10471976f;
    float dx = std::sin(theta);
    float dy = -std::cos(theta);
    float len = kind == 0 ? 11.f : kind == 1 ? 16.5f : 19.f;
    float tail = kind == 2 ? 0.f : 3.2f;
    float thick = kind == 2 ? 0.55f : 1.25f;
    int col = kind == 2 ? 3 : kind == 1 ? 2 : 1;
    float c = (kDial - 1) * 0.5f;
    for (int y = 0; y < kDial; y++) {
        for (int x = 0; x < kDial; x++) {
            float px = x + 0.5f - c;
            float py = y + 0.5f - c;
            float along = px * dx + py * dy;
            float side = std::fabs(px * -dy + py * dx);
            if (along > -tail && along < len && side <= thick) b.set(x, y, col);
        }
    }
}

gs::Bitmap bellArt() {
    gs::Bitmap b(16, 20);
    b.rect(7, 0, 2, 3, 4);
    b.ellipse(8.f, 6.f, 3.2f, 2.4f, 1);
    b.ellipse(8.f, 11.5f, 6.4f, 5.2f, 1);
    b.ellipse(8.f, 11.0f, 4.6f, 3.6f, 2);
    b.ellipse(6.2f, 10.2f, 1.4f, 1.6f, 3);
    b.rect(2, 15, 12, 3, 1);
    b.rect(3, 15, 10, 1, 3);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(5, 8);
    b.rect(2, 0, 1, 4, 1);
    b.ellipse(2.5f, 5.6f, 1.8f, 1.8f, 1);
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
           {gs::rgb4(1, 1, 2), gs::rgb4(10, 13, 14), gs::rgb4(13, 15, 15), gs::rgb4(15, 15, 15), gs::rgb4(6, 9, 11),
            gs::rgb4(1, 2, 6), gs::rgb4(12, 2, 2), gs::rgb4(2, 4, 11), gs::rgb4(11, 2, 3), gs::rgb4(14, 11, 3),
            gs::rgb4(2, 2, 4), gs::rgb4(5, 3, 2), gs::rgb4(8, 5, 3), gs::rgb4(2, 1, 1), gs::rgb4(4, 2, 2),
            gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_RED,
           {0, gs::rgb4(4, 1, 1), gs::rgb4(12, 3, 3), gs::rgb4(15, 10, 8), gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_YEL,
           {0, gs::rgb4(6, 4, 1), gs::rgb4(14, 11, 3), gs::rgb4(15, 15, 9), gs::rgb4(5, 3, 1)});
    setPal(vdp, PAL_GHOST,
           {0, gs::rgb4(3, 5, 7), gs::rgb4(8, 11, 13), gs::rgb4(13, 15, 15), gs::rgb4(5, 7, 8)});
    textPal(vdp, PAL_INK, gs::rgb4(14, 15, 15), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 4), gs::rgb4(3, 2, 1));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 8), gs::rgb4(1, 2, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 10, 12), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 14, 8), gs::rgb4(3, 2, 0));
    setPal(vdp, PAL_BROOM, {0, gs::rgb4(5, 3, 1), gs::rgb4(11, 8, 4), gs::rgb4(13, 11, 6), gs::rgb4(7, 5, 2)});
    setPal(vdp, PAL_SKIP,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(12, 2, 2), gs::rgb4(14, 10, 7), gs::rgb4(2, 3, 6), gs::rgb4(8, 2, 2),
            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FACE,
           {0, gs::rgb4(5, 3, 2), gs::rgb4(12, 9, 6), gs::rgb4(8, 5, 2), gs::rgb4(3, 2, 2), gs::rgb4(10, 8, 6),
            gs::rgb4(15, 13, 6), gs::rgb4(14, 10, 3)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(4, 3, 2), gs::rgb4(14, 12, 8), gs::rgb4(15, 3, 2), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(8, 6, 2), gs::rgb4(13, 10, 4), gs::rgb4(15, 14, 8), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_PUFF, {0, gs::rgb4(14, 15, 15), gs::rgb4(10, 13, 14)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::bitmapToPlane(tiles, vdp.A, 0, 0, sheetArt(), PAL_ICE);
    vdp.A.scroll(0, 0);
    vdp.A.enabled = true;
    vdp.B.enabled = false;

    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.skip[0] = gs::uploadMipped(vdp, skipArt(false));
    art.skip[1] = gs::uploadMipped(vdp, skipArt(true));
    art.broom = gs::uploadImage(vdp, broomArt());
    art.puff = gs::uploadImage(vdp, puffArt());
    art.dot = gs::uploadImage(vdp, dotArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
    art.face = gs::uploadImage(vdp, faceArt());
    art.ring = gs::uploadImage(vdp, ringArt());
    art.cap = gs::uploadImage(vdp, capArt());
    for (int kind = 0; kind < 3; kind++) {
        for (int step = 0; step < 60; step++) {
            gs::Bitmap b(kDial, kDial);
            paintHand(b, kind, step);
            art.hand[kind][step] = gs::uploadImage(vdp, b);
        }
    }
    art.bell = gs::uploadImage(vdp, bellArt());
    art.clapper = gs::uploadImage(vdp, clapperArt());
}

}  // namespace curlchime
