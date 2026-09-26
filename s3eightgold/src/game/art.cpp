#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace eightgold {
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

void paintBall(gs::Bitmap& b, int kind, int color) {
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            float dx = x + 0.5f - 8.f;
            float dy = y + 0.5f - 8.f;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d > 7.15f) continue;
            int c = 2;
            if (d <= 6.15f) {
                if (kind == 0) c = dy > 3.2f ? 10 : 1;
                else if (kind == 8) {
                    c = 2;
                    if (d >= 5.15f) c = 3;
                    if (d <= 3.35f) c = 1;
                } else {
                    c = color;
                }
                float hx = dx + 2.4f, hy = dy + 2.6f;
                if (kind != 8 && hx * hx + hy * hy < 3.0f) c = 1;
                if (kind == 8 && d > 4.6f && hx * hx + hy * hy < 2.0f) c = 12;
            }
            b.set(x, y, c);
        }
    }
    if (kind == 8) {
        static const char* g[5] = {"###", "# #", "###", "# #", "###"};
        for (int y = 0; y < 5; y++)
            for (int x = 0; x < 3; x++)
                if (g[y][x] == '#') b.set(7 + x, 6 + y, 2);
    }
}

gs::Bitmap cueFlat() {
    gs::Bitmap b(40, 10);
    b.rect(0, 3, 6, 4, 6);
    b.rect(5, 2, 22, 6, 2);
    b.rect(5, 2, 22, 1, 1);
    b.rect(27, 2, 4, 6, 3);
    b.rect(31, 3, 6, 4, 4);
    b.rect(36, 3, 4, 4, 5);
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

void diamond(gs::Bitmap& b, int x, int y, int c) {
    b.set(x, y, c);
    b.set(x - 1, y, c);
    b.set(x + 1, y, c);
    b.set(x, y - 1, c);
    b.set(x, y + 1, c);
}

void pocket(gs::Bitmap& b, float x, float y, float r, bool gold) {
    int ring = gold ? 12 : 13;
    if (gold) b.ellipse(x, y, r + 5.2f, r + 5.2f, ring);
    b.ellipse(x, y, r + 2.8f, r + 2.8f, ring);
    b.ellipse(x, y, r + 1.15f, r + 1.15f, 7);
    b.ellipse(x, y, r * 0.78f, r * 0.78f, 6);
}

void digit(gs::Bitmap& b, int x, int y, char ch, int c) {
    const uint8_t* g = gs::glyph(ch);
    if (!g) return;
    for (int gy = 0; gy < 7; gy++)
        for (int gx = 0; gx < 5; gx++)
            if (g[gy * 5 + gx]) b.set(x + gx, y + gy, c);
}

gs::Bitmap roomArt() {
    gs::Bitmap b(gs::SCREEN_W, gs::SCREEN_H);
    b.rect(0, 0, gs::SCREEN_W, gs::SCREEN_H, 4);
    b.rect(kTableX, kTableY, kTableW, kTableH, 2);
    b.rect(kTableX, kTableY, kTableW, 4, 1);
    b.rect(kTableX, kTableY, 4, kTableH, 1);
    b.rect(kTableX, kTableY + kTableH - 5, kTableW, 5, 3);
    b.rect(kTableX + kTableW - 5, kTableY, 5, kTableH, 3);
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
    for (int y = ft + 4; y < fb - 4; y += 3) b.set(hx, y, 9);
    b.ellipse(kHeadX, kMidY, 1.6f, 1.6f, 10);
    b.ellipse(kFootX, kMidY, 2.2f, 2.2f, 10);
    b.rect(kTableX + 8, kTableY + 8, kTableW - 16, 1, 11);
    b.rect(kTableX + 8, kTableY + kTableH - 9, kTableW - 16, 1, 11);
    const int railsY[2] = {ft - 10, fb + 8};
    const float xs[3] = {0.36f, 0.50f, 0.64f};
    for (int s = 0; s < 2; s++)
        for (float u : xs) diamond(b, int(kFeltL + (kFeltR - kFeltL) * u), railsY[s], 8);
    pocket(b, 24, 32, 11, true);
    pocket(b, 296, 32, 11, true);
    pocket(b, 24, 192, 11, true);
    pocket(b, 296, 192, 11, true);
    pocket(b, 160, 28, 10, false);
    pocket(b, 160, 196, 10, false);
    digit(b, 46, 18, '2', 12);
    digit(b, 258, 18, '2', 12);
    digit(b, 46, 198, '2', 12);
    digit(b, 258, 198, '2', 12);
    digit(b, 176, 18, '1', 13);
    digit(b, 176, 200, '1', 13);
    return b;
}

int ballColor(int n) {
    static const int c[8] = {3, 5, 6, 8, 7, 9, 4, 2};
    if (n < 1 || n > 8) return 1;
    return c[n - 1];
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 3), gs::rgb4(12, 8, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 3, 1)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 14, 11), gs::rgb4(12, 10, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_BALL,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 1), gs::rgb4(15, 12, 2), gs::rgb4(14, 12, 8), gs::rgb4(3, 6, 14),
            gs::rgb4(13, 2, 2), gs::rgb4(12, 4, 13), gs::rgb4(15, 8, 1), gs::rgb4(2, 11, 4), gs::rgb4(11, 11, 10),
            gs::rgb4(8, 5, 1), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_TABLE,
           {0, gs::rgb4(14, 9, 4), gs::rgb4(10, 6, 2), gs::rgb4(5, 3, 1), gs::rgb4(2, 1, 2), gs::rgb4(1, 1, 1),
            gs::rgb4(1, 1, 1), gs::rgb4(4, 2, 1), gs::rgb4(13, 10, 4), gs::rgb4(10, 12, 8), gs::rgb4(15, 14, 10),
            gs::rgb4(12, 9, 3), gs::rgb4(15, 12, 2), gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_CUE,
           {0, gs::rgb4(15, 12, 6), gs::rgb4(12, 7, 3), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 13), gs::rgb4(4, 8, 13),
            gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 13, 4), gs::rgb4(5, 3, 1), gs::rgb4(2, 1, 1)});
    vdp.setFogColor(gs::rgb4(0, 2, 1));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    gs::bitmapToPlane(tiles, vdp.A, 0, 0, roomArt(), PAL_TABLE);
    vdp.A.enabled = true;
    vdp.B.enabled = false;
    vdp.A.scroll(0, 0);

    for (int n = 0; n <= kRack; n++) {
        gs::Bitmap ball(16, 16);
        int kind = n == 0 ? 0 : n;
        paintBall(ball, kind, ballColor(n));
        art.ball[n] = gs::uploadMipped(vdp, ball);
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

    gs::Bitmap glow(24, 24);
    glow.ellipse(12, 12, 10.5f, 10.5f, 1);
    glow.ellipse(12, 12, 7.2f, 7.2f, 0);
    glow.ellipse(12, 12, 6.0f, 6.0f, 2);
    glow.ellipse(12, 12, 3.6f, 3.6f, 0);
    art.glow = gs::uploadMipped(vdp, glow);

    const gs::TextStyle st{2, 1, 2, 3, 1};
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("S3 EIGHT GOLD", st));
    art.sub = gs::uploadMipped(vdp, gs::textBitmap("SHORT EIGHT", st));
    art.win = gs::uploadMipped(vdp, gs::textBitmap("DOUBLE", {3, 1, 2, 3, 1}));
    art.tag = gs::uploadMipped(vdp, gs::textBitmap("GOLD x2", st));
}

}  // namespace eightgold
