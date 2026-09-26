#include "art.h"

#include <cmath>
#include <vector>

namespace tugmark {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

Bitmap paintTug(float heading) {
    Bitmap b(176, 176);
    const float cx = 88.f, cy = 88.f, sc = kPixelsPerMetre;
    const float co = std::cos(heading), sn = std::sin(heading);
    auto polyB = [&](std::initializer_list<Pt> meters, int col) {
        std::vector<Pt> p;
        p.reserve(meters.size());
        for (const Pt& m : meters) {
            float dx = (m.first * co + m.second * sn) * sc;
            float dy = (-m.first * sn + m.second * co) * sc;
            p.push_back({cx + dx, cy + dy});
        }
        b.poly(p, col);
    };
    auto ell = [&](float bx, float by, float rx, float ry, int col) {
        float dx = (bx * co + by * sn) * sc;
        float dy = (-bx * sn + by * co) * sc;
        b.ellipse(cx + dx, cy + dy, rx, ry, col);
    };

    polyB({{7.7f, 0.f},
           {5.9f, 2.55f},
           {0.4f, 3.35f},
           {-5.4f, 3.25f},
           {-7.15f, 2.05f},
           {-7.7f, 0.85f},
           {-7.7f, -0.85f},
           {-7.15f, -2.05f},
           {-5.4f, -3.25f},
           {0.4f, -3.35f},
           {5.9f, -2.55f}},
          2);
    polyB({{7.25f, 0.f},
           {5.55f, 2.2f},
           {0.5f, 2.95f},
           {-5.1f, 2.85f},
           {-6.7f, 1.75f},
           {-7.15f, 0.7f},
           {-7.15f, -0.7f},
           {-6.7f, -1.75f},
           {-5.1f, -2.85f},
           {0.5f, -2.95f},
           {5.55f, -2.2f}},
          1);
    polyB({{6.6f, 0.f}, {5.1f, 1.85f}, {-4.6f, 2.15f}, {-6.2f, 1.15f}, {-6.2f, -1.15f}, {-4.6f, -2.15f}, {5.1f, -1.85f}},
          8);
    polyB({{7.55f, 0.f}, {6.2f, 1.05f}, {6.2f, -1.05f}}, 10);
    polyB({{5.7f, 1.55f}, {1.2f, 2.05f}, {1.05f, 1.55f}, {5.35f, 1.15f}}, 4);
    polyB({{5.7f, -1.55f}, {5.35f, -1.15f}, {1.05f, -1.55f}, {1.2f, -2.05f}}, 4);
    polyB({{4.8f, 0.22f}, {-5.6f, 0.22f}, {-5.6f, -0.22f}, {4.8f, -0.22f}}, 9);
    polyB({{-0.2f, 1.65f}, {-3.7f, 1.65f}, {-3.7f, -1.65f}, {-0.2f, -1.65f}}, 5);
    polyB({{-0.45f, 1.15f}, {-1.55f, 1.15f}, {-1.55f, -1.15f}, {-0.45f, -1.15f}}, 6);
    polyB({{-1.85f, 1.05f}, {-3.35f, 1.05f}, {-3.35f, 0.35f}, {-1.85f, 0.35f}}, 7);
    polyB({{-1.85f, -0.35f}, {-3.35f, -0.35f}, {-3.35f, -1.05f}, {-1.85f, -1.05f}}, 7);
    polyB({{-2.55f, 0.35f}, {-3.15f, 0.35f}, {-3.15f, -0.55f}, {-2.55f, -0.55f}}, 11);
    ell(-4.65f, 0.f, 6.4f, 5.2f, 3);
    ell(-4.65f, 0.f, 3.6f, 2.8f, 14);
    ell(-4.65f, 0.f, 1.5f, 1.2f, 11);
    polyB({{-2.05f, 0.12f}, {-2.05f, -0.12f}, {-5.35f, -0.12f}, {-5.35f, 0.12f}}, 13);
    polyB({{-6.55f, 0.85f}, {-5.55f, 0.85f}, {-5.55f, 0.45f}, {-6.55f, 0.45f}}, 11);
    polyB({{-6.55f, -0.45f}, {-5.55f, -0.45f}, {-5.55f, -0.85f}, {-6.55f, -0.85f}}, 11);
    polyB({{-6.15f, 0.85f}, {-6.15f, -0.85f}, {-5.85f, -0.85f}, {-5.85f, 0.85f}}, 12);
    ell(2.4f, 2.15f, 2.6f, 2.4f, 12);
    ell(2.4f, -2.15f, 2.6f, 2.4f, 12);
    ell(-1.6f, 2.35f, 2.3f, 2.1f, 12);
    ell(-1.6f, -2.35f, 2.3f, 2.1f, 12);
    ell(0.2f, 1.15f, 2.2f, 2.2f, 4);
    ell(0.2f, 1.15f, 1.0f, 1.0f, 9);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintShade() {
    Bitmap b(46, 26);
    b.ellipse(23, 13, 20, 9, 1);
    return b;
}

Bitmap paintQuay() {
    Bitmap b(26, 52);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x / 2, y / 3);
            int c = 2;
            if (x < 4) c = 1;
            if (x > 20) c = 4;
            if ((h & 7) == 0) c = 3;
            if (y % 13 < 2) c = 5;
            b.set(x, y, c);
        }
    }
    b.rect(8, 18, 8, 10, 6);
    b.rect(10, 8, 4, 8, 7);
    return b;
}

