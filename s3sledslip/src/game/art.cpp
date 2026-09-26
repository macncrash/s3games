#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace sledslip {
namespace {

constexpr float kTau = 6.2831853f;
constexpr int kTeam = 120;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    uint16_t c[16] = {};
    int i = 0;
    for (uint16_t v : cs)
        if (i < 16) c[i++] = v;
    for (int k = 0; k < 16; k++) vdp.setColor(pal * 16 + k, c[k]);
}

// Source art faces up. Heading 0 stores a team whose nose points east.
Bitmap spinTeam(const Bitmap& src, float heading) {
    Bitmap dst(src.w, src.h);
    const float c = std::cos(heading), s = std::sin(heading);
    const float cx = (src.w - 1) * 0.5f, cy = (src.h - 1) * 0.5f;
    for (int y = 0; y < src.h; y++) {
        for (int x = 0; x < src.w; x++) {
            float dx = x - cx, dy = y - cy;
            int ix = int(std::lround(cx + s * dx + c * dy));
            int iy = int(std::lround(cy - c * dx + s * dy));
            if (ix < 0 || iy < 0 || ix >= src.w || iy >= src.h) continue;
            int v = src.get(ix, iy);
            if (v) dst.set(x, y, v);
        }
    }
    return dst;
}

Bitmap paintTeam() {
    Bitmap b(kTeam, kTeam);
    const float cx = 59.5f, cy = 59.5f;
    auto blob = [&](float lx, float fy, float rx, float ry, int col) { b.ellipse(cx + lx, cy - fy, rx, ry, col); };
    auto stroke = [&](float x0, float f0, float x1, float f1, int col, float th) {
        b.line(cx + x0, cy - f0, cx + x1, cy - f1, col, th);
    };
    auto polyF = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        w.reserve(local.size());
        for (const Pt& p : local) w.push_back({cx + p.first, cy - p.second});
        b.poly(w, col);
    };

    stroke(-13.f, -34.f, -11.f, 28.f, 14, 3.2f);
    stroke(13.f, -34.f, 11.f, 28.f, 14, 3.2f);
    stroke(-13.f, -34.f, -11.f, 28.f, 4, 1.6f);
    stroke(13.f, -34.f, 11.f, 28.f, 4, 1.6f);
    stroke(-12.f, 28.f, -6.f, 36.f, 4, 2.2f);
    stroke(12.f, 28.f, 6.f, 36.f, 4, 2.2f);
    stroke(-12.f, -34.f, -8.f, -38.f, 14, 2.4f);
    stroke(12.f, -34.f, 8.f, -38.f, 14, 2.4f);

    polyF({{-9.f, -26.f}, {9.f, -26.f}, {8.f, 8.f}, {-8.f, 8.f}}, 1);
    polyF({{-6.f, -22.f}, {6.f, -22.f}, {5.f, 4.f}, {-5.f, 4.f}}, 2);
    stroke(-8.f, -18.f, 8.f, -18.f, 3, 1.4f);
    stroke(-7.5f, -10.f, 7.5f, -10.f, 3, 1.4f);
    stroke(-7.f, -2.f, 7.f, -2.f, 3, 1.4f);
    stroke(-8.f, 8.f, 0.f, 16.f, 3, 2.2f);
    stroke(8.f, 8.f, 0.f, 16.f, 3, 2.2f);
    stroke(-8.f, -16.f, 8.f, -16.f, 3, 2.6f);

    auto dog = [&](float lx, float fy, float sc) {
        blob(lx, fy - 6.f * sc, 2.2f * sc, 1.6f * sc, 8);
        blob(lx, fy, 7.2f * sc, 4.6f * sc, 7);
        blob(lx - 2.f * sc, fy + 1.2f * sc, 3.2f * sc, 2.2f * sc, 8);
        blob(lx, fy + 7.2f * sc, 3.6f * sc, 3.1f * sc, 7);
        blob(lx - 1.6f * sc, fy + 9.4f * sc, 1.5f * sc, 2.0f * sc, 8);
        blob(lx + 1.7f * sc, fy + 9.2f * sc, 1.3f * sc, 1.8f * sc, 8);
        blob(lx + 1.2f * sc, fy + 7.4f * sc, 0.7f, 0.7f, 10);
        blob(lx + 2.6f * sc, fy + 6.6f * sc, 1.1f * sc, 0.8f * sc, 11);
        stroke(lx - 3.f * sc, fy - 1.f, lx - 4.f * sc, fy - 6.f, 7, 1.5f);
        stroke(lx + 2.f * sc, fy - 1.f, lx + 3.f * sc, fy - 6.f, 7, 1.5f);
    };
    dog(-5.2f, 30.f, 1.05f);
    dog(5.6f, 18.f, 0.92f);
    stroke(0.f, 8.f, -5.f, 22.f, 9, 1.5f);
    stroke(-5.f, 24.f, 5.f, 16.f, 9, 1.3f);
    stroke(-5.f, 32.f, -4.f, 22.f, 9, 1.2f);

    blob(0.f, -8.f, 6.2f, 7.4f, 5);
    blob(0.f, 0.5f, 4.4f, 3.4f, 6);
    blob(0.f, 1.6f, 2.3f, 2.2f, 11);
    blob(-1.1f, 2.2f, 0.7f, 0.6f, 13);
    blob(1.1f, 2.2f, 0.7f, 0.6f, 13);
    stroke(-5.f, -6.f, -8.f, -16.f, 5, 2.f);
    stroke(5.f, -6.f, 8.f, -16.f, 5, 2.f);
    blob(0.f, -16.f, 1.3f, 1.3f, 6);

    b.outline(15, false);
    return b;
}

