#include "art.h"

#include <algorithm>
#include <initializer_list>
#include <vector>

namespace plowturn {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t shade) {
    setPal(vdp, pal, {0, ink, shade, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 0)});
}

// Rear view of a tractor and plow. pose 0 heels left, 2 sits flat, 4 heels right.
Bitmap paintPlow(int pose) {
    Bitmap b(96, 72);
    const float roll = (pose - 2) * 6.5f;
    auto X = [&](float x, float y) { return x + roll * (1.f - y / 72.f); };
    auto poly = [&](std::initializer_list<gs::Pt> pts, int c) {
        std::vector<gs::Pt> w;
        for (const gs::Pt& p : pts) w.push_back({X(p.first, p.second), p.second});
        b.poly(w, c);
    };
    b.ellipse(X(48, 64), 64, 28, 5, 3);
    poly({{22, 58}, {30, 46}, {38, 58}}, 4);
    poly({{58, 58}, {66, 46}, {74, 58}}, 4);
    poly({{18, 40}, {78, 40}, {82, 58}, {14, 58}}, 1);
    poly({{28, 28}, {68, 28}, {74, 42}, {22, 42}}, 2);
    b.rect(X(40, 16), 16, 16, 16, 6);
    b.rect(X(44, 20), 20, 8, 6, 8);
    b.rect(X(30, 46), 46, 8, 8, 7);
    b.rect(X(58, 46), 46, 8, 8, 7);
    b.ellipse(X(34, 52), 52, 6, 6, 5);
    b.ellipse(X(62, 52), 52, 6, 6, 5);
    b.rect(X(46, 8), 8, 4, 10, 9);
    return b;
}

Bitmap paintWreck() {
    Bitmap b(96, 40);
    b.poly({{6, 28}, {90, 18}, {92, 32}, {10, 36}}, 1);
    b.ellipse(22, 22, 8, 5, 5);
    b.ellipse(70, 20, 8, 5, 5);
    b.rect(40, 12, 16, 10, 6);
    b.line(48, 12, 40, 4, 9, 2.f);
    return b;
}

Bitmap paintPost() {
    Bitmap b(10, 28);
    b.rect(4, 2, 2, 24, 1);
    b.rect(2, 8, 6, 2, 2);
    return b;
}

Bitmap paintStalk() {
    Bitmap b(14, 28);
    b.line(4, 26, 3, 6, 1, 1.4f);
    b.line(8, 26, 9, 2, 2, 1.4f);
    b.line(11, 26, 10, 8, 1, 1.2f);
    b.ellipse(9, 3, 2, 2, 3);
    return b;
}

Bitmap paintBarn() {
    Bitmap b(64, 44);
    b.rect(8, 18, 48, 22, 1);
    b.poly({{6, 18}, {32, 4}, {58, 18}}, 2);
    b.rect(26, 26, 12, 14, 4);
    b.rect(12, 22, 8, 6, 3);
    b.rect(44, 22, 8, 6, 3);
    b.rect(8, 36, 48, 3, 5);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(20, 34);
    b.rect(3, 4, 2, 28, 2);
    b.poly({{5, 5}, {17, 9}, {5, 15}}, 1);
    return b;
}

Bitmap paintMark(int n) {
    Bitmap b(24, 32);
    b.rect(10, 14, 3, 16, 2);
    b.rect(2, 2, 20, 14, 1);
    static const int dig[3][5] = {
        {0b010, 0b110, 0b010, 0b010, 0b111},
        {0b111, 0b001, 0b111, 0b100, 0b111},
        {0b111, 0b001, 0b111, 0b001, 0b111},
    };
    int d = std::clamp(n, 0, 2);
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 3; x++)
            if (dig[d][y] & (1 << (2 - x))) b.set(8 + x * 2, 6 + y, 3);
    return b;
}

Bitmap paintDust() {
    Bitmap b(28, 10);
    b.ellipse(14, 5, 12, 3, 1);
    b.ellipse(8, 5, 4, 1.5f, 2);
    return b;
}

