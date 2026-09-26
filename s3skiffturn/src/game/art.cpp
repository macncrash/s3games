#include "art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace skiffturn {
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

// Screen y grows downward. Positive bank drops the starboard side.
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

// Stern of a varnished skiff: cream topsides, green bottom, one skipper, a kicker.
Bitmap paintStern(float bank) {
    Bitmap b(96, 96);
    const float cx = 48.f, cy = 48.f;
    const float lean = -bank * 8.f;
    polyL(b, bank, cx, cy,
          {{-34.f, -6.f}, {34.f, -6.f}, {30.f, 12.f}, {12.f, 20.f}, {-12.f, 20.f}, {-30.f, 12.f}}, 2);
    polyL(b, bank, cx, cy, {{-28.f, 8.f}, {28.f, 8.f}, {22.f, 16.f}, {10.f, 19.f}, {-10.f, 19.f}, {-22.f, 16.f}}, 5);
    polyL(b, bank, cx, cy, {{-30.f, 2.f}, {30.f, 2.f}, {28.f, 7.f}, {-28.f, 7.f}}, 6);
    polyL(b, bank, cx, cy, {{-32.f, -10.f}, {32.f, -10.f}, {33.f, -5.f}, {-33.f, -5.f}}, 1);
    polyL(b, bank, cx, cy, {{-22.f, -4.f}, {22.f, -4.f}, {18.f, 6.f}, {-18.f, 6.f}}, 4);
    polyL(b, bank, cx, cy, {{-3.f, 16.f}, {3.f, 16.f}, {2.f, 30.f}, {-2.f, 30.f}}, 9);
    polyL(b, bank, cx, cy, {{-5.f, 28.f}, {5.f, 28.f}, {0.f, 33.f}}, 11);
    polyL(b, bank, cx, cy, {{-6.f, -2.f}, {6.f, -2.f}, {5.f, 14.f}, {-5.f, 14.f}}, 9);
    blob(b, bank, cx, cy, 0.f, -1.f, 7.f, 5.f, 13);
    blob(b, bank, cx, cy, -2.f, -3.f, 1.4f, 1.1f, 11);
    lineL(b, bank, cx, cy, -26.f, -8.f, -20.f, -14.f, 11, 1.4f);
    lineL(b, bank, cx, cy, 26.f, -8.f, 20.f, -14.f, 11, 1.4f);
    blob(b, bank, cx, cy, lean, -22.f, 5.2f, 5.f, 7);
    blob(b, bank, cx, cy, lean, -26.f, 6.4f, 2.6f, 14);
    blob(b, bank, cx, cy, lean - 1.4f, -23.f, 0.7f, 0.7f, 15);
    blob(b, bank, cx, cy, lean + 1.6f, -23.f, 0.7f, 0.7f, 15);
    polyL(b, bank, cx, cy,
          {{lean - 7.f, -16.f}, {lean + 7.f, -16.f}, {lean + 6.f, -4.f}, {lean - 6.f, -4.f}}, 8);
    polyL(b, bank, cx, cy, {{lean - 4.f, -14.f}, {lean + 4.f, -14.f}, {lean + 3.f, -8.f}, {lean - 3.f, -8.f}}, 6);
    lineL(b, bank, cx, cy, lean - 6.f, -12.f, -24.f, -8.f, 7, 2.2f);
    lineL(b, bank, cx, cy, lean + 6.f, -12.f, 8.f, -2.f, 7, 2.2f);
    lineL(b, bank, cx, cy, 4.f, 2.f, lean + 5.f, -8.f, 13, 1.6f);
    b.outline(15, false);
    return b;
}

Bitmap paintWreck() {
    Bitmap b(104, 72);
    b.ellipse(58, 46, 28, 8, 10);
    b.ellipse(40, 44, 16, 5, 10);
    b.poly({{18, 30}, {78, 22}, {86, 36}, {22, 46}}, 2);
    b.poly({{24, 34}, {74, 28}, {78, 36}, {28, 42}}, 5);
    b.poly({{30, 28}, {70, 24}, {72, 30}, {32, 34}}, 1);
    b.poly({{70, 26}, {92, 18}, {94, 28}, {74, 34}}, 9);
    b.ellipse(90, 24, 3.2f, 3.2f, 11);
    b.ellipse(34, 18, 5, 5, 7);
    b.ellipse(34, 14, 6, 2.4f, 14);
    b.ellipse(48, 22, 4, 3, 8);
    b.line(40, 20, 58, 26, 7, 2.f);
    b.line(22, 40, 8, 28, 1, 1.6f);
    b.line(16, 36, 6, 40, 1, 1.4f);
    b.outline(15, false);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(18, 40);
    b.rect(8, 16, 2, 16, 1);
    b.poly({{9, 1}, {16, 16}, {2, 16}}, 2);
    b.poly({{9, 8}, {13, 16}, {5, 16}}, 3);
    b.ellipse(9, 32, 5.5f, 3.2f, 4);
    b.ellipse(7, 6, 1.3f, 1.3f, 5);
    b.ellipse(9, 36, 2.2f, 1.1f, 1);
    return b;
}

