#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace dartbell {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 1));
}

bool grain(int x, int y) {
    uint32_t h = uint32_t(x) * 2246822519u ^ uint32_t(y) * 3266489917u;
    h ^= h >> 15;
    return (h & 15u) == 0;
}

bool wireAt(float dx, float dy, float r) {
    const float rings[] = {kBellR, kLipOut, kTripIn, kTripOut, kDoubIn, kDoubOut};
    for (float rr : rings)
        if (std::fabs(r - rr) <= 0.7f) return true;
    if (r <= kLipOut || r > kDoubOut) return false;
    float deg = std::atan2(dx, -dy) * (180.f / 3.14159265f);
    if (deg < 0.f) deg += 360.f;
    float e = std::fmod(deg + 9.f, 18.f);
    float ded = std::min(e, 18.f - e);
    return ded * 0.017453292f * r <= 0.7f;
}

bool screwAt(float dx, float dy) {
    for (int i = 0; i < 4; i++) {
        float a = 0.78539816f + float(i) * 1.5707963f;
        float sx = std::cos(a) * (kRim - 3.2f);
        float sy = std::sin(a) * (kRim - 3.2f);
        if (std::hypot(dx - sx, dy - sy) < 1.8f) return true;
    }
    return false;
}

int boardPixel(int x, int y) {
    float dx = (x + 0.5f) - kBmpC;
    float dy = (y + 0.5f) - kBmpC;
    float r = std::hypot(dx, dy);
    if (r > kRim) return 0;
    if (screwAt(dx, dy)) return 15;
    bool spec = grain(x, y);
    if (r <= kDoubOut + 0.8f && wireAt(dx, dy, r)) return 9;
    if (r > kRim - 2.6f) return spec ? 15 : 13;
    if (r > kDoubOut) return spec ? 11 : 10;
    if (r <= kBellR) return 1;
    if (r <= kLipOut) return spec ? 12 : 13;
    int s = sectorAt(dx, dy);
    bool dark = (s & 1) == 0;
    bool ring = (r > kTripIn && r <= kTripOut) || r > kDoubIn;
    if (ring) return dark ? (spec ? 6 : 5) : (spec ? 8 : 7);
    return dark ? (spec ? 2 : 1) : (spec ? 4 : 3);
}

gs::Bitmap boardArt() {
    gs::Bitmap b(kBmp, kBmp);
    for (int y = 0; y < kBmp; y++)
        for (int x = 0; x < kBmp; x++) b.set(x, y, boardPixel(x, y));
    for (int s = 0; s < 20; s++) {
        char buf[4];
        std::snprintf(buf, sizeof buf, "%d", kSeg[s]);
        gs::Bitmap t = gs::textBitmap(buf, {1, 14, 1, 0, 0});
        float ang = float(s) * (3.14159265f / 10.f);
        float nx = kBmpC + std::sin(ang) * kNumR;
        float ny = kBmpC - std::cos(ang) * kNumR;
        b.blit(t, int(std::lround(nx - t.w * 0.5f)), int(std::lround(ny - t.h * 0.5f)));
    }
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(34, 36);
    b.ellipse(17.f, 18.f, 11.f, 11.f, 3);
    b.ellipse(17.f, 17.f, 8.2f, 8.4f, 2);
    b.ellipse(17.f, 18.f, 5.1f, 4.3f, 1);
    b.ellipse(17.f, 9.2f, 2.3f, 2.1f, 6);
    b.line(12.f, 12.f, 14.f, 18.f, 6, 1.1f);
    b.ellipse(17.f, 25.5f, 9.5f, 2.4f, 5);
    b.ellipse(17.f, 18.5f, 4.2f, 3.2f, 1);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(7, 10);
    b.line(3, 0, 3, 5, 1, 1.2f);
    b.ellipse(3.f, 7.f, 2.3f, 2.3f, 2);
    return b;
}

gs::Bitmap yokeArt() {
    gs::Bitmap b(22, 12);
    b.rect(0, 0, 22, 3, 1);
    b.rect(9, 2, 4, 9, 2);
    b.ellipse(11.f, 9.f, 2.4f, 2.2f, 3);
    return b;
}

