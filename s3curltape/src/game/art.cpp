#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace curltape {
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
    uint32_t h = uint32_t(x) * 0x9E3779B1u ^ uint32_t(y) * 0x85EBCA6Bu;
    h ^= h >> 16;
    h *= 0xC2B2AE35u;
    return h ^ (h >> 15);
}

int sheetPixel(int x, int y) {
    if (y < kViewTop || y >= kViewTop + kViewH) return ((x / 10) & 1) ? 11 : 10;
    if (x < kIceX || x >= kIceX + kIceW) {
        bool paper = x >= kIceX + kIceW + 4;
        if (paper) {
            bool rule = (y % 19) == 8 || y == kViewTop + 8 || y == kViewTop + kViewH - 10;
            return rule ? 14 : 13;
        }
        for (int i = 0; i < 3; i++) {
            float sx = slotX(i);
            float sy = slotY(i);
            if (std::fabs(float(x) - sx) <= 16.f && std::fabs(float(y) - sy) <= 8.f) return 10;
            if (std::fabs(float(x) - sx) <= 18.f && std::fabs(float(y) - sy) <= 10.f) return 12;
        }
        bool stud = (x % 22) < 2 && (y % 18) < 2;
        if (stud) return 15;
        return ((y / 7) & 1) ? 11 : 10;
    }

    float wx = worldX(x);
    float wy = worldY(y);
    float ax = std::fabs(wx);
    if (ax > kSide) return ((int(wy * 4.f) & 1) == 0) ? 12 : 11;

    float d = std::hypot(wx, wy - kTee);
    if (d <= kButton * 0.72f) return 7;
    if (std::fabs(wy - kHog) <= 0.07f) return 5;
    if (std::fabs(wy - kBack) <= 0.05f || std::fabs(ax - kSide) <= 0.05f) return 8;
    if (std::fabs(wy - kTee) <= 0.045f && ax < kHouse + 0.08f && d > kButton) return 8;
    if (ax <= 0.035f && d > kHouse && wy > kHog && wy < kBack) return 8;
    bool lane = std::fabs(ax - kGuardLane) < 0.03f && wy > kHog + 0.1f && wy < kTee - kHouse &&
                ((int(wy * 7.f) & 1) == 0);
    if (lane) return 9;

    if (d <= kButton) return 7;
    if (d <= kFour) return 6;
    if (d <= kEight) return 4;
    if (d <= kHouse) return 5;
    if (wy < kTee && d > kHouse && d <= kHouse + kBiteFrac * kStoneR) return 3;
    if (std::fabs(wy - kHack) < 0.16f && std::fabs(ax - 0.82f) < 0.20f) return 15;

    uint32_t h = mix(x, y);
    if ((h & 31u) == 0u) return 2;
    if ((h & 47u) == 0u) return 3;
    return wy > (kTee + kHog) * 0.5f ? 1 : 2;
}

gs::Bitmap sheetArt() {
    gs::Bitmap b(gs::SCREEN_W, gs::SCREEN_H);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) b.set(x, y, sheetPixel(x, y));
    return b;
}

gs::Bitmap rockArt() {
    gs::Bitmap b(20, 20);
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 20; x++) {
            float dx = x - 9.2f;
            float dy = y - 9.4f;
            float r = std::sqrt(dx * dx + dy * dy);
            if (r > 8.4f) continue;
            int c = 2;
            if (r > 7.1f) c = 1;
            else if (r > 5.5f && x > 11) c = 4;
            else if (dx < -1.6f && dy < -1.8f && r < 5.2f) c = 3;
            b.set(x, y, c);
        }
    }
    return b;
}

gs::Bitmap skipArt(bool sweep) {
    gs::Bitmap b(sweep ? 26 : 18, sweep ? 20 : 32);
    if (!sweep) {
        b.ellipse(9.f, 4.2f, 5.4f, 2.6f, 5);
        b.rect(5, 5, 8, 2, 5);
        b.ellipse(9.f, 8.6f, 3.0f, 2.8f, 3);
        b.rect(2, 8, 3, 2, 3);
        b.rect(5, 12, 8, 9, 2);
        b.rect(2, 13, 3, 6, 2);
        b.rect(13, 13, 3, 5, 2);
        b.rect(5, 21, 3, 7, 4);
        b.rect(10, 21, 3, 6, 4);
        b.rect(4, 27, 5, 3, 6);
        b.rect(10, 26, 4, 3, 1);
        b.rect(7, 14, 4, 3, 7);
        return b;
    }
    b.ellipse(18.f, 4.4f, 4.8f, 2.6f, 5);
    b.ellipse(18.f, 8.0f, 2.6f, 2.4f, 3);
    b.rect(10, 7, 8, 6, 2);
    b.rect(4, 9, 7, 3, 2);
    b.rect(8, 13, 9, 3, 4);
    b.rect(6, 16, 6, 2, 6);
    b.rect(14, 16, 5, 2, 1);
    return b;
}

