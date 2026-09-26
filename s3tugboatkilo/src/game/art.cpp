#include "game/art.h"

#include <cmath>
#include <string>
#include <vector>

namespace tugkilo {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr double kTau = 6.283185307179586;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void ink(gs::VDP& vdp, int pal) { vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 1)); }

void textPal(gs::VDP& vdp, int pal, uint16_t face, uint16_t edge) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, face);
    vdp.setColor(pal * 16 + 2, edge);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

Bitmap paintTug(float heading) {
    Bitmap b(210, 210);
    const float cx = 105.f, cy = 105.f, sc = 9.1f;
    const float co = std::cos(heading), sn = std::sin(heading);
    const Pt hull[] = {{4.40f, 0.f},   {3.55f, 1.35f}, {1.40f, 1.78f}, {-2.80f, 1.82f}, {-4.05f, 1.55f},
                       {-4.55f, 0.72f}, {-4.55f, -0.72f}, {-4.05f, -1.55f}, {-2.80f, -1.82f},
                       {1.40f, -1.78f}, {3.55f, -1.35f}};
    auto polyF = [&](const Pt* pts, int n, float scale, int col) {
        std::vector<Pt> p;
        p.reserve(size_t(n));
        for (int i = 0; i < n; i++) {
            float lf = pts[i].first * scale;
            float lr = pts[i].second * scale;
            float wx = lf * co + lr * sn;
            float wy = lf * sn - lr * co;
            p.push_back({cx + wx * sc, cy - wy * sc});
        }
        b.poly(p, col);
    };
    auto polyL = [&](std::initializer_list<Pt> meters, int col) {
        std::vector<Pt> p;
        p.reserve(meters.size());
        for (const Pt& m : meters) {
            float wx = m.first * co + m.second * sn;
            float wy = m.first * sn - m.second * co;
            p.push_back({cx + wx * sc, cy - wy * sc});
        }
        b.poly(p, col);
    };
    auto blob = [&](float lf, float lr, float rx, float ry, int col) {
        float wx = lf * co + lr * sn;
        float wy = lf * sn - lr * co;
        b.ellipse(cx + wx * sc, cy - wy * sc, rx, ry, col);
    };

    polyF(hull, 11, 1.07f, 3);
    polyF(hull, 11, 1.0f, 1);
    polyF(hull, 11, 0.90f, 2);
    polyL({{3.9f, 0.f}, {3.15f, 1.15f}, {1.1f, 1.48f}, {-2.55f, 1.50f}, {-3.7f, 1.22f}, {-4.05f, 0.55f},
           {-4.05f, -0.55f}, {-3.7f, -1.22f}, {-2.55f, -1.50f}, {1.1f, -1.48f}, {3.15f, -1.15f}},
          8);
    polyL({{-0.05f, 1.08f}, {-2.85f, 1.08f}, {-2.85f, -1.08f}, {-0.05f, -1.08f}}, 4);
    polyL({{-0.28f, 0.82f}, {-2.55f, 0.82f}, {-2.55f, -0.82f}, {-0.28f, -0.82f}}, 6);
    polyL({{-0.55f, 0.62f}, {-1.35f, 0.62f}, {-1.35f, 0.18f}, {-0.55f, 0.18f}}, 5);
    polyL({{-0.55f, -0.18f}, {-1.35f, -0.18f}, {-1.35f, -0.62f}, {-0.55f, -0.62f}}, 5);
    polyL({{-1.85f, 0.22f}, {-2.35f, 0.22f}, {-2.35f, -0.42f}, {-1.85f, -0.42f}}, 11);
    blob(-3.35f, 0.f, 6.4f, 6.4f, 3);
    blob(-3.35f, 0.f, 4.5f, 4.5f, 7);
    blob(-3.35f, 0.f, 2.2f, 2.2f, 12);
    polyL({{1.55f, -0.09f}, {2.20f, -0.09f}, {2.20f, 0.09f}, {1.55f, 0.09f}}, 3);
    polyL({{2.18f, 0.f}, {2.18f, 0.62f}, {1.62f, 0.28f}}, 10);
    polyL({{-3.85f, 0.85f}, {-4.25f, 0.85f}, {-4.25f, 0.45f}, {-3.85f, 0.45f}}, 11);
    polyL({{-3.85f, -0.45f}, {-4.25f, -0.45f}, {-4.25f, -0.85f}, {-3.85f, -0.85f}}, 11);
    polyL({{-3.95f, 0.85f}, {-3.95f, -0.85f}, {-4.15f, -0.85f}, {-4.15f, 0.85f}}, 14);
    blob(0.35f, 1.22f, 3.3f, 3.3f, 10);
    blob(0.35f, 1.22f, 1.5f, 1.5f, 6);
    polyL({{2.35f, 0.42f}, {3.15f, 0.42f}, {3.15f, -0.42f}, {2.35f, -0.42f}}, 6);
    blob(3.55f, 0.f, 2.2f, 2.2f, 13);
    blob(-4.85f, 0.f, 2.4f, 1.6f, 12);
    b.outline(15, false);
    return b.cropToContent(1);
}

