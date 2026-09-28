#include "art.h"

#include <algorithm>
#include <initializer_list>
#include <vector>

namespace scullturn {
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
    setPal(vdp, pal, {0, ink, shade, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 3)});
}

// Stern view of a single scull. pose 0 heels to port, 2 sits flat, 4 heels to starboard.
Bitmap paintShell(int pose) {
    Bitmap b(96, 72);
    const float roll = (pose - 2) * 7.4f;
    auto X = [&](float x, float y) { return x + roll * (1.f - y / 72.f); };
    auto poly = [&](std::initializer_list<gs::Pt> pts, int c) {
        std::vector<gs::Pt> w;
        for (const gs::Pt& p : pts) w.push_back({X(p.first, p.second), p.second});
        b.poly(w, c);
    };
    const float oar = (pose - 2) * 3.f;
    b.line(X(8, 34 + oar), 34 + oar, X(22, 40), 40, 4, 2.2f);
    b.line(X(88, 34 - oar), 34 - oar, X(74, 40), 40, 4, 2.2f);
    b.ellipse(X(8, 34 + oar), 34 + oar, 5, 2.2f, 5);
    b.ellipse(X(88, 34 - oar), 34 - oar, 5, 2.2f, 5);
    poly({{40, 18}, {56, 18}, {60, 58}, {36, 58}}, 1);
    poly({{44, 22}, {52, 22}, {54, 54}, {42, 54}}, 2);
    b.ellipse(X(48, 16), 16, 6, 5, 6);
    b.rect(X(45, 20), 20, 6, 8, 7);
    b.line(X(46, 28), 28, X(42, 40), 40, 8, 1.6f);
    b.line(X(50, 28), 28, X(54, 40), 40, 8, 1.6f);
    b.rect(X(44, 48), 48, 8, 4, 3);
    b.line(X(38, 56), 56, X(58, 56), 56, 9, 1.4f);
    return b;
}

Bitmap paintWreck() {
    Bitmap b(96, 48);
    b.poly({{8, 28}, {88, 16}, {92, 30}, {12, 38}}, 1);
    b.ellipse(20, 22, 7, 4, 6);
    b.line(30, 20, 6, 8, 4, 2.f);
    b.line(70, 18, 90, 6, 4, 2.f);
    b.ellipse(6, 8, 4, 2, 5);
    return b;
}

Bitmap paintPost() {
    Bitmap b(12, 32);
    b.rect(5, 8, 2, 22, 2);
    b.rect(3, 4, 6, 6, 1);
    b.rect(4, 6, 4, 2, 3);
    return b;
}

Bitmap paintReed() {
    Bitmap b(16, 26);
    b.line(3, 24, 2, 4, 1, 1.3f);
    b.line(8, 24, 9, 1, 2, 1.3f);
    b.line(13, 24, 12, 7, 1, 1.3f);
    return b;
}

Bitmap paintHouse() {
    Bitmap b(56, 40);
    b.rect(6, 16, 40, 22, 1);
    b.poly({{4, 16}, {26, 4}, {48, 16}}, 2);
    b.rect(20, 24, 10, 14, 4);
    b.rect(10, 20, 6, 5, 3);
    b.rect(36, 20, 6, 5, 3);
    b.rect(8, 34, 36, 3, 5);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(22, 36);
    b.rect(4, 6, 2, 28, 2);
    b.poly({{6, 6}, {18, 10}, {6, 16}}, 1);
    b.rect(8, 8, 4, 3, 3);
    return b;
}

Bitmap paintMark(int n) {
    Bitmap b(26, 34);
    b.rect(11, 14, 3, 18, 2);
    b.rect(3, 3, 20, 13, 1);
    b.rect(5, 5, 16, 9, 3);
    static const int dig[3][5] = {
        {0b010, 0b110, 0b010, 0b010, 0b111},
        {0b111, 0b001, 0b111, 0b100, 0b111},
        {0b111, 0b001, 0b111, 0b001, 0b111},
    };
    int d = std::clamp(n, 0, 2);
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 3; x++)
            if (dig[d][y] & (1 << (2 - x))) b.set(9 + x * 2, 7 + y, 4);
    return b;
}

Bitmap paintWake() {
    Bitmap b(36, 10);
    b.ellipse(18, 5, 16, 3, 1);
    b.ellipse(12, 5, 5, 1.6f, 2);
    return b;
}

