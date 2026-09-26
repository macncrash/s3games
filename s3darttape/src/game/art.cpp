#include "game/art.h"

#include <algorithm>
#include <cstdint>
#include <initializer_list>

namespace darttape {
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
    return (h & 7u) == 0;
}

bool tapeDouble(int number) {
    for (int i = 0; i < 3; i++)
        if (rule::number(i) == number) return true;
    return false;
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
    return ded * 0.017453292f * r <= 0.65f;
}

bool screwAt(float dx, float dy) {
    for (int i = 0; i < 4; i++) {
        float a = 0.78539816f + float(i) * 1.5707963f;
        float sx = std::cos(a) * (kRim - 2.2f);
        float sy = std::sin(a) * (kRim - 2.2f);
        if (std::hypot(dx - sx, dy - sy) < 1.6f) return true;
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
    if (r > kRim - 2.8f) return spec ? 15 : 2;
    if (r > kDoubOut) return spec ? 11 : 10;
    if (r <= kBullIn) return spec ? 6 : 5;
    if (r <= kBullOut) return spec ? 8 : 7;
    int s = sectorAt(dx, dy);
    bool dark = (s & 1) == 0;
    bool triple = r > kTripIn && r <= kTripOut;
    bool doubl = r > kDoubIn;
    if (doubl && tapeDouble(kSeg[s])) return spec ? 13 : 12;
    if (triple || doubl) return dark ? (spec ? 6 : 5) : (spec ? 8 : 7);
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

gs::Bitmap dartArt() {
    gs::Bitmap b(14, 30);
    b.line(7, 3, 1, 13, 1, 1.8f);
    b.line(7, 3, 13, 13, 1, 1.8f);
    b.line(7, 5, 3, 12, 2, 1.1f);
    b.line(7, 5, 11, 12, 2, 1.1f);
    b.line(7, 12, 7, 23, 3, 2.5f);
    b.line(7, 15, 7, 22, 4, 1.3f);
    b.line(7, 22, 7, 29, 5, 1.15f);
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
    gs::Bitmap b(7, 7);
    b.ellipse(3.f, 3.f, 2.8f, 2.8f, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(12, 5);
    b.ellipse(6.f, 2.f, 5.4f, 1.8f, 1);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(5, 5);
    b.ellipse(2.f, 2.f, 1.8f, 1.8f, 1);
    return b;
}

gs::Bitmap paperArt() {
    gs::Bitmap b(kPaperW, kPaperH);
    b.rect(0, 0, float(kPaperW), float(kPaperH), 3);
    b.rect(0, 0, 6, float(kPaperH), 2);
    b.rect(6, 0, float(kPaperW - 6), 3, 4);
    b.rect(8, 22, float(kPaperW - 16), 1, 4);
    b.rect(8, 78, float(kPaperW - 16), 1, 2);
    return b;
}

gs::Bitmap drawerArt() {
    gs::Bitmap b(kDrawerW, kDrawerH);
    b.rect(0, 0, float(kDrawerW), float(kDrawerH), 1);
    b.rect(2, 2, float(kDrawerW - 4), float(kDrawerH - 4), 2);
    b.rect(3, 3, float(kDrawerW - 8), 3, 3);
    for (int i = 0; i < 3; i++) {
        float cx = slotX(i) - kDrawerL;
        b.rect(cx - 21.f, 4, 42, 16, 4);
        b.rect(cx - 19.f, 6, 38, 12, 5);
    }
    b.ellipse(float(kDrawerW - 12), 12.f, 5.f, 5.f, 6);
    return b;
}

gs::Bitmap slipArt(const char* label) {
    gs::Bitmap b(40, 16);
    b.rect(0, 0, 40, 16, 3);
    b.rect(0, 0, 40, 2, 2);
    b.rect(33, 0, 7, 7, 5);
    gs::Bitmap t = gs::textBitmap(label, {1, 1, 0, 0, 0});
    b.blit(t, 5, 5);
    return b;
}

gs::Bitmap playerArt(int step) {
    gs::Bitmap b(24, 40);
    b.rect(7, 1, 11, 3, 1);
    b.rect(4, 3, 9, 2, 1);
    b.ellipse(12.f, 8.5f, 4.2f, 4.1f, 2);
    b.set(10, 8, 1);
    b.rect(8, 13, 9, 11, 3);
    b.rect(8, 13, 2, 11, 5);
    b.line(8, 16, 1, 8, 2, 1.7f);
    b.rect(8, 23, 9, 2, 1);
    if (step == 0) {
        b.line(11, 25, 9, 36, 4, 2.1f);
        b.line(15, 25, 16, 36, 4, 2.1f);
        b.rect(6, 35, 6, 3, 1);
        b.rect(14, 35, 6, 3, 1);
    } else {
        b.line(11, 25, 7, 35, 4, 2.1f);
        b.line(15, 25, 18, 35, 4, 2.1f);
        b.rect(4, 34, 6, 3, 1);
        b.rect(16, 34, 6, 3, 1);
    }
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
           {0, gs::rgb4(2, 1, 1), gs::rgb4(5, 3, 2), gs::rgb4(14, 12, 8), gs::rgb4(15, 14, 11), gs::rgb4(11, 2, 2),
            gs::rgb4(15, 5, 4), gs::rgb4(1, 7, 3), gs::rgb4(5, 12, 6), gs::rgb4(14, 14, 12), gs::rgb4(4, 2, 1),
            gs::rgb4(6, 4, 2), gs::rgb4(12, 9, 2), gs::rgb4(15, 13, 5), gs::rgb4(15, 14, 10), gs::rgb4(12, 8, 3)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 12), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 4), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 7), gs::rgb4(0, 2, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 4, 3), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_AIM, gs::rgb4(15, 13, 6), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 5), gs::rgb4(4, 1, 0));
    setPal(vdp, PAL_DART,
           {0, gs::rgb4(2, 3, 8), gs::rgb4(6, 8, 14), gs::rgb4(12, 9, 3), gs::rgb4(7, 5, 2), gs::rgb4(14, 14, 13)});
    setPal(vdp, PAL_PAPER,
           {0, gs::rgb4(2, 3, 2), gs::rgb4(12, 2, 2), gs::rgb4(15, 14, 11), gs::rgb4(10, 8, 6), gs::rgb4(12, 11, 8)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(3, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3), gs::rgb4(2, 1, 1), gs::rgb4(1, 1, 1),
            gs::rgb4(13, 10, 4)});
    setPal(vdp, PAL_PLAYER,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(14, 10, 7), gs::rgb4(14, 13, 11), gs::rgb4(3, 3, 6), gs::rgb4(9, 8, 7)});

    loadFont(vdp, art);
    art.board = gs::uploadImage(vdp, boardArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("DART", {2, 1, 2, 0, 1}));
    art.paid = gs::uploadImage(vdp, gs::textBitmap("PAID", {2, 1, 2, 0, 1}));
    art.open = gs::uploadImage(vdp, gs::textBitmap("OPEN", {2, 1, 2, 0, 1}));
    art.dart = gs::uploadMipped(vdp, dartArt());
    art.cross = gs::uploadImage(vdp, crossArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
    art.dot = gs::uploadImage(vdp, dotArt());
    art.paper = gs::uploadImage(vdp, paperArt());
    art.drawer = gs::uploadImage(vdp, drawerArt());
    for (int i = 0; i < 3; i++) {
        char buf[8];
        std::snprintf(buf, sizeof buf, "D%d", rule::number(i));
        art.slip[i] = gs::uploadImage(vdp, slipArt(buf));
    }
    art.player[0] = gs::uploadImage(vdp, playerArt(0));
    art.player[1] = gs::uploadImage(vdp, playerArt(1));
}

}  // namespace darttape
