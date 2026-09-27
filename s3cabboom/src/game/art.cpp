#include "art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace cabboom {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float kTau = 6.28318530718f;
constexpr float kPx = 4.2f;

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

Bitmap paintCab(float heading) {
    Bitmap b(96, 96);
    auto poly = [&](std::initializer_list<Pt> m, int c) { paintBody(b, heading, kPx, m, c); };
    auto dot = [&](float x, float y, float rx, float ry, int c) { paintDot(b, heading, kPx, x, y, rx, ry, c); };
    poly({{5.2f, 0.f}, {3.6f, 1.85f}, {-4.4f, 1.95f}, {-5.05f, 1.15f}, {-5.05f, -1.15f}, {-4.4f, -1.95f}, {3.6f, -1.85f}}, 2);
    poly({{4.85f, 0.f}, {3.3f, 1.55f}, {-4.05f, 1.65f}, {-4.6f, 0.95f}, {-4.6f, -0.95f}, {-4.05f, -1.65f}, {3.3f, -1.55f}}, 1);
    for (int i = -3; i <= 3; i++) {
        float x0 = -1.6f + i * 0.42f;
        poly({{x0, 1.55f}, {x0 + 0.2f, 1.55f}, {x0 + 0.2f, -1.55f}, {x0, -1.55f}}, (i & 1) ? 3 : 4);
    }
    poly({{1.1f, 1.05f}, {3.15f, 0.85f}, {3.15f, -0.85f}, {1.1f, -1.05f}}, 6);
    poly({{1.35f, 0.72f}, {2.7f, 0.58f}, {2.7f, -0.58f}, {1.35f, -0.72f}}, 8);
    poly({{-1.2f, 1.15f}, {-3.4f, 1.15f}, {-3.4f, -1.15f}, {-1.2f, -1.15f}}, 7);
    poly({{-1.5f, 0.75f}, {-3.05f, 0.75f}, {-3.05f, -0.75f}, {-1.5f, -0.75f}}, 10);
    dot(-2.2f, 0.f, 5.5f, 4.2f, 5);
    dot(-2.2f, 0.f, 2.2f, 1.6f, 9);
    dot(4.55f, 0.f, 2.4f, 2.0f, 11);
    dot(4.55f, 0.f, 1.1f, 0.9f, 12);
    dot(-4.55f, 1.35f, 1.8f, 1.6f, 13);
    dot(-4.55f, -1.35f, 1.8f, 1.6f, 13);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintDrive(float heading) {
    Bitmap b(110, 80);
    auto poly = [&](std::initializer_list<Pt> m, int c) { paintBody(b, heading, kPx, m, c); };
    auto dot = [&](float x, float y, float rx, float ry, int c) { paintDot(b, heading, kPx, x, y, rx, ry, c); };
    poly({{6.3f, 3.15f}, {-6.3f, 3.15f}, {-6.3f, -3.15f}, {6.3f, -3.15f}}, 2);
    poly({{5.9f, 2.75f}, {-5.9f, 2.75f}, {-5.9f, -2.75f}, {5.9f, -2.75f}}, 1);
    poly({{5.4f, 2.2f}, {1.2f, 2.2f}, {1.2f, -2.2f}, {5.4f, -2.2f}}, 4);
    poly({{0.6f, 2.2f}, {-2.4f, 2.2f}, {-2.4f, -2.2f}, {0.6f, -2.2f}}, 8);
    poly({{-3.0f, 2.2f}, {-5.5f, 2.2f}, {-5.5f, -2.2f}, {-3.0f, -2.2f}}, 3);
    poly({{2.4f, 1.35f}, {4.6f, 1.35f}, {4.6f, 0.15f}, {2.4f, 0.15f}}, 6);
    poly({{2.4f, -0.15f}, {4.6f, -0.15f}, {4.6f, -1.35f}, {2.4f, -1.35f}}, 7);
    dot(6.05f, 2.4f, 2.2f, 2.0f, 5);
    dot(6.05f, -2.4f, 2.2f, 2.0f, 5);
    dot(-6.05f, 2.4f, 2.2f, 2.0f, 5);
    dot(-6.05f, -2.4f, 2.2f, 2.0f, 5);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintShade() {
    Bitmap b(36, 18);
    b.ellipse(18, 9, 16, 7, 1);
    return b;
}

Bitmap paintGirderH() {
    Bitmap b(34, 12);
    b.rect(0, 1, 34, 3, 1);
    b.rect(0, 8, 34, 3, 1);
    for (int x = 0; x < 34; x += 6) b.line(float(x), 2, float(x + 6), 9, 2, 1.2f);
    b.rect(0, 4, 34, 1, 3);
    return b;
}

Bitmap paintGirderV() {
    Bitmap b(12, 34);
    b.rect(1, 0, 3, 34, 1);
    b.rect(8, 0, 3, 34, 1);
    for (int y = 0; y < 34; y += 6) b.line(2, float(y), 9, float(y + 6), 2, 1.2f);
    b.rect(4, 0, 1, 34, 3);
    return b;
}

Bitmap paintPost() {
    Bitmap b(12, 28);
    b.rect(4, 6, 4, 20, 1);
    b.rect(2, 4, 8, 4, 2);
    b.ellipse(6, 4, 4.2f, 3.2f, 3);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(14, 20);
    b.ellipse(7, 8, 5.5f, 6, 1);
    b.rect(5, 3, 4, 10, 2);
    b.ellipse(7, 8, 2.0f, 2.4f, 3);
    b.rect(6, 14, 2, 5, 4);
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
    Bitmap b(22, 36);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x, y);
            int c = 2;
            if (x > 14) c = 1;
            if (x > 18) c = 3;
            if ((h & 11) == 0) c = 4;
            b.set(x, y, c);
        }
    }
    return b;
}

