#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace eighttape {
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

void paintBall(gs::Bitmap& b, int kind) {
    const float cx = 8.f, cy = 8.f;
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            float dx = x + 0.5f - cx;
            float dy = y + 0.5f - cy;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d > 7.15f) continue;
            int c = 7;
            if (d < 6.15f) {
                if (kind == 1) c = 2;
                else if (kind == 2) c = dy > 2.4f ? 8 : 3;
                else if (kind == 3) c = std::fabs(dy) < 2.15f ? 4 : 1;
                else c = dy > 3.1f ? 7 : 1;
                float hx = dx + 2.3f, hy = dy + 2.5f;
                if (kind != 1 && hx * hx + hy * hy < 2.3f) c = 6;
            }
            if (kind == 4 && d < 2.5f) c = 5;
            b.set(x, y, c);
        }
    }
    if (kind != 1) return;
    b.ellipse(8.f, 8.f, 3.3f, 3.3f, 1);
    static const char* g[5] = {"###", "# #", "###", "# #", "###"};
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 3; x++)
            if (g[y][x] == '#') b.set(6 + x, 5 + y, 2);
}

gs::Bitmap cueFlat() {
    gs::Bitmap b(34, 8);
    b.rect(0, 2, 6, 4, 5);
    b.rect(5, 1, 20, 6, 2);
    b.rect(5, 1, 20, 2, 1);
    b.rect(25, 1, 4, 6, 3);
    b.rect(29, 2, 4, 4, 4);
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
    b.set(x, y, 9);
    b.set(x - 1, y, 9);
    b.set(x + 1, y, 9);
    b.set(x, y - 1, 9);
    b.set(x, y + 1, 9);
}

gs::Bitmap roomArt() {
    gs::Bitmap b(gs::SCREEN_W, gs::SCREEN_H);
    b.rect(0, 0, gs::SCREEN_W, gs::SCREEN_H, 4);
    b.rect(0, 0, gs::SCREEN_W, 22, 5);
    b.rect(kRailL, kRailT, kRailR - kRailL, kRailB - kRailT, 2);
    b.rect(kRailL, kRailT, kRailR - kRailL, 5, 1);
    b.rect(kRailL, kRailT, 5, kRailB - kRailT, 1);
    b.rect(kRailL, kRailB - 6, kRailR - kRailL, 6, 3);
    b.rect(kRailR - 6, kRailT, 6, kRailB - kRailT, 3);
    const int fl = int(kFeltL), ft = int(kFeltT), fr = int(kFeltR), fb = int(kFeltB);
    for (int y = ft; y < fb; y++)
        for (int x = fl; x < fr; x++) b.set(x, y, 0);
    for (int x = fl; x < fr; x++) {
        b.set(x, ft, 10);
        b.set(x, fb - 1, 3);
    }
    for (int y = ft; y < fb; y++) {
        b.set(fl, y, 10);
        b.set(fr - 1, y, 3);
    }
    const int head = int(kFeltB - (kFeltB - kFeltT) * 0.28f);
    for (int y = ft + 8; y < fb - 8; y += 2) b.set(head, y, 10);
    b.ellipse(108.f, (kFeltT + kFeltB) * 0.5f, 2.2f, 2.2f, 10);
    const int railsY[2] = {ft - 8, fb + 6};
    const float xs[3] = {0.22f, 0.48f, 0.74f};
    for (int s = 0; s < 2; s++)
        for (float u : xs) diamond(b, int(kFeltL + (kFeltR - kFeltL) * u), railsY[s]);
    diamond(b, fl - 8, int(kFeltT + (kFeltB - kFeltT) * 0.38f));
    diamond(b, fl - 8, int(kFeltT + (kFeltB - kFeltT) * 0.68f));
    diamond(b, fr + 6, int(kFeltT + (kFeltB - kFeltT) * 0.38f));
    diamond(b, fr + 6, int(kFeltT + (kFeltB - kFeltT) * 0.68f));
    auto hole = [&](float x, float y, float rx, float ry) {
        b.ellipse(x, y, rx + 2.4f, ry + 2.2f, 8);
        b.ellipse(x, y, rx, ry, 6);
    };
    hole(kPocket[0].x, kPocket[0].y, 9.f, 9.f);
    hole(kPocket[2].x, kPocket[2].y, 9.f, 9.f);
    hole(kPocket[3].x, kPocket[3].y, 8.f, 8.f);
    hole(kPocket[5].x, kPocket[5].y, 8.f, 8.f);
    hole(kPocket[1].x, kPocket[1].y, 12.f, 8.f);
    hole(kPocket[4].x, kPocket[4].y, 12.f, 8.f);
    b.rect(kPaperL, kPaperT, kPaperR - kPaperL, kPaperB - kPaperT, 11);
    b.rect(kPaperL, kPaperT, 5, kPaperB - kPaperT, 12);
    for (int y = int(kPaperT) + 8; y < int(kPaperB) - 6; y += 6) b.rect(kPaperL + 10, y, 86, 1, 13);
    b.rect(kPaperL, 156, kPaperR - kPaperL, 58, 2);
    b.rect(kPaperL, 156, kPaperR - kPaperL, 4, 1);
    b.rect(kPaperL, 210, kPaperR - kPaperL, 8, 3);
    for (int i = 0; i < 3; i++) {
        float x = kSlotX0 + float(i) * kSlotPitch;
        b.ellipse(x, kSlotY, 11.f, 7.f, 3);
        b.ellipse(x, kSlotY, 7.f, 3.4f, 6);
    }
    b.rect(0, 214, gs::SCREEN_W, 10, 5);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(14, 6);
    b.ellipse(7.f, 3.f, 6.f, 2.1f, 1);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(5, 5);
    b.ellipse(2.5f, 2.5f, 1.8f, 1.8f, 1);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(22, 12);
    b.ellipse(11.f, 6.f, 10.f, 5.2f, 1);
    b.ellipse(11.f, 6.f, 6.2f, 3.f, 0);
    return b;
}

