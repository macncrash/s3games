#include "art.h"

#include <algorithm>
#include <initializer_list>
#include <vector>

namespace bargeturn {
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

// Rear view of a hopper barge with a tug tucked behind. pose 0 lists port, 2 sits flat, 4 lists starboard.
Bitmap paintBarge(int pose) {
    Bitmap b(120, 96);
    const float roll = (pose - 2) * 8.2f;
    auto X = [&](float x, float y) { return x + roll * (1.f - y / 96.f); };
    auto poly = [&](std::initializer_list<gs::Pt> pts, int c) {
        std::vector<gs::Pt> w;
        for (const gs::Pt& p : pts) w.push_back({X(p.first, p.second), p.second});
        b.poly(w, c);
    };
    const float liftL = std::max(0, pose - 2) * 6.f;
    const float liftR = std::max(0, 2 - pose) * 6.f;
    const float slide = (pose - 2) * 4.2f;

    poly({{18, 58 - liftL * 0.2f}, {102, 58 - liftR * 0.2f}, {108, 78 - liftR}, {12, 78 - liftL}}, 1);
    poly({{26, 62}, {94, 62}, {98, 74}, {22, 74}}, 2);
    b.line(X(22, 76 - liftL), 76 - liftL, X(98, 76 - liftR), 76 - liftR, 10, 2.2f);
    poly({{34 + slide, 40}, {86 + slide, 40}, {90 + slide, 60}, {30 + slide, 60}}, 3);
    poly({{40 + slide * 1.15f, 44}, {58 + slide * 1.15f, 44}, {60 + slide * 1.15f, 56}, {38 + slide * 1.15f, 56}}, 4);
    poly({{62 + slide * 1.15f, 46}, {78 + slide * 1.15f, 46}, {80 + slide * 1.15f, 56}, {60 + slide * 1.15f, 56}}, 12);
    b.rect(X(46, 28), 28, 28, 14, 6);
    b.rect(X(52, 32), 32, 8, 6, 7);
    b.rect(X(64, 32), 32, 6, 6, 7);
    b.ellipse(X(60, 22), 22, 5, 3, 9);
    b.line(X(14, 70), 70, X(8, 86), 86, 5, 2.f);
    b.line(X(106, 70), 70, X(112, 86), 86, 5, 2.f);
    return b;
}

Bitmap paintWreck() {
    Bitmap b(120, 70);
    b.poly({{10, 40}, {110, 28}, {108, 52}, {14, 58}}, 1);
    b.poly({{30, 22}, {70, 18}, {74, 36}, {28, 40}}, 3);
    b.rect(48, 12, 18, 10, 6);
    b.ellipse(20, 48, 10, 4, 5);
    b.ellipse(90, 36, 12, 5, 5);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(16, 36);
    b.rect(7, 14, 2, 20, 2);
    b.ellipse(8, 10, 6, 6, 1);
    b.rect(6, 8, 4, 2, 3);
    return b;
}

Bitmap paintReed() {
    Bitmap b(18, 28);
    b.line(4, 26, 3, 6, 1, 1.4f);
    b.line(8, 26, 9, 2, 2, 1.4f);
    b.line(13, 26, 15, 8, 1, 1.4f);
    b.ellipse(3, 6, 2, 1.2f, 3);
    b.ellipse(9, 2, 2, 1.2f, 3);
    return b;
}

Bitmap paintMill() {
    Bitmap b(48, 56);
    b.rect(8, 22, 28, 30, 1);
    b.poly({{6, 22}, {22, 8}, {38, 22}}, 2);
    b.rect(18, 34, 8, 12, 4);
    b.ellipse(40, 36, 8, 8, 3);
    b.line(32, 36, 48, 36, 5, 1.2f);
    b.line(40, 28, 40, 44, 5, 1.2f);
    return b;
}

Bitmap paintCrane() {
    Bitmap b(40, 64);
    b.rect(4, 50, 22, 12, 1);
    b.line(10, 50, 28, 8, 2, 2.4f);
    b.line(18, 50, 32, 12, 2, 2.f);
    b.line(28, 8, 36, 14, 3, 1.6f);
    b.line(34, 14, 34, 28, 4, 1.2f);
    return b;
}

Bitmap paintSign(int n) {
    Bitmap b(28, 36);
    b.rect(12, 16, 3, 18, 2);
    b.rect(4, 4, 20, 14, 1);
    b.rect(6, 6, 16, 10, 3);
    static const int dig[3][5] = {
        {0b010, 0b110, 0b010, 0b010, 0b111},
        {0b111, 0b001, 0b111, 0b100, 0b111},
        {0b111, 0b001, 0b111, 0b001, 0b111},
    };
    int d = std::clamp(n - 1, 0, 2);
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 3; x++)
            if (dig[d][y] & (1 << (2 - x))) b.set(10 + x * 2, 8 + y, 4);
    return b;
}

