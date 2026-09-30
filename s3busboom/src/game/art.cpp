#include "art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace busboom {
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

Bitmap paintBus(float heading) {
    Bitmap b(96, 96);
    auto poly = [&](std::initializer_list<Pt> m, int c) { paintBody(b, heading, kPx, m, c); };
    auto dot = [&](float x, float y, float rx, float ry, int c) { paintDot(b, heading, kPx, x, y, rx, ry, c); };
    poly({{5.05f, 0.f}, {4.2f, 1.85f}, {-4.5f, 1.9f}, {-4.9f, 1.2f}, {-4.9f, -1.2f}, {-4.5f, -1.9f}, {4.2f, -1.85f}}, 2);
    poly({{4.7f, 0.f}, {3.8f, 1.55f}, {-4.2f, 1.6f}, {-4.55f, 1.0f}, {-4.55f, -1.0f}, {-4.2f, -1.6f}, {3.8f, -1.55f}}, 1);
    for (int i = -2; i <= 2; i++) {
        float x0 = -2.4f + i * 1.15f;
        poly({{x0, 1.35f}, {x0 + 0.72f, 1.35f}, {x0 + 0.72f, -1.35f}, {x0, -1.35f}}, (i & 1) ? 6 : 8);
    }
    poly({{2.6f, 1.05f}, {4.15f, 0.7f}, {4.15f, -0.7f}, {2.6f, -1.05f}}, 7);
    poly({{2.9f, 0.55f}, {3.85f, 0.4f}, {3.85f, -0.4f}, {2.9f, -0.55f}}, 10);
    poly({{-3.6f, 0.55f}, {-4.35f, 0.55f}, {-4.35f, -0.55f}, {-3.6f, -0.55f}}, 4);
    dot(1.6f, 1.55f, 3.2f, 2.2f, 3);
    dot(-1.8f, 1.55f, 3.2f, 2.2f, 3);
    dot(1.6f, -1.55f, 3.2f, 2.2f, 3);
    dot(-1.8f, -1.55f, 3.2f, 2.2f, 3);
    dot(4.7f, 1.2f, 1.6f, 1.4f, 11);
    dot(4.7f, -1.2f, 1.6f, 1.4f, 11);
    dot(-4.6f, 1.15f, 1.5f, 1.3f, 13);
    dot(-4.6f, -1.15f, 1.5f, 1.3f, 13);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintDrive(float heading) {
    Bitmap b(110, 80);
    auto poly = [&](std::initializer_list<Pt> m, int c) { paintBody(b, heading, kPx, m, c); };
    auto dot = [&](float x, float y, float rx, float ry, int c) { paintDot(b, heading, kPx, x, y, rx, ry, c); };
    poly({{6.3f, 3.15f}, {-6.3f, 3.15f}, {-6.3f, -3.15f}, {6.3f, -3.15f}}, 2);
    poly({{5.9f, 2.7f}, {-5.9f, 2.7f}, {-5.9f, -2.7f}, {5.9f, -2.7f}}, 1);
    poly({{5.2f, 2.0f}, {0.4f, 2.0f}, {0.4f, -2.0f}, {5.2f, -2.0f}}, 4);
    poly({{-0.4f, 2.0f}, {-5.2f, 2.0f}, {-5.2f, -2.0f}, {-0.4f, -2.0f}}, 8);
    poly({{1.2f, 1.1f}, {4.2f, 1.1f}, {4.2f, 0.15f}, {1.2f, 0.15f}}, 6);
    poly({{1.2f, -0.15f}, {4.2f, -0.15f}, {4.2f, -1.1f}, {1.2f, -1.1f}}, 7);
    poly({{-1.4f, 0.9f}, {-4.0f, 0.9f}, {-4.0f, -0.9f}, {-1.4f, -0.9f}}, 3);
    dot(5.6f, 2.35f, 2.0f, 1.8f, 5);
    dot(5.6f, -2.35f, 2.0f, 1.8f, 5);
    dot(-5.6f, 2.35f, 2.0f, 1.8f, 5);
    dot(-5.6f, -2.35f, 2.0f, 1.8f, 5);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintShade() {
    Bitmap b(36, 16);
    b.ellipse(18, 8, 16, 6, 1);
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

Bitmap paintStripe() {
    Bitmap b(48, 8);
    for (int x = 0; x < 48; x++) {
        int c = ((x / 6) & 1) ? 1 : 2;
        for (int y = 1; y < 7; y++) b.set(x, y, c);
    }
    return b;
}

Bitmap paintWalk() {
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

Bitmap paintShelter() {
    Bitmap b(28, 22);
    b.rect(2, 10, 24, 10, 2);
    b.rect(2, 4, 24, 4, 1);
    b.rect(4, 14, 2, 7, 3);
    b.rect(22, 14, 2, 7, 3);
    b.rect(8, 12, 10, 6, 4);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(10, 22);
    b.rect(4, 6, 2, 16, 2);
    b.ellipse(5, 4, 3.4f, 3.0f, 1);
    b.ellipse(5, 4, 1.6f, 1.4f, 3);
    return b;
}

Bitmap paintExhaust() {
    Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4, 1);
    b.ellipse(6, 6, 2.4f, 2, 2);
    return b;
}

Bitmap paintHitch() {
    Bitmap b(8, 8);
    b.rect(1, 3, 6, 2, 1);
    b.rect(3, 1, 2, 6, 2);
    return b;
}

Bitmap paintBird(int flap) {
    Bitmap b(16, 10);
    b.ellipse(8, 6, 2.2f, 1.8f, 2);
    if (flap) {
        b.line(8, 5, 1, 1, 1, 1.2f);
        b.line(8, 5, 15, 1, 1, 1.2f);
    } else {
        b.line(8, 6, 1, 7, 1, 1.2f);
        b.line(8, 6, 15, 7, 1, 1.2f);
    }
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
    setPal(vdp, PAL_BUS,
           {0, gs::rgb4(15, 12, 2), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), gs::rgb4(14, 4, 2), gs::rgb4(12, 2, 2),
            gs::rgb4(6, 11, 14), gs::rgb4(3, 4, 6), gs::rgb4(11, 15, 15), gs::rgb4(15, 15, 14), gs::rgb4(8, 13, 15),
            gs::rgb4(15, 14, 6), gs::rgb4(15, 8, 2), gs::rgb4(8, 8, 9), gs::rgb4(13, 11, 1), ink});
    setPal(vdp, PAL_DRIVE,
           {0, gs::rgb4(5, 6, 7), gs::rgb4(1, 1, 2), gs::rgb4(12, 10, 3), gs::rgb4(8, 9, 10), gs::rgb4(14, 12, 2),
            gs::rgb4(15, 14, 8), gs::rgb4(9, 8, 3), gs::rgb4(14, 6, 2), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BOOM,
           {0, gs::rgb4(14, 7, 1), gs::rgb4(10, 4, 1), gs::rgb4(15, 12, 3), gs::rgb4(6, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, ink});
    setPal(vdp, PAL_WALK,
           {0, gs::rgb4(9, 9, 8), gs::rgb4(5, 5, 5), gs::rgb4(7, 6, 5), gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, ink});
    setPal(vdp, PAL_EXHAUST, {0, gs::rgb4(10, 10, 11), gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_END,
           {0, gs::rgb4(14, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 5), gs::rgb4(4, 4, 4), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 3), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 13, 2), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, 0);
    vdp.setColor(PAL_ROAD * 16 + 1, gs::rgb4(4, 4, 5));
    vdp.setColor(PAL_ROAD * 16 + 2, gs::rgb4(3, 3, 4));
    vdp.setColor(PAL_ROAD * 16 + 3, gs::rgb4(12, 11, 4));
    vdp.setColor(PAL_ROAD * 16 + 4, gs::rgb4(5, 5, 6));
    vdp.setColor(PAL_ROAD * 16 + 5, gs::rgb4(2, 2, 3));
    vdp.setColor(PAL_ROAD * 16 + 8, gs::rgb4(6, 6, 6));
    vdp.setFogColor(gs::rgb4(6, 6, 7));

    loadFont(vdp, art);
    for (int i = 0; i < 8; i++) {
        float h = i * kTau / 8.f;
        art.bus[i] = gs::uploadMipped(vdp, paintBus(h));
        art.drive[i] = gs::uploadMipped(vdp, paintDrive(h));
    }
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.girderH = gs::uploadMipped(vdp, paintGirderH());
    art.girderV = gs::uploadMipped(vdp, paintGirderV());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.stripe = gs::uploadMipped(vdp, paintStripe());
    art.walk = gs::uploadMipped(vdp, paintWalk());
    art.shelter = gs::uploadMipped(vdp, paintShelter());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.exhaust = gs::uploadMipped(vdp, paintExhaust());
    art.hitch = gs::uploadMipped(vdp, paintHitch());
    art.bird[0] = gs::uploadMipped(vdp, paintBird(0));
    art.bird[1] = gs::uploadMipped(vdp, paintBird(1));
    art.title = words(vdp, "BUS BOOM", 3);
    art.delivered = words(vdp, "DELIVERED", 3);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.shortOf = words(vdp, "SHORT", 3);
    art.offBoom = words(vdp, "NOT ON THE BOOM", 2);
    art.broke = words(vdp, "BROKE THE BOOM", 2);
    art.paused = words(vdp, "PAUSED", 3);
}

}  // namespace busboom