Bitmap paintShed() {
    Bitmap b(44, 30);
    b.rect(2, 10, 40, 18, 2);
    b.poly({{0, 10}, {22, 1}, {43, 10}}, 3);
    b.rect(6, 16, 8, 8, 5);
    b.rect(18, 15, 10, 7, 8);
    b.rect(32, 16, 6, 12, 6);
    b.rect(8, 6, 3, 5, 7);
    b.outline(1, false);
    return b.cropToContent(0);
}

Bitmap paintCrane() {
    Bitmap b(48, 40);
    b.rect(8, 14, 6, 24, 2);
    b.rect(6, 12, 10, 4, 3);
    b.poly({{14, 16}, {42, 6}, {42, 10}, {14, 20}}, 4);
    b.line(40, 8, 40, 28, 1, 1.2f);
    b.ellipse(40, 30, 2.2f, 2.2f, 5);
    b.rect(9, 18, 4, 3, 6);
    return b.cropToContent(0);
}

Bitmap paintLamp() {
    Bitmap b(12, 26);
    b.rect(5, 10, 2, 16, 2);
    b.ellipse(6, 6, 4.2f, 4.2f, 1);
    b.ellipse(6, 6, 2.1f, 2.1f, 3);
    return b;
}

Bitmap paintDolphin() {
    Bitmap b(14, 32);
    b.rect(5, 8, 4, 22, 2);
    b.rect(3, 10, 8, 3, 1);
    b.rect(3, 18, 8, 3, 3);
    b.ellipse(7, 5, 4, 3.2f, 4);
    b.rect(6, 28, 2, 4, 2);
    return b;
}

Bitmap paintBulk() {
    Bitmap b(64, 14);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            int c = (y < 3 || y > 10) ? 1 : (((x / 5) & 1) ? 3 : 2);
            b.set(x, y, c);
        }
    }
    return b;
}

Bitmap paintMark() {
    Bitmap b(kMarkBmp, kMarkBmp);
    const float c = (kMarkBmp - 1) * 0.5f;
    b.ellipse(c, c, kPaintPx + 2.2f, kPaintPx + 2.2f, 3);
    b.ellipse(c, c, float(kPaintPx), float(kPaintPx), 4);
    b.ellipse(c, c, 18.f, 18.f, 0);
    b.ellipse(c, c, kHeartPx + 1.6f, kHeartPx + 1.6f, 3);
    b.ellipse(c, c, float(kHeartPx), float(kHeartPx), 1);
    b.line(c - 15.f, c, c + 15.f, c, 2, 1.6f);
    b.line(c, c - 15.f, c, c + 15.f, 2, 1.6f);
    b.ellipse(c, c, 2.2f, 2.2f, 1);
    return b;
}

Bitmap paintBoom() {
    Bitmap b(56, 12);
    for (int x = 0; x < b.w; x++) {
        int c = ((x / 7) & 1) ? 1 : 2;
        for (int y = 2; y < 10; y++) b.set(x, y, c);
    }
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(16, 24);
    b.ellipse(8, 10, 6, 7, 1);
    b.rect(6, 4, 4, 12, 2);
    b.ellipse(8, 10, 2.2f, 3.f, 3);
    b.rect(7, 16, 2, 6, 4);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 7, 4, 1);
    b.ellipse(8, 6, 3.2f, 1.7f, 2);
    return b;
}

Bitmap paintSmoke() {
    Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 5, 1);
    b.ellipse(6, 6, 3, 2.3f, 2);
    return b;
}

Bitmap paintGull(int flap) {
    Bitmap b(22, 12);
    float tip = flap ? 2.f : 8.f;
    b.line(1, tip, 10, 6, 1, 1.5f);
    b.line(20, tip, 12, 6, 1, 1.5f);
    b.ellipse(11, 6.5f, 1.7f, 1.3f, 2);
    return b;
}

Bitmap paintPip() {
    Bitmap b(9, 9);
    b.ellipse(4.5f, 4.5f, 4, 4, 2);
    b.ellipse(4.5f, 4.5f, 2, 2, 1);
    return b;
}