Bitmap paintWake() {
    Bitmap b(28, 12);
    b.ellipse(14, 6, 12, 4, 1);
    b.ellipse(10, 6, 4, 2, 2);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(28, 12);
    float tip = up ? 2.f : 8.f;
    b.line(14, 6, 1, tip, 1, 1.6f);
    b.line(14, 6, 27, tip, 1, 1.6f);
    b.set(14, 6, 2);
    return b;
}

Bitmap paintSun() {
    Bitmap b(20, 20);
    b.ellipse(10, 10, 7, 7, 1);
    b.ellipse(10, 10, 3.5f, 3.5f, 2);
    return b;
}

Bitmap paintCloud() {
    Bitmap b(44, 18);
    b.ellipse(14, 10, 10, 5, 1);
    b.ellipse(28, 9, 10, 5, 1);
    b.ellipse(20, 7, 7, 4, 2);
    return b;
}

Bitmap paintShadow() {
    Bitmap b(70, 16);
    b.ellipse(35, 8, 30, 5, 1);
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
    textPal(vdp, PAL_HUD, gs::rgb4(14, 15, 14), gs::rgb4(3, 6, 7));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1));
    textPal(vdp, PAL_WIN, gs::rgb4(8, 15, 10), gs::rgb4(1, 5, 3));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 12, 6), gs::rgb4(6, 4, 1));
    textPal(vdp, PAL_TAG, gs::rgb4(11, 15, 15), gs::rgb4(2, 5, 7));

    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(8, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(11, 8, 4), gs::rgb4(12, 3, 2), gs::rgb4(6, 10, 12),
            gs::rgb4(2, 2, 3), gs::rgb4(14, 13, 8), gs::rgb4(9, 6, 3), gs::rgb4(15, 15, 14), gs::rgb4(13, 9, 5),
            gs::rgb4(5, 4, 3), gs::rgb4(10, 7, 3), gs::rgb4(3, 5, 6), gs::rgb4(1, 1, 2), ink});
    setPal(vdp, PAL_BUOY,
           {0, gs::rgb4(14, 4, 2), gs::rgb4(6, 5, 3), gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_REED,
           {0, gs::rgb4(2, 7, 3), gs::rgb4(4, 10, 4), gs::rgb4(8, 12, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MILL,
           {0, gs::rgb4(12, 10, 8), gs::rgb4(8, 3, 2), gs::rgb4(5, 6, 7), gs::rgb4(3, 3, 4), gs::rgb4(14, 14, 13), 0,
            0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CRANE,
           {0, gs::rgb4(7, 8, 8), gs::rgb4(4, 5, 5), gs::rgb4(12, 10, 4), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(10, 13, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 12), gs::rgb4(14, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, ink});
    setPal(vdp, PAL_SIGN,
           {0, gs::rgb4(12, 9, 4), gs::rgb4(5, 4, 2), gs::rgb4(14, 13, 8), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, ink});
    setPal(vdp, PAL_RIVER,
           {gs::rgb4(2, 5, 7), gs::rgb4(3, 7, 9), gs::rgb4(4, 9, 10), gs::rgb4(6, 11, 11), gs::rgb4(8, 13, 12),
            gs::rgb4(5, 10, 11), gs::rgb4(3, 8, 9), gs::rgb4(2, 6, 8), gs::rgb4(7, 12, 11), gs::rgb4(9, 14, 13),
            gs::rgb4(4, 8, 8), gs::rgb4(1, 4, 6), gs::rgb4(10, 14, 12), gs::rgb4(3, 6, 7), gs::rgb4(12, 15, 14),
            gs::rgb4(5, 9, 9)});

    vdp.setFogColor(gs::rgb4(10, 12, 13));
    loadFont(vdp, art);
    for (int i = 0; i < kPoses; i++) art.barge[i] = gs::uploadMipped(vdp, paintBarge(i));
    art.wreck = gs::uploadMipped(vdp, paintWreck());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.mill = gs::uploadMipped(vdp, paintMill());
    art.crane = gs::uploadMipped(vdp, paintCrane());
    for (int i = 0; i < 3; i++) art.sign[i] = gs::uploadMipped(vdp, paintSign(i + 1));
    art.wake = gs::uploadMipped(vdp, paintWake());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(true));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(false));
    art.sun = gs::uploadMipped(vdp, paintSun());
    art.cloud = gs::uploadMipped(vdp, paintCloud());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.title = words(vdp, "BARGE TURN", 3);
    art.tipped = words(vdp, "TIPPED", 3);
    art.missed = words(vdp, "MISSED", 3);
    art.upright = words(vdp, "UPRIGHT", 3);
    art.paused = words(vdp, "HELD", 3);
}

}  // namespace bargeturn