void wheelRim(Bitmap& b, int rim, int face) {
    b.ellipse(26, 26, 22, 22, rim);
    b.ellipse(26, 26, 18, 18, face);
}

Bitmap paintMill(float rot) {
    Bitmap b(52, 52);
    wheelRim(b, 3, 2);
    for (int i = 0; i < 6; i++) {
        float a = rot + float(i) * float(kTau) / 6.f;
        float cs = std::cos(a), sn = std::sin(a);
        b.line(26 + cs * 4.f, 26 + sn * 4.f, 26 + cs * 18.f, 26 + sn * 18.f, 1, 2.4f);
    }
    b.ellipse(26, 26, 4.2f, 4.2f, 4);
    b.ellipse(26, 26, 1.6f, 1.6f, 3);
    return b;
}

Bitmap paintPaddle(float rot) {
    Bitmap b(52, 52);
    wheelRim(b, 3, 1);
    for (int i = 0; i < 8; i++) {
        float a = rot + float(i) * float(kTau) / 8.f;
        float cs = std::cos(a), sn = std::sin(a);
        b.line(26 + cs * 8.f, 26 + sn * 8.f, 26 + cs * 19.f, 26 + sn * 19.f, 2, 3.6f);
    }
    b.ellipse(26, 26, 6.f, 6.f, 5);
    b.ellipse(26, 26, 2.4f, 2.4f, 4);
    return b;
}

Bitmap paintDrum(float rot) {
    Bitmap b(52, 52);
    wheelRim(b, 3, 1);
    b.ellipse(26, 26, 12, 12, 5);
    float cs = std::cos(rot), sn = std::sin(rot);
    b.line(26 - cs * 16.f, 26 - sn * 16.f, 26 + cs * 16.f, 26 + sn * 16.f, 6, 2.6f);
    b.line(26 - sn * 16.f, 26 + cs * 16.f, 26 + sn * 16.f, 26 - cs * 16.f, 6, 2.6f);
    for (int i = 0; i < 8; i++) {
        float a = rot * 0.5f + float(i) * float(kTau) / 8.f;
        b.ellipse(26 + std::cos(a) * 15.f, 26 + std::sin(a) * 15.f, 1.7f, 1.7f, 4);
    }
    b.ellipse(26, 26, 3.2f, 3.2f, 3);
    return b;
}

Bitmap paintQuay() {
    Bitmap b(32, 48);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            int c = ((y / 6) & 1) ? 1 : 2;
            if ((hash2(x, y) & 15) == 0) c = 3;
            if (x < 3) c = 4;
            if ((hash2(x + 3, y + 9) & 31) == 0) c = 8;
            b.set(x, y, c);
        }
    }
    b.rect(14, 6, 4, 10, 5);
    b.ellipse(16, 5, 3.2f, 3.2f, 9);
    return b;
}

Bitmap paintHouse() {
    Bitmap b(40, 34);
    b.rect(2, 4, 36, 26, 3);
    b.rect(2, 4, 36, 7, 6);
    b.rect(6, 14, 7, 6, 7);
    b.rect(16, 14, 7, 6, 7);
    b.rect(27, 14, 7, 6, 7);
    b.rect(16, 22, 8, 8, 4);
    b.outline(15, false);
    return b.cropToContent(0);
}

Bitmap paintPier() {
    Bitmap b(44, 10);
    for (int x = 0; x < b.w; x++) {
        int c = ((x / 4) & 1) ? 5 : 4;
        for (int y = 1; y < 9; y++) b.set(x, y, c);
    }
    return b;
}

Bitmap paintBarge() {
    Bitmap b(70, 24);
    b.rect(2, 3, 66, 18, 12);
    b.rect(8, 6, 14, 12, 11);
    b.rect(28, 6, 14, 12, 11);
    b.rect(48, 6, 14, 12, 11);
    b.rect(2, 10, 6, 4, 13);
    b.outline(15, false);
    return b.cropToContent(0);
}

Bitmap paintCrane() {
    Bitmap b(36, 28);
    b.rect(4, 16, 16, 10, 4);
    b.rect(10, 8, 4, 14, 14);
    b.line(12, 10, 32, 4, 10, 2.2f);
    b.line(30, 4, 30, 14, 13, 1.4f);
    b.rect(6, 18, 6, 5, 3);
    return b.cropToContent(0);
}

Bitmap paintPost() {
    Bitmap b(12, 30);
    b.rect(4, 6, 4, 22, 1);
    b.rect(3, 8, 6, 4, 4);
    b.ellipse(6, 4, 4, 3.2f, 1);
    b.rect(5, 26, 2, 4, 4);
    return b;
}

Bitmap paintSpar() {
    Bitmap b(10, 22);
    b.rect(4, 6, 2, 14, 4);
    b.ellipse(5, 4, 3.4f, 3.4f, 5);
    b.ellipse(5, 4, 1.5f, 1.5f, 3);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(14, 18);
    b.ellipse(7, 8, 5.5f, 6.f, 2);
    b.ellipse(7, 8, 2.4f, 3.f, 3);
    b.rect(6, 13, 2, 4, 4);
    return b;
}