Bitmap paintHeron(bool up) {
    Bitmap b(24, 28);
    b.line(12, 22, 12, 10, 1, 1.4f);
    b.line(12, 12, up ? 4.f : 8.f, 6, 1, 1.2f);
    b.line(12, 22, 8, 27, 2, 1.2f);
    b.line(12, 22, 16, 27, 2, 1.2f);
    b.ellipse(12, 8, 3, 2.4f, 1);
    b.line(14, 8, 20, 6, 3, 1.f);
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
    b.ellipse(12, 9, 9, 4, 1);
    b.ellipse(24, 8, 10, 5, 2);
    return b;
}

Bitmap paintShadow() {
    Bitmap b(64, 12);
    b.ellipse(32, 6, 28, 3.5f, 1);
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
    const uint16_t ink = gs::rgb4(1, 2, 3);
    textPal(vdp, PAL_HUD, gs::rgb4(14, 15, 14), gs::rgb4(2, 5, 6));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 7, 3), gs::rgb4(5, 1, 1));
    textPal(vdp, PAL_WIN, gs::rgb4(8, 15, 11), gs::rgb4(1, 5, 4));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 13, 7), gs::rgb4(5, 4, 1));
    textPal(vdp, PAL_TAG, gs::rgb4(12, 15, 15), gs::rgb4(2, 6, 8));

    setPal(vdp, PAL_SHELL,
           {0, gs::rgb4(13, 14, 14), gs::rgb4(7, 9, 10), gs::rgb4(14, 6, 3), gs::rgb4(4, 6, 7), gs::rgb4(10, 13, 14),
            gs::rgb4(12, 8, 5), gs::rgb4(3, 3, 4), gs::rgb4(9, 6, 4), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_POST,
           {0, gs::rgb4(14, 5, 2), gs::rgb4(8, 7, 5), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_REED,
           {0, gs::rgb4(2, 8, 3), gs::rgb4(5, 11, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_HOUSE,
           {0, gs::rgb4(10, 8, 6), gs::rgb4(8, 3, 2), gs::rgb4(13, 14, 12), gs::rgb4(3, 3, 4), gs::rgb4(6, 5, 3), 0, 0,
            0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FLAG,
           {0, gs::rgb4(12, 3, 3), gs::rgb4(9, 8, 6), gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(9, 13, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BIRD,
           {0, gs::rgb4(6, 7, 8), gs::rgb4(3, 4, 5), gs::rgb4(14, 10, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SKY,
           {0, gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 13), gs::rgb4(13, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MARK,
           {0, gs::rgb4(11, 9, 4), gs::rgb4(5, 4, 2), gs::rgb4(14, 13, 9), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, ink});
    setPal(vdp, PAL_RIVER,
           {gs::rgb4(1, 4, 7), gs::rgb4(2, 6, 9), gs::rgb4(3, 8, 11), gs::rgb4(5, 10, 12), gs::rgb4(7, 12, 13),
            gs::rgb4(4, 9, 11), gs::rgb4(2, 7, 10), gs::rgb4(1, 5, 8), gs::rgb4(6, 11, 12), gs::rgb4(8, 13, 13),
            gs::rgb4(3, 7, 9), gs::rgb4(1, 3, 6), gs::rgb4(9, 14, 13), gs::rgb4(2, 5, 7), gs::rgb4(11, 15, 14),
            gs::rgb4(4, 8, 10)});

    vdp.setFogColor(gs::rgb4(9, 12, 13));
    loadFont(vdp, art);
    for (int i = 0; i < kPoses; i++) art.shell[i] = gs::uploadMipped(vdp, paintShell(i));
    art.wreck = gs::uploadMipped(vdp, paintWreck());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.house = gs::uploadMipped(vdp, paintHouse());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    for (int i = 0; i < 3; i++) art.mark[i] = gs::uploadMipped(vdp, paintMark(i));
    art.wake = gs::uploadMipped(vdp, paintWake());
    art.heron[0] = gs::uploadMipped(vdp, paintHeron(true));
    art.heron[1] = gs::uploadMipped(vdp, paintHeron(false));
    art.sun = gs::uploadMipped(vdp, paintSun());
    art.cloud = gs::uploadMipped(vdp, paintCloud());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.title = words(vdp, "SCULL TURN", 3);
    art.tipped = words(vdp, "TIPPED", 3);
    art.missed = words(vdp, "MISSED", 3);
    art.made = words(vdp, "LEG MADE", 3);
    art.held = words(vdp, "HELD", 3);
}

}  // namespace scullturn
