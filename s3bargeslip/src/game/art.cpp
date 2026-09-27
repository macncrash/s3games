#include "art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace bargeslip {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float kScale = 2.6f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap paintBarge(float heading) {
    Bitmap b(150, 72);
    const float cx = 75.f, cy = 36.f;
    const float co = std::cos(heading), sn = std::sin(heading);
    auto polyB = [&](std::initializer_list<Pt> meters, int col) {
        std::vector<Pt> p;
        for (const Pt& m : meters) {
            float dx = (m.first * co + m.second * sn) * kScale;
            float dy = (-m.first * sn + m.second * co) * kScale;
            p.push_back({cx + dx, cy + dy});
        }
        b.poly(p, col);
    };
    auto blob = [&](float bx, float by, float rx, float ry, int col) {
        float dx = (bx * co + by * sn) * kScale;
        float dy = (-bx * sn + by * co) * kScale;
        b.ellipse(cx + dx, cy + dy, rx, ry, col);
    };
    polyB({{8.6f, 0.f}, {6.8f, 2.15f}, {-7.2f, 2.35f}, {-8.0f, 1.6f}, {-8.0f, -1.6f}, {-7.2f, -2.35f}, {6.8f, -2.15f}}, 2);
    polyB({{8.1f, 0.f}, {6.3f, 1.75f}, {-6.7f, 1.95f}, {-7.4f, 1.25f}, {-7.4f, -1.25f}, {-6.7f, -1.95f}, {6.3f, -1.75f}}, 3);
    polyB({{4.2f, -1.2f}, {4.2f, 1.2f}, {1.0f, 1.2f}, {1.0f, -1.2f}}, 8);
    polyB({{3.9f, -0.9f}, {3.9f, 0.9f}, {1.3f, 0.9f}, {1.3f, -0.9f}}, 9);
    polyB({{-0.4f, -1.15f}, {-0.4f, 1.15f}, {-3.6f, 1.15f}, {-3.6f, -1.15f}}, 5);
    polyB({{-0.7f, -0.85f}, {-0.7f, 0.85f}, {-3.3f, 0.85f}, {-3.3f, -0.85f}}, 6);
    polyB({{-4.6f, -0.9f}, {-4.6f, 0.9f}, {-6.6f, 0.9f}, {-6.6f, -0.9f}}, 4);
    blob(-5.6f, 0.f, 3.2f, 2.2f, 7);
    blob(7.4f, 0.f, 2.6f, 1.6f, 1);
    blob(6.2f, 1.05f, 1.6f, 1.1f, 10);
    blob(6.2f, -1.05f, 1.6f, 1.1f, 10);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintPierH() {
    Bitmap b(64, 18);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            int c = (y < 3) ? 4 : ((x / 6) & 1) ? 2 : 3;
            if (y > 14) c = 1;
            b.set(x, y, c);
        }
    return b;
}

Bitmap paintPierV() {
    Bitmap b(18, 64);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            int c = (x < 3) ? 4 : ((y / 6) & 1) ? 2 : 3;
            if (x > 14) c = 1;
            b.set(x, y, c);
        }
    return b;
}

Bitmap paintCleat() {
    Bitmap b(16, 8);
    b.rect(1, 3, 14, 2, 2);
    b.rect(3, 1, 2, 6, 1);
    b.rect(11, 1, 2, 6, 1);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(8, 20);
    b.rect(3, 7, 2, 12, 2);
    b.ellipse(4, 4, 3.f, 3.f, 1);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(12, 18);
    b.ellipse(6, 8, 4.f, 5.f, 1);
    b.rect(5, 13, 2, 4, 2);
    b.ellipse(6, 4, 2.f, 2.f, 3);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(14, 22);
    b.rect(2, 2, 2, 18, 2);
    b.poly({{4, 3}, {12, 6}, {4, 10}}, 1);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(12, 8);
    b.ellipse(6, 4, 5.f, 3.f, 1);
    return b;
}

Bitmap paintPin() {
    Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
    return b;
}

Bitmap paintDot() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

Bitmap paintPanel() {
    Bitmap b(64, 80);
    b.rect(1, 1, 62, 78, 1);
    b.rect(4, 4, 56, 72, 2);
    b.outline(3, false);
    return b;
}

gs::Mipped word(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st{scale, 1, 14, 0, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 13);
    const uint16_t line = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 9, 8), gs::rgb4(15, 12, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3), line});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(15, 13, 6), gs::rgb4(3, 3, 4), gs::rgb4(6, 7, 8), gs::rgb4(8, 5, 3), gs::rgb4(4, 5, 6),
            gs::rgb4(10, 12, 13), gs::rgb4(2, 2, 2), gs::rgb4(9, 6, 3), gs::rgb4(12, 8, 4), gs::rgb4(14, 10, 4), 0, 0, 0,
            0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_PIER,
           {0, gs::rgb4(5, 5, 5), gs::rgb4(8, 7, 6), gs::rgb4(11, 10, 8), gs::rgb4(13, 12, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(14, 3, 3), gs::rgb4(12, 12, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 8), gs::rgb4(2, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(6, 3, 2), gs::rgb4(15, 14, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BANNER, {0, ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, 0});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 6), gs::rgb4(4, 4, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(2, 4, 5), gs::rgb4(1, 2, 3), gs::rgb4(8, 10, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SLIP,
           {0, gs::rgb4(4, 8, 9), gs::rgb4(6, 10, 9), gs::rgb4(3, 6, 7), gs::rgb4(8, 9, 6), gs::rgb4(2, 4, 5), 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0});
    setPal(vdp, PAL_SHORE,
           {0, gs::rgb4(7, 6, 4), gs::rgb4(9, 8, 5), gs::rgb4(5, 5, 3), gs::rgb4(11, 10, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0});
    vdp.setFogColor(gs::rgb4(2, 3, 4));

    const float tau = 6.2831853f;
    for (int i = 0; i < 8; i++) art.hull[i] = gs::uploadMipped(vdp, paintBarge(tau * float(i) / 8.f));
    art.pierH = gs::uploadMipped(vdp, paintPierH());
    art.pierV = gs::uploadMipped(vdp, paintPierV());
    art.cleat = gs::uploadMipped(vdp, paintCleat());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.panel = gs::uploadMipped(vdp, paintPanel());
    art.dot = gs::uploadMipped(vdp, paintDot());
    art.title = word(vdp, "BARGE SLIP", 3);
    art.berthed = word(vdp, "BERTHED", 3);
    art.inSlip = word(vdp, "IN THE SLIP", 2);
    art.tide = word(vdp, "TIDE TURNED", 2);
    art.scraped = word(vdp, "SCRAPED THE PIER", 2);
    art.missed = word(vdp, "MISSED THE END", 2);
    art.leg = word(vdp, "LEG FAILED", 2);
    art.paused = word(vdp, "PAUSED", 3);
    loadFont(vdp, art);
}

}  // namespace bargeslip
