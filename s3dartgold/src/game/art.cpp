#include "game/art.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace dartgold {
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

bool onWire(float dx, float dy, float r) {
    const float rings[] = {kBullIn, kBullOut, kTripIn, kTripOut, kDoubIn, kDoubOut};
    for (float rr : rings)
        if (std::fabs(r - rr) <= 0.55f) return true;
    if (r <= kBullOut || r > kDoubOut) return false;
    float deg = std::atan2(dx, -dy) * (180.f / 3.14159265f);
    if (deg < 0.f) deg += 360.f;
    float along = std::fmod(deg + 9.f, 18.f);
    float gap = std::min(along, 18.f - along);
    return gap * 0.017453292f * r <= 0.6f;
}

int wood(int x, int y, float r) {
    if (r > kRim - 2.1f) return 13;
    uint32_t h = uint32_t(x) * 2246822519u ^ uint32_t(y) * 3266489917u;
    h ^= h >> 15;
    int n = int(h & 7u);
    if (n == 0) return 10;
    if (n < 3) return 12;
    return 11;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        bool any = false;
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    any = true;
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        if (!any) {
            a.font[c - 32] = 0;
            continue;
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Bitmap boardArt() {
    gs::Bitmap b(kBoard, kBoard);
    for (int y = 0; y < kBoard; y++) {
        for (int x = 0; x < kBoard; x++) {
            float dx = (x + 0.5f) - kBmpC;
            float dy = (y + 0.5f) - kBmpC;
            float r = std::hypot(dx, dy);
            if (r > kRim) continue;
            int c = r > kDoubOut ? wood(x, y, r) : zoneAt(dx, dy).paint;
            if (r <= kDoubOut + 0.8f && onWire(dx, dy, r)) c = kPaintWire;
            b.set(x, y, c);
        }
    }
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
    gs::Bitmap b(17, 17);
    b.line(4, 1, 8, 6, 1, 1.5f);
    b.line(12, 1, 8, 6, 1, 1.5f);
    b.line(6, 2, 8, 6, 1, 1.1f);
    b.line(10, 2, 8, 6, 1, 1.1f);
    b.line(8, 5, 8, 8, 2, 2.3f);
    b.set(7, 6, 2);
    b.set(9, 6, 2);
    b.outline(3, false);
    b.set(8, 8, 4);
    return b;
}

gs::Bitmap crossArt() {
    gs::Bitmap b(13, 13);
    b.rect(6, 0, 1, 4, 1);
    b.rect(6, 9, 1, 4, 1);
    b.rect(0, 6, 4, 1, 1);
    b.rect(9, 6, 4, 1, 1);
    b.set(6, 6, 1);
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

}  // namespace

Zone zoneAt(float dx, float dy) {
    Zone z;
    float r = std::hypot(dx, dy);
    if (r > kDoubOut) return z;
    if (r <= kBullIn) {
        z.score = 50;
        z.face = 50;
        z.dbl = true;
        z.gold = true;
        z.paint = kPaintGold;
        z.kind = 'B';
        return z;
    }
    if (r <= kBullOut) {
        z.score = 25;
        z.face = 25;
        z.paint = kPaintPale;
        z.kind = 'O';
        return z;
    }
    float deg = std::atan2(dx, -dy) * (180.f / 3.14159265f);
    if (deg < 0.f) deg += 360.f;
    int s = int(std::floor((deg + 9.f) / 18.f));
    if (s < 0 || s >= 20) s = 0;
    z.face = kSeg[s];
    z.gold = goldSector(s);
    z.score = z.face;
    z.kind = 'S';
    z.paint = z.gold ? kPaintBlack : kPaintCream;
    if (r > kTripIn && r <= kTripOut) {
        z.score = z.face * 3;
        z.kind = 'T';
        z.paint = z.gold ? kPaintRed : kPaintGreen;
        return z;
    }
    if (r > kDoubIn) {
        // Only the gold arc of this ring is a double. Cream keeps the face.
        if (z.gold) {
            z.score = z.face * 2;
            z.dbl = true;
            z.kind = 'D';
            z.paint = kPaintGold;
        } else {
            z.kind = 'C';
            z.paint = kPaintPale;
        }
    }
    return z;
}

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_BOARD,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5), gs::rgb4(14, 12, 8), gs::rgb4(15, 14, 10), gs::rgb4(15, 12, 2),
            gs::rgb4(8, 8, 7), gs::rgb4(13, 2, 2), gs::rgb4(1, 9, 3), gs::rgb4(14, 14, 12), gs::rgb4(5, 3, 1),
            gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3), gs::rgb4(3, 2, 1), gs::rgb4(15, 13, 4), gs::rgb4(15, 14, 12)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 3), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_CREAM, gs::rgb4(12, 11, 8), gs::rgb4(2, 2, 1));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 6), gs::rgb4(0, 2, 1));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_DIM, gs::rgb4(9, 8, 7), gs::rgb4(1, 1, 1));
    textPal(vdp, PAL_SCORE, gs::rgb4(15, 14, 6), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 3), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_BUST, gs::rgb4(15, 4, 3), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 14, 5), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_AIM, gs::rgb4(15, 13, 4), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_SWEET, gs::rgb4(6, 15, 7), gs::rgb4(0, 2, 1));

    auto flight = [&](int pal, uint16_t c) {
        for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
        vdp.setColor(pal * 16 + 1, c);
        vdp.setColor(pal * 16 + 2, gs::rgb4(12, 12, 13));
        vdp.setColor(pal * 16 + 3, gs::rgb4(1, 1, 1));
        vdp.setColor(pal * 16 + 4, gs::rgb4(15, 15, 14));
    };
    flight(PAL_DART0, gs::rgb4(14, 3, 2));
    flight(PAL_DART1, gs::rgb4(15, 12, 3));
    flight(PAL_DART2, gs::rgb4(3, 7, 14));

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
    art.title = gs::uploadImage(vdp, gs::textBitmap("DART GOLD", {2, 1, 2, 0, 1}));
    art.bust = gs::uploadImage(vdp, gs::textBitmap("BUST", {3, 1, 2, 0, 1}));
    art.win = gs::uploadImage(vdp, gs::textBitmap("LEFT", {3, 1, 2, 0, 1}));
    art.dart = gs::uploadMipped(vdp, dartArt());
    art.cross = gs::uploadImage(vdp, crossArt());
    art.dot = gs::uploadImage(vdp, dotArt());
}

}  // namespace dartgold