Bitmap paintPin() {
    Bitmap b(9, 9);
    b.poly({{4, 0}, {8, 4}, {4, 8}, {0, 4}}, 1);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 11, 12), gs::rgb4(14, 12, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TUG,
           {0, gs::rgb4(3, 6, 5), gs::rgb4(1, 2, 2), gs::rgb4(11, 3, 2), gs::rgb4(13, 2, 2), gs::rgb4(14, 13, 9),
            gs::rgb4(4, 10, 13), gs::rgb4(11, 15, 15), gs::rgb4(8, 6, 3), gs::rgb4(15, 15, 13), gs::rgb4(15, 13, 1),
            gs::rgb4(2, 2, 2), gs::rgb4(3, 3, 4), gs::rgb4(13, 10, 2), gs::rgb4(1, 1, 1), ink});
    setPal(vdp, PAL_QUAY,
           {0, gs::rgb4(12, 11, 9), gs::rgb4(7, 7, 6), gs::rgb4(9, 4, 3), gs::rgb4(5, 5, 5), gs::rgb4(4, 4, 4),
            gs::rgb4(10, 12, 13), gs::rgb4(3, 3, 3), gs::rgb4(14, 13, 8), gs::rgb4(6, 8, 9), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MARK,
           {0, gs::rgb4(15, 13, 2), gs::rgb4(8, 5, 1), gs::rgb4(2, 2, 1), gs::rgb4(15, 14, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            ink});
    setPal(vdp, PAL_END,
           {0, gs::rgb4(14, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 4), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(12, 12, 13), gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN,
           {0, gs::rgb4(8, 15, 5), gs::rgb4(2, 6, 2), gs::rgb4(1, 2, 1), gs::rgb4(12, 15, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            ink});
    setPal(vdp, PAL_ALERT,
           {0, gs::rgb4(15, 6, 2), gs::rgb4(6, 1, 1), gs::rgb4(2, 1, 1), gs::rgb4(15, 10, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 5), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 6), gs::rgb4(3, 3, 3), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    for (int i = 0; i < 16; i++) vdp.setColor(PAL_WATER * 16 + i, 0);
    vdp.setColor(PAL_WATER * 16 + 1, gs::rgb4(10, 9, 7));
    vdp.setColor(PAL_WATER * 16 + 2, gs::rgb4(6, 6, 5));
    vdp.setColor(PAL_WATER * 16 + 3, gs::rgb4(8, 7, 6));
    vdp.setColor(PAL_WATER * 16 + 4, gs::rgb4(9, 8, 7));
    vdp.setColor(PAL_WATER * 16 + 5, gs::rgb4(5, 5, 4));
    vdp.setColor(PAL_WATER * 16 + 6, gs::rgb4(7, 7, 6));
    vdp.setColor(PAL_WATER * 16 + 7, gs::rgb4(5, 5, 4));
    vdp.setColor(PAL_WATER * 16 + 8, gs::rgb4(9, 8, 6));
    vdp.setColor(PAL_WATER * 16 + 9, gs::rgb4(4, 4, 3));
    vdp.setColor(PAL_WATER * 16 + 10, gs::rgb4(12, 11, 9));
    vdp.setColor(PAL_WATER * 16 + 11, gs::rgb4(3, 8, 12));
    vdp.setColor(PAL_WATER * 16 + 12, gs::rgb4(2, 5, 9));
    vdp.setColor(PAL_WATER * 16 + 13, gs::rgb4(8, 13, 14));
    vdp.setColor(PAL_WATER * 16 + 14, gs::rgb4(12, 11, 8));
    vdp.setColor(PAL_WATER * 16 + 15, gs::rgb4(7, 7, 6));

    loadFont(vdp, art);
    const float tau = 6.28318530718f;
    for (int i = 0; i < 16; i++) art.tug[i] = gs::uploadMipped(vdp, paintTug(i * tau / 16.f));
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.quay = gs::uploadMipped(vdp, paintQuay());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.crane = gs::uploadMipped(vdp, paintCrane());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.dolphin = gs::uploadMipped(vdp, paintDolphin());
    art.bulk = gs::uploadMipped(vdp, paintBulk());
    art.mark = gs::uploadMipped(vdp, paintMark());
    art.boom = gs::uploadMipped(vdp, paintBoom());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.smoke = gs::uploadMipped(vdp, paintSmoke());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(0));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(1));
    art.pip = gs::uploadMipped(vdp, paintPip());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.title = words(vdp, "TUGBOAT MARK", 3);
    art.setDown = words(vdp, "SET DOWN", 3);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.off = words(vdp, "OFF THE MARK", 2);
    art.shortB = words(vdp, "SHORT", 3);
    art.longB = words(vdp, "LONG", 3);
    art.late = words(vdp, "TOO LATE", 3);
    art.paused = words(vdp, "PAUSED", 3);
}

}  // namespace tugmark