Bitmap paintStand() {
    Bitmap b(36, 26);
    b.rect(2, 10, 32, 14, 2);
    b.poly({{0, 10}, {18, 1}, {36, 10}}, 4);
    b.rect(6, 13, 8, 6, 5);
    b.rect(22, 13, 8, 6, 6);
    b.rect(15, 16, 6, 8, 1);
    b.rect(8, 4, 10, 3, 7);
    b.outline(15, false);
    return b.cropToContent(0);
}

Bitmap paintLamp() {
    Bitmap b(10, 22);
    b.rect(4, 8, 2, 13, 2);
    b.ellipse(5, 5, 3.6f, 3.4f, 1);
    b.ellipse(5, 5, 1.5f, 1.5f, 3);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(14, 10);
    b.ellipse(7, 5, 6, 3.4f, 1);
    b.ellipse(7, 5, 2.4f, 1.3f, 2);
    return b;
}

Bitmap paintExhaust() {
    Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4.2f, 1);
    b.ellipse(5, 5, 2.2f, 1.8f, 2);
    return b;
}

Bitmap paintBird(int flap) {
    Bitmap b(18, 10);
    float tip = flap ? 1.5f : 6.f;
    b.line(1, tip, 8, 5, 1, 1.3f);
    b.line(17, tip, 10, 5, 1, 1.3f);
    b.ellipse(9, 5, 1.4f, 1.1f, 2);
    return b;
}

Bitmap paintPin() {
    Bitmap b(9, 9);
    b.poly({{4, 0}, {8, 4}, {4, 8}, {0, 4}}, 1);
    return b;
}

