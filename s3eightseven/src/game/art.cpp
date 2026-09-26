#include "game/art.h"

#include <cmath>

namespace eightseven {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

// 3x5 digits, bits 0..2 left to right.
constexpr uint8_t kDig[10][5] = {
    {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
    {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
};

void digit(gs::Bitmap& b, int n, int x, int y) {
    if (n < 0 || n > 9) return;
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 3; col++) {
            if ((kDig[n][row] & (4 >> col)) == 0) continue;
            b.set(x + col, y + row, 2);
        }
    }
}

void paintBall(gs::Bitmap& b, int kind, int color, int number) {
    const float cx = 8.f, cy = 8.f;
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            float dx = x + 0.5f - cx;
            float dy = y + 0.5f - cy;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d > 7.15f) continue;
            int c = 2;
            if (d <= 6.35f) {
                if (kind == 0) c = dy > 3.2f ? 10 : 1;
                else if (kind == 3) c = 2;
                else if (kind == 2) c = std::fabs(dy) <= 2.15f ? color : 1;
                else c = color;
            }
            b.set(x, y, c);
        }
    }
    if (kind == 0) {
        b.ellipse(5.4f, 5.2f, 1.7f, 1.5f, 1);
        return;
    }
    b.ellipse(cx, cy, 3.15f, 3.15f, 1);
    if (number >= 10) {
        digit(b, number / 10, 4, 5);
        digit(b, number % 10, 8, 5);
    } else {
        digit(b, number, 6, 5);
    }
    b.ellipse(5.2f, 4.8f, 1.35f, 1.15f, 1);
}

int ballColor(int n) {
    if (n <= 0 || n == 8) return 2;
    int k = n >= 9 ? n - 8 : n;
    static const int c[8] = {0, 3, 4, 5, 6, 7, 8, 9};
    return c[k];
}

gs::Bitmap cueFlat() {
    gs::Bitmap b(42, 8);
    b.rect(0, 1, 7, 6, 6);
    b.rect(5, 2, 4, 4, 3);
    b.rect(8, 2, 22, 4, 2);
    b.rect(8, 2, 22, 1, 1);
    b.rect(30, 1, 5, 6, 4);
    b.rect(35, 1, 6, 6, 5);
    b.rect(40, 2, 2, 4, 1);
    return b;
}

gs::Bitmap rotateCue(const gs::Bitmap& src, float ang) {
    gs::Bitmap o(kCueBox, kCueBox);
    float ca = std::cos(ang), sa = std::sin(ang);
    float scx = src.w * 0.5f, scy = src.h * 0.5f;
    float oc = kCueBox * 0.5f;
    for (int y = 0; y < kCueBox; y++) {
        for (int x = 0; x < kCueBox; x++) {
            float dx = x + 0.5f - oc, dy = y + 0.5f - oc;
            float sx = scx + ca * dx + sa * dy;
            float sy = scy - sa * dx + ca * dy;
            int ix = int(std::floor(sx)), iy = int(std::floor(sy));
            int c = src.get(ix, iy);
            if (!c) c = src.get(ix + 1, iy);
            if (!c) c = src.get(ix, iy + 1);
            if (c) o.set(x, y, c);
        }
    }
    return o;
}

void diamond(gs::Bitmap& b, int x, int y) {
    b.set(x, y, 8);
    b.set(x - 1, y, 8);
    b.set(x + 1, y, 8);
    b.set(x, y - 1, 8);
    b.set(x, y + 1, 8);
}

void pocket(gs::Bitmap& b, float x, float y, float r) {
    b.ellipse(x, y, r + 2.3f, r + 2.3f, 7);
    b.ellipse(x, y, r, r, 6);
}

