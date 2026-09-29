#include "art.h"

#include <algorithm>
#include <initializer_list>
#include <vector>

namespace headerturn {
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
    setPal(vdp, pal, {0, ink, shade, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 4)});
}

// Stern view of a small keelboat. pose 0 heels to port, 2 sits flat, 4 heels to starboard.
Bitmap paintBoat(int pose) {
    Bitmap b(88, 80);
    const float roll = (pose - 2) * 8.2f;
    auto X = [&](float x, float y) { return x + roll * (1.f - y / 80.f); };
    auto poly = [&](std::initializer_list<gs::Pt> pts, int c) {
        std::vector<gs::Pt> w;
        for (const gs::Pt& p : pts) w.push_back({X(p.first, p.second), p.second});
        b.poly(w, c);
    };
    const float boom = (pose - 2) * 4.f;
    poly({{28, 22}, {60, 18}, {70, 58}, {18, 62}}, 1);
    poly({{34, 28}, {54, 26}, {60, 54}, {26, 56}}, 2);
    b.rect(X(40, 30), 30, 6, 16, 3);
    b.line(X(43, 14), 14, X(43, 46), 46, 4, 1.6f);
    poly({{44, 16}, {68, 28 + boom}, {44, 42}}, 5);
    b.line(X(44, 42), 42, X(70, 46 + boom * 0.4f), 46 + boom * 0.4f, 6, 1.5f);
    b.ellipse(X(44, 12), 12, 5, 4, 7);
    b.rect(X(36, 58), 58, 16, 4, 8);
    return b;
}

Bitmap paintWreck() {
    Bitmap b(88, 40);
    b.poly({{6, 22}, {80, 10}, {84, 24}, {10, 34}}, 1);
    b.line(40, 16, 18, 4, 4, 1.6f);
    b.poly({{40, 16}, {62, 8}, {46, 22}}, 5);
    b.ellipse(16, 18, 6, 3, 7);
    return b;
}

Bitmap paintStake() {
    Bitmap b(10, 30);
    b.rect(4, 6, 2, 22, 2);
    b.rect(2, 3, 6, 5, 1);
    return b;
}

Bitmap paintReef() {
    Bitmap b(18, 22);
    b.line(3, 20, 4, 6, 1, 1.4f);
    b.line(9, 20, 8, 2, 2, 1.5f);
    b.line(15, 20, 13, 8, 1, 1.3f);
    return b;
}

Bitmap paintShed() {
    Bitmap b(48, 36);
    b.rect(6, 16, 36, 18, 1);
    b.poly({{4, 16}, {24, 4}, {44, 16}}, 2);
    b.rect(20, 22, 8, 12, 3);
    b.rect(10, 20, 6, 5, 4);
    return b;
}

Bitmap paintFinish() {
    Bitmap b(16, 40);
    b.rect(6, 8, 2, 30, 2);
    for (int i = 0; i < 4; i++) b.rect(8, 8 + i * 4, 6, 2, (i & 1) ? 1 : 3);
    return b;
}

Bitmap paintMark(int n) {
    Bitmap b(24, 32);
    b.ellipse(12, 22, 7, 4, 2);
    b.rect(10, 10, 4, 14, 1);
    b.rect(4, 4, 16, 10, 3);
    static const int dig[3][5] = {
        {0b010, 0b110, 0b010, 0b010, 0b111},
        {0b111, 0b001, 0b111, 0b100, 0b111},
        {0b111, 0b001, 0b111, 0b001, 0b111},
    };
    int d = std::clamp(n, 0, 2);
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 3; x++)
            if (dig[d][y] & (1 << (2 - x))) b.set(8 + x * 2, 6 + y, 4);
    return b;
}

Bitmap paintWake() {
    Bitmap b(40, 12);
    b.ellipse(20, 6, 16, 3, 1);
    b.ellipse(14, 6, 5, 1.5f, 2);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(28, 14);
    b.line(14, 8, up ? 2.f : 4.f, up ? 2.f : 6.f, 1, 1.3f);
    b.line(14, 8, up ? 26.f : 24.f, up ? 2.f : 6.f, 1, 1.3f);
    b.ellipse(14, 8, 2, 1.4f, 2);
    return b;
}

