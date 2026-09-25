#include "game/art.h"

#include <cmath>
#include <cstdint>

namespace eight {
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

// kind: 0 cue, 1 solid, 2 stripe, 3 eight. color is a PAL_BALL index.
void paintBall(gs::Bitmap& b, int kind, int color) {
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            float dx = x + 0.5f - 8.f;
            float dy = y + 0.5f - 8.f;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d > 7.15f) continue;
            int c = 2;
            if (d <= 6.05f) {
                if (kind == 0) c = dy > 3.4f ? 10 : 1;
                else if (kind == 3) c = d <= 4.15f ? 1 : 2;
                else if (kind == 2) c = std::fabs(dy) <= 2.2f ? color : 1;
                else c = color;
                float hx = dx + 2.5f, hy = dy + 2.7f;
                if (kind != 3 && hx * hx + hy * hy < 3.1f) c = 1;
                if (kind == 3 && d > 4.4f && hx * hx + hy * hy < 2.2f) c = 10;
            }
            b.set(x, y, c);
        }
    }
    if (kind == 3) {
        static const char* g[5] = {"####", "#  #", "####", "#  #", "####"};
        for (int y = 0; y < 5; y++)
            for (int x = 0; x < 4; x++)
                if (g[y][x] == '#') b.set(6 + x, 6 + y, 2);
    }
}

gs::Bitmap cueFlat() {
    gs::Bitmap b(40, 10);
    b.rect(0, 2, 8, 6, 6);
    b.rect(6, 3, 24, 4, 2);
    b.rect(6, 3, 24, 1, 1);
    b.rect(30, 2, 5, 6, 4);
    b.rect(35, 2, 5, 6, 5);
    b.rect(1, 3, 2, 4, 3);
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
    b.ellipse(x, y, r + 2.4f, r + 2.4f, 7);
    b.ellipse(x, y, r, r, 6);
}

gs::Bitmap roomArt() {
    gs::Bitmap b(gs::SCREEN_W, gs::SCREEN_H);
    b.rect(0, 0, gs::SCREEN_W, gs::SCREEN_H, 4);
    b.rect(kTableX, kTableY, kTableW, kTableH, 2);
    b.rect(kTableX, kTableY, kTableW, 3, 1);
    b.rect(kTableX, kTableY, 3, kTableH, 1);
    b.rect(kTableX, kTableY + kTableH - 4, kTableW, 4, 3);
    b.rect(kTableX + kTableW - 4, kTableY, 4, kTableH, 3);
    // Cloth is transparent so the per-line backdrop shows through, and shadows land on it.
    const int fl = int(kFeltL), ft = int(kFeltT), fr = int(kFeltR), fb = int(kFeltB);
    for (int y = ft; y < fb; y++)
        for (int x = fl; x < fr; x++) b.set(x, y, 0);
    for (int x = fl; x < fr; x++) {
        b.set(x, ft - 1, 1);
        b.set(x, fb, 3);
    }
    for (int y = ft; y < fb; y++) {
        b.set(fl - 1, y, 1);
        b.set(fr, y, 3);
    }
    const int hx = int(kHeadX);
    for (int y = ft + 2; y < fb - 2; y += 2) b.set(hx, y, 9);
    b.ellipse(kHeadX, kMidY, 1.7f, 1.7f, 10);
    b.ellipse(kFootX, kMidY, 2.3f, 2.3f, 10);
    const int railsY[2] = {ft - 8, fb + 7};
    const float xs[4] = {0.28f, 0.40f, 0.60f, 0.72f};
    for (int s = 0; s < 2; s++)
        for (float u : xs) diamond(b, int(kFeltL + (kFeltR - kFeltL) * u), railsY[s]);
    const int railsX[2] = {fl - 8, fr + 7};
    for (int s = 0; s < 2; s++) {
        diamond(b, railsX[s], int(kFeltT + (kFeltB - kFeltT) * 0.32f));
        diamond(b, railsX[s], int(kFeltT + (kFeltB - kFeltT) * 0.68f));
    }
    pocket(b, 24, 32, 11);
    pocket(b, 296, 32, 11);
    pocket(b, 24, 192, 11);
    pocket(b, 296, 192, 11);
    pocket(b, 160, 28, 10);
    pocket(b, 160, 196, 10);
    // Cabinet inlay just inside the outer wood.
    b.rect(kTableX + 6, kTableY + 6, kTableW - 12, 1, 11);
    b.rect(kTableX + 6, kTableY + kTableH - 7, kTableW - 12, 1, 11);
    return b;
}