Bitmap paintLine() {
    Bitmap b(64, 8);
    for (int i = 0; i < 8; i++) b.rect(i * 8, 0, 8, 8, (i & 1) ? 2 : 3);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(18, 12);
    b.ellipse(6, 6, 4.2f, 2.6f, 1);
    b.ellipse(12, 6, 3.4f, 2.2f, 2);
    return b;
}

Bitmap paintSmoke() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6.5f, 5.5f, 1);
    b.ellipse(7, 7, 3.2f, 2.6f, 2);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(26, 14);
    float tip = up ? 3.f : 10.f;
    b.line(2, tip, 12, 7, 1, 1.5f);
    b.line(24, tip, 14, 7, 1, 1.5f);
    b.ellipse(13, 7, 1.8f, 1.4f, 2);
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
    setPal(vdp, PAL_TUG,
           {0, gs::rgb4(12, 3, 2), gs::rgb4(8, 2, 1), gs::rgb4(2, 2, 2), gs::rgb4(14, 12, 8), gs::rgb4(5, 9, 12),
            gs::rgb4(15, 15, 14), gs::rgb4(13, 10, 4), gs::rgb4(9, 7, 5), gs::rgb4(2, 6, 4), gs::rgb4(14, 7, 2),
            gs::rgb4(5, 3, 2), gs::rgb4(6, 6, 7), gs::rgb4(15, 14, 6), gs::rgb4(4, 3, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_MILL, {0, gs::rgb4(12, 9, 5), gs::rgb4(7, 5, 3), gs::rgb4(4, 4, 5), gs::rgb4(8, 6, 3),
                           gs::rgb4(5, 6, 3), gs::rgb4(3, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_PADDLE,
           {0, gs::rgb4(13, 12, 9), gs::rgb4(12, 3, 2), gs::rgb4(3, 3, 4), gs::rgb4(12, 10, 4), gs::rgb4(6, 5, 4),
            gs::rgb4(8, 7, 6), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_DRUM, {0, gs::rgb4(6, 6, 7), gs::rgb4(8, 5, 3), gs::rgb4(3, 3, 4), gs::rgb4(11, 10, 8),
                           gs::rgb4(4, 4, 5), gs::rgb4(5, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_QUAY,
           {0, gs::rgb4(9, 8, 7), gs::rgb4(6, 5, 4), gs::rgb4(8, 3, 2), gs::rgb4(3, 3, 3), gs::rgb4(7, 5, 3),
            gs::rgb4(4, 4, 5), gs::rgb4(8, 10, 11), gs::rgb4(3, 6, 3), gs::rgb4(14, 12, 5), gs::rgb4(11, 3, 2),
            gs::rgb4(5, 5, 4), gs::rgb4(3, 4, 4), gs::rgb4(8, 7, 4), gs::rgb4(5, 6, 6), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(13, 11, 5), gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 2),
                           gs::rgb4(4, 11, 4), gs::rgb4(8, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 13, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(8, 8, 8), gs::rgb4(12, 12, 12), gs::rgb4(4, 4, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 9, 10), gs::rgb4(14, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, gs::rgb4(1, 1, 1)});
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14), gs::rgb4(4, 5, 6));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 13, 6), gs::rgb4(4, 3, 1));
    textPal(vdp, PAL_WIN, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1));
    textPal(vdp, PAL_TAG, gs::rgb4(15, 14, 8), gs::rgb4(5, 4, 1));
    ink(vdp, PAL_HUD);

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.tug[i] = gs::uploadMipped(vdp, paintTug(float(i) * float(kTau) / 16.f));
    for (int i = 0; i < 8; i++) {
        float a = float(i) * float(kTau) / 8.f;
        art.mill[i] = gs::uploadMipped(vdp, paintMill(a));
        art.paddle[i] = gs::uploadMipped(vdp, paintPaddle(a));
        art.drum[i] = gs::uploadMipped(vdp, paintDrum(a));
    }
    art.quay = gs::uploadMipped(vdp, paintQuay());
    art.house = gs::uploadMipped(vdp, paintHouse());
    art.pier = gs::uploadMipped(vdp, paintPier());
    art.barge = gs::uploadMipped(vdp, paintBarge());
    art.crane = gs::uploadMipped(vdp, paintCrane());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.spar = gs::uploadMipped(vdp, paintSpar());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.line = gs::uploadMipped(vdp, paintLine());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.smoke = gs::uploadMipped(vdp, paintSmoke());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(true));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(false));
    art.title = words(vdp, "TUGBOAT KILO", 2);
    art.made = words(vdp, "LEG MADE", 2);
    art.clean = words(vdp, "WHEELS UNTOUCHED", 2);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.touched = words(vdp, "TOUCHED A WHEEL", 2);
    art.cut = words(vdp, "LEFT THE CUT", 2);
    art.paused = words(vdp, "PAUSED", 2);
    art.m250 = words(vdp, "250", 1);
    art.m500 = words(vdp, "500", 1);
    art.m750 = words(vdp, "750", 1);
    art.m1000 = words(vdp, "1000", 1);
    art.endTag = words(vdp, "END", 1);
}

}  // namespace tugkilo