gs::Bitmap slipArt() {
    gs::Bitmap b(16, 22);
    b.rect(0, 0, 16, 22, 1);
    b.rect(0, 0, 3, 22, 3);
    for (int y = 4; y < 20; y += 4) b.rect(5, y, 9, 1, 2);
    b.rect(5, 2, 8, 1, 4);
    return b;
}

gs::Bitmap slotArt() {
    gs::Bitmap b(18, 10);
    b.ellipse(9.f, 5.f, 8.f, 4.f, 1);
    b.ellipse(9.f, 5.f, 5.f, 2.2f, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(18, 16);
    b.rect(8, 0, 2, 4, 3);
    b.ellipse(9.f, 8.f, 8.f, 5.f, 1);
    b.ellipse(9.f, 8.f, 4.f, 2.4f, 2);
    b.rect(4, 13, 10, 2, 3);
    return b;
}

gs::Bitmap playerArt(int step) {
    gs::Bitmap b(18, 26);
    int foot = step ? 2 : 0;
    b.ellipse(6.f + foot, 23.f, 3.f, 1.6f, 5);
    b.ellipse(12.f - foot, 23.f, 3.f, 1.6f, 5);
    b.rect(6, 13, 3, 9, 4);
    b.rect(10, 13, 3, 9, 4);
    b.rect(5, 9, 9, 6, 2);
    b.ellipse(9.f, 6.f, 3.6f, 3.4f, 1);
    b.ellipse(9.f, 4.2f, 4.2f, 2.f, 3);
    b.rect(12, 10, 4, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 13);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 5, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 0)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(6, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 2, 1)});
    setPal(vdp, PAL_BALL,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 1), gs::rgb4(15, 12, 2), gs::rgb4(2, 5, 14), gs::rgb4(13, 2, 2),
            gs::rgb4(15, 15, 13), gs::rgb4(5, 5, 6), gs::rgb4(10, 7, 1), gs::rgb4(1, 2, 6)});
    setPal(vdp, PAL_TABLE,
           {0, gs::rgb4(14, 9, 4), gs::rgb4(9, 5, 2), gs::rgb4(4, 2, 1), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2),
            gs::rgb4(0, 0, 0), gs::rgb4(3, 2, 1), gs::rgb4(2, 1, 1), gs::rgb4(14, 12, 8), gs::rgb4(8, 14, 8),
            gs::rgb4(14, 13, 10), gs::rgb4(12, 3, 2), gs::rgb4(9, 8, 6)});
    setPal(vdp, PAL_CUE,
           {0, gs::rgb4(15, 13, 8), gs::rgb4(12, 8, 3), gs::rgb4(15, 15, 14), gs::rgb4(3, 8, 13), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(14, 13, 9), gs::rgb4(8, 7, 5), gs::rgb4(6, 3, 1), gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_SOLID, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 6, 1), gs::rgb4(6, 3, 1), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_STRIPE, {0, gs::rgb4(12, 14, 15), gs::rgb4(3, 5, 10), gs::rgb4(1, 2, 5), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_PLAYER,
           {0, gs::rgb4(14, 11, 8), gs::rgb4(3, 6, 12), gs::rgb4(4, 2, 1), gs::rgb4(2, 2, 4), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_AIM, {0, gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_GLOW, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 12, 4), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(0, 0, 0)});
    vdp.setFogColor(gs::rgb4(0, 2, 1));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    gs::bitmapToPlane(tiles, vdp.A, 0, 0, roomArt(), PAL_TABLE);
    vdp.A.enabled = true;
    vdp.B.enabled = false;

    for (int n = 0; n < kBalls; n++) {
        gs::Bitmap ball(16, 16);
        paintBall(ball, n);
        art.ball[n] = gs::uploadMipped(vdp, ball);
    }
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    gs::Bitmap flat = cueFlat();
    for (int i = 0; i < kCueAngles; i++)
        art.cue[i] = gs::uploadMipped(vdp, rotateCue(flat, float(i) * kTau / float(kCueAngles)));
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.slip = gs::uploadMipped(vdp, slipArt());
    art.slot = gs::uploadMipped(vdp, slotArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.player[0] = gs::uploadMipped(vdp, playerArt(0));
    art.player[1] = gs::uploadMipped(vdp, playerArt(1));
}

}  // namespace eighttape