Bitmap paintCrib() {
    Bitmap b(36, 78);
    b.rect(4, 10, 28, 62, 1);
    for (int i = 0; i < 6; i++) b.rect(6, 12 + i * 10, 24, 7, (i & 1) ? 2 : 1);
    b.rect(6, 12, 3, 58, 3);
    b.rect(27, 12, 3, 58, 3);
    b.ellipse(18, 12, 16, 7, 4);
    b.rect(8, 8, 20, 4, 5);
    b.outline(15, false);
    return b;
}

Bitmap paintHead() {
    Bitmap b(96, 28);
    b.rect(2, 8, 92, 14, 1);
    for (int i = 0; i < 8; i++) b.rect(4 + i * 11, 10, 9, 10, (i & 1) ? 2 : 1);
    b.rect(2, 6, 92, 4, 4);
    b.rect(2, 20, 92, 4, 3);
    b.outline(15, false);
    return b;
}

Bitmap paintHut() {
    Bitmap b(78, 64);
    b.ellipse(39, 50, 30, 8, 4);
    b.rect(16, 28, 46, 22, 1);
    b.rect(18, 30, 42, 8, 2);
    b.poly({{10, 32}, {39, 12}, {68, 32}}, 4);
    b.poly({{18, 30}, {39, 16}, {60, 30}}, 2);
    b.rect(34, 34, 10, 16, 5);
    b.rect(22, 34, 8, 7, 6);
    b.rect(50, 34, 8, 7, 6);
    b.rect(48, 14, 6, 14, 7);
    b.rect(46, 12, 10, 4, 3);
    b.outline(15, false);
    return b;
}

Bitmap paintSmoke(bool high) {
    Bitmap b(20, 20);
    b.ellipse(10, high ? 8 : 12, 5, 4, 1);
    b.ellipse(8, high ? 6 : 10, 2, 2, 2);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(22, 40);
    b.rect(10, 14, 3, 22, 1);
    b.ellipse(11, 32, 6, 3, 1);
    b.ellipse(11, 10, 7, 6, 3);
    b.ellipse(11, 10, 4, 3, 2);
    b.rect(10, 4, 2, 4, 1);
    return b;
}

Bitmap paintRaven(bool up) {
    Bitmap b(28, 18);
    b.ellipse(14, 10, 4, 3, 1);
    float tip = up ? 3.f : 12.f;
    b.line(14, 9, 2, tip, 1, 1.8f);
    b.line(14, 9, 26, tip, 1, 1.8f);
    b.ellipse(18, 9, 2.2f, 1.6f, 1);
    b.set(20, 8, 3);
    return b;
}

