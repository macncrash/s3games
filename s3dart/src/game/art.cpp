#include "game/art.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>

namespace dart {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t edge = 0) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 2, edge ? edge : gs::rgb4(1, 1, 2));
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

bool wireAt(float dx, float dy, float r) {
    const float rings[] = {kBullIn, kBullOut, kTripIn, kTripOut, kDoubIn, kDoubOut};
    for (float rr : rings)
        if (std::fabs(r - rr) <= 0.65f) return true;
    if (r <= kBullOut || r > kDoubOut) return false;
    float deg = std::atan2(dx, -dy) * (180.f / 3.14159265f);
    if (deg < 0.f) deg += 360.f;
    float e = std::fmod(deg + 9.f, 18.f);
    float ded = std::min(e, 18.f - e);
    return ded * 0.017453292f * r <= 0.7f;
}

int boardPixel(int x, int y) {
    float dx = (x + 0.5f) - kBmpC;
    float dy = (y + 0.5f) - kBmpC;
    float r = std::hypot(dx, dy);
    if (r > kRim) return 0;
    if (r > kWoodIn) {
        uint32_t h = uint32_t(x) * 374761393u ^ uint32_t(y) * 668265263u;
        h ^= h >> 13;
        int n = int((h ^ (h >> 7)) & 7u);
        if (r > kRim - 2.4f) return 14;
        return n == 0 ? 12 : 13;
    }
    if (r <= kDoubOut + 0.8f && wireAt(dx, dy, r)) return 11;
    if (r > kDoubOut) return 1;
    bool bristle = ((x * 13 + y * 7) & 7) == 0;
    if (r <= kBullIn) return 9;
    if (r <= kBullOut) return bristle ? 8 : 10;
    int s = sectorAt(dx, dy);
    bool hot = (s & 1) == 0;
    bool ring = (r > kTripIn && r <= kTripOut) || r > kDoubIn;
    if (ring) return hot ? (bristle ? 6 : 5) : (bristle ? 8 : 7);
    return hot ? (bristle ? 2 : 1) : (bristle ? 4 : 3);
}

gs::Bitmap boardArt() {
    gs::Bitmap b(kBoard, kBoard);
    for (int y = 0; y < kBoard; y++)
        for (int x = 0; x < kBoard; x++) b.set(x, y, boardPixel(x, y));
    for (int s = 0; s < 20; s++) {
        char buf[4];
        std::snprintf(buf, sizeof buf, "%d", kSeg[s]);
        gs::Bitmap t = gs::textBitmap(buf, {1, 15, 0, 0, 1});
        float ang = float(s) * (3.14159265f / 10.f);
        float nx = kBmpC + std::sin(ang) * kNumR;
        float ny = kBmpC - std::cos(ang) * kNumR;
        b.blit(t, int(std::lround(nx - t.w * 0.5f)), int(std::lround(ny - t.h * 0.5f)));
    }
    return b;
}

gs::Bitmap dartArt() {
    gs::Bitmap b(15, 15);
    b.line(7, 0, 7, 5, 1, 2.2f);
    b.line(7, 9, 7, 14, 1, 2.2f);
    b.line(0, 7, 5, 7, 1, 2.2f);
    b.line(9, 7, 14, 7, 1, 2.2f);
    b.outline(3, false);
    b.ellipse(7, 7, 2.1f, 2.1f, 2);
    b.ellipse(6.2f, 6.2f, 0.8f, 0.8f, 1);
    return b;
}

gs::Bitmap crossArt() {
    gs::Bitmap b(13, 13);
    b.rect(6, 0, 1, 4, 1);
    b.rect(6, 9, 1, 4, 1);
    b.rect(0, 6, 4, 1, 1);
    b.rect(9, 6, 4, 1, 1);
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
    setPal(vdp, PAL_BOARD,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5), gs::rgb4(14, 12, 9), gs::rgb4(11, 9, 6), gs::rgb4(13, 2, 2),
            gs::rgb4(15, 5, 3), gs::rgb4(1, 8, 3), gs::rgb4(3, 12, 5), gs::rgb4(15, 2, 2), gs::rgb4(2, 11, 4),
            gs::rgb4(13, 13, 12), gs::rgb4(5, 3, 1), gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 4), gs::rgb4(15, 15, 14)});
    textPal(vdp, PAL_SCORE, gs::rgb4(15, 14, 5), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 6));
    textPal(vdp, PAL_AIM, gs::rgb4(15, 14, 4));
    textPal(vdp, PAL_SWEET, gs::rgb4(4, 15, 6));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 4), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_BUST, gs::rgb4(15, 3, 2), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 14, 6), gs::rgb4(2, 1, 0));

    auto flight = [&](int pal, uint16_t c) {
        for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
        vdp.setColor(pal * 16 + 1, c);
        vdp.setColor(pal * 16 + 2, gs::rgb4(12, 13, 14));
        vdp.setColor(pal * 16 + 3, gs::rgb4(1, 1, 1));
    };
    flight(PAL_DART0, gs::rgb4(14, 2, 2));
    flight(PAL_DART1, gs::rgb4(14, 13, 8));
    flight(PAL_DART2, gs::rgb4(3, 6, 14));

    loadFont(vdp, art);
    art.board = gs::uploadImage(vdp, boardArt());
    gs::TextStyle dig{2, 1, 2, 0, 1};
    for (int d = 0; d < 10; d++) {
        char ch[2] = {char('0' + d), 0};
        gs::Bitmap bm = gs::textBitmap(ch, dig);
        art.digit[d] = gs::uploadImage(vdp, bm);
        art.digitW = bm.w;
        art.digitH = bm.h;
    }
    art.title = gs::uploadImage(vdp, gs::textBitmap("S3 DART", {2, 1, 2, 0, 1}));
    art.bust = gs::uploadImage(vdp, gs::textBitmap("BUST", {3, 1, 2, 0, 1}));
    art.win = gs::uploadImage(vdp, gs::textBitmap("GAME SHOT", {2, 1, 2, 0, 1}));
    art.dart = gs::uploadMipped(vdp, dartArt());
    art.cross = gs::uploadImage(vdp, crossArt());
    art.dot = gs::uploadImage(vdp, dotArt());
}

}  // namespace dart
