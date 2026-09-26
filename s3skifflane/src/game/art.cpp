#include "art.h"

#include <initializer_list>
#include <vector>

namespace skifflane {
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

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t shade) {
    const uint16_t edge = gs::rgb4(1, 1, 2);
    setPal(vdp, pal, {0, ink, shade, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, edge});
}

// Rear view of a transom skiff. bank -1, 0, 1 rolls the sheer.
Bitmap paintStern(int bank) {
    Bitmap b(84, 100);
    const float roll = float(bank) * 8.f;
    auto P = [&](float x, float y) -> Pt {
        float t = (y - 48.f) / 52.f;
        return {x + roll * t, y};
    };
    auto poly = [&](std::initializer_list<Pt> pts, int col) {
        std::vector<Pt> w;
        w.reserve(pts.size());
        for (const Pt& p : pts) w.push_back(P(p.first, p.second));
        b.poly(w, col);
    };
    auto blob = [&](float x, float y, float rx, float ry, int col) {
        Pt p = P(x, y);
        b.ellipse(p.first, p.second, rx, ry, col);
    };
    auto stroke = [&](float x0, float y0, float x1, float y1, int col, float th) {
        Pt a = P(x0, y0);
        Pt c = P(x1, y1);
        b.line(a.first, a.second, c.first, c.second, col, th);
    };

    poly({{42, 8}, {68, 78}, {62, 94}, {22, 94}, {16, 78}}, 1);
    poly({{42, 16}, {60, 76}, {54, 88}, {30, 88}, {24, 76}}, 2);
    poly({{42, 24}, {52, 74}, {32, 74}}, 3);
    poly({{20, 80}, {64, 80}, {60, 88}, {24, 88}}, 4);
    poly({{28, 86}, {56, 86}, {54, 92}, {30, 92}}, 11);
    stroke(30, 40, 54, 40, 9, 2.2f);
    stroke(28, 56, 56, 56, 9, 2.2f);
    stroke(26, 70, 58, 70, 9, 2.4f);
    blob(42, 30, 5.2f, 5.4f, 5);
    blob(42, 27, 5.4f, 3.2f, 6);
    poly({{34, 34}, {50, 34}, {54, 48}, {30, 48}}, 4);
    poly({{36, 36}, {48, 36}, {50, 46}, {34, 46}}, 6);
    stroke(50, 42, 62, 64, 5, 2.3f);
    stroke(34, 44, 24, 62, 5, 2.1f);
    stroke(58, 66, 46, 78, 9, 2.4f);
    b.rect(38, 86, 8, 12, 7);
    b.rect(39, 88, 6, 4, 8);
    blob(42, 98, 3.2f, 1.6f, 8);
    stroke(18, 76, 66, 76, 12, 1.6f);
    b.outline(15, false);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(18, 40);
    b.rect(8, 14, 2, 18, 2);
    b.poly({{9, 2}, {16, 16}, {2, 16}}, 1);
    b.poly({{9, 5}, {13, 15}, {9, 15}}, 4);
    b.ellipse(9, 34, 5, 2.2f, 3);
    b.rect(8, 16, 2, 3, 4);
    return b;
}

Bitmap paintPost() {
    Bitmap b(14, 64);
    b.rect(5, 8, 4, 50, 2);
    b.rect(4, 6, 6, 8, 1);
    b.rect(4, 22, 6, 3, 4);
    b.rect(4, 40, 6, 3, 4);
    b.ellipse(7, 60, 5, 2.2f, 3);
    b.outline(3, false);
    return b;
}

Bitmap paintReed() {
    Bitmap b(22, 32);
    b.line(4, 30, 3, 8, 1, 1.5f);
    b.line(8, 30, 7, 4, 2, 1.6f);
    b.line(12, 30, 14, 2, 1, 1.7f);
    b.line(16, 30, 15, 9, 2, 1.4f);
    b.line(19, 30, 18, 12, 1, 1.3f);
    b.ellipse(14, 3, 1.7f, 1.5f, 3);
    b.ellipse(7, 5, 1.3f, 1.2f, 3);
    return b;
}

Bitmap paintShack() {
    Bitmap b(56, 44);
    b.rect(8, 16, 40, 24, 1);
    b.poly({{6, 18}, {28, 4}, {50, 18}}, 2);
    b.rect(24, 24, 10, 16, 3);
    b.rect(12, 22, 8, 7, 4);
    b.rect(36, 22, 8, 7, 5);
    b.rect(40, 6, 2, 12, 6);
    b.outline(7, false);
    return b;
}

Bitmap paintDock() {
    Bitmap b(48, 28);
    b.rect(4, 8, 40, 14, 1);
    for (int x = 8; x < 42; x += 6) b.rect(x, 8, 2, 14, 2);
    b.rect(6, 20, 4, 6, 3);
    b.rect(38, 20, 4, 6, 3);
    b.ellipse(16, 14, 3.2f, 2.2f, 4);
    b.outline(6, false);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(18, 12);
    b.ellipse(9, 6, 7, 3.4f, 1);
    b.ellipse(6, 6, 2.4f, 1.4f, 2);
    b.ellipse(12, 5, 2.2f, 1.3f, 2);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(20, 16);
    b.rect(2, 1, 2, 14, 2);
    b.poly({{4, 2}, {17, 6}, {4, 10}}, 1);
    b.line(4, 6, 15, 6, 3, 1.f);
    return b;
}