Bitmap paintBale() {
    Bitmap b(28, 20);
    b.rect(4, 6, 20, 12, 1);
    b.line(4, 10, 24, 10, 2, 1.f);
    b.line(14, 6, 14, 18, 2, 1.f);
    return b;
}

Bitmap paintSun() {
    Bitmap b(18, 18);
    b.ellipse(9, 9, 6, 6, 1);
    b.ellipse(9, 9, 3, 3, 2);
    return b;
}

Bitmap paintCloud() {
    Bitmap b(40, 16);
    b.ellipse(14, 9, 10, 4, 1);
    b.ellipse(26, 8, 10, 5, 2);
    return b;
}

Bitmap paintShadow() {
    Bitmap b(64, 12);
    b.ellipse(32, 6, 26, 3.5f, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
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
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(2, 1, 0);
    textPal(vdp, PAL_HUD, gs::rgb4(15, 14, 10), gs::rgb4(4, 3, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 6, 2), gs::rgb4(5, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(8, 14, 5), gs::rgb4(2, 4, 1));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 12, 4), gs::rgb4(6, 3, 1));
    textPal(vdp, PAL_TAG, gs::rgb4(14, 15, 12), gs::rgb4(4, 5, 2));

    setPal(vdp, PAL_PLOW,
           {0, gs::rgb4(12, 4, 2), gs::rgb4(6, 7, 4), gs::rgb4(3, 2, 1), gs::rgb4(9, 9, 8), gs::rgb4(2, 2, 2),
            gs::rgb4(14, 13, 8), gs::rgb4(4, 4, 3), gs::rgb4(8, 12, 14), gs::rgb4(15, 8, 2), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(8, 6, 3), gs::rgb4(12, 10, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_STALK, {0, gs::rgb4(3, 8, 2), gs::rgb4(7, 12, 3), gs::rgb4(12, 10, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BARN,
           {0, gs::rgb4(12, 4, 3), gs::rgb4(7, 2, 2), gs::rgb4(13, 14, 12), gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), 0, 0,
            0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(13, 3, 2), gs::rgb4(9, 8, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(10, 8, 5), gs::rgb4(13, 11, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BALE, {0, gs::rgb4(12, 9, 3), gs::rgb4(8, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 12), gs::rgb4(14, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MARK,
           {0, gs::rgb4(12, 10, 4), gs::rgb4(5, 4, 2), gs::rgb4(2, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FIELD,
           {gs::rgb4(2, 5, 1), gs::rgb4(3, 7, 2), gs::rgb4(2, 6, 1), gs::rgb4(5, 8, 2), gs::rgb4(6, 5, 2),
            gs::rgb4(4, 4, 1), gs::rgb4(7, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(8, 7, 4), gs::rgb4(4, 3, 1),
            gs::rgb4(9, 7, 3), gs::rgb4(3, 4, 2), gs::rgb4(6, 6, 3), gs::rgb4(8, 8, 5), gs::rgb4(11, 10, 6),
            gs::rgb4(6, 5, 3)});

    vdp.setFogColor(gs::rgb4(10, 9, 6));
    loadFont(vdp, art);
    for (int i = 0; i < kPoses; i++) art.plow[i] = gs::uploadMipped(vdp, paintPlow(i));
    art.wreck = gs::uploadMipped(vdp, paintWreck());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.stalk = gs::uploadMipped(vdp, paintStalk());
    art.barn = gs::uploadMipped(vdp, paintBarn());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    for (int i = 0; i < 3; i++) art.mark[i] = gs::uploadMipped(vdp, paintMark(i));
    art.dust = gs::uploadMipped(vdp, paintDust());
    art.bale = gs::uploadMipped(vdp, paintBale());
    art.sun = gs::uploadMipped(vdp, paintSun());
    art.cloud = gs::uploadMipped(vdp, paintCloud());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.title = words(vdp, "PLOW TURN", 3);
    art.tipped = words(vdp, "TIPPED", 3);
    art.missed = words(vdp, "MISSED", 3);
    art.made = words(vdp, "LEG MADE", 3);
    art.held = words(vdp, "HELD", 3);
}

}  // namespace plowturn