gs::Bitmap roomArt() {
    gs::Bitmap b(gs::SCREEN_W, gs::SCREEN_H);
    b.rect(0, 0, gs::SCREEN_W, gs::SCREEN_H, 4);
    b.rect(0, 0, gs::SCREEN_W, 16, 5);
    b.rect(0, 208, gs::SCREEN_W, 16, 5);
    b.rect(kTableX, kTableY, kTableW, kTableH, 2);
    b.rect(kTableX, kTableY, kTableW, 3, 1);
    b.rect(kTableX, kTableY, 3, kTableH, 1);
    b.rect(kTableX, kTableY + kTableH - 4, kTableW, 4, 3);
    b.rect(kTableX + kTableW - 4, kTableY, 4, kTableH, 3);
    const int fl = int(kFeltL), ft = int(kFeltT), fr = int(kFeltR), fb = int(kFeltB);
    for (int y = ft; y < fb; y++)
        for (int x = fl; x < fr; x++) b.set(x, y, 0);
    for (int x = fl; x < fr; x++) {
        b.set(x, ft, 1);
        b.set(x, fb - 1, 3);
    }
    for (int y = ft; y < fb; y++) {
        b.set(fl, y, 1);
        b.set(fr - 1, y, 3);
    }
    const int hx = int(kHeadX);
    for (int y = ft + 3; y < fb - 3; y += 2) b.set(hx, y, 9);
    b.ellipse(kHeadX, kMidY, 1.6f, 1.6f, 10);
    b.ellipse(kFootX, kMidY, 2.4f, 2.4f, 10);
    const int railsY[2] = {ft - 8, fb + 6};
    const float xs[4] = {0.22f, 0.38f, 0.62f, 0.78f};
    for (int s = 0; s < 2; s++)
        for (float u : xs) diamond(b, int(kFeltL + (kFeltR - kFeltL) * u), railsY[s]);
    const int railsX[2] = {fl - 8, fr + 6};
    for (int s = 0; s < 2; s++) {
        diamond(b, railsX[s], int(kFeltT + (kFeltB - kFeltT) * 0.30f));
        diamond(b, railsX[s], int(kFeltT + (kFeltB - kFeltT) * 0.70f));
    }
    for (int i = 0; i < 6; i++) {
        const Pocket& p = pocketAt(i);
        float r = (i == 1 || i == 4) ? 10.f : 11.f;
        pocket(b, p.x, p.y, r);
    }
    b.rect(kTableX + 6, kTableY + 6, kTableW - 12, 1, 11);
    b.rect(kTableX + 6, kTableY + kTableH - 7, kTableW - 12, 1, 11);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 3, 1)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 5, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_BALL,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 1), gs::rgb4(15, 13, 2), gs::rgb4(2, 6, 15), gs::rgb4(14, 2, 2),
            gs::rgb4(11, 3, 14), gs::rgb4(15, 8, 1), gs::rgb4(2, 12, 4), gs::rgb4(12, 2, 3), gs::rgb4(12, 12, 11)});
    setPal(vdp, PAL_TABLE,
           {0, gs::rgb4(14, 10, 6), gs::rgb4(10, 6, 3), gs::rgb4(5, 2, 1), gs::rgb4(3, 2, 4), gs::rgb4(2, 1, 3),
            gs::rgb4(1, 1, 1), gs::rgb4(4, 3, 2), gs::rgb4(15, 14, 10), gs::rgb4(11, 13, 9), gs::rgb4(15, 15, 12),
            gs::rgb4(13, 10, 4)});
    setPal(vdp, PAL_CUE,
           {0, gs::rgb4(15, 13, 8), gs::rgb4(12, 8, 3), gs::rgb4(6, 3, 1), gs::rgb4(15, 15, 13), gs::rgb4(3, 8, 14),
            gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_GLOW, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 12), gs::rgb4(10, 6, 1)});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 14, 8), gs::rgb4(5, 3, 1), gs::rgb4(2, 1, 2)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(15, 15, 12), gs::rgb4(6, 4, 1), gs::rgb4(2, 2, 1)});
    vdp.setFogColor(gs::rgb4(0, 2, 1));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    gs::bitmapToPlane(tiles, vdp.A, 0, 0, roomArt(), PAL_TABLE);
    vdp.A.enabled = true;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;

    for (int n = 0; n < 16; n++) {
        gs::Bitmap bm(16, 16);
        int kind = n == 0 ? 0 : n == 8 ? 3 : n >= 9 ? 2 : 1;
        paintBall(bm, kind, ballColor(n), n);
        art.ball[n] = gs::uploadMipped(vdp, bm);
    }
    gs::Bitmap sh(16, 8);
    sh.ellipse(8, 4, 6.0f, 2.3f, 1);
    art.shadow = gs::uploadMipped(vdp, sh);

    gs::Bitmap flat = cueFlat();
    for (int i = 0; i < kCueAngles; i++) {
        float a = float(i) * 6.2831853f / float(kCueAngles);
        art.cue[i] = gs::uploadMipped(vdp, rotateCue(flat, a));
    }

    gs::Bitmap dot(5, 5);
    dot.ellipse(2.5f, 2.5f, 1.8f, 1.8f, 1);
    art.dot = gs::uploadMipped(vdp, dot);

    gs::Bitmap glow(22, 22);
    glow.ellipse(11, 11, 9.4f, 9.4f, 1);
    glow.ellipse(11, 11, 6.0f, 6.0f, 0);
    glow.ellipse(11, 11, 5.0f, 5.0f, 2);
    glow.ellipse(11, 11, 3.1f, 3.1f, 0);
    art.glow = gs::uploadMipped(vdp, glow);

    art.logo = gs::uploadMipped(vdp, gs::textBitmap("EIGHT", {2, 1, 2, 3, 1}));
    art.sub = gs::uploadMipped(vdp, gs::textBitmap("TO SEVEN", {2, 1, 2, 3, 1}));
    art.leave = gs::uploadMipped(vdp, gs::textBitmap("LEAVE", {2, 1, 2, 3, 1}));
    art.lost = gs::uploadMipped(vdp, gs::textBitmap("SHORT", {2, 1, 2, 3, 1}));
}

}  // namespace eightseven
