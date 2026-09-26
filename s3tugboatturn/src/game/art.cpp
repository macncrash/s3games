#include "art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace tugturn {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Pt spin(float x, float y, float bank, float cx, float cy) {
    float c = std::cos(bank), s = std::sin(bank);
    return {cx + x * c - y * s, cy + x * s + y * c};
}

void polyL(Bitmap& b, float bank, float cx, float cy, std::initializer_list<Pt> local, int col) {
    std::vector<Pt> w;
    w.reserve(local.size());
    for (const Pt& p : local) w.push_back(spin(p.first, p.second, bank, cx, cy));
    b.poly(w, col);
}

void lineL(Bitmap& b, float bank, float cx, float cy, float x0, float y0, float x1, float y1, int col, float th) {
    Pt a = spin(x0, y0, bank, cx, cy);
    Pt c = spin(x1, y1, bank, cx, cy);
    b.line(a.first, a.second, c.first, c.second, col, th);
}

void blob(Bitmap& b, float bank, float cx, float cy, float x, float y, float rx, float ry, int col) {
    Pt p = spin(x, y, bank, cx, cy);
    b.ellipse(p.first, p.second, rx, ry, col);
}

// Stern of a harbor tug. Positive bank drops the starboard side.
// Screen y grows downward; local +y is toward the keel.
Bitmap paintTug(float bank) {
    Bitmap b(128, 128);
    const float cx = 64.f, cy = 64.f;
    const float lean = -bank * 6.f;
    polyL(b, bank, cx, cy, {{-38.f, -6.f}, {38.f, -6.f}, {34.f, 16.f}, {16.f, 26.f}, {-16.f, 26.f}, {-34.f, 16.f}}, 1);
    polyL(b, bank, cx, cy, {{-32.f, 8.f}, {32.f, 8.f}, {28.f, 20.f}, {14.f, 24.f}, {-14.f, 24.f}, {-28.f, 20.f}}, 2);
    polyL(b, bank, cx, cy, {{-40.f, -12.f}, {40.f, -12.f}, {38.f, -4.f}, {-38.f, -4.f}}, 3);
    polyL(b, bank, cx, cy, {{-30.f, -4.f}, {30.f, -4.f}, {28.f, 8.f}, {-28.f, 8.f}}, 4);
    polyL(b, bank, cx, cy, {{-16.f, -32.f}, {16.f, -32.f}, {15.f, -4.f}, {-15.f, -4.f}}, 5);
    polyL(b, bank, cx, cy, {{-12.f, -26.f}, {-4.f, -26.f}, {-4.f, -14.f}, {-12.f, -14.f}}, 6);
    polyL(b, bank, cx, cy, {{-2.f, -26.f}, {6.f, -26.f}, {6.f, -14.f}, {-2.f, -14.f}}, 6);
    polyL(b, bank, cx, cy, {{8.f, -24.f}, {13.f, -24.f}, {13.f, -16.f}, {8.f, -16.f}}, 6);
    polyL(b, bank, cx, cy, {{-7.f, -54.f}, {7.f, -54.f}, {6.f, -30.f}, {-6.f, -30.f}}, 7);
    polyL(b, bank, cx, cy, {{-8.f, -58.f}, {8.f, -58.f}, {8.f, -52.f}, {-8.f, -52.f}}, 8);
    polyL(b, bank, cx, cy, {{-7.f, -44.f}, {7.f, -44.f}, {7.f, -39.f}, {-7.f, -39.f}}, 9);
    lineL(b, bank, cx, cy, 0.f, -32.f, 0.f, -66.f, 13, 1.5f);
    polyL(b, bank, cx, cy, {{0.f, -66.f}, {12.f, -61.f}, {0.f, -56.f}}, 2);
    blob(b, bank, cx, cy, -36.f, 6.f, 8.f, 8.f, 10);
    blob(b, bank, cx, cy, 36.f, 6.f, 8.f, 8.f, 10);
    blob(b, bank, cx, cy, -36.f, 6.f, 3.4f, 3.4f, 11);
    blob(b, bank, cx, cy, 36.f, 6.f, 3.4f, 3.4f, 11);
    blob(b, bank, cx, cy, -34.f, 4.f, 1.6f, 1.6f, 3);
    blob(b, bank, cx, cy, 38.f, 4.f, 1.6f, 1.6f, 3);
    polyL(b, bank, cx, cy, {{-12.f, 10.f}, {12.f, 10.f}, {12.f, 18.f}, {-12.f, 18.f}}, 12);
    lineL(b, bank, cx, cy, -4.f, 14.f, -4.f, 26.f, 13, 2.f);
    lineL(b, bank, cx, cy, 4.f, 14.f, 4.f, 26.f, 13, 2.f);
    lineL(b, bank, cx, cy, -6.f, 24.f, 6.f, 24.f, 13, 1.6f);
    blob(b, bank, cx, cy, -16.f, 4.f, 2.2f, 2.2f, 6);
    blob(b, bank, cx, cy, 16.f, 4.f, 2.2f, 2.2f, 6);
    blob(b, bank, cx, cy, lean, -20.f, 3.2f, 3.2f, 14);
    blob(b, bank, cx, cy, lean, -23.f, 3.6f, 1.6f, 8);
    lineL(b, bank, cx, cy, -28.f, -8.f, 28.f, -8.f, 3, 1.3f);
    blob(b, bank, cx, cy, 0.f, -60.f, 2.4f, 2.f, 12);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintWreck() {
    Bitmap b(120, 72);
    b.ellipse(64, 44, 40, 14, 1);
    b.ellipse(64, 40, 34, 8, 3);
    b.ellipse(64, 38, 22, 4, 2);
    b.rect(28, 22, 18, 26, 7);
    b.rect(26, 18, 22, 8, 8);
    b.rect(30, 28, 14, 5, 9);
    b.ellipse(46, 36, 5, 4, 6);
    b.ellipse(78, 34, 4, 3, 6);
    b.ellipse(92, 46, 8, 8, 10);
    b.ellipse(92, 46, 3, 3, 11);
    b.ellipse(24, 40, 10, 4, 3);
    b.ellipse(40, 28, 6, 3, 12);
    b.ellipse(18, 34, 4, 2, 3);
    b.rect(54, 16, 8, 10, 5);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintBuoy() {
    Bitmap b(18, 32);
    b.ellipse(9, 18, 7.2f, 8.f, 1);
    b.rect(3, 14, 12, 5, 2);
    b.rect(8, 6, 2, 8, 3);
    b.ellipse(9, 5, 2.3f, 2.3f, 4);
    b.ellipse(9, 24, 5.f, 2.2f, 1);
    b.outline(15, false);
    return b.cropToContent(1);
}

void digit(Bitmap& b, int n, int x, int y, int c) {
    static const int mask[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
    const int W = 16, H = 24, T = 3;
    int s = mask[n < 0 || n > 9 ? 0 : n];
    auto hbar = [&](int yy) { b.rect(float(x + 2), float(yy), float(W - 4), float(T), c); };
    auto vbar = [&](int xx, int yy, int hh) { b.rect(float(xx), float(yy), float(T), float(hh), c); };
    if (s & 0x01) hbar(y);
    if (s & 0x02) vbar(x + W - T - 2, y + 2, H / 2 - 2);
    if (s & 0x04) vbar(x + W - T - 2, y + H / 2 + 1, H / 2 - 2);
    if (s & 0x08) hbar(y + H - T);
    if (s & 0x10) vbar(x, y + H / 2 + 1, H / 2 - 2);
    if (s & 0x20) vbar(x, y + 2, H / 2 - 2);
    if (s & 0x40) hbar(y + H / 2 - T / 2);
}

Bitmap paintBoard(int n, int dir) {
    Bitmap b(48, 68);
    b.rect(20, 40, 8, 26, 2);
    b.rect(4, 4, 40, 40, 1);
    b.rect(7, 7, 34, 34, 3);
    digit(b, n, 16, 12, 4);
    if (dir < 0) {
        b.poly({{8.f, 24.f}, {16.f, 18.f}, {16.f, 30.f}}, 5);
    } else {
        b.poly({{40.f, 24.f}, {32.f, 18.f}, {32.f, 30.f}}, 5);
    }
    b.ellipse(10, 10, 1.1f, 1.1f, 6);
    b.ellipse(38, 10, 1.1f, 1.1f, 6);
    b.ellipse(10, 38, 1.1f, 1.1f, 6);
    b.ellipse(38, 38, 1.1f, 1.1f, 6);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintShed() {
    Bitmap b(76, 56);
    b.rect(6, 18, 62, 34, 1);
    b.rect(4, 12, 66, 10, 6);
    b.rect(8, 14, 58, 4, 7);
    for (int i = 0; i < 4; i++) b.rect(12 + i * 14, 26, 8, 8, 4);
    b.rect(32, 36, 12, 16, 5);
    b.rect(54, 4, 8, 12, 7);
    b.rect(52, 2, 12, 4, 2);
    for (int y = 24; y < 50; y += 6) b.rect(6, y, 62, 1, 2);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintCrane() {
    Bitmap b(72, 96);
    b.rect(12, 28, 8, 64, 1);
    b.rect(8, 24, 16, 12, 2);
    b.rect(12, 26, 8, 6, 4);
    b.line(18, 30, 64, 10, 1, 2.6f);
    b.line(18, 36, 58, 20, 2, 1.5f);
    b.line(56, 14, 56, 44, 6, 1.3f);
    b.rect(52, 44, 8, 5, 5);
    b.rect(10, 88, 12, 4, 2);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintLamp() {
    Bitmap b(18, 48);
    b.rect(7, 14, 3, 30, 1);
    b.line(8, 14, 13, 8, 1, 1.6f);
    b.ellipse(13, 8, 3.4f, 2.8f, 3);
    b.ellipse(13, 8, 1.7f, 1.4f, 4);
    b.ellipse(8, 44, 4.f, 1.6f, 2);
    return b.cropToContent(1);
}

Bitmap paintBollard() {
    Bitmap b(18, 26);
    b.ellipse(9, 8, 6.5f, 4.2f, 1);
    b.rect(6, 8, 6, 12, 2);
    b.ellipse(9, 20, 5.5f, 2.4f, 5);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintGull(bool up) {
    Bitmap b(30, 16);
    if (up) {
        b.line(1, 11, 10, 3, 1, 1.7f);
        b.line(10, 3, 15, 8, 1, 1.6f);
        b.line(15, 8, 20, 2, 1, 1.6f);
        b.line(20, 2, 28, 10, 1, 1.7f);
    } else {
        b.line(1, 6, 10, 9, 1, 1.7f);
        b.line(10, 9, 15, 7, 1, 1.6f);
        b.line(15, 7, 20, 9, 1, 1.6f);
        b.line(20, 9, 28, 5, 1, 1.7f);
    }
    b.ellipse(15, 8, 2.4f, 1.5f, 2);
    b.line(17, 8, 21, 8, 3, 1.2f);
    return b.cropToContent(0);
}

Bitmap paintFoam() {
    Bitmap b(26, 16);
    b.line(3, 3, 13, 13, 1, 1.8f);
    b.line(23, 3, 13, 13, 1, 1.8f);
    b.line(7, 3, 13, 9, 2, 1.3f);
    b.line(19, 3, 13, 9, 2, 1.3f);
    return b.cropToContent(0);
}

Bitmap paintSpray() {
    Bitmap b(20, 16);
    b.ellipse(6, 9, 4.2f, 3.1f, 1);
    b.ellipse(13, 6, 3.4f, 2.6f, 2);
    b.ellipse(10, 12, 2.2f, 1.6f, 1);
    return b.cropToContent(0);
}

Bitmap paintSmoke() {
    Bitmap b(18, 18);
    b.ellipse(9, 10, 7.f, 5.5f, 1);
    b.ellipse(8, 9, 3.6f, 2.6f, 2);
    return b.cropToContent(0);
}

Bitmap paintShadow() {
    Bitmap b(40, 14);
    b.ellipse(20, 7, 17.f, 4.5f, 1);
    return b.cropToContent(0);
}

Bitmap paintCloud() {
    Bitmap b(52, 20);
    b.ellipse(16, 12, 12, 6, 1);
    b.ellipse(30, 9, 14, 7, 1);
    b.ellipse(40, 13, 8, 4, 2);
    b.ellipse(22, 13, 7, 3, 2);
    return b.cropToContent(0);
}

Bitmap paintSun() {
    Bitmap b(24, 24);
    b.ellipse(12, 12, 9, 9, 4);
    b.ellipse(12, 12, 5.5f, 5.5f, 3);
    b.ellipse(10, 10, 1.8f, 1.8f, 1);
    return b.cropToContent(0);
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
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(5, 7, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TUG,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(12, 2, 2), gs::rgb4(14, 14, 13), gs::rgb4(5, 5, 6), gs::rgb4(15, 15, 14),
            gs::rgb4(3, 6, 10), gs::rgb4(13, 2, 2), gs::rgb4(1, 1, 2), gs::rgb4(13, 10, 3), gs::rgb4(13, 11, 3),
            gs::rgb4(2, 2, 2), gs::rgb4(15, 13, 6), gs::rgb4(8, 8, 7), gs::rgb4(13, 9, 6), ink});
    setPal(vdp, PAL_RED, {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(6, 6, 6), gs::rgb4(15, 13, 6), 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GREEN,
           {0, gs::rgb4(2, 10, 4), gs::rgb4(15, 15, 14), gs::rgb4(5, 6, 6), gs::rgb4(14, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, ink});
    setPal(vdp, PAL_PIER, {0, gs::rgb4(10, 4, 3), gs::rgb4(6, 2, 2), gs::rgb4(9, 9, 8), gs::rgb4(3, 6, 9),
                           gs::rgb4(4, 3, 2), gs::rgb4(8, 4, 2), gs::rgb4(4, 2, 1), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_STEEL, {0, gs::rgb4(9, 10, 11), gs::rgb4(4, 5, 6), gs::rgb4(12, 10, 4), gs::rgb4(15, 14, 6),
                            gs::rgb4(8, 5, 2), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 13, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 10, 11), gs::rgb4(14, 10, 3), gs::rgb4(2, 2, 3), 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 8, 3), gs::rgb4(6, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 7), gs::rgb4(1, 5, 2), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 7), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TAG, {0, gs::rgb4(12, 14, 14), gs::rgb4(4, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_HARBOR,
           {0, gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 5), gs::rgb4(9, 7, 5), gs::rgb4(12, 10, 3), gs::rgb4(3, 3, 4), 0, 0,
            gs::rgb4(10, 9, 8), 0, 0, gs::rgb4(1, 3, 7), gs::rgb4(2, 5, 9), gs::rgb4(8, 13, 14), 0, 0});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(13, 12, 9), gs::rgb4(2, 4, 9), gs::rgb4(14, 13, 11), gs::rgb4(6, 5, 4), gs::rgb4(15, 14, 12),
            gs::rgb4(4, 7, 11), gs::rgb4(2, 5, 12), gs::rgb4(1, 1, 2), gs::rgb4(15, 15, 14), gs::rgb4(5, 5, 5),
            gs::rgb4(2, 2, 3), gs::rgb4(15, 14, 6), gs::rgb4(7, 7, 6), gs::rgb4(12, 8, 6), ink});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 14), gs::rgb4(11, 12, 13), gs::rgb4(15, 14, 8), gs::rgb4(14, 9, 3), 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(9, 6, 3), gs::rgb4(5, 3, 2), gs::rgb4(15, 14, 10), gs::rgb4(1, 1, 2),
                           gs::rgb4(12, 3, 2), gs::rgb4(8, 8, 7), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    vdp.setFogColor(gs::rgb4(7, 9, 12));

    const float banks[7] = {-0.62f, -0.40f, -0.20f, 0.f, 0.20f, 0.40f, 0.62f};
    for (int i = 0; i < 7; i++) art.stern[i] = gs::uploadMipped(vdp, paintTug(banks[i]));
    art.wreck = gs::uploadMipped(vdp, paintWreck());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.board[0] = gs::uploadMipped(vdp, paintBoard(1, -1));
    art.board[1] = gs::uploadMipped(vdp, paintBoard(2, 1));
    art.board[2] = gs::uploadMipped(vdp, paintBoard(3, -1));
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.crane = gs::uploadMipped(vdp, paintCrane());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.bollard = gs::uploadMipped(vdp, paintBollard());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(false));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(true));
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.spray = gs::uploadMipped(vdp, paintSpray());
    art.smoke = gs::uploadMipped(vdp, paintSmoke());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.cloud = gs::uploadMipped(vdp, paintCloud());
    art.sun = gs::uploadMipped(vdp, paintSun());
    loadFont(vdp, art);
    art.title = words(vdp, "THREE TURNS", 3);
    art.steady = words(vdp, "STEADY", 3);
    art.tipped = words(vdp, "TIPPED", 3);
    art.beaten = words(vdp, "BEATEN", 3);
    art.missed = words(vdp, "MISSED", 3);
    art.paused = words(vdp, "PAUSED", 2);
}

}  // namespace tugturn
