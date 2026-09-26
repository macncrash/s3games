#include "art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace tugboom {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float kTau = 6.28318530718f;
constexpr float kPx = 4.4f;

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

void paintBody(Bitmap& b, float heading, float sc, std::initializer_list<Pt> meters, int col) {
    const float co = std::cos(heading), sn = std::sin(heading);
    const float cx = b.w * 0.5f, cy = b.h * 0.5f;
    std::vector<Pt> p;
    p.reserve(meters.size());
    for (const Pt& m : meters) {
        float dx = (m.first * co + m.second * sn) * sc;
        float dy = (-m.first * sn + m.second * co) * sc;
        p.push_back({cx + dx, cy + dy});
    }
    b.poly(p, col);
}

void paintDot(Bitmap& b, float heading, float sc, float lx, float ly, float rx, float ry, int col) {
    const float co = std::cos(heading), sn = std::sin(heading);
    float dx = (lx * co + ly * sn) * sc;
    float dy = (-lx * sn + ly * co) * sc;
    b.ellipse(b.w * 0.5f + dx, b.h * 0.5f + dy, rx, ry, col);
}

Bitmap paintTug(float heading) {
    Bitmap b(104, 104);
    auto poly = [&](std::initializer_list<Pt> m, int c) { paintBody(b, heading, kPx, m, c); };
    auto dot = [&](float x, float y, float rx, float ry, int c) { paintDot(b, heading, kPx, x, y, rx, ry, c); };
    poly({{6.5f, 0.f}, {4.3f, 2.45f}, {-5.6f, 2.7f}, {-6.45f, 1.7f}, {-6.45f, -1.7f}, {-5.6f, -2.7f}, {4.3f, -2.45f}}, 2);
    poly({{6.05f, 0.f}, {4.0f, 2.1f}, {-5.2f, 2.3f}, {-6.0f, 1.4f}, {-6.0f, -1.4f}, {-5.2f, -2.3f}, {4.0f, -2.1f}}, 1);
    poly({{5.7f, 0.f}, {3.6f, 1.55f}, {-4.8f, 1.7f}, {-5.5f, 0.9f}, {-5.5f, -0.9f}, {-4.8f, -1.7f}, {3.6f, -1.55f}}, 9);
    poly({{-0.4f, 1.85f}, {-4.3f, 1.85f}, {-4.3f, -1.85f}, {-0.4f, -1.85f}}, 4);
    poly({{-0.7f, 1.35f}, {-3.9f, 1.35f}, {-3.9f, -1.35f}, {-0.7f, -1.35f}}, 7);
    poly({{-1.5f, 0.85f}, {-3.2f, 0.85f}, {-3.2f, -0.85f}, {-1.5f, -0.85f}}, 10);
    poly({{1.3f, 0.55f}, {3.4f, 0.55f}, {3.4f, -0.55f}, {1.3f, -0.55f}}, 6);
    poly({{2.6f, 1.15f}, {3.5f, 1.15f}, {3.5f, 0.35f}, {2.6f, 0.35f}}, 13);
    poly({{2.6f, -0.35f}, {3.5f, -0.35f}, {3.5f, -1.15f}, {2.6f, -1.15f}}, 13);
    dot(-2.55f, 0.f, 7.2f, 6.4f, 5);
    dot(-2.55f, 0.f, 4.4f, 3.8f, 8);
    dot(-2.55f, 0.f, 2.1f, 1.8f, 2);
    dot(5.55f, 0.f, 3.2f, 2.6f, 6);
    dot(-5.7f, 1.55f, 2.3f, 2.3f, 12);
    dot(-5.7f, -1.55f, 2.3f, 2.3f, 12);
    dot(0.35f, 2.15f, 2.6f, 2.4f, 3);
    dot(0.35f, -2.15f, 2.6f, 2.4f, 3);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintDrive(float heading) {
    Bitmap b(120, 88);
    auto poly = [&](std::initializer_list<Pt> m, int c) { paintBody(b, heading, kPx, m, c); };
    auto dot = [&](float x, float y, float rx, float ry, int c) { paintDot(b, heading, kPx, x, y, rx, ry, c); };
    const float ys[5] = {-3.15f, -1.55f, 0.f, 1.55f, 3.15f};
    const int cols[5] = {1, 2, 8, 2, 3};
    for (int i = 0; i < 5; i++) {
        float y = ys[i];
        poly({{7.55f, y - 0.62f}, {-7.55f, y - 0.62f}, {-7.55f, y + 0.62f}, {7.55f, y + 0.62f}}, cols[i]);
        dot(7.15f, y, 2.5f, 2.4f, 4);
        dot(-7.15f, y, 2.5f, 2.4f, 9);
    }
    poly({{4.7f, -3.7f}, {5.45f, -3.7f}, {5.45f, 3.7f}, {4.7f, 3.7f}}, 5);
    poly({{-5.45f, -3.7f}, {-4.7f, -3.7f}, {-4.7f, 3.7f}, {-5.45f, 3.7f}}, 5);
    poly({{0.15f, -3.55f}, {0.7f, -3.55f}, {0.7f, 3.55f}, {0.15f, 3.55f}}, 7);
    dot(4.9f, -3.35f, 2.0f, 2.0f, 6);
    dot(4.9f, 3.35f, 2.0f, 2.0f, 6);
    dot(-4.9f, -3.35f, 2.0f, 2.0f, 6);
    dot(-4.9f, 3.35f, 2.0f, 2.0f, 6);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintShade() {
    Bitmap b(40, 22);
    b.ellipse(20, 11, 18, 8, 1);
    return b;
}

Bitmap paintLogH() {
    Bitmap b(36, 12);
    for (int y = 2; y < 10; y++) {
        for (int x = 0; x < 36; x++) {
            int c = ((x / 6) & 1) ? 1 : 2;
            if (y < 4) c = 1;
            if ((x % 9) == 0) c = 3;
            b.set(x, y, c);
        }
    }
    b.ellipse(4, 6, 3.2f, 4.2f, 2);
    b.ellipse(32, 6, 3.2f, 4.2f, 2);
    return b;
}

Bitmap paintLogV() {
    Bitmap b(12, 36);
    for (int y = 0; y < 36; y++) {
        for (int x = 2; x < 10; x++) {
            int c = ((y / 6) & 1) ? 1 : 2;
            if (x < 4) c = 1;
            if ((y % 9) == 0) c = 3;
            b.set(x, y, c);
        }
    }
    b.ellipse(6, 4, 4.2f, 3.2f, 2);
    b.ellipse(6, 32, 4.2f, 3.2f, 2);
    return b;
}

Bitmap paintPost() {
    Bitmap b(12, 26);
    b.rect(4, 6, 4, 18, 1);
    b.rect(3, 8, 6, 3, 2);
    b.ellipse(6, 4, 4, 3.2f, 1);
    b.rect(5, 22, 2, 4, 2);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(14, 20);
    b.ellipse(7, 8, 5.5f, 6, 1);
    b.rect(5, 4, 4, 9, 2);
    b.ellipse(7, 8, 2.1f, 2.6f, 3);
    b.rect(6, 14, 2, 5, 3);
    return b;
}

Bitmap paintStripe() {
    Bitmap b(48, 8);
    for (int x = 0; x < 48; x++) {
        int c = ((x / 6) & 1) ? 1 : 2;
        for (int y = 1; y < 7; y++) b.set(x, y, c);
    }
    return b;
}

Bitmap paintQuay() {
    Bitmap b(22, 40);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x, y);
            int c = 2;
            if (x > 14) c = 1;
            if (x > 18) c = 3;
            if ((h & 9) == 0) c = x > 14 ? 1 : 3;
            b.set(x, y, c);
        }
    }
    return b;
}

