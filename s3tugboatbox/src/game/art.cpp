#include "art.h"

#include <cmath>
#include <vector>

namespace tugbox {
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
    Bitmap b(180, 180);
    const float cx = 90.f, cy = 90.f, sc = 5.1f;
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

    polyB({{8.35f, 0.f}, {6.5f, 3.25f}, {-7.5f, 3.55f}, {-8.3f, 2.15f}, {-8.3f, -2.15f}, {-7.5f, -3.55f}, {6.5f, -3.25f}}, 2);
    polyB({{7.9f, 0.f}, {6.15f, 2.95f}, {-7.1f, 3.2f}, {-7.85f, 1.9f}, {-7.85f, -1.9f}, {-7.1f, -3.2f}, {6.15f, -2.95f}}, 3);
    polyB({{7.4f, 0.f}, {5.7f, 2.55f}, {-6.7f, 2.8f}, {-7.35f, 1.6f}, {-7.35f, -1.6f}, {-6.7f, -2.8f}, {5.7f, -2.55f}}, 1);
    polyB({{7.05f, 0.f}, {5.4f, 2.15f}, {1.5f, 2.3f}, {1.5f, -2.3f}, {5.4f, -2.15f}}, 8);
    polyB({{1.35f, -2.35f}, {1.35f, 2.35f}, {1.05f, 2.35f}, {1.05f, -2.35f}}, 7);
    polyB({{0.95f, 1.65f}, {-2.55f, 1.65f}, {-2.55f, -1.65f}, {0.95f, -1.65f}}, 4);
    polyB({{0.55f, 1.25f}, {-2.15f, 1.25f}, {-2.15f, -1.25f}, {0.55f, -1.25f}}, 12);
    polyB({{0.72f, 1.15f}, {0.15f, 1.15f}, {0.15f, -1.15f}, {0.72f, -1.15f}}, 5);
    polyB({{-0.15f, 0.38f}, {-1.15f, 0.38f}, {-1.15f, -0.38f}, {-0.15f, -0.38f}}, 9);
    polyB({{-2.35f, 0.16f}, {-5.15f, 0.16f}, {-5.15f, -0.16f}, {-2.35f, -0.16f}}, 2);
    polyB({{-4.7f, 0.9f}, {-5.15f, 0.15f}, {-4.7f, 0.15f}, {-4.35f, 0.7f}}, 3);
    ell(-3.55f, 0.f, 7.2f, 7.2f, 6);
    ell(-3.55f, 0.f, 5.1f, 5.1f, 3);
    ell(-3.55f, 0.f, 2.6f, 2.6f, 2);
    ell(-6.55f, 1.35f, 2.4f, 2.4f, 12);
    ell(-6.55f, -1.35f, 2.4f, 2.4f, 12);
    ell(-7.15f, 0.f, 2.8f, 2.2f, 2);
    ell(6.55f, 0.f, 3.6f, 3.2f, 2);
    ell(6.05f, 1.55f, 3.1f, 2.8f, 2);
    ell(6.05f, -1.55f, 3.1f, 2.8f, 2);
    ell(3.4f, 2.05f, 3.4f, 3.4f, 3);
    ell(3.4f, 2.05f, 1.6f, 1.6f, 4);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintShade() {
    Bitmap b(48, 28);
    b.ellipse(24, 14, 22, 10, 1);
    return b;
}

Bitmap paintQuay() {
    Bitmap b(28, 56);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x, y);
            int c = 2;
            if (x > 20) c = 1;
            if (x > 23) c = 4;
            if ((h & 11) == 0) c = x > 20 ? 5 : 3;
            b.set(x, y, c);
        }
    }
    b.rect(16, 8, 6, 6, 6);
    b.rect(17, 4, 4, 5, 7);
    b.ellipse(19, 3, 2.2f, 2.2f, 8);
    b.rect(8, 30, 8, 5, 9);
    return b;
}

Bitmap paintShed() {
    Bitmap b(40, 28);
    b.rect(2, 8, 36, 18, 2);
    b.poly({{1, 8}, {20, 1}, {39, 8}}, 3);
    b.rect(6, 14, 8, 8, 5);
    b.rect(24, 13, 9, 6, 4);
    b.rect(16, 18, 6, 8, 6);
    b.outline(7, false);
    return b.cropToContent(0);
}

Bitmap paintBulk() {
    Bitmap b(64, 16);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) b.set(x, y, (y < 4 || y > 11) ? 1 : ((x / 4) & 1) ? 4 : 2);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(10, 22);
    b.rect(4, 8, 2, 14, 2);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2.2f, 3);
    return b;
}

Bitmap paintPost() {
    Bitmap b(14, 28);
    b.rect(5, 6, 4, 20, 2);
    b.rect(4, 8, 6, 4, 1);
    b.rect(4, 16, 6, 3, 2);
    b.ellipse(7, 4, 4, 3, 1);
    b.rect(6, 24, 2, 4, 3);
    return b;
}

Bitmap paintHbar() {
    Bitmap b(36, 8);
    b.rect(0, 2, 36, 4, 2);
    b.rect(1, 3, 34, 2, 1);
    return b;
}

