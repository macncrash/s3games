#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace eightchime {
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

void paintBall(gs::Bitmap& b, int kind, int color) {
    const float cx = b.w * 0.5f, cy = b.h * 0.5f;
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            float dx = x + 0.5f - cx;
            float dy = y + 0.5f - cy;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d > 7.15f) continue;
            int c = 2;
            if (d < 6.25f) {
                if (kind == 0) c = 1;
                else if (kind == 3) c = d < 4.35f ? 1 : 2;
                else if (kind == 2) c = std::fabs(dy) < 2.15f ? color : 1;
                else c = color;
                float hx = dx + 2.1f, hy = dy + 2.3f;
                if (kind != 3 && hx * hx + hy * hy < 2.5f) c = 10;
                if (kind == 3 && d > 3.5f && hx * hx + hy * hy < 1.5f) c = 10;
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
    gs::Bitmap b(42, 8);
    b.rect(0, 2, 6, 4, 6);
    b.rect(5, 1, 26, 6, 2);
    b.rect(5, 1, 26, 2, 1);
    b.rect(31, 1, 5, 6, 5);
    b.rect(36, 1, 5, 6, 4);
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

gs::Bitmap roomArt() {
    gs::Bitmap b(gs::SCREEN_W, gs::SCREEN_H);
    b.rect(0, 50, gs::SCREEN_W, 8, 4);
    b.rect(0, 56, gs::SCREEN_W, 2, 5);
    b.rect(14, 58, 292, 162, 2);
    b.rect(14, 58, 292, 5, 1);
    b.rect(14, 58, 5, 162, 1);
    b.rect(14, 215, 292, 5, 3);
    b.rect(301, 58, 5, 162, 3);

    const int fl = int(kFeltL), ft = int(kFeltT), fr = int(kFeltR), fb = int(kFeltB);
    for (int y = ft; y < fb; y++) {
        int nap = ((y / 2) & 1) ? 10 : 9;
        for (int x = fl; x < fr; x++) b.set(x, y, nap);
    }
    for (int x = fl; x < fr; x++) {
        b.set(x, ft - 1, 13);
        b.set(x, ft - 2, 13);
        b.set(x, fb, 14);
        b.set(x, fb + 1, 14);
    }
    for (int y = ft; y < fb; y++) {
        b.set(fl - 1, y, 13);
        b.set(fl - 2, y, 13);
        b.set(fr, y, 14);
        b.set(fr + 1, y, 14);
    }

    const float span = kFeltB - kFeltT;
    int head = int(kFeltB - span * 0.25f);
    for (int x = fl + 8; x < fr - 8; x += 2) b.set(x, head, 11);
    b.ellipse(160.f, kFeltT + span * 0.28f, 2.2f, 2.2f, 11);
    b.ellipse(160.f, (kFeltT + kFeltB) * 0.5f, 1.5f, 1.5f, 11);

    const int railsY[2] = {ft - 8, fb + 6};
    const float xs[3] = {0.22f, 0.5f, 0.78f};
    for (int s = 0; s < 2; s++)
        for (float u : xs) diamond(b, int(kFeltL + (kFeltR - kFeltL) * u), railsY[s]);
    diamond(b, fl - 8, int(kFeltT + span * 0.33f));
    diamond(b, fl - 8, int(kFeltT + span * 0.66f));
    diamond(b, fr + 8, int(kFeltT + span * 0.33f));
    diamond(b, fr + 8, int(kFeltT + span * 0.66f));

    auto hole = [&](float x, float y, float r, bool hour) {
        if (hour) b.ellipse(x, y, r + 3.4f, r + 3.4f, 12);
        b.ellipse(x, y, r + 1.8f, r + 1.8f, 7);
        b.ellipse(x, y, r, r, 6);
    };
    for (int i = 0; i < 6; i++) {
        Pocket p = pocketAt(i);
        float r = (i == 1 || i == 4) ? 9.f : 11.f;
        hole(p.x, p.y, r, i == kHourPocket);
    }
    gs::Bitmap num = gs::textBitmap("12", gs::TextStyle{1, 12, 0, 0, 0});
    b.blit(num, 252, 60);
    return b;
}

void shaft(gs::Bitmap& b, float theta, float len, float tail, float w, int col) {
    const float cx = kDial * 0.5f, cy = kDial * 0.5f;
    float dx = std::sin(theta), dy = -std::cos(theta);
    float px = -dy, py = dx;
    float x0 = cx - dx * tail, y0 = cy - dy * tail;
    float x1 = cx + dx * len, y1 = cy + dy * len;
    b.poly({{x0 + px * w * 0.4f, y0 + py * w * 0.4f},
            {x1 + px * w, y1 + py * w},
            {x1 - px * w, y1 - py * w},
            {x0 - px * w * 0.4f, y0 - py * w * 0.4f}},
           col);
}

gs::Bitmap handArt(int kind, int step) {
    gs::Bitmap b(kDial, kDial);
    float theta = step * (kTau / 60.f);
    if (kind == 0) {
        shaft(b, theta, 9.f, 3.f, 2.1f, 1);
        shaft(b, theta, 8.f, 2.f, 1.0f, 2);
    } else if (kind == 1) {
        shaft(b, theta, 13.f, 3.5f, 1.05f, 3);
    } else {
        shaft(b, theta, 16.f, 4.5f, 0.5f, 4);
    }
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(kDial, kDial);
    b.ellipse(20, 20, 19, 19, 1);
    b.ellipse(20, 20, 16.6f, 16.6f, 2);
    b.ellipse(20, 20, 15.2f, 15.2f, 3);
    b.ellipse(18, 18, 8, 6, 4);
    b.ellipse(20, 20, 15.2f, 15.2f, 3);
    for (int i = 0; i < 60; i += 5) {
        float a = i * (kTau / 60.f);
        float s = std::sin(a), c = std::cos(a);
        int col = (i % 15 == 0) ? 6 : 5;
        float inner = (i % 15 == 0) ? 10.5f : 12.2f;
        b.line(20 + s * inner, 20 - c * inner, 20 + s * 14.4f, 20 - c * 14.4f, col, i == 0 ? 1.8f : 1.f);
    }
    b.ellipse(20, 20, 1.5f, 1.5f, 7);
    return b;
}

gs::Bitmap bezelArt() {
    gs::Bitmap b(kDial, kDial);
    b.ellipse(20, 20, 19.4f, 19.4f, 1);
    b.ellipse(20, 20, 16.2f, 16.2f, 2);
    b.ellipse(20, 20, 14.6f, 14.6f, 0);
    return b;
}

gs::Bitmap capArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 2.4f, 2.4f, 1);
    b.ellipse(3.4f, 3.4f, 1.f, 1.f, 2);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(16, 18);
    b.rect(7, 0, 2, 3, 3);
    b.poly({{3, 4}, {13, 4}, {15, 12}, {1, 12}}, 1);
    b.ellipse(8, 8, 4, 3, 2);
    b.ellipse(8, 13, 6.2f, 2.4f, 1);
    b.ellipse(8, 12.2f, 3.2f, 1.2f, 4);
    b.ellipse(8, 10, 1.3f, 1.6f, 5);
    return b;
}