Bitmap paintShed() {
    Bitmap b(34, 24);
    b.rect(2, 8, 30, 14, 2);
    b.poly({{1, 8}, {17, 1}, {33, 8}}, 4);
    b.rect(6, 12, 7, 6, 5);
    b.rect(20, 12, 7, 5, 5);
    b.rect(14, 14, 5, 8, 6);
    b.outline(15, false);
    return b.cropToContent(0);
}

Bitmap paintLamp() {
    Bitmap b(10, 20);
    b.rect(4, 7, 2, 12, 7);
    b.ellipse(5, 5, 3.6f, 3.4f, 8);
    b.ellipse(5, 5, 1.6f, 1.6f, 1);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(14, 10);
    b.ellipse(7, 5, 6, 3.4f, 1);
    b.ellipse(7, 5, 2.6f, 1.4f, 2);
    return b;
}

Bitmap paintSmoke() {
    Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 5, 1);
    b.ellipse(6, 6, 2.8f, 2.2f, 2);
    return b;
}

Bitmap paintGull(int flap) {
    Bitmap b(20, 12);
    float tip = flap ? 2.f : 7.f;
    b.line(2, tip, 9, 6, 1, 1.4f);
    b.line(18, tip, 11, 6, 1, 1.4f);
    b.ellipse(10, 6, 1.5f, 1.2f, 2);
    return b;
}