Bitmap paintBar() {
    Bitmap b(96, 10);
    b.rect(2, 3, 92, 4, 1);
    b.rect(2, 3, 92, 1, 2);
    for (int x = 6; x < 90; x += 10) b.rect(x, 3, 2, 4, 3);
    return b;
}

Bitmap paintCan() {
    Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

Bitmap paintShadow() {
    Bitmap b(20, 10);
    b.ellipse(10, 5, 8, 3, 1);
    return b;
}

Bitmap paintSun() {
    Bitmap b(18, 18);
    b.ellipse(9, 9, 7, 7, 1);
    b.ellipse(9, 9, 3.5f, 3.5f, 2);
    return b;
}

Bitmap paintCloud() {
    Bitmap b(40, 18);
    b.ellipse(14, 10, 10, 6, 1);
    b.ellipse(26, 9, 9, 5, 1);
    b.ellipse(20, 8, 7, 4, 2);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(28, 14);
    b.ellipse(14, 8, 2.2f, 1.6f, 1);
    float tip = up ? 2.f : 11.f;
    b.line(14, 7, 2, tip, 1, 1.6f);
    b.line(14, 7, 26, tip, 1, 1.6f);
    b.set(18, 7, 2);
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
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14), gs::rgb4(7, 10, 11));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 7, 3), gs::rgb4(5, 1, 1));
    textPal(vdp, PAL_WIN, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 13, 6), gs::rgb4(6, 3, 1));
    textPal(vdp, PAL_TAG, gs::rgb4(12, 15, 14), gs::rgb4(3, 6, 7));

    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(9, 12, 13), gs::rgb4(11, 8, 4), gs::rgb4(13, 2, 2),
            gs::rgb4(13, 9, 6), gs::rgb4(2, 6, 3), gs::rgb4(2, 2, 3), gs::rgb4(10, 11, 12), gs::rgb4(8, 5, 2), 0,
            gs::rgb4(3, 5, 6), gs::rgb4(14, 14, 12), 0, 0, ink});
    setPal(vdp, PAL_RED,
           {0, gs::rgb4(14, 2, 2), gs::rgb4(7, 7, 7), gs::rgb4(2, 2, 3), gs::rgb4(15, 14, 12), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, ink});
    setPal(vdp, PAL_GREEN,
           {0, gs::rgb4(3, 12, 4), gs::rgb4(7, 7, 7), gs::rgb4(2, 2, 3), gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, ink});
    setPal(vdp, PAL_REED,
           {0, gs::rgb4(3, 8, 2), gs::rgb4(6, 11, 3), gs::rgb4(10, 12, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(12, 8, 3), gs::rgb4(8, 5, 2), gs::rgb4(4, 3, 2), gs::rgb4(9, 6, 3), gs::rgb4(14, 12, 6),
            gs::rgb4(3, 3, 3), ink, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(14, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 15, 14), gs::rgb4(13, 3, 2), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(13, 2, 2), gs::rgb4(4, 4, 4), gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    // Channel. 6/7 stay water-like if a scanline borrows the centre-line style.
    // 4/5 are the buoy line, 1-3 the reed banks, 11-13 the lane itself.
    setPal(vdp, PAL_LANE,
           {0, gs::rgb4(6, 10, 3), gs::rgb4(3, 7, 2), gs::rgb4(9, 8, 4), gs::rgb4(15, 14, 8), gs::rgb4(11, 8, 3),
            gs::rgb4(6, 12, 13), gs::rgb4(3, 8, 11), gs::rgb4(13, 11, 6), 0, 0, gs::rgb4(8, 14, 14),
            gs::rgb4(3, 9, 12), gs::rgb4(14, 15, 14), gs::rgb4(15, 15, 15), gs::rgb4(2, 4, 5)});

    loadFont(vdp, art);
    for (int i = 0; i < 3; i++) art.stern[i] = gs::uploadMipped(vdp, paintStern(i - 1));
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.shack = gs::uploadMipped(vdp, paintShack());
    art.dock = gs::uploadMipped(vdp, paintDock());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.bar = gs::uploadMipped(vdp, paintBar());
    art.can = gs::uploadMipped(vdp, paintCan());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.sun = gs::uploadMipped(vdp, paintSun());
    art.cloud = gs::uploadMipped(vdp, paintCloud());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(true));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(false));
    art.title = words(vdp, "SKIFF LANE", 3);
    art.held = words(vdp, "HELD THE LANE", 2);
    art.whole = words(vdp, "THE WHOLE LEG", 2);
    art.left = words(vdp, "LEFT THE LANE", 2);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.paused = words(vdp, "PAUSED", 3);
    art.end = words(vdp, "END", 2);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(12, 14, 12));
}

}  // namespace skifflane