void stampDigit(Bitmap& b, int digit, int cx, int cy, int scale, int col) {
    const char* rows[5];
    if (digit == 1) {
        rows[0] = ".#.";
        rows[1] = ".#.";
        rows[2] = ".#.";
        rows[3] = ".#.";
        rows[4] = ".#.";
    } else if (digit == 2) {
        rows[0] = "###";
        rows[1] = "..#";
        rows[2] = "###";
        rows[3] = "#..";
        rows[4] = "###";
    } else {
        rows[0] = "###";
        rows[1] = "..#";
        rows[2] = "###";
        rows[3] = "..#";
        rows[4] = "###";
    }
    int w = 3 * scale, h = 5 * scale;
    int x0 = cx - w / 2, y0 = cy - h / 2;
    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 3; c++) {
            if (rows[r][c] == '#') b.rect(float(x0 + c * scale), float(y0 + r * scale), float(scale), float(scale), col);
        }
    }
}

Bitmap paintBoard(int digit, int dir) {
    Bitmap b(40, 56);
    b.rect(18, 28, 4, 22, 1);
    b.poly({{20, 2}, {36, 18}, {20, 34}, {4, 18}}, 2);
    b.poly({{20, 6}, {32, 18}, {20, 30}, {8, 18}}, 3);
    stampDigit(b, digit, 20, 16, 2, 4);
    if (dir < 0) b.poly({{14, 24}, {22, 21}, {22, 27}}, 4);
    else b.poly({{26, 24}, {18, 21}, {18, 27}}, 4);
    b.ellipse(20, 50, 6, 2.2f, 5);
    b.outline(4, false);
    return b;
}

Bitmap paintReed() {
    Bitmap b(22, 32);
    b.line(4, 30, 3, 8, 1, 1.5f);
    b.line(10, 30, 12, 3, 2, 1.7f);
    b.line(16, 30, 15, 10, 1, 1.4f);
    b.line(8, 30, 6, 14, 2, 1.2f);
    b.ellipse(12, 3, 1.8f, 2.2f, 3);
    return b;
}

Bitmap paintHeron() {
    Bitmap b(28, 44);
    b.line(12, 40, 10, 26, 5, 1.3f);
    b.line(16, 40, 15, 26, 5, 1.3f);
    b.ellipse(14, 24, 6, 4, 2);
    b.ellipse(16, 22, 3, 2, 3);
    b.line(14, 22, 18, 8, 2, 1.6f);
    b.ellipse(18, 7, 2.2f, 2.f, 2);
    b.line(20, 7, 26, 6, 4, 1.4f);
    return b;
}

Bitmap paintShack() {
    Bitmap b(56, 44);
    b.rect(8, 18, 4, 20, 1);
    b.rect(44, 18, 4, 20, 1);
    b.rect(10, 16, 36, 16, 2);
    b.poly({{6, 18}, {28, 4}, {50, 18}}, 3);
    b.rect(24, 20, 10, 12, 4);
    b.rect(14, 20, 7, 6, 5);
    b.rect(36, 20, 7, 6, 5);
    b.rect(40, 8, 2, 8, 1);
    b.outline(6, false);
    return b;
}

Bitmap paintDock() {
    Bitmap b(48, 28);
    b.rect(4, 8, 40, 8, 1);
    b.rect(4, 8, 40, 2, 2);
    for (int x = 8; x < 42; x += 8) b.rect(x, 10, 1, 6, 3);
    b.rect(8, 16, 3, 10, 4);
    b.rect(36, 16, 3, 10, 4);
    b.ellipse(9, 26, 4, 1.4f, 5);
    b.ellipse(37, 26, 4, 1.4f, 5);
    b.outline(6, false);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(30, 14);
    b.ellipse(15, 8, 2.4f, 1.6f, 1);
    float tip = up ? 2.f : 11.f;
    b.line(15, 7, 2, tip, 1, 1.6f);
    b.line(15, 7, 28, tip, 1, 1.6f);
    b.line(15, 8, 6, tip + 1.f, 2, 1.1f);
    b.line(15, 8, 24, tip + 1.f, 2, 1.1f);
    b.set(18, 7, 4);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(18, 10);
    b.ellipse(9, 5, 7, 3.2f, 1);
    b.ellipse(9, 5, 3.4f, 1.5f, 2);
    return b;
}