Bitmap paintVbar() {
    Bitmap b(8, 36);
    b.rect(2, 0, 4, 36, 2);
    b.rect(3, 1, 2, 34, 1);
    return b;
}

Bitmap paintHatch() {
    Bitmap b(48, 48);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            bool edge = x < 2 || y < 2 || x >= 46 || y >= 46;
            bool hatch = ((x + y) & 7) < 2;
            if (edge) b.set(x, y, 1);
            else if (hatch) b.set(x, y, 2);
        }
    }
    return b;
}

Bitmap paintBuoy(bool green) {
    Bitmap b(16, 22);
    b.ellipse(8, 9, 6, 7, green ? 4 : 1);
    b.rect(6, 3, 4, 12, 2);
    b.ellipse(8, 9, 2.2f, 3.2f, green ? 5 : 3);
    b.rect(7, 16, 2, 5, 3);
    return b;
}

Bitmap paintBoom() {
    Bitmap b(48, 10);
    for (int x = 0; x < b.w; x++) {
        int c = ((x / 6) & 1) ? 1 : 2;
        for (int y = 1; y < 9; y++) b.set(x, y, c);
    }
    return b;
}

Bitmap paintFoam() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 7, 4, 1);
    b.ellipse(8, 6, 3, 1.6f, 2);
    return b;
}

Bitmap paintSmoke() {
    Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 5, 1);
    b.ellipse(6, 6, 3, 2.4f, 2);
    return b;
}

Bitmap paintGull(int flap) {
    Bitmap b(20, 12);
    float tip = flap ? 2.f : 8.f;
    b.line(2, tip, 9, 6, 1, 1.4f);
    b.line(18, tip, 11, 6, 1, 1.4f);
    b.ellipse(10, 6, 1.6f, 1.3f, 2);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 10, 11), gs::rgb4(14, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TUG,
           {0, gs::rgb4(14, 13, 10), gs::rgb4(1, 1, 1), gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(5, 12, 14),
            gs::rgb4(2, 2, 3), gs::rgb4(15, 12, 2), gs::rgb4(2, 7, 5), gs::rgb4(9, 6, 3), gs::rgb4(15, 15, 15),
            gs::rgb4(2, 4, 8), gs::rgb4(8, 8, 9), gs::rgb4(7, 1, 1), gs::rgb4(11, 8, 4), ink});
    setPal(vdp, PAL_QUAY,
           {0, gs::rgb4(11, 10, 8), gs::rgb4(7, 7, 6), gs::rgb4(5, 5, 4), gs::rgb4(4, 4, 5), gs::rgb4(9, 12, 13),
            gs::rgb4(3, 3, 3), gs::rgb4(12, 11, 9), gs::rgb4(14, 12, 4), gs::rgb4(8, 5, 3), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 13, 2), gs::rgb4(12, 9, 1), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_END,
           {0, gs::rgb4(14, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 1), gs::rgb4(3, 12, 4), gs::rgb4(1, 6, 2), 0, 0, 0,
            0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(12, 12, 12), gs::rgb4(7, 7, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 6), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 6), gs::rgb4(3, 3, 3), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    for (int i = 0; i < 16; i++) vdp.setColor(PAL_CH * 16 + i, 0);
    vdp.setColor(PAL_CH * 16 + 1, gs::rgb4(8, 8, 5));
    vdp.setColor(PAL_CH * 16 + 2, gs::rgb4(5, 6, 4));
    vdp.setColor(PAL_CH * 16 + 3, gs::rgb4(4, 4, 3));
    vdp.setColor(PAL_CH * 16 + 4, gs::rgb4(9, 9, 7));
    vdp.setColor(PAL_CH * 16 + 5, gs::rgb4(6, 6, 5));
    vdp.setColor(PAL_CH * 16 + 11, gs::rgb4(5, 11, 13));
    vdp.setColor(PAL_CH * 16 + 12, gs::rgb4(2, 7, 11));
    vdp.setColor(PAL_CH * 16 + 13, gs::rgb4(11, 15, 15));

    loadFont(vdp, art);
    const float tau = 6.28318530718f;
    for (int i = 0; i < 16; i++) art.tug[i] = gs::uploadMipped(vdp, paintTug(i * tau / 16.f));
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.quay = gs::uploadMipped(vdp, paintQuay());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.bulk = gs::uploadMipped(vdp, paintBulk());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.hbar = gs::uploadMipped(vdp, paintHbar());
    art.vbar = gs::uploadMipped(vdp, paintVbar());
    art.hatch = gs::uploadMipped(vdp, paintHatch());
    art.buoyR = gs::uploadMipped(vdp, paintBuoy(false));
    art.buoyG = gs::uploadMipped(vdp, paintBuoy(true));
    art.boom = gs::uploadMipped(vdp, paintBoom());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.smoke = gs::uploadMipped(vdp, paintSmoke());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(0));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(1));
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.title = words(vdp, "TUGBOAT BOX", 3);
    art.stopped = words(vdp, "STOPPED", 3);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.outside = words(vdp, "OUTSIDE", 3);
    art.stoppedShort = words(vdp, "SHORT", 3);
    art.paused = words(vdp, "PAUSED", 3);
}

}  // namespace tugbox
