#include "game/art.h"

#include <cstdint>
#include <initializer_list>

#include "game/ice.h"

namespace curl {
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
    vdp.setColor(pal * 16 + 15, edge);
}

int rinkPixel(int x, int y) {
    float wx = (x + 0.5f) / kPx - kHalfW;
    float wy = kImgTop - (y + 0.5f) / kPx;
    float ax = std::fabs(wx);
    if (ax >= kHalfW - 0.42f) return ax >= kHalfW - 0.12f ? 9 : 8;

    uint32_t h = uint32_t(x) * 374761393u ^ uint32_t(y) * 668265263u;
    h ^= h >> 13;
    int ice = 1;
    if ((h & 63u) == 0) ice = 11;
    else if ((h & 31u) == 0) ice = 2;

    float dy = wy - kTee;
    float r = std::hypot(wx, dy);
    int c = ice;
    if (r <= kHouse) {
        if (r <= 0.28f) c = 15;
        else if (r <= 0.55f) c = 5;
        else if (r <= 2.f) c = (h & 7u) == 0 ? 12 : 4;
        else if (r <= 4.f) c = 5;
        else c = (h & 7u) == 0 ? 13 : 3;
    }
    auto stroke = [&](float rad) { return std::fabs(r - rad) <= 0.13f; };
    if (stroke(kHouse) || stroke(4.f) || stroke(2.f) || stroke(0.5f)) c = 15;

    if (std::fabs(wy - kHogLine) <= 0.11f && ax < kSideLine) c = 4;
    if (std::fabs(dy) <= 0.08f && ax > 0.75f && ax < kSideLine) c = 15;
    if (std::fabs(wy - kBackLine) <= 0.1f && ax < kSideLine) c = 15;
    if (ax <= 0.055f && r > kHouse + 0.2f && wy > kImgBot + 1.5f && wy < kBackLine) c = 6;
    if (std::fabs(ax - kSideLine) <= 0.08f) c = 15;

    // The hack, a pair of blocks the skip pushes from.
    if (wy > kRelease - 2.4f && wy < kRelease - 0.85f && ax > 0.28f && ax < 1.25f) c = 10;
    return c;
}

gs::Bitmap rinkArt() {
    const int w = rinkPixelW();
    const int h = rinkPixelH();
    gs::Bitmap b(w, h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) b.set(x, y, rinkPixel(x, y));
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(21, 21);
    const float cx = 10.f;
    for (int y = 0; y < 21; y++) {
        for (int x = 0; x < 21; x++) {
            float d = std::hypot(float(x) - cx, float(y) - cx);
            if (d > 9.4f) continue;
            int c = 3;
            if (d > 8.5f) c = 1;
            else if (d > 6.4f) c = 2;
            else if (d < 2.3f) c = 5;
            b.set(x, y, c);
        }
    }
    b.rect(4, 9, 13, 2, 5);
    b.rect(9, 6, 3, 8, 5);
    b.set(7, 6, 4);
    b.set(6, 5, 4);
    return b;
}

gs::Bitmap broomArt() {
    gs::Bitmap b(11, 24);
    b.rect(5, 0, 2, 16, 1);
    b.rect(2, 15, 7, 6, 2);
    b.rect(3, 16, 5, 4, 3);
    b.set(5, 0, 3);
    b.set(6, 0, 3);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(14, 12);
    b.ellipse(7, 6, 6, 4.2f, 1);
    b.ellipse(5, 5, 2.4f, 1.8f, 2);
    b.ellipse(9, 6.5f, 2.2f, 1.6f, 2);
    return b;
}

gs::Bitmap skipArt() {
    gs::Bitmap b(18, 24);
    b.ellipse(9, 5, 3.2f, 3.2f, 2);
    b.rect(7, 3, 4, 2, 5);
    b.rect(6, 9, 6, 8, 4);
    b.rect(5, 10, 2, 6, 4);
    b.rect(11, 10, 2, 6, 4);
    b.rect(6, 17, 2, 6, 3);
    b.rect(10, 17, 2, 6, 3);
    b.set(8, 5, 1);
    b.set(11, 5, 1);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(1, 1, 1);
    b.set(2, 1, 1);
    b.set(3, 1, 1);
    b.set(0, 2, 1);
    b.set(1, 2, 1);
    b.set(2, 2, 1);
    b.set(3, 2, 1);
    b.set(4, 2, 1);
    b.set(1, 3, 1);
    b.set(2, 3, 1);
    b.set(3, 3, 1);
    b.set(2, 4, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        if (c == ' ') {
            a.font[0] = 0;
            continue;
        }
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_ICE,
           {0, gs::rgb4(9, 12, 14), gs::rgb4(7, 10, 12), gs::rgb4(2, 6, 13), gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 15),
            gs::rgb4(8, 1, 1), gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 6), gs::rgb4(6, 6, 9), gs::rgb4(2, 2, 3),
            gs::rgb4(13, 15, 15), gs::rgb4(8, 1, 1), gs::rgb4(1, 3, 8), 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_RED,
           {0, gs::rgb4(4, 0, 0), gs::rgb4(10, 2, 2), gs::rgb4(13, 4, 3), gs::rgb4(15, 9, 7), gs::rgb4(15, 13, 10), 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_YEL,
           {0, gs::rgb4(5, 3, 0), gs::rgb4(12, 8, 1), gs::rgb4(14, 11, 2), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12),
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 3));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_DIM, gs::rgb4(10, 12, 14), gs::rgb4(1, 1, 3));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 3), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 14, 6), gs::rgb4(2, 1, 0));
    setPal(vdp, PAL_BROOM,
           {0, gs::rgb4(12, 8, 3), gs::rgb4(13, 2, 2), gs::rgb4(15, 12, 6), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0});
    setPal(vdp, PAL_DOT, {0, gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_PUFF, {0, gs::rgb4(14, 15, 15), gs::rgb4(11, 13, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SKIP,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(14, 10, 7), gs::rgb4(2, 2, 4), gs::rgb4(12, 2, 2), gs::rgb4(3, 2, 1), 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0});

    vdp.setFogColor(gs::rgb4(1, 2, 4));
    art.rink = gs::uploadImage(vdp, rinkArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.broom = gs::uploadMipped(vdp, broomArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.skip = gs::uploadMipped(vdp, skipArt());
    art.dot = gs::uploadImage(vdp, dotArt());
    loadFont(vdp, art);
}

}  // namespace curl