Bitmap paintLash() {
    Bitmap b(8, 8);
    b.rect(1, 3, 6, 2, 1);
    b.rect(3, 1, 2, 6, 2);
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
    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(15, 13, 2), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), gs::rgb4(14, 14, 13), gs::rgb4(12, 2, 2),
            gs::rgb4(6, 10, 13), gs::rgb4(3, 3, 4), gs::rgb4(10, 14, 15), gs::rgb4(15, 15, 14), gs::rgb4(4, 8, 12),
            gs::rgb4(15, 4, 3), gs::rgb4(15, 14, 6), gs::rgb4(8, 8, 9), gs::rgb4(13, 11, 1), ink});
    setPal(vdp, PAL_DRIVE,
           {0, gs::rgb4(4, 4, 5), gs::rgb4(1, 1, 2), gs::rgb4(12, 10, 3), gs::rgb4(7, 7, 8), gs::rgb4(14, 12, 2),
            gs::rgb4(15, 14, 8), gs::rgb4(9, 8, 3), gs::rgb4(14, 6, 2), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BOOM,
           {0, gs::rgb4(14, 7, 1), gs::rgb4(10, 4, 1), gs::rgb4(15, 12, 3), gs::rgb4(6, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, ink});
    setPal(vdp, PAL_QUAY,
           {0, gs::rgb4(9, 9, 8), gs::rgb4(5, 5, 5), gs::rgb4(7, 6, 5), gs::rgb4(6, 6, 7), gs::rgb4(8, 3, 2),
            gs::rgb4(4, 7, 9), gs::rgb4(15, 12, 2), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_END,
           {0, gs::rgb4(14, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(2, 1, 1), gs::rgb4(3, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 5), gs::rgb4(4, 4, 4), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 3), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 13, 2), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    for (int i = 0; i < 16; i++) vdp.setColor(PAL_WATER * 16 + i, 0);
    vdp.setColor(PAL_WATER * 16 + 1, gs::rgb4(6, 7, 4));
    vdp.setColor(PAL_WATER * 16 + 2, gs::rgb4(4, 5, 3));
    vdp.setColor(PAL_WATER * 16 + 3, gs::rgb4(8, 8, 5));
    vdp.setColor(PAL_WATER * 16 + 4, gs::rgb4(7, 8, 5));
    vdp.setColor(PAL_WATER * 16 + 5, gs::rgb4(3, 4, 3));
    vdp.setColor(PAL_WATER * 16 + 8, gs::rgb4(9, 8, 5));
    vdp.setColor(PAL_WATER * 16 + 11, gs::rgb4(4, 10, 14));
    vdp.setColor(PAL_WATER * 16 + 12, gs::rgb4(2, 6, 11));
    vdp.setColor(PAL_WATER * 16 + 13, gs::rgb4(11, 15, 15));
    vdp.setFogColor(gs::rgb4(5, 7, 9));

    loadFont(vdp, art);
    for (int i = 0; i < 8; i++) {
        float h = i * kTau / 8.f;
        art.cab[i] = gs::uploadMipped(vdp, paintCab(h));
        art.drive[i] = gs::uploadMipped(vdp, paintDrive(h));
    }
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.girderH = gs::uploadMipped(vdp, paintGirderH());
    art.girderV = gs::uploadMipped(vdp, paintGirderV());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.stripe = gs::uploadMipped(vdp, paintStripe());
    art.quay = gs::uploadMipped(vdp, paintQuay());
    art.stand = gs::uploadMipped(vdp, paintStand());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.exhaust = gs::uploadMipped(vdp, paintExhaust());
    art.bird[0] = gs::uploadMipped(vdp, paintBird(0));
    art.bird[1] = gs::uploadMipped(vdp, paintBird(1));
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.lash = gs::uploadMipped(vdp, paintLash());
    art.title = words(vdp, "CAB BOOM", 3);
    art.delivered = words(vdp, "DELIVERED", 3);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.shortOf = words(vdp, "SHORT", 3);
    art.offBoom = words(vdp, "NOT ON THE BOOM", 2);
    art.broke = words(vdp, "BROKE THE BOOM", 2);
    art.paused = words(vdp, "PAUSED", 3);
}

}  // namespace cabboom