Bitmap paintSpray() {
    Bitmap b(16, 14);
    b.ellipse(8, 9, 5, 3, 1);
    b.ellipse(5, 6, 2.f, 2.4f, 2);
    b.ellipse(11, 5, 1.6f, 2.f, 1);
    return b;
}

Bitmap paintShadow() {
    Bitmap b(36, 12);
    b.ellipse(18, 6, 16, 4, 1);
    return b;
}

Bitmap paintCloud() {
    Bitmap b(48, 18);
    b.ellipse(16, 10, 10, 5, 1);
    b.ellipse(28, 8, 12, 6, 1);
    b.ellipse(36, 11, 8, 4, 2);
    b.ellipse(22, 11, 6, 3, 2);
    return b;
}

Bitmap paintSun() {
    Bitmap b(22, 22);
    b.ellipse(11, 11, 8, 8, 4);
    b.ellipse(11, 11, 5, 5, 3);
    b.ellipse(9, 9, 1.6f, 1.6f, 1);
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
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(6, 8, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(15, 14, 11), gs::rgb4(14, 14, 12), gs::rgb4(10, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(2, 6, 4),
            gs::rgb4(12, 2, 2), gs::rgb4(13, 8, 5), gs::rgb4(2, 4, 8), gs::rgb4(2, 2, 3), gs::rgb4(14, 15, 14),
            gs::rgb4(12, 10, 4), gs::rgb4(4, 3, 2), gs::rgb4(8, 5, 3), gs::rgb4(14, 12, 6), ink});
    setPal(vdp, PAL_RED, {0, gs::rgb4(3, 3, 3), gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(7, 1, 1),
                          gs::rgb4(15, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(3, 3, 3), gs::rgb4(2, 10, 4), gs::rgb4(15, 15, 14), gs::rgb4(1, 5, 2),
                            gs::rgb4(8, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_REED, {0, gs::rgb4(6, 9, 3), gs::rgb4(3, 6, 2), gs::rgb4(10, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           ink});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(12, 8, 4), gs::rgb4(15, 13, 8), gs::rgb4(8, 4, 2), gs::rgb4(4, 3, 2),
                           gs::rgb4(9, 12, 13), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 14, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 11, 12), gs::rgb4(5, 5, 6), gs::rgb4(14, 10, 3),
                           gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 8, 3), gs::rgb4(6, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 7), gs::rgb4(1, 5, 2), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 7), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TAG, {0, gs::rgb4(12, 14, 13), gs::rgb4(4, 6, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CREEK,
           {0, gs::rgb4(7, 9, 3), gs::rgb4(4, 6, 2), gs::rgb4(9, 8, 4), gs::rgb4(8, 6, 3), gs::rgb4(5, 4, 2), 0, 0,
            gs::rgb4(11, 10, 7), 0, 0, gs::rgb4(2, 5, 6), gs::rgb4(3, 8, 8), gs::rgb4(13, 15, 12), 0, 0});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(8, 5, 2), gs::rgb4(14, 10, 3), gs::rgb4(15, 14, 10), gs::rgb4(2, 2, 3),
                           gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 11, 10), gs::rgb4(15, 14, 8), gs::rgb4(14, 9, 3), 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, ink});
    vdp.setFogColor(gs::rgb4(10, 11, 8));

    const float banks[7] = {-0.70f, -0.46f, -0.23f, 0.f, 0.23f, 0.46f, 0.70f};
    for (int i = 0; i < 7; i++) art.stern[i] = gs::uploadMipped(vdp, paintStern(banks[i]));
    art.wreck = gs::uploadMipped(vdp, paintWreck());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.board[0] = gs::uploadMipped(vdp, paintBoard(1, -1));
    art.board[1] = gs::uploadMipped(vdp, paintBoard(2, 1));
    art.board[2] = gs::uploadMipped(vdp, paintBoard(3, -1));
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.heron = gs::uploadMipped(vdp, paintHeron());
    art.shack = gs::uploadMipped(vdp, paintShack());
    art.dock = gs::uploadMipped(vdp, paintDock());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(false));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(true));
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.spray = gs::uploadMipped(vdp, paintSpray());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.cloud = gs::uploadMipped(vdp, paintCloud());
    art.sun = gs::uploadMipped(vdp, paintSun());
    art.title = words(vdp, "SKIFF TURN", 3);
    art.steady = words(vdp, "THREE TURNS", 3);
    art.tipped = words(vdp, "TIPPED", 3);
    art.missed = words(vdp, "MISSED", 3);
    art.paused = words(vdp, "PAUSED", 3);
    loadFont(vdp, art);
}

}  // namespace skiffturn