Bitmap paintStake() {
    Bitmap b(14, 36);
    b.rect(6, 6, 3, 26, 3);
    b.poly({{6, 4}, {14, 10}, {6, 16}}, 2);
    b.ellipse(7, 5, 2, 2, 1);
    return b;
}

Bitmap paintBar() {
    Bitmap b(72, 14);
    for (int i = 0; i < 8; i++) b.rect(2 + i * 8, 3, 8, 8, (i & 1) ? 2 : 1);
    b.outline(3, false);
    return b;
}

Bitmap paintStaff() {
    Bitmap b(12, 48);
    b.rect(5, 2, 2, 42, 4);
    for (int i = 0; i < 6; i++) b.rect(3, 6 + i * 6, 6, 2, (i & 1) ? 2 : 1);
    b.rect(3, 42, 6, 3, 3);
    return b;
}

Bitmap paintBead() {
    Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(4, 4, 1.4f, 1.4f, 3);
    return b;
}

Bitmap paintMound() {
    Bitmap b(32, 20);
    b.ellipse(16, 12, 13, 6, 1);
    b.ellipse(10, 10, 6, 4, 2);
    b.ellipse(20, 9, 5, 3, 3);
    return b;
}

Bitmap paintPuff() {
    Bitmap b(16, 12);
    b.ellipse(8, 7, 6, 3, 1);
    b.ellipse(5, 6, 3, 2, 2);
    return b;
}

Bitmap paintFlake() {
    Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 1, 1);
    b.set(2, 2, 1);
    b.set(2, 3, 1);
    b.set(2, 4, 1);
    b.set(0, 2, 1);
    b.set(1, 2, 1);
    b.set(3, 2, 1);
    b.set(4, 2, 1);
    return b;
}

Bitmap paintPanel() {
    Bitmap b(70, 100);
    b.rect(1, 1, 68, 98, 1);
    b.rect(4, 4, 62, 92, 2);
    b.outline(3, false);
    return b;
}

