#include "game/art.h"

#include <algorithm>
#include <cstdint>
#include <initializer_list>

namespace dartchime {
namespace {

constexpr int kDial = 48;
constexpr float kTau = 6.2831853f;

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
        if (std::fabs(r - rr) <= 0.55f) return true;
    if (r <= kBullOut || r > kDoubOut) return false;
    float deg = std::atan2(dx, -dy) * (180.f / 3.14159265f);
    if (deg < 0.f) deg += 360.f;
    float e = std::fmod(deg + 9.f, 18.f);
    float ded = std::min(e, 18.f - e);
    return ded * 0.017453292f * r <= 0.65f;
}

int boardPixel(int x, int y) {
    float dx = (x + 0.5f) - kBmpC;
    float dy = (y + 0.5f) - kBmpC;
    float r = std::hypot(dx, dy);
    if (r > kRim) return 0;
    if (r > kDoubOut) {
        uint32_t h = uint32_t(x) * 2246822519u ^ uint32_t(y) * 3266489917u;
        h ^= h >> 13;
        int n = int((h ^ (h >> 7)) & 7u);
        if (r > kRim - 2.2f) return 11;
        return n == 0 ? 3 : 14;
    }
    if (wireAt(dx, dy, r)) return 11;
    bool bristle = ((x * 5 + y * 3) & 7) == 0;
    if (r <= kBullIn) return bristle ? 6 : 9;
    if (r <= kBullOut) return bristle ? 8 : 10;
    int s = sectorAt(dx, dy);
    bool black = (s & 1) == 0;
    bool ring = (r > kTripIn && r <= kTripOut) || r > kDoubIn;
    bool hour = ring && r > kDoubIn && kSeg[s] == 12;
    if (hour) return bristle ? 13 : 12;
    if (ring) return black ? (bristle ? 6 : 5) : (bristle ? 8 : 7);
    return black ? (bristle ? 4 : 3) : (bristle ? 2 : 1);
}

gs::Bitmap boardArt() {
    gs::Bitmap b(kBmp, kBmp);
    for (int y = 0; y < kBmp; y++)
        for (int x = 0; x < kBmp; x++) b.set(x, y, boardPixel(x, y));
    for (int s = 0; s < 20; s++) {
        char buf[4];
        std::snprintf(buf, sizeof buf, "%d", kSeg[s]);
        int col = kSeg[s] == 12 ? 12 : 15;
        gs::Bitmap t = gs::textBitmap(buf, {1, col, 0, 0, 1});
        float ang = float(s) * (3.14159265f / 10.f);
        float nx = kBmpC + std::sin(ang) * kNumR;
        float ny = kBmpC - std::cos(ang) * kNumR;
        b.blit(t, int(std::lround(nx - t.w * 0.5f)), int(std::lround(ny - t.h * 0.5f)));
    }
    return b;
}

gs::Bitmap dartArt() {
    gs::Bitmap b(18, 34);
    b.poly({{9.f, 15.f}, {1.f, 31.f}, {8.f, 26.f}}, 1);
    b.poly({{9.f, 15.f}, {17.f, 31.f}, {10.f, 26.f}}, 2);
    b.rect(8, 14, 2, 8, 3);
    b.rect(6, 7, 6, 8, 4);
    b.rect(6, 9, 6, 2, 5);
    b.poly({{9.f, 1.f}, {7.f, 8.f}, {11.f, 8.f}}, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap crossArt() {
    gs::Bitmap b(15, 15);
    b.rect(7, 0, 1, 5, 1);
    b.rect(7, 10, 1, 5, 1);
    b.rect(0, 7, 5, 1, 1);
    b.rect(10, 7, 5, 1, 1);
    b.set(7, 7, 1);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(7, 7);
    b.ellipse(3.5f, 3.5f, 3.f, 3.f, 1);
    b.ellipse(3.5f, 3.5f, 1.3f, 1.3f, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7, 2.2f, 1);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(5, 5);
    b.ellipse(2.5f, 2.5f, 2.f, 2.f, 1);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(10, 14);
    b.poly({{5.f, 1.f}, {1.f, 12.f}, {9.f, 12.f}}, 1);
    b.poly({{5.f, 5.f}, {3.f, 12.f}, {7.f, 12.f}}, 2);
    b.ellipse(5, 12, 3.2f, 1.4f, 3);
    return b;
}

gs::Bitmap ocheArt() {
    gs::Bitmap b(88, 6);
    b.rect(0, 2, 34, 2, 1);
    b.rect(54, 2, 34, 2, 1);
    b.rect(40, 1, 8, 4, 1);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(20, 18);
    b.rect(9, 0, 2, 3, 2);
    b.poly({{10.f, 3.f}, {3.f, 13.f}, {17.f, 13.f}}, 1);
    b.ellipse(10, 13.5f, 8.f, 3.1f, 1);
    b.ellipse(10, 13.5f, 4.2f, 1.5f, 2);
    b.ellipse(10, 10, 1.3f, 1.3f, 3);
    return b;
}

gs::Bitmap yokeArt() {
    gs::Bitmap b(52, 6);
    b.rect(0, 2, 52, 3, 1);
    b.rect(0, 1, 52, 1, 2);
    return b;
}

void shaft(gs::Bitmap& b, float theta, float len, float tail, float w, int col) {
    const float cx = kDial * 0.5f, cy = kDial * 0.5f;
    float dx = std::sin(theta), dy = -std::cos(theta);
    float px = -dy, py = dx;
    float x0 = cx - dx * tail, y0 = cy - dy * tail;
    float x1 = cx + dx * len, y1 = cy + dy * len;
    b.poly({{x0 + px * w * 0.35f, y0 + py * w * 0.35f},
            {x1 + px * w, y1 + py * w},
            {x1 - px * w, y1 - py * w},
            {x0 - px * w * 0.35f, y0 - py * w * 0.35f}},
           col);
}

gs::Bitmap handArt(int kind, int step) {
    gs::Bitmap b(kDial, kDial);
    float theta = step * (kTau / 60.f);
    if (kind == 0) {
        shaft(b, theta, 12.f, 3.2f, 2.15f, 1);
        shaft(b, theta, 11.f, 2.4f, 1.f, 2);
    } else if (kind == 1) {
        shaft(b, theta, 17.f, 3.6f, 1.05f, 3);
    } else {
        shaft(b, theta, 19.f, 5.2f, 0.45f, 4);
        float dx = std::sin(theta), dy = -std::cos(theta);
        b.ellipse(24.f - dx * 4.2f, 24.f - dy * 4.2f, 1.5f, 1.5f, 5);
    }
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(kDial, kDial);
    b.ellipse(24, 24, 23.2f, 23.2f, 1);
    b.ellipse(24, 24, 21.2f, 21.2f, 2);
    b.ellipse(24, 24, 19.4f, 19.4f, 3);
    for (int i = 0; i < 60; i++) {
        float a = i * (kTau / 60.f);
        float s = std::sin(a), c = std::cos(a);
        float inner = (i % 5 == 0) ? 14.2f : 16.4f;
        int col = i == 0 ? 6 : 5;
        float thick = (i % 5 == 0) ? 1.35f : 0.7f;
        b.line(24 + s * inner, 24 - c * inner, 24 + s * 18.2f, 24 - c * 18.2f, col, thick);
    }
    auto stamp = [&](const char* t, int x, int y) {
        gs::Bitmap num = gs::textBitmap(t, {1, 7, 0, 0, 0});
        b.blit(num, x - num.w / 2, y - num.h / 2);
    };
    stamp("12", 24, 8);
    stamp("3", 40, 24);
    stamp("6", 24, 40);
    stamp("9", 8, 24);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(kDial, kDial);
    for (int y = 0; y < kDial; y++) {
        for (int x = 0; x < kDial; x++) {
            float d = std::hypot((x + 0.5f) - 24.f, (y + 0.5f) - 24.f);
            if (d > 20.6f && d < 23.6f) b.set(x, y, 1);
        }
    }
    return b;
}

gs::Bitmap capArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4.2f, 4.2f, 1);
    b.ellipse(5, 5, 1.8f, 1.8f, 2);
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
                    if (y + 1 < 8 && x + 2 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_BOARD,
           {0, gs::rgb4(14, 12, 8), gs::rgb4(11, 9, 6), gs::rgb4(2, 2, 2), gs::rgb4(4, 4, 4), gs::rgb4(13, 2, 2),
            gs::rgb4(15, 5, 3), gs::rgb4(1, 9, 3), gs::rgb4(3, 12, 5), gs::rgb4(15, 1, 1), gs::rgb4(2, 11, 3),
            gs::rgb4(13, 13, 12), gs::rgb4(15, 12, 3), gs::rgb4(12, 8, 1), gs::rgb4(7, 4, 2), gs::rgb4(15, 15, 13)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 4, 3), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(4, 15, 6), gs::rgb4(0, 2, 0));
    textPal(vdp, PAL_AIM, gs::rgb4(15, 14, 8));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 4), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_CHALK, gs::rgb4(14, 13, 11));

    setPal(vdp, PAL_DART,
           {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 14, 12), gs::rgb4(10, 7, 4), gs::rgb4(2, 2, 2), gs::rgb4(14, 11, 3),
            gs::rgb4(12, 13, 14), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CLOCK,
           {0, gs::rgb4(12, 9, 3), gs::rgb4(5, 3, 1), gs::rgb4(14, 13, 10), gs::rgb4(0, 0, 0), gs::rgb4(2, 2, 2),
            gs::rgb4(15, 12, 3), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_HAND,
           {0, gs::rgb4(1, 1, 1), gs::rgb4(6, 4, 2), gs::rgb4(1, 1, 2), gs::rgb4(13, 2, 2), gs::rgb4(14, 11, 3)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(14, 11, 4), gs::rgb4(7, 5, 2), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(6, 3, 1), gs::rgb4(10, 7, 3)});
    setPal(vdp, PAL_FLAME, {0, gs::rgb4(15, 8, 1), gs::rgb4(15, 14, 4), gs::rgb4(12, 3, 1)});

    loadFont(vdp, art);
    art.board = gs::uploadImage(vdp, boardArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("S3 DARTCHIME", {2, 1, 2, 0, 1}));
    art.face = gs::uploadImage(vdp, faceArt());
    art.ring = gs::uploadImage(vdp, ringArt());
    art.cap = gs::uploadImage(vdp, capArt());
    for (int k = 0; k < 3; k++)
        for (int s = 0; s < 60; s++) art.hand[k][s] = gs::uploadImage(vdp, handArt(k, s));
    art.bell = gs::uploadImage(vdp, bellArt());
    art.yoke = gs::uploadImage(vdp, yokeArt());
    art.dart = gs::uploadMipped(vdp, dartArt());
    art.cross = gs::uploadImage(vdp, crossArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
    art.dot = gs::uploadImage(vdp, dotArt());
    art.flame = gs::uploadImage(vdp, flameArt());
    art.oche = gs::uploadImage(vdp, ocheArt());
}

}  // namespace dartchime