gs::Bitmap dartArt() {
    gs::Bitmap b(16, 28);
    b.line(8, 1, 1, 9, 1, 1.6f);
    b.line(8, 1, 15, 9, 1, 1.6f);
    b.line(8, 3, 3, 10, 2, 1.1f);
    b.line(8, 3, 13, 10, 2, 1.1f);
    b.line(8, 9, 8, 21, 3, 1.5f);
    b.line(8, 15, 8, 20, 4, 2.3f);
    b.line(8, 21, 8, 27, 5, 1.2f);
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

gs::Bitmap pipArt() {
    gs::Bitmap b(9, 9);
    b.ellipse(4.f, 4.f, 3.6f, 3.6f, 1);
    b.ellipse(4.f, 4.f, 1.6f, 1.6f, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(12, 5);
    b.ellipse(6.f, 2.f, 5.5f, 2.f, 1);
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

gs::Bitmap beamArt() {
    gs::Bitmap b(14, 108);
    b.rect(0, 0, 14, 108, 2);
    b.rect(2, 0, 10, 108, 1);
    b.rect(6, 0, 2, 108, 3);
    return b;
}

gs::Bitmap candleArt() {
    gs::Bitmap b(10, 16);
    b.rect(3, 6, 4, 9, 1);
    b.rect(4, 5, 2, 2, 2);
    b.rect(3, 14, 4, 2, 3);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(8, 10);
    b.ellipse(4.f, 6.f, 2.8f, 3.6f, 1);
    b.ellipse(4.f, 6.4f, 1.3f, 2.1f, 2);
    return b;
}

gs::Bitmap ocheArt() {
    gs::Bitmap b(96, 4);
    b.rect(0, 1, 96, 2, 1);
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
           {0, gs::rgb4(2, 1, 1), gs::rgb4(5, 3, 2), gs::rgb4(14, 12, 9), gs::rgb4(15, 14, 12), gs::rgb4(12, 2, 2),
            gs::rgb4(15, 5, 3), gs::rgb4(1, 7, 3), gs::rgb4(4, 11, 5), gs::rgb4(13, 13, 12), gs::rgb4(5, 3, 1),
            gs::rgb4(8, 5, 2), gs::rgb4(7, 5, 2), gs::rgb4(13, 9, 3), gs::rgb4(15, 14, 11), gs::rgb4(15, 13, 6)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 13), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 4), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 7), gs::rgb4(0, 2, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 4, 3), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_AIM, gs::rgb4(15, 13, 5), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 5), gs::rgb4(4, 2, 0));
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(3, 1, 0), gs::rgb4(12, 8, 2), gs::rgb4(8, 5, 1), gs::rgb4(14, 11, 4), gs::rgb4(13, 9, 3),
            gs::rgb4(15, 14, 8), gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_DART,
           {0, gs::rgb4(14, 2, 2), gs::rgb4(9, 1, 1), gs::rgb4(4, 3, 3), gs::rgb4(12, 9, 3), gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(10, 6, 2), gs::rgb4(5, 3, 1), gs::rgb4(13, 9, 4)});
    setPal(vdp, PAL_FLAME, {0, gs::rgb4(15, 8, 1), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_WAX, {0, gs::rgb4(13, 12, 9), gs::rgb4(3, 2, 1), gs::rgb4(8, 6, 3)});
    setPal(vdp, PAL_CHALK, {0, gs::rgb4(13, 12, 9)});

    loadFont(vdp, art);
    art.board = gs::uploadImage(vdp, boardArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("BELL", {3, 1, 2, 0, 1}));
    art.rung = gs::uploadImage(vdp, gs::textBitmap("RUNG", {3, 1, 2, 0, 1}));
    art.dead = gs::uploadImage(vdp, gs::textBitmap("DEAD", {3, 1, 2, 0, 1}));
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.clapper = gs::uploadImage(vdp, clapperArt());
    art.yoke = gs::uploadImage(vdp, yokeArt());
    art.dart = gs::uploadMipped(vdp, dartArt());
    art.cross = gs::uploadImage(vdp, crossArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
    art.dot = gs::uploadImage(vdp, dotArt());
    art.beam = gs::uploadImage(vdp, beamArt());
    art.candle = gs::uploadImage(vdp, candleArt());
    art.flame = gs::uploadImage(vdp, flameArt());
    art.oche = gs::uploadImage(vdp, ocheArt());
}

}  // namespace dartbell