Bitmap paintPin() {
    Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
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
    const uint16_t line = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 10, 12), gs::rgb4(15, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, gs::rgb4(2, 3, 5), line});
    setPal(vdp, PAL_SLED,
           {0, gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(12, 9, 5), gs::rgb4(12, 14, 15), gs::rgb4(14, 3, 2),
            gs::rgb4(15, 13, 10), gs::rgb4(3, 3, 4), gs::rgb4(8, 7, 6), gs::rgb4(13, 10, 3), gs::rgb4(1, 1, 1),
            gs::rgb4(13, 7, 5), gs::rgb4(14, 15, 15), gs::rgb4(14, 10, 3), gs::rgb4(5, 6, 7), line});
    setPal(vdp, PAL_CRIB,
           {0, gs::rgb4(10, 7, 4), gs::rgb4(13, 10, 6), gs::rgb4(5, 4, 3), gs::rgb4(15, 15, 15), gs::rgb4(10, 13, 14),
            gs::rgb4(3, 3, 3), 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(4, 3, 3), gs::rgb4(15, 12, 4), gs::rgb4(12, 7, 2), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0,
            0, line, line});
    setPal(vdp, PAL_MARK,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(13, 2, 2), gs::rgb4(3, 2, 2), gs::rgb4(14, 10, 3), 0, 0, 0, 0, 0, 0, 0, 0,
            0, line, line});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15), gs::rgb4(9, 11, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, line, line});
    setPal(vdp, PAL_BIRD,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 6, 8), gs::rgb4(14, 10, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_TIDE,
           {0, gs::rgb4(2, 5, 8), gs::rgb4(4, 8, 10), gs::rgb4(12, 14, 15), gs::rgb4(8, 7, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0,
            line, line});
    setPal(vdp, PAL_MAP,
           {0, gs::rgb4(2, 4, 6), gs::rgb4(6, 10, 12), gs::rgb4(12, 14, 15), gs::rgb4(14, 3, 2), gs::rgb4(12, 14, 6), 0,
            0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(10, 15, 7), gs::rgb4(4, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 1),
                          line});
    setPal(vdp, PAL_ALERT,
           {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 1), line});
    setPal(vdp, PAL_BANNER,
           {0, gs::rgb4(15, 13, 7), gs::rgb4(8, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 1), line});
    setPal(vdp, PAL_ICE,
           {0, gs::rgb4(14, 15, 15), gs::rgb4(11, 13, 15), gs::rgb4(8, 10, 13), gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15),
            gs::rgb4(12, 14, 15), gs::rgb4(9, 12, 14), gs::rgb4(7, 9, 12), gs::rgb4(8, 11, 13), gs::rgb4(13, 15, 15),
            gs::rgb4(2, 5, 8), gs::rgb4(1, 3, 6), gs::rgb4(5, 9, 11), gs::rgb4(14, 15, 15), gs::rgb4(10, 13, 14)});
    setPal(vdp, PAL_BERTH,
           {0, gs::rgb4(15, 14, 12), gs::rgb4(13, 12, 11), gs::rgb4(10, 9, 8), gs::rgb4(15, 15, 14), gs::rgb4(14, 13, 11),
            gs::rgb4(14, 13, 11), gs::rgb4(12, 11, 9), gs::rgb4(9, 8, 7), gs::rgb4(11, 10, 8), gs::rgb4(15, 14, 10),
            gs::rgb4(3, 5, 7), gs::rgb4(2, 3, 5), gs::rgb4(8, 8, 6), gs::rgb4(15, 14, 8), gs::rgb4(12, 11, 8)});
    setPal(vdp, PAL_BANK,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), gs::rgb4(10, 12, 14), gs::rgb4(14, 15, 15), gs::rgb4(12, 13, 14),
            gs::rgb4(14, 15, 15), gs::rgb4(12, 14, 15), gs::rgb4(9, 11, 13), gs::rgb4(11, 13, 14), gs::rgb4(15, 15, 15),
            gs::rgb4(3, 5, 7), gs::rgb4(2, 4, 6), gs::rgb4(6, 8, 10), gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15)});
    setPal(vdp, PAL_HUT,
           {0, gs::rgb4(9, 6, 3), gs::rgb4(12, 9, 5), gs::rgb4(5, 4, 3), gs::rgb4(15, 15, 15), gs::rgb4(3, 2, 2),
            gs::rgb4(14, 11, 4), gs::rgb4(4, 4, 4), 0, 0, 0, 0, 0, 0, line, line});
    vdp.setFogColor(gs::rgb4(3, 5, 8));

    loadFont(vdp, art);
    Bitmap team = paintTeam();
    for (int i = 0; i < 16; i++) art.team[i] = gs::uploadMipped(vdp, spinTeam(team, i * kTau / 16.f));
    art.crib = gs::uploadMipped(vdp, paintCrib());
    art.head = gs::uploadMipped(vdp, paintHead());
    art.hut = gs::uploadMipped(vdp, paintHut());
    art.smoke[0] = gs::uploadMipped(vdp, paintSmoke(false));
    art.smoke[1] = gs::uploadMipped(vdp, paintSmoke(true));
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.raven[0] = gs::uploadMipped(vdp, paintRaven(false));
    art.raven[1] = gs::uploadMipped(vdp, paintRaven(true));
    art.stake = gs::uploadMipped(vdp, paintStake());
    art.bar = gs::uploadMipped(vdp, paintBar());
    art.staff = gs::uploadMipped(vdp, paintStaff());
    art.bead = gs::uploadMipped(vdp, paintBead());
    art.mound = gs::uploadMipped(vdp, paintMound());
    art.puff = gs::uploadMipped(vdp, paintPuff());
    art.flake = gs::uploadMipped(vdp, paintFlake());
    art.panel = gs::uploadMipped(vdp, paintPanel());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.title = word(vdp, "SLED SLIP", 3);
    art.berthed = word(vdp, "BERTHED", 3);
    art.inSlip = word(vdp, "IN THE SLIP", 2);
    art.missed = word(vdp, "MISSED THE END", 2);
    art.tide = word(vdp, "TIDE TURNED", 2);
    art.scraped = word(vdp, "SCRAPED THE CRIB", 2);
    art.offIce = word(vdp, "OFF THE ICE", 2);
    art.leg = word(vdp, "LEG FAILED", 2);
    art.paused = word(vdp, "PAUSED", 3);
    art.endMark = word(vdp, "END", 2);
}

}  // namespace sledslip
