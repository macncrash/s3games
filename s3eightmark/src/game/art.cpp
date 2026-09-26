#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace eightmark {
namespace {

constexpr float kPi = 3.14159265f;

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
            if (d > 7.2f) continue;
            int c = 2;
            if (d <= 6.15f) {
                if (kind == 0) c = dy > 3.2f ? 11 : 1;
                else if (kind == 3) c = d <= 5.15f ? 1 : 2;
                else if (kind == 2) c = std::fabs(dy) <= 2.15f ? color : 1;
                else c = color;
                float hx = dx + 2.6f, hy = dy + 2.8f;
                if (kind != 3 && hx * hx + hy * hy < 2.8f) c = 10;
                if (kind == 3 && d > 5.3f && hx * hx + hy * hy < 1.7f) c = 10;
            }
            b.set(x, y, c);
        }
    }
    if (kind == 3) {
        static const char* g[7] = {" ### ", "#   #", "#   #", " ### ", "#   #", "#   #", " ### "};
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y][x] == '#') b.set(5 + x, 4 + y, 2);
    }
}

gs::Bitmap cueFlat() {
    gs::Bitmap b(36, 8);
    b.rect(0, 2, 7, 4, 3);
    b.rect(6, 1, 20, 6, 2);
    b.rect(6, 1, 20, 2, 1);
    b.rect(26, 1, 5, 6, 4);
    b.rect(31, 1, 5, 6, 5);
    b.rect(1, 3, 2, 2, 6);
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
    b.set(x - 1, y, 7);
    b.set(x + 1, y, 7);
    b.set(x, y - 1, 7);
    b.set(x, y + 1, 7);
}

void pocket(gs::Bitmap& b, float x, float y, float r) {
    b.ellipse(x, y, r + 2.2f, r + 2.2f, 6);
    b.ellipse(x, y, r, r, 5);
}

gs::Bitmap roomArt() {
    gs::Bitmap b(gs::SCREEN_W, gs::SCREEN_H);
    b.rect(0, 0, gs::SCREEN_W, gs::SCREEN_H, 4);
    b.rect(0, 0, gs::SCREEN_W, 8, 12);
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
    b.ellipse(kEightX, kEightY, 2.2f, 2.2f, 10);
    b.ellipse(kCueX, kCueY, 1.6f, 1.6f, 9);
    const float us[4] = {0.22f, 0.38f, 0.62f, 0.78f};
    for (float u : us) diamond(b, int(kFeltL + (kFeltR - kFeltL) * u), ft - 8);
    for (float u : us) diamond(b, int(kFeltL + (kFeltR - kFeltL) * u), fb + 7);
    const float vs[2] = {0.30f, 0.70f};
    for (float v : vs) {
        int y = int(kFeltT + (kFeltB - kFeltT) * v);
        diamond(b, fl - 7, y);
        diamond(b, fr + 6, y);
    }
    pocket(b, 32, 36, 12);
    pocket(b, 288, 36, 12);
    pocket(b, 32, 184, 12);
    pocket(b, 288, 184, 12);
    pocket(b, 160, 30, 11);
    pocket(b, 160, 190, 11);
    b.rect(kTableX + 6, kTableY + 6, kTableW - 12, 1, 11);
    b.rect(kTableX + 6, kTableY + kTableH - 7, kTableW - 12, 1, 11);
    gs::Bitmap plate = gs::textBitmap("MARK", {1, 7, 0, 0, 1});
    b.blit(plate, 198, 189);
    return b;
}

gs::Bitmap coinArt() {
    gs::Bitmap b(12, 12);
    for (int y = 0; y < 12; y++) {
        for (int x = 0; x < 12; x++) {
            float dx = x + 0.5f - 6.f, dy = y + 0.5f - 6.f;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d > 5.2f) continue;
            int c = d > 4.15f ? 3 : 2;
            if (dx * dx + dy * dy < 1.3f) c = 4;
            if ((dx + 1.8f) * (dx + 1.8f) + (dy + 1.8f) * (dy + 1.8f) < 1.5f) c = 1;
            b.set(x, y, c);
        }
    }
    return b;
}