gs::Bitmap plaqueArt() {
    gs::Bitmap b(56, 50);
    b.rect(2, 4, 52, 42, 5);
    b.rect(2, 4, 52, 3, 4);
    b.rect(2, 4, 3, 42, 4);
    b.rect(6, 10, 44, 32, 6);
    b.ellipse(28, 24, 18, 16, 0);
    b.rect(24, 0, 8, 6, 4);
    b.rect(26, 0, 4, 3, 3);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(5, 5);
    b.ellipse(2.5f, 2.5f, 1.8f, 1.8f, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(14, 6);
    b.ellipse(7.f, 3.f, 6.f, 2.1f, 1);
    return b;
}

gs::Bitmap pocketRing() {
    gs::Bitmap b(28, 16);
    b.ellipse(14.f, 8.f, 12.f, 6.4f, 1);
    b.ellipse(14.f, 8.f, 7.f, 3.4f, 0);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 3)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, gs::rgb4(4, 2, 0)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 11, 4), gs::rgb4(4, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_BALL,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 1), gs::rgb4(15, 12, 2), gs::rgb4(2, 6, 15), gs::rgb4(14, 2, 2),
            gs::rgb4(10, 3, 14), gs::rgb4(15, 8, 1), gs::rgb4(2, 12, 4), gs::rgb4(12, 12, 11), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_TABLE,
           {0, gs::rgb4(12, 7, 3), gs::rgb4(8, 4, 2), gs::rgb4(4, 2, 1), gs::rgb4(3, 2, 4), gs::rgb4(1, 1, 2),
            gs::rgb4(0, 0, 0), gs::rgb4(3, 2, 1), gs::rgb4(14, 13, 10), gs::rgb4(1, 7, 4), gs::rgb4(2, 10, 5),
            gs::rgb4(6, 12, 7), gs::rgb4(14, 11, 3), gs::rgb4(3, 8, 4), gs::rgb4(1, 4, 2)});
    setPal(vdp, PAL_CUE,
           {0, gs::rgb4(15, 12, 7), gs::rgb4(12, 8, 3), gs::rgb4(5, 3, 1), gs::rgb4(15, 15, 13), gs::rgb4(3, 8, 14),
            gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_GLOW, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_FACE,
           {0, gs::rgb4(12, 9, 3), gs::rgb4(6, 4, 1), gs::rgb4(15, 14, 11), gs::rgb4(13, 12, 9), gs::rgb4(4, 3, 2),
            gs::rgb4(2, 2, 2), gs::rgb4(14, 3, 2), gs::rgb4(8, 6, 2)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 5, 4), gs::rgb4(1, 1, 2), gs::rgb4(14, 2, 2),
                           gs::rgb4(15, 14, 12)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(15, 12, 3), gs::rgb4(10, 7, 2), gs::rgb4(6, 4, 1), gs::rgb4(12, 8, 3),
                           gs::rgb4(5, 3, 1), gs::rgb4(2, 1, 1), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(2, 6, 3), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, gs::rgb4(0, 3, 1)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    vdp.setFogColor(gs::rgb4(1, 1, 2));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    gs::bitmapToPlane(tiles, vdp.A, 0, 0, roomArt(), PAL_TABLE);
    vdp.A.enabled = true;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;

    for (int n = 0; n < kBalls; n++) {
        gs::Bitmap ball(16, 16);
        Home h = homeAt(n);
        paintBall(ball, h.kind, h.color);
        art.ball[n] = gs::uploadMipped(vdp, ball);
    }
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    gs::Bitmap flat = cueFlat();
    for (int i = 0; i < kCueAngles; i++)
        art.cue[i] = gs::uploadMipped(vdp, rotateCue(flat, float(i) * kTau / kCueAngles));
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.ring = gs::uploadMipped(vdp, pocketRing());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.plaque = gs::uploadMipped(vdp, plaqueArt());
    for (int k = 0; k < 3; k++)
        for (int s = 0; s < 60; s++) art.hand[k][s] = gs::uploadImage(vdp, handArt(k, s));
    art.face = gs::uploadImage(vdp, faceArt());
    art.bezel = gs::uploadImage(vdp, bezelArt());
    art.cap = gs::uploadImage(vdp, capArt());
}

}  // namespace eightchime