gs::Bitmap broomArt() {
    gs::Bitmap b(16, 26);
    b.rect(7, 0, 2, 16, 1);
    b.rect(8, 1, 1, 14, 2);
    b.ellipse(8.f, 19.5f, 6.4f, 4.6f, 3);
    b.rect(3, 16, 10, 2, 4);
    b.rect(4, 21, 8, 1, 1);
    return b;
}

gs::Bitmap slipArt() {
    gs::Bitmap b(22, 12);
    b.rect(0, 0, 22, 12, 1);
    b.rect(0, 0, 22, 2, 2);
    b.rect(2, 5, 12, 1, 3);
    b.rect(2, 8, 8, 1, 3);
    b.rect(18, 4, 2, 6, 4);
    return b;
}

gs::Bitmap arrowArt() {
    gs::Bitmap b(14, 8);
    b.rect(1, 3, 8, 2, 1);
    b.rect(8, 1, 2, 6, 1);
    b.rect(10, 2, 2, 4, 1);
    b.rect(12, 3, 1, 2, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(7.5f, 2.6f, 7.0f, 2.1f, 1);
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

gs::Bitmap puffArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(3.5f, 3.6f, 3.2f, 2.6f, 1);
    b.ellipse(2.4f, 2.6f, 1.3f, 1.0f, 2);
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
           {gs::rgb4(1, 1, 2), gs::rgb4(11, 14, 15), gs::rgb4(13, 15, 15), gs::rgb4(8, 12, 13), gs::rgb4(2, 5, 12),
            gs::rgb4(12, 2, 3), gs::rgb4(14, 15, 15), gs::rgb4(14, 12, 3), gs::rgb4(1, 2, 6), gs::rgb4(3, 5, 8),
            gs::rgb4(4, 2, 1), gs::rgb4(7, 4, 2), gs::rgb4(10, 7, 4), gs::rgb4(14, 13, 10), gs::rgb4(6, 5, 3),
            gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(4, 1, 1), gs::rgb4(12, 3, 3), gs::rgb4(15, 12, 10), gs::rgb4(14, 11, 4)});
    setPal(vdp, PAL_GHOST, {0, gs::rgb4(2, 4, 6), gs::rgb4(7, 10, 12), gs::rgb4(12, 15, 15), gs::rgb4(8, 9, 6)});
    setPal(vdp, PAL_SKIP,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(12, 2, 2), gs::rgb4(14, 10, 7), gs::rgb4(2, 3, 7), gs::rgb4(6, 6, 7),
            gs::rgb4(1, 1, 1), gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_BROOM, {0, gs::rgb4(5, 3, 1), gs::rgb4(12, 9, 5), gs::rgb4(13, 11, 6), gs::rgb4(8, 6, 2)});
    setPal(vdp, PAL_SLIP, {0, gs::rgb4(14, 13, 9), gs::rgb4(6, 10, 5), gs::rgb4(5, 4, 3), gs::rgb4(12, 8, 3)});
    textPal(vdp, PAL_INK, gs::rgb4(14, 15, 15), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 4), gs::rgb4(3, 2, 1));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 8), gs::rgb4(1, 2, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 10, 12), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 14, 8), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_AIM, gs::rgb4(10, 14, 15), gs::rgb4(1, 2, 3));
    setPal(vdp, PAL_PUFF, {0, gs::rgb4(14, 15, 15), gs::rgb4(10, 13, 14)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::bitmapToPlane(tiles, vdp.A, 0, 0, sheetArt(), PAL_ICE);
    vdp.A.scroll(0, 0);
    vdp.A.enabled = true;
    vdp.B.enabled = false;

    art.rock = gs::uploadMipped(vdp, rockArt());
    art.skip[0] = gs::uploadMipped(vdp, skipArt(false));
    art.skip[1] = gs::uploadMipped(vdp, skipArt(true));
    art.broom = gs::uploadImage(vdp, broomArt());
    art.slip = gs::uploadImage(vdp, slipArt());
    art.arrow = gs::uploadImage(vdp, arrowArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
    art.dot = gs::uploadImage(vdp, dotArt());
    art.puff = gs::uploadImage(vdp, puffArt());
}

}  // namespace curltape