gs::Bitmap gloveArt() {
    gs::Bitmap b(16, 16);
    b.rect(4, 2, 8, 3, 1);
    b.rect(3, 5, 10, 5, 1);
    b.rect(5, 6, 2, 3, 2);
    b.rect(9, 6, 2, 3, 2);
    b.rect(3, 10, 10, 3, 3);
    b.rect(4, 10, 8, 1, 1);
    b.set(4, 4, 2);
    b.set(11, 4, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 3)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 3, 1)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_BALL,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 2), gs::rgb4(15, 12, 1), gs::rgb4(2, 5, 14), gs::rgb4(13, 2, 2),
            gs::rgb4(10, 3, 13), gs::rgb4(15, 8, 1), gs::rgb4(2, 11, 4), gs::rgb4(10, 2, 3), gs::rgb4(15, 15, 12),
            gs::rgb4(11, 12, 13)});
    setPal(vdp, PAL_TABLE,
           {0, gs::rgb4(13, 8, 4), gs::rgb4(8, 4, 2), gs::rgb4(4, 2, 1), gs::rgb4(2, 2, 4), gs::rgb4(1, 1, 1),
            gs::rgb4(5, 3, 2), gs::rgb4(14, 11, 4), gs::rgb4(15, 14, 8), gs::rgb4(8, 12, 7), gs::rgb4(13, 15, 11),
            gs::rgb4(11, 8, 3), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_CUE,
           {0, gs::rgb4(15, 12, 7), gs::rgb4(12, 7, 3), gs::rgb4(6, 3, 1), gs::rgb4(15, 15, 13), gs::rgb4(3, 8, 14),
            gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_GLOW, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12), gs::rgb4(10, 7, 2)});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 13, 5), gs::rgb4(4, 2, 1), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_COIN, {0, gs::rgb4(15, 15, 10), gs::rgb4(13, 10, 2), gs::rgb4(8, 5, 1), gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(14, 10, 7), gs::rgb4(9, 6, 4), gs::rgb4(3, 4, 8)});
    vdp.setFogColor(gs::rgb4(0, 2, 1));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    gs::bitmapToPlane(tiles, vdp.A, 0, 0, roomArt(), PAL_TABLE);
    vdp.A.enabled = true;
    vdp.B.enabled = false;

    for (int n = 0; n < kBalls; n++) {
        Home h = homeAt(n);
        gs::Bitmap ball(16, 16);
        paintBall(ball, h.kind, h.color);
        art.ball[n] = gs::uploadMipped(vdp, ball);
    }
    gs::Bitmap sh(14, 7);
    sh.ellipse(7, 3.5f, 5.4f, 2.2f, 1);
    art.shadow = gs::uploadMipped(vdp, sh);

    gs::Bitmap flat = cueFlat();
    for (int i = 0; i < kCueAngles; i++) {
        float a = float(i) * (kPi * 2.f) / float(kCueAngles);
        art.cue[i] = gs::uploadMipped(vdp, rotateCue(flat, a));
    }

    gs::Bitmap dot(5, 5);
    dot.ellipse(2.5f, 2.5f, 1.8f, 1.8f, 1);
    art.dot = gs::uploadMipped(vdp, dot);

    gs::Bitmap glow(22, 22);
    glow.ellipse(11, 11, 9.4f, 9.4f, 1);
    glow.ellipse(11, 11, 6.4f, 6.4f, 0);
    glow.ellipse(11, 11, 5.2f, 5.2f, 2);
    glow.ellipse(11, 11, 3.2f, 3.2f, 0);
    art.glow = gs::uploadMipped(vdp, glow);

    art.coin = gs::uploadMipped(vdp, coinArt());
    art.glove = gs::uploadMipped(vdp, gloveArt());
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("EIGHT", {2, 1, 2, 3, 1}));
    art.sub = gs::uploadMipped(vdp, gs::textBitmap("THE MARK", {2, 1, 2, 3, 1}));
    art.win = gs::uploadMipped(vdp, gs::textBitmap("FINISHED", {2, 1, 2, 3, 1}));
    art.lose = gs::uploadMipped(vdp, gs::textBitmap("NO MARK", {2, 1, 2, 3, 1}));
}

}  // namespace eightmark
