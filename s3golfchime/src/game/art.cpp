#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace golfchime {
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

gs::Bitmap greenArt() {
    gs::Bitmap b(280, 150);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            float px = float(x) - 140.f;
            float py = float(y) - 78.f;
            float e = (px * px) / (132.f * 132.f) + (py * py) / (68.f * 68.f);
            if (e > 1.f) continue;
            int body = ((x / 8 + y / 8) & 1) ? 2 : 1;
            if (e > 0.92f) body = 3;
            float sx = float(x) + 20.f;
            float sy = float(y) + 40.f;
            float dx = sx - kCupX;
            float dy = sy - kCupY;
            if (dx * dx + dy * dy < (kCupR + 1.f) * (kCupR + 1.f)) body = 4;
            else if (dx * dx + dy * dy < (kCupR + 3.5f) * (kCupR + 3.5f)) body = 5;
            b.set(x, y, body);
        }
    }
    return b;
}

gs::Bitmap ballArt() {
    gs::Bitmap b(9, 9);
    b.ellipse(4.f, 4.f, 3.6f, 3.6f, 1);
    b.ellipse(3.f, 3.f, 1.3f, 1.1f, 2);
    b.set(6, 5, 3);
    return b;
}

gs::Bitmap golferArt() {
    gs::Bitmap b(18, 28);
    b.ellipse(7.f, 4.5f, 3.5f, 3.5f, 1);
    b.rect(5.f, 8.f, 5.f, 9.f, 2);
    b.rect(1.f, 10.f, 5.f, 2.f, 3);
    b.rect(9.f, 12.f, 8.f, 2.f, 4);
    b.rect(4.f, 16.f, 3.f, 9.f, 2);
    b.rect(8.f, 16.f, 3.f, 9.f, 5);
    b.ellipse(16.f, 13.f, 1.6f, 1.6f, 6);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(16, 28);
    b.rect(1.f, 2.f, 2.f, 24.f, 1);
    b.poly({{3.f, 3.f}, {14.f, 7.f}, {3.f, 12.f}}, 2);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(54, 62);
    b.rect(22.f, 0.f, 10.f, 8.f, 3);
    b.rect(24.f, 0.f, 6.f, 4.f, 4);
    b.ellipse(27.f, 34.f, 22.f, 22.f, 1);
    b.ellipse(27.f, 34.f, 18.f, 18.f, 2);
    b.ellipse(27.f, 34.f, 2.f, 2.f, 5);
    for (int i = 0; i < 12; i++) {
        float a = float(i) * 3.14159265f / 6.f;
        int x = int(std::lround(27.f + std::sin(a) * 15.f));
        int y = int(std::lround(34.f - std::cos(a) * 15.f));
        b.set(x, y, i == 0 ? 6 : 5);
        b.set(x, y + 1, i == 0 ? 6 : 5);
    }
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_GREEN,
           {0, gs::rgb4(2, 10, 3), gs::rgb4(1, 8, 2), gs::rgb4(4, 13, 5), gs::rgb4(1, 1, 2), gs::rgb4(6, 12, 5),
            gs::rgb4(3, 5, 2)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 12, 10), gs::rgb4(8, 8, 7)});
    setPal(vdp, PAL_GOLFER,
           {0, gs::rgb4(13, 9, 6), gs::rgb4(2, 3, 8), gs::rgb4(14, 12, 8), gs::rgb4(10, 7, 3), gs::rgb4(1, 2, 5),
            gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(14, 14, 12), gs::rgb4(12, 2, 3)});
    setPal(vdp, PAL_CLOCK,
           {0, gs::rgb4(10, 8, 6), gs::rgb4(15, 14, 11), gs::rgb4(6, 5, 4), gs::rgb4(4, 6, 8), gs::rgb4(3, 3, 3),
            gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(2, 2, 3), gs::rgb4(14, 3, 3)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(8, 15, 8), gs::rgb4(1, 2, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_AIM, gs::rgb4(15, 12, 3), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 14, 6), gs::rgb4(3, 1, 1));

    loadFont(vdp, art);
    art.green = gs::uploadImage(vdp, greenArt());
    art.ball = gs::uploadImage(vdp, ballArt());
    art.golfer = gs::uploadImage(vdp, golferArt());
    art.flag = gs::uploadImage(vdp, flagArt());
    art.clock = gs::uploadImage(vdp, clockArt());
    gs::Bitmap hand(3, 8);
    hand.rect(1.f, 0.f, 1.f, 8.f, 1);
    art.hand = gs::uploadImage(vdp, hand);
    gs::Bitmap pip(3, 3);
    pip.ellipse(1.f, 1.f, 1.2f, 1.2f, 1);
    art.pip = gs::uploadImage(vdp, pip);
    art.title = gs::uploadImage(vdp, gs::textBitmap("S3 GOLFCHIME", {2, 1, 2, 0, 1}));
}

}  // namespace golfchime