int ballColor(int n) {
    int k = n >= 9 ? n - 8 : n;
    static const int c[8] = {0, 3, 4, 5, 6, 7, 8, 9};
    return c[k];
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 15);
    setPal(vdp, PAL_HUD, {0, ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 3)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 3, 1)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_BALL,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 1), gs::rgb4(15, 13, 1), gs::rgb4(2, 6, 15), gs::rgb4(14, 2, 2),
            gs::rgb4(11, 3, 14), gs::rgb4(15, 8, 1), gs::rgb4(2, 12, 3), gs::rgb4(11, 2, 3), gs::rgb4(12, 12, 11)});
    setPal(vdp, PAL_TABLE,
           {0, gs::rgb4(13, 8, 4), gs::rgb4(9, 5, 2), gs::rgb4(5, 2, 1), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2),
            gs::rgb4(1, 1, 1), gs::rgb4(4, 2, 1), gs::rgb4(14, 13, 9), gs::rgb4(11, 13, 9), gs::rgb4(15, 15, 12),
            gs::rgb4(13, 10, 3)});
    setPal(vdp, PAL_CUE,
           {0, gs::rgb4(14, 11, 6), gs::rgb4(11, 7, 3), gs::rgb4(6, 3, 1), gs::rgb4(15, 15, 13), gs::rgb4(3, 7, 14),
            gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_GLOW, {0, gs::rgb4(15, 14, 5), gs::rgb4(15, 15, 13), gs::rgb4(12, 8, 2)});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 14, 8), gs::rgb4(5, 3, 1), gs::rgb4(1, 1, 2)});
    vdp.setFogColor(gs::rgb4(0, 2, 1));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    gs::bitmapToPlane(tiles, vdp.A, 0, 0, roomArt(), PAL_TABLE);
    vdp.A.enabled = true;
    vdp.B.enabled = false;

    for (int n = 0; n < 16; n++) {
        gs::Bitmap b(16, 16);
        int kind = n == 0 ? 0 : n == 8 ? 3 : n >= 9 ? 2 : 1;
        paintBall(b, kind, ballColor(n));
        art.ball[n] = gs::uploadMipped(vdp, b);
    }
    gs::Bitmap sh(16, 8);
    sh.ellipse(8, 4, 6.2f, 2.4f, 1);
    art.shadow = gs::uploadMipped(vdp, sh);

    gs::Bitmap flat = cueFlat();
    for (int i = 0; i < kCueAngles; i++) {
        float a = float(i) * 6.2831853f / float(kCueAngles);
        art.cue[i] = gs::uploadMipped(vdp, rotateCue(flat, a));
    }

    gs::Bitmap dot(5, 5);
    dot.ellipse(2.5f, 2.5f, 2.0f, 2.0f, 1);
    art.dot = gs::uploadMipped(vdp, dot);

    gs::Bitmap glow(22, 22);
    glow.ellipse(11, 11, 9.5f, 9.5f, 1);
    glow.ellipse(11, 11, 6.2f, 6.2f, 0);
    glow.ellipse(11, 11, 5.2f, 5.2f, 2);
    glow.ellipse(11, 11, 3.4f, 3.4f, 0);
    art.glow = gs::uploadMipped(vdp, glow);

    art.logo = gs::uploadMipped(vdp, gs::textBitmap("S3 EIGHT", {2, 1, 2, 3, 1}));
    art.sub = gs::uploadMipped(vdp, gs::textBitmap("ONE RACK", {2, 1, 2, 3, 1}));
    art.win = gs::uploadMipped(vdp, gs::textBitmap("RACK CLEAR", {2, 1, 2, 3, 1}));
    art.lose = gs::uploadMipped(vdp, gs::textBitmap("RACK LOST", {2, 1, 2, 3, 1}));
}

}  // namespace eight