Bitmap paintPin() {
    Bitmap b(9, 9);
    b.poly({{4, 0}, {8, 4}, {4, 8}, {0, 4}}, 1);
    return b;
}

Bitmap paintLink() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3.1f, 3.1f, 1);
    b.ellipse(4, 4, 1.4f, 1.4f, 0);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 10, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TUG,
           {0, gs::rgb4(10, 12, 9), gs::rgb4(1, 2, 2), gs::rgb4(12, 2, 2), gs::rgb4(14, 13, 10), gs::rgb4(2, 8, 4),
            gs::rgb4(14, 12, 2), gs::rgb4(2, 4, 8), gs::rgb4(10, 5, 2), gs::rgb4(4, 5, 4), gs::rgb4(15, 15, 14),
            gs::rgb4(3, 10, 10), gs::rgb4(6, 1, 1), gs::rgb4(13, 10, 3), gs::rgb4(12, 14, 13), ink});
    setPal(vdp, PAL_DRIVE,
           {0, gs::rgb4(13, 10, 5), gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(12, 8, 4), gs::rgb4(9, 8, 5),
            gs::rgb4(5, 5, 6), gs::rgb4(8, 6, 3), gs::rgb4(14, 12, 8), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BOOM,
           {0, gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(14, 12, 2), gs::rgb4(3, 6, 3), gs::rgb4(7, 7, 8), 0, 0, 0,
            0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_QUAY,
           {0, gs::rgb4(9, 9, 8), gs::rgb4(5, 5, 5), gs::rgb4(7, 7, 6), gs::rgb4(8, 3, 2), gs::rgb4(4, 6, 8),
            gs::rgb4(4, 3, 2), gs::rgb4(3, 3, 3), gs::rgb4(15, 14, 6), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_END,
           {0, gs::rgb4(14, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(2, 1, 1), gs::rgb4(2, 10, 3), gs::rgb4(1, 5, 2), 0, 0,
            0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(12, 12, 12), gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 6), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 13, 2), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    for (int i = 0; i < 16; i++) vdp.setColor(PAL_CH * 16 + i, 0);
    vdp.setColor(PAL_CH * 16 + 1, gs::rgb4(6, 7, 4));
    vdp.setColor(PAL_CH * 16 + 2, gs::rgb4(4, 5, 3));
    vdp.setColor(PAL_CH * 16 + 3, gs::rgb4(8, 8, 5));
    vdp.setColor(PAL_CH * 16 + 4, gs::rgb4(7, 8, 5));
    vdp.setColor(PAL_CH * 16 + 5, gs::rgb4(3, 4, 3));
    vdp.setColor(PAL_CH * 16 + 8, gs::rgb4(9, 8, 5));
    vdp.setColor(PAL_CH * 16 + 11, gs::rgb4(5, 12, 14));
    vdp.setColor(PAL_CH * 16 + 12, gs::rgb4(2, 7, 11));
    vdp.setColor(PAL_CH * 16 + 13, gs::rgb4(12, 15, 15));
    vdp.setFogColor(gs::rgb4(6, 8, 9));

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) {
        float h = i * kTau / 16.f;
        art.tug[i] = gs::uploadMipped(vdp, paintTug(h));
        art.drive[i] = gs::uploadMipped(vdp, paintDrive(h));
    }
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.logH = gs::uploadMipped(vdp, paintLogH());
    art.logV = gs::uploadMipped(vdp, paintLogV());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.stripe = gs::uploadMipped(vdp, paintStripe());
    art.quay = gs::uploadMipped(vdp, paintQuay());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.smoke = gs::uploadMipped(vdp, paintSmoke());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(0));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(1));
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.link = gs::uploadMipped(vdp, paintLink());
    art.title = words(vdp, "TUGBOAT BOOM", 3);
    art.delivered = words(vdp, "DELIVERED", 3);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.shortOf = words(vdp, "SHORT", 3);
    art.offBoom = words(vdp, "NOT ON THE BOOM", 2);
    art.broke = words(vdp, "BROKE THE BOOM", 2);
    art.paused = words(vdp, "PAUSED", 3);
}

}  // namespace tugboom