Bitmap paintSun() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 5, 5, 1);
    b.ellipse(8, 8, 2.4f, 2.4f, 2);
    return b;
}

Bitmap paintCloud() {
    Bitmap b(36, 14);
    b.ellipse(12, 8, 8, 4, 1);
    b.ellipse(22, 7, 9, 4, 2);
    return b;
}

Bitmap paintShade() {
    Bitmap b(56, 12);
    b.ellipse(28, 6, 24, 3, 1);
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
    const uint16_t ink = gs::rgb4(1, 2, 4);
    textPal(vdp, PAL_HUD, gs::rgb4(14, 15, 15), gs::rgb4(2, 4, 7));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 8, 3), gs::rgb4(5, 2, 1));
    textPal(vdp, PAL_WIN, gs::rgb4(9, 15, 12), gs::rgb4(1, 5, 4));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 13, 6), gs::rgb4(5, 3, 1));
    textPal(vdp, PAL_TAG, gs::rgb4(11, 15, 15), gs::rgb4(2, 5, 8));

    setPal(vdp, PAL_BOAT,
           {0, gs::rgb4(14, 14, 12), gs::rgb4(8, 9, 10), gs::rgb4(12, 4, 2), gs::rgb4(6, 5, 4), gs::rgb4(13, 6, 3),
            gs::rgb4(9, 5, 2), gs::rgb4(3, 3, 4), gs::rgb4(4, 6, 8), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BUOY,
           {0, gs::rgb4(14, 8, 2), gs::rgb4(6, 5, 3), gs::rgb4(15, 14, 8), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, ink});
    setPal(vdp, PAL_REEF, {0, gs::rgb4(2, 8, 4), gs::rgb4(5, 11, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SHED,
           {0, gs::rgb4(9, 8, 6), gs::rgb4(7, 3, 2), gs::rgb4(3, 3, 4), gs::rgb4(12, 13, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, ink});
    setPal(vdp, PAL_FINISH,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(7, 7, 6), gs::rgb4(12, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SKY,
           {0, gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MARK,
           {0, gs::rgb4(12, 10, 4), gs::rgb4(4, 4, 2), gs::rgb4(15, 12, 4), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, ink});
    setPal(vdp, PAL_SEA,
           {gs::rgb4(1, 3, 8), gs::rgb4(2, 5, 10), gs::rgb4(3, 7, 12), gs::rgb4(4, 9, 13), gs::rgb4(6, 11, 14),
            gs::rgb4(3, 8, 12), gs::rgb4(2, 6, 11), gs::rgb4(1, 4, 9), gs::rgb4(5, 10, 13), gs::rgb4(7, 12, 14),
            gs::rgb4(2, 6, 10), gs::rgb4(1, 3, 7), gs::rgb4(8, 13, 14), gs::rgb4(2, 4, 8), gs::rgb4(10, 14, 15),
            gs::rgb4(3, 7, 11)});

    vdp.setFogColor(gs::rgb4(8, 11, 14));
    loadFont(vdp, art);
    for (int i = 0; i < kHeels; i++) art.boat[i] = gs::uploadMipped(vdp, paintBoat(i));
    art.wreck = gs::uploadMipped(vdp, paintWreck());
    art.stake = gs::uploadMipped(vdp, paintStake());
    art.reef = gs::uploadMipped(vdp, paintReef());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.finish = gs::uploadMipped(vdp, paintFinish());
    for (int i = 0; i < 3; i++) art.mark[i] = gs::uploadMipped(vdp, paintMark(i));
    art.wake = gs::uploadMipped(vdp, paintWake());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(true));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(false));
    art.sun = gs::uploadMipped(vdp, paintSun());
    art.cloud = gs::uploadMipped(vdp, paintCloud());
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.title = words(vdp, "HEADER TURN", 3);
    art.tipped = words(vdp, "TIPPED", 3);
    art.missed = words(vdp, "MISSED", 3);
    art.made = words(vdp, "LEG MADE", 3);
    art.held = words(vdp, "HELD", 3);
}

}  // namespace headerturn
