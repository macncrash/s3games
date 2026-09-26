#include "game/art.h"

#include <cstdint>
#include <initializer_list>

#include "game/sheet.h"

namespace curlgold {
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
}

int sheetPixel(int x, int y) {
    float wx = (float(x) + 0.5f) / kPx - kHalfW;
    float wy = kImgTop - (float(y) + 0.5f) / kPx;
    float ax = std::fabs(wx);
    if (ax >= kHalfW - 0.08f) return 13;
    if (ax >= kSide + 0.28f) return 12;

    uint32_t h = uint32_t(x) * 2246822519u ^ uint32_t(y) * 3266489917u;
    h ^= h >> 13;
    int ice = 1;
    if ((h & 31u) == 0) ice = 2;
    else if ((h & 63u) == 1) ice = 3;

    float d = std::hypot(wx, wy - kTee);
    int c = ice;
    if (d <= kHouse) {
        if (d <= 0.22f) c = 6;
        else if (d <= kGold) c = ((h & 11u) == 0) ? 5 : 4;
        else c = ((h & 9u) == 0) ? 8 : 7;
    }
    auto ring = [&](float rad, int ink) {
        if (std::fabs(d - rad) <= 0.09f) c = ink;
    };
    ring(kHouse, 9);
    ring(kEight, 10);
    ring(kGold, 6);

    if (std::fabs(wy - kHog) <= 0.1f && ax < kSide) c = 11;
    if (std::fabs(wy - kBack) <= 0.09f && ax < kSide) c = 10;
    if (std::fabs(wy - kTee) <= 0.07f && ax > kHouse + 0.05f && ax < kSide) c = 10;
    if (ax <= 0.055f && wy > kHog + 0.4f && wy < kTee - kHouse - 0.15f) c = 14;
    if (std::fabs(ax - kSide) <= 0.07f && wy > kImgBot + 1.f && wy < kBack + 0.4f) c = 10;
    if (wy > kRelease - 2.3f && wy < kRelease - 1.05f && ax > 0.32f && ax < 1.2f) c = 15;
    return c;
}

gs::Bitmap rinkArt() {
    const int w = rinkW();
    const int h = rinkH();
    gs::Bitmap b(w, h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) b.set(x, y, sheetPixel(x, y));
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(32, 32);
    b.ellipse(16, 16, 14.2f, 14.2f, 6);
    b.ellipse(16, 16, 11.6f, 11.6f, 1);
    b.ellipse(16, 16, 10.4f, 10.4f, 3);
    b.ellipse(16, 16, 8.2f, 8.2f, 4);
    b.ellipse(12.2f, 12.0f, 2.4f, 1.7f, 5);
    b.rect(9, 14, 14, 3, 7);
    b.rect(22, 13, 4, 5, 7);
    b.set(23, 14, 5);
    return b;
}

gs::Bitmap broomArt() {
    gs::Bitmap b(12, 28);
    b.rect(5, 0, 2, 18, 1);
    b.rect(4, 1, 1, 4, 2);
    b.rect(2, 17, 8, 8, 3);
    b.rect(3, 18, 6, 5, 4);
    b.set(5, 0, 2);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(16, 12);
    b.ellipse(8, 6, 7, 4.2f, 1);
    b.ellipse(5.5f, 5, 2.6f, 1.8f, 2);
    b.ellipse(10.5f, 7, 2.4f, 1.6f, 2);
    return b;
}

gs::Bitmap skipArt() {
    gs::Bitmap b(22, 30);
    b.ellipse(9, 5, 4.1f, 3.6f, 4);
    b.rect(6, 2, 6, 3, 4);
    b.ellipse(9, 8.2f, 3.1f, 2.8f, 2);
    b.set(8, 8, 1);
    b.set(11, 8, 1);
    b.rect(5, 11, 8, 9, 5);
    b.rect(3, 12, 3, 7, 5);
    b.rect(12, 12, 3, 7, 4);
    b.rect(6, 20, 3, 8, 3);
    b.rect(10, 20, 3, 8, 3);
    b.rect(15, 6, 2, 16, 6);
    b.rect(13, 20, 8, 6, 7);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(5, 5);
    b.set(2, 1, 1);
    b.set(1, 2, 1);
    b.set(2, 2, 1);
    b.set(3, 2, 1);
    b.set(2, 3, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        if (c == ' ') {
            art.font[0] = 0;
            continue;
        }
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_ICE,
           {0, gs::rgb4(12, 14, 15), gs::rgb4(9, 12, 14), gs::rgb4(14, 15, 15), gs::rgb4(13, 10, 2),
            gs::rgb4(15, 13, 5), gs::rgb4(8, 6, 1), gs::rgb4(14, 12, 9), gs::rgb4(11, 9, 7), gs::rgb4(12, 2, 2),
            gs::rgb4(15, 15, 15), gs::rgb4(9, 1, 1), gs::rgb4(2, 3, 6), gs::rgb4(4, 5, 8), gs::rgb4(3, 5, 10),
            gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_RED,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(11, 11, 12), gs::rgb4(14, 14, 15),
            gs::rgb4(13, 2, 2), gs::rgb4(15, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_YEL,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(11, 11, 12), gs::rgb4(14, 14, 15),
            gs::rgb4(13, 10, 1), gs::rgb4(15, 14, 6), 0, 0, 0, 0, 0, 0, 0, 0});
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 3));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_CREAM, gs::rgb4(14, 12, 9), gs::rgb4(3, 2, 1));
    textPal(vdp, PAL_DIM, gs::rgb4(10, 12, 14), gs::rgb4(1, 1, 3));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 3), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 14, 6), gs::rgb4(3, 2, 0));
    setPal(vdp, PAL_BROOM,
           {0, gs::rgb4(10, 7, 3), gs::rgb4(13, 3, 2), gs::rgb4(14, 12, 6), gs::rgb4(8, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0});
    setPal(vdp, PAL_DOT, {0, gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_PUFF, {0, gs::rgb4(14, 15, 15), gs::rgb4(11, 13, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SKIP,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(14, 10, 7), gs::rgb4(2, 2, 4), gs::rgb4(12, 2, 2), gs::rgb4(8, 2, 2),
            gs::rgb4(9, 6, 3), gs::rgb4(12, 9, 4), 0, 0, 0, 0, 0, 0, 0, 0});

    vdp.setFogColor(gs::rgb4(10, 13, 15));
    art.rink = gs::uploadImage(vdp, rinkArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.broom = gs::uploadMipped(vdp, broomArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.skip = gs::uploadMipped(vdp, skipArt());
    gs::TextStyle st;
    st.scale = 3;
    st.color = 1;
    st.outline = 2;
    st.spacing = 0;
    art.badge2 = gs::uploadMipped(vdp, gs::textBitmap("2", st));
    art.badge1 = gs::uploadMipped(vdp, gs::textBitmap("1", st));
    art.dot = gs::uploadImage(vdp, dotArt());
    loadFont(vdp, art);
}

}  // namespace curlgold
