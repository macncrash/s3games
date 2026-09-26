#include "game/art.h"

#include <cstdint>
#include <cstdio>
#include <initializer_list>

namespace dartmark {
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

bool fiber(int x, int y) {
    uint32_t h = uint32_t(x) * 2246822519u ^ uint32_t(y) * 3266489917u;
    h ^= h >> 15;
    return (h & 11u) == 0;
}

bool wireAt(float dx, float dy, float r) {
    const float rings[] = {kBullIn, kBullOut, kTripIn, kTripOut, kDoubIn, kDoubOut};
    for (float rr : rings)
        if (std::fabs(r - rr) <= 0.8f) return true;
    if (r <= kBullOut || r > kDoubOut) return false;
    float deg = std::atan2(dx, -dy) * (180.f / 3.14159265f);
    if (deg < 0.f) deg += 360.f;
    float e = std::fmod(deg + 9.f, 18.f);
    float ded = std::min(e, 18.f - e);
    return ded * 0.017453292f * r <= 0.75f;
}

int boardPixel(int x, int y) {
    float dx = (x + 0.5f) - kBmpC;
    float dy = (y + 0.5f) - kBmpC;
    float r = std::hypot(dx, dy);
    if (r > kRim) return 0;
    if (r > kRim - 2.1f) return 12;
    if (r > kWoodIn) {
        bool grain = ((x * 2 + y) & 7) == 0;
        return grain ? 12 : 13;
    }
    if (r > kDoubOut + 0.8f && r <= kDoubOut + 3.3f && sectorAt(dx, dy) == kMarkSeg) return 15;
    if (r <= kDoubOut + 0.9f && wireAt(dx, dy, r)) return 11;
    if (r > kDoubOut) return 12;
    bool spec = fiber(x, y);
    if (r <= kBullIn) return 9;
    if (r <= kBullOut) return spec ? 8 : 10;
    int s = sectorAt(dx, dy);
    bool black = (s & 1) == 0;
    bool ring = (r > kTripIn && r <= kTripOut) || r > kDoubIn;
    if (ring) return black ? (spec ? 6 : 5) : (spec ? 8 : 7);
    return black ? (spec ? 2 : 1) : (spec ? 4 : 3);
}

gs::Bitmap boardArt() {
    gs::Bitmap b(kBoard, kBoard);
    for (int y = 0; y < kBoard; y++)
        for (int x = 0; x < kBoard; x++) b.set(x, y, boardPixel(x, y));
    for (int s = 0; s < 20; s++) {
        char buf[4];
        std::snprintf(buf, sizeof buf, "%d", kSeg[s]);
        int ink = s == kMarkSeg ? 15 : 14;
        gs::Bitmap t = gs::textBitmap(buf, {1, ink, 0, 12, 1});
        float ang = float(s) * (3.14159265f / 10.f);
        float nx = kBmpC + std::sin(ang) * kNumR;
        float ny = kBmpC - std::cos(ang) * kNumR;
        b.blit(t, int(std::lround(nx - t.w * 0.5f)), int(std::lround(ny - t.h * 0.5f)));
    }
    return b;
}

gs::Bitmap slateArt() {
    gs::Bitmap b(70, 108);
    b.rect(0, 0, 70, 108, 2);
    b.rect(3, 3, 64, 102, 1);
    b.rect(6, 6, 58, 96, 3);
    b.rect(12, 62, 14, 26, 4);
    b.rect(28, 62, 14, 26, 4);
    b.rect(44, 62, 14, 26, 4);
    return b;
}

gs::Bitmap dartArt() {
    gs::Bitmap b(13, 16);
    b.line(6, 1, 1, 5, 1, 1.7f);
    b.line(6, 1, 11, 5, 1, 1.7f);
    b.line(6, 1, 3, 7, 1, 1.3f);
    b.line(6, 1, 9, 7, 1, 1.3f);
    b.line(6, 5, 6, 9, 3, 1.5f);
    b.ellipse(6.f, 11.4f, 2.5f, 3.1f, 2);
    b.set(5, 10, 1);
    b.line(6, 14, 6, 15, 2, 1.2f);
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

gs::Bitmap slashArt() {
    gs::Bitmap b(9, 16);
    b.line(1, 14, 7, 1, 1, 2.1f);
    b.outline(2, false);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(7, 7);
    b.ellipse(3.f, 3.f, 3.1f, 3.1f, 1);
    b.ellipse(3.f, 3.f, 1.3f, 1.3f, 0);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(9, 5);
    b.ellipse(4.f, 2.f, 4.f, 2.f, 1);
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

void flightPal(gs::VDP& vdp, int pal, uint16_t flight) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, flight);
    vdp.setColor(pal * 16 + 2, gs::rgb4(12, 13, 14));
    vdp.setColor(pal * 16 + 3, gs::rgb4(3, 2, 2));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_BOARD,
           {0, gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 5), gs::rgb4(13, 12, 10), gs::rgb4(15, 14, 12), gs::rgb4(12, 2, 2),
            gs::rgb4(15, 5, 4), gs::rgb4(1, 7, 3), gs::rgb4(3, 11, 5), gs::rgb4(14, 1, 1), gs::rgb4(2, 10, 3),
            gs::rgb4(14, 14, 13), gs::rgb4(4, 2, 1), gs::rgb4(9, 6, 3), gs::rgb4(14, 13, 11), gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_SLATE,
           {0, gs::rgb4(4, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(1, 2, 3), gs::rgb4(2, 3, 4)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 7), gs::rgb4(1, 2, 1));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_AIM, gs::rgb4(15, 12, 4), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 4), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 14, 6), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_LOSE, gs::rgb4(15, 4, 3), gs::rgb4(2, 0, 0));
    flightPal(vdp, PAL_DART0, gs::rgb4(15, 15, 13));
    flightPal(vdp, PAL_DART1, gs::rgb4(14, 10, 3));
    flightPal(vdp, PAL_DART2, gs::rgb4(4, 8, 15));
    setPal(vdp, PAL_CHALK, {0, gs::rgb4(15, 15, 13), gs::rgb4(6, 6, 5)});
    setPal(vdp, PAL_PIP, {0, gs::rgb4(15, 12, 3)});

    loadFont(vdp, art);
    art.board = gs::uploadImage(vdp, boardArt());
    art.slate = gs::uploadImage(vdp, slateArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("DARTMARK", {2, 1, 2, 0, 1}));
    art.win = gs::uploadImage(vdp, gs::textBitmap("FINISHED", {2, 1, 2, 0, 1}));
    art.lose = gs::uploadImage(vdp, gs::textBitmap("OPEN", {2, 1, 2, 0, 1}));
    art.dart = gs::uploadMipped(vdp, dartArt());
    art.cross = gs::uploadImage(vdp, crossArt());
    art.dot = gs::uploadImage(vdp, dotArt());
    art.slash = gs::uploadImage(vdp, slashArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
}

}  // namespace dartmark
