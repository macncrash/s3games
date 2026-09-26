#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace eightbell {
namespace {

constexpr float kTau = 6.2831853f;

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
    const float cx = b.w * 0.5f, cy = b.h * 0.5f;
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            float dx = x + 0.5f - cx;
            float dy = y + 0.5f - cy;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d > 7.2f) continue;
            int c = 2;
            if (d < 6.2f) {
                if (kind == 0) c = dy > 2.8f ? 9 : 1;
                else if (kind == 3) c = d < 4.5f ? 1 : 2;
                else if (kind == 2) c = std::fabs(dy) < 2.15f ? color : 1;
                else c = color;
                float hx = dx + 2.2f, hy = dy + 2.4f;
                if (kind != 3 && hx * hx + hy * hy < 2.6f) c = 10;
                if (kind == 3 && d > 4.0f && hx * hx + hy * hy < 1.8f) c = 10;
            }
            b.set(x, y, c);
        }
    }
    if (kind == 3) {
        static const char* g[5] = {"###", "# #", "###", "# #", "###"};
        for (int y = 0; y < 5; y++)
            for (int x = 0; x < 3; x++)
                if (g[y][x] == '#') b.set(6 + x, 5 + y, 2);
    }
}

gs::Bitmap cueFlat() {
    gs::Bitmap b(46, 8);
    b.rect(0, 2, 7, 4, 6);
    b.rect(6, 1, 28, 6, 2);
    b.rect(6, 1, 28, 2, 1);
    b.rect(34, 1, 6, 6, 5);
    b.rect(40, 1, 5, 6, 4);
    b.rect(1, 3, 2, 2, 3);
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

gs::Bitmap bellArt() {
    gs::Bitmap b(36, 32);
    b.rect(16, 0, 4, 5, 6);
    b.ellipse(18.f, 5.f, 3.2f, 2.6f, 6);
    b.ellipse(18.f, 16.f, 14.f, 12.f, 3);
    b.ellipse(18.f, 15.f, 11.f, 9.2f, 2);
    b.ellipse(16.f, 13.f, 5.f, 4.2f, 5);
    b.ellipse(18.f, 24.5f, 13.2f, 3.6f, 4);
    b.ellipse(18.f, 23.5f, 8.4f, 2.4f, 1);
    b.line(8.f, 12.f, 12.f, 20.f, 5, 1.1f);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(7, 12);
    b.line(3, 0, 3, 6, 7, 1.2f);
    b.ellipse(3.f, 8.5f, 2.4f, 2.4f, 7);
    return b;
}

gs::Bitmap yokeArt() {
    gs::Bitmap b(28, 8);
    b.rect(0, 2, 28, 3, 3);
    b.rect(12, 0, 4, 7, 2);
    b.ellipse(14.f, 6.f, 2.2f, 1.6f, 6);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 18);
    b.rect(5, 0, 2, 6, 3);
    b.ellipse(6.f, 9.f, 5.f, 3.4f, 2);
    b.ellipse(6.f, 9.f, 2.4f, 1.6f, 4);
    b.rect(2, 15, 8, 2, 3);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(8, 10);
    b.ellipse(4.f, 6.2f, 2.8f, 3.6f, 1);
    b.ellipse(4.f, 6.6f, 1.4f, 2.2f, 2);
    b.set(4, 6, 3);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(28, 16);
    b.ellipse(14.f, 8.f, 12.f, 6.5f, 1);
    b.ellipse(14.f, 8.f, 7.5f, 3.6f, 0);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(5, 5);
    b.ellipse(2.5f, 2.5f, 2.f, 2.f, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(14, 6);
    b.ellipse(7.f, 3.f, 6.f, 2.2f, 1);
    return b;
}

gs::Bitmap roomArt() {
    gs::Bitmap b(gs::SCREEN_W, gs::SCREEN_H);
    b.rect(0, 0, gs::SCREEN_W, gs::SCREEN_H, 4);
    b.rect(0, 0, gs::SCREEN_W, 28, 5);
    b.rect(kTableX, kTableY, kTableW, kTableH, 2);
    b.rect(kTableX, kTableY, kTableW, 4, 1);
    b.rect(kTableX, kTableY, 4, kTableH, 1);
    b.rect(kTableX, kTableY + kTableH - 5, kTableW, 5, 3);
    b.rect(kTableX + kTableW - 5, kTableY, 5, kTableH, 3);
    const int fl = int(kFeltL), ft = int(kFeltT), fr = int(kFeltR), fb = int(kFeltB);
    for (int y = ft; y < fb; y++)
        for (int x = fl; x < fr; x++) b.set(x, y, 0);
    for (int x = fl; x < fr; x++) {
        b.set(x, ft - 1, 13);
        b.set(x, fb, 3);
    }
    for (int y = ft; y < fb; y++) {
        b.set(fl - 1, y, 13);
        b.set(fr, y, 3);
    }
    const float spanY = kFeltB - kFeltT;
    const int head = int(kFeltB - spanY * 0.25f);
    for (int x = fl + 10; x < fr - 10; x += 2) b.set(x, head, 9);
    b.ellipse(160.f, kFeltT + spanY * 0.25f, 2.2f, 2.2f, 10);
    b.ellipse(160.f, (kFeltT + kFeltB) * 0.5f, 1.6f, 1.6f, 9);
    const int railsY[2] = {ft - 10, fb + 8};
    const float xs[3] = {0.22f, 0.40f, 0.62f};
    for (int s = 0; s < 2; s++)
        for (float u : xs) diamond(b, int(kFeltL + (kFeltR - kFeltL) * u), railsY[s]);
    diamond(b, fl - 10, int(kFeltT + spanY * 0.35f));
    diamond(b, fl - 10, int(kFeltT + spanY * 0.68f));
    diamond(b, fr + 9, int(kFeltT + spanY * 0.35f));
    diamond(b, fr + 9, int(kFeltT + spanY * 0.68f));
    b.rect(kTableX + 8, kTableY + 8, kTableW - 16, 1, 11);
    b.rect(kTableX + 8, kTableY + kTableH - 9, kTableW - 16, 1, 11);
    auto hole = [&](float x, float y, float r) {
        b.ellipse(x, y, r + 2.2f, r + 2.2f, 7);
        b.ellipse(x, y, r, r, 6);
    };
    hole(34.f, 50.f, 11.f);
    hole(286.f, 50.f, 11.f);
    hole(34.f, 190.f, 11.f);
    hole(286.f, 190.f, 11.f);
    hole(160.f, 198.f, 10.f);
    b.ellipse(160.f, 44.f, 16.f, 12.f, 12);
    b.ellipse(160.f, 44.f, 12.f, 8.5f, 6);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 3)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 2, 0)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(5, 1, 1), gs::rgb4(2, 0, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_BALL,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 1), gs::rgb4(15, 12, 1), gs::rgb4(2, 6, 15), gs::rgb4(14, 2, 2),
            gs::rgb4(10, 3, 14), gs::rgb4(15, 8, 1), gs::rgb4(2, 12, 4), gs::rgb4(12, 12, 11), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_TABLE,
           {0, gs::rgb4(12, 7, 3), gs::rgb4(8, 4, 2), gs::rgb4(4, 2, 1), gs::rgb4(1, 1, 2), gs::rgb4(0, 0, 1),
            gs::rgb4(0, 0, 0), gs::rgb4(3, 2, 1), gs::rgb4(14, 13, 10), gs::rgb4(6, 12, 7), gs::rgb4(9, 15, 10),
            gs::rgb4(13, 10, 3), gs::rgb4(14, 11, 4), gs::rgb4(2, 6, 3)});
    setPal(vdp, PAL_CUE,
           {0, gs::rgb4(15, 12, 7), gs::rgb4(12, 8, 3), gs::rgb4(5, 3, 1), gs::rgb4(15, 15, 13), gs::rgb4(3, 8, 14),
            gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_GLOW, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 13, 5), gs::rgb4(5, 3, 1), gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(3, 1, 0), gs::rgb4(12, 8, 2), gs::rgb4(7, 4, 1), gs::rgb4(14, 11, 4), gs::rgb4(15, 14, 8),
            gs::rgb4(15, 13, 6), gs::rgb4(9, 9, 8)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), gs::rgb4(0, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          gs::rgb4(0, 3, 1)});
    setPal(vdp, PAL_FLAME, {0, gs::rgb4(15, 8, 1), gs::rgb4(15, 14, 4), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    vdp.setFogColor(gs::rgb4(0, 2, 1));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    gs::bitmapToPlane(tiles, vdp.A, 0, 0, roomArt(), PAL_TABLE);
    vdp.A.enabled = true;
    vdp.B.enabled = false;

    for (int n = 0; n < kBalls; n++) {
        gs::Bitmap ball(16, 16);
        Home h = homeAt(n);
        paintBall(ball, h.kind, h.color);
        art.ball[n] = gs::uploadMipped(vdp, ball);
    }
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    gs::Bitmap flat = cueFlat();
    for (int i = 0; i < kCueAngles; i++) art.cue[i] = gs::uploadMipped(vdp, rotateCue(flat, float(i) * kTau / kCueAngles));
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.clapper = gs::uploadMipped(vdp, clapperArt());
    art.yoke = gs::uploadMipped(vdp, yokeArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.flame = gs::uploadMipped(vdp, flameArt());
    gs::TextStyle gold{3, 1, 2, 3, 1};
    gs::TextStyle dead{3, 1, 2, 3, 1};
    art.eight = gs::uploadMipped(vdp, gs::textBitmap("EIGHT", gold));
    art.bellWord = gs::uploadMipped(vdp, gs::textBitmap("BELL", gold));
    art.left = gs::uploadMipped(vdp, gs::textBitmap("LEFT", gold));
    art.dead = gs::uploadMipped(vdp, gs::textBitmap("DEAD", dead));
    art.rung = gs::uploadMipped(vdp, gs::textBitmap("RUNG", gold));
}

}  // namespace eightbell
