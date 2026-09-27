#include "art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace bargeboom {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float kTau = 6.28318530718f;
constexpr float kPx = 3.6f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void paintBody(Bitmap& b, float heading, float sc, std::initializer_list<Pt> meters, int col) {
    const float co = std::cos(heading), sn = std::sin(heading);
    const float cx = b.w * 0.5f, cy = b.h * 0.5f;
    std::vector<Pt> p;
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

Bitmap paintBarge(float heading) {
    Bitmap b(120, 96);
    auto poly = [&](std::initializer_list<Pt> m, int c) { paintBody(b, heading, kPx, m, c); };
    auto dot = [&](float x, float y, float rx, float ry, int c) { paintDot(b, heading, kPx, x, y, rx, ry, c); };
    poly({{8.6f, 0.f}, {7.2f, 4.2f}, {-8.4f, 4.4f}, {-9.1f, 3.2f}, {-9.1f, -3.2f}, {-8.4f, -4.4f}, {7.2f, -4.2f}}, 2);
    poly({{8.1f, 0.f}, {6.6f, 3.6f}, {-7.8f, 3.8f}, {-8.5f, 2.6f}, {-8.5f, -2.6f}, {-7.8f, -3.8f}, {6.6f, -3.6f}}, 1);
    poly({{1.2f, 2.4f}, {-6.4f, 2.4f}, {-6.4f, -2.4f}, {1.2f, -2.4f}}, 4);
    poly({{0.4f, 1.7f}, {-5.6f, 1.7f}, {-5.6f, -1.7f}, {0.4f, -1.7f}}, 7);
    poly({{-1.4f, 1.0f}, {-4.2f, 1.0f}, {-4.2f, -1.0f}, {-1.4f, -1.0f}}, 10);
    poly({{3.2f, 3.1f}, {6.4f, 3.1f}, {6.4f, -3.1f}, {3.2f, -3.1f}}, 3);
    poly({{3.6f, 2.5f}, {6.0f, 2.5f}, {6.0f, -2.5f}, {3.6f, -2.5f}}, 8);
    dot(-3.0f, 0.f, 5.5f, 4.2f, 5);
    dot(-3.0f, 0.f, 2.6f, 2.0f, 9);
    dot(4.8f, 0.f, 3.4f, 2.2f, 6);
    dot(7.6f, 2.6f, 2.2f, 2.0f, 12);
    dot(7.6f, -2.6f, 2.2f, 2.0f, 12);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintDrive(float heading) {
    Bitmap b(96, 80);
    auto poly = [&](std::initializer_list<Pt> m, int c) { paintBody(b, heading, kPx, m, c); };
    auto dot = [&](float x, float y, float rx, float ry, int c) { paintDot(b, heading, kPx, x, y, rx, ry, c); };
    poly({{5.2f, 3.1f}, {-5.2f, 3.1f}, {-5.2f, -3.1f}, {5.2f, -3.1f}}, 2);
    poly({{4.7f, 2.5f}, {-4.7f, 2.5f}, {-4.7f, -2.5f}, {4.7f, -2.5f}}, 1);
    poly({{1.6f, 1.5f}, {-1.6f, 1.5f}, {-1.6f, -1.5f}, {1.6f, -1.5f}}, 4);
    poly({{0.7f, 2.8f}, {-0.7f, 2.8f}, {-0.7f, -2.8f}, {0.7f, -2.8f}}, 8);
    dot(0.f, 0.f, 7.2f, 7.2f, 5);
    dot(0.f, 0.f, 4.0f, 4.0f, 6);
    dot(0.f, 0.f, 1.6f, 1.6f, 9);
    dot(4.2f, 2.2f, 2.0f, 2.0f, 3);
    dot(-4.2f, -2.2f, 2.0f, 2.0f, 3);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintShade() {
    Bitmap b(36, 18);
    b.ellipse(18, 9, 16, 7, 1);
    return b;
}

Bitmap paintTimber() {
    Bitmap b(14, 28);
    for (int y = 0; y < 28; y++)
        for (int x = 2; x < 12; x++) b.set(x, y, (y / 5 & 1) ? 2 : 1);
    b.ellipse(7, 3, 4, 3, 3);
    b.ellipse(7, 25, 4, 3, 3);
    return b;
}

Bitmap paintPile() {
    Bitmap b(10, 22);
    b.rect(3, 0, 4, 22, 1);
    b.rect(2, 16, 6, 4, 2);
    return b;
}

Bitmap paintBoomArm() {
    Bitmap b(48, 16);
    b.line(2, 12, 46, 3, 1, 3.2f);
    b.line(2, 13, 46, 4, 2, 1.4f);
    b.ellipse(6, 12, 3, 3, 3);
    b.ellipse(44, 4, 3, 3, 3);
    return b;
}

Bitmap paintReed() {
    Bitmap b(16, 20);
    b.rect(0, 8, 16, 12, 1);
    b.rect(2, 4, 12, 6, 2);
    b.rect(6, 0, 4, 6, 3);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(8, 18);
    b.rect(3, 6, 2, 12, 1);
    b.ellipse(4, 4, 3, 3, 2);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(12, 8);
    b.ellipse(6, 4, 5, 3, 1);
    b.ellipse(3, 4, 2, 1.4f, 2);
    return b;
}

Bitmap paintBird(int flap) {
    Bitmap b(18, 10);
    if (flap) {
        b.line(1, 7, 9, 3, 1, 1.4f);
        b.line(17, 7, 9, 3, 1, 1.4f);
    } else {
        b.line(1, 2, 9, 5, 1, 1.4f);
        b.line(17, 2, 9, 5, 1, 1.4f);
    }
    b.ellipse(9, 5, 1.4f, 1.2f, 2);
    return b;
}

Bitmap paintPin() {
    Bitmap b(9, 9);
    b.poly({{4, 0}, {8, 4}, {4, 8}, {0, 4}}, 1);
    return b;
}

Bitmap paintChain() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.ellipse(4, 4, 1.3f, 1.3f, 0);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 13), gs::rgb4(7, 9, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BARGE,
           {0, gs::rgb4(7, 8, 7), gs::rgb4(2, 2, 2), gs::rgb4(12, 3, 2), gs::rgb4(11, 9, 6), gs::rgb4(4, 6, 8),
            gs::rgb4(14, 12, 3), gs::rgb4(3, 4, 6), gs::rgb4(13, 11, 8), gs::rgb4(15, 14, 10), gs::rgb4(8, 10, 12),
            gs::rgb4(2, 5, 4), gs::rgb4(15, 8, 2), 0, 0, ink});
    setPal(vdp, PAL_DRIVE,
           {0, gs::rgb4(12, 10, 4), gs::rgb4(6, 5, 3), gs::rgb4(14, 6, 2), gs::rgb4(4, 4, 5), gs::rgb4(9, 9, 8),
            gs::rgb4(15, 13, 2), gs::rgb4(3, 3, 3), gs::rgb4(14, 14, 12), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BOOM,
           {0, gs::rgb4(9, 6, 3), gs::rgb4(5, 3, 2), gs::rgb4(14, 11, 3), gs::rgb4(3, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, ink});
    setPal(vdp, PAL_BANK,
           {0, gs::rgb4(4, 7, 3), gs::rgb4(3, 5, 2), gs::rgb4(8, 8, 5), gs::rgb4(6, 5, 3), gs::rgb4(12, 11, 4), 0, 0, 0,
            0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 14, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_END, {0, gs::rgb4(14, 3, 2), gs::rgb4(15, 15, 13), gs::rgb4(3, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(8, 8, 9), gs::rgb4(3, 3, 4), gs::rgb4(12, 4, 3), gs::rgb4(5, 5, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(7, 15, 5), gs::rgb4(1, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 2), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 5), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(15, 15, 15), gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 12, 2), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    for (int i = 0; i < 16; i++) vdp.setColor(PAL_CH * 16 + i, 0);
    vdp.setColor(PAL_CH * 16 + 1, gs::rgb4(5, 8, 6));
    vdp.setColor(PAL_CH * 16 + 2, gs::rgb4(3, 5, 4));
    vdp.setColor(PAL_CH * 16 + 3, gs::rgb4(7, 9, 6));
    vdp.setColor(PAL_CH * 16 + 11, gs::rgb4(4, 11, 13));
    vdp.setColor(PAL_CH * 16 + 12, gs::rgb4(2, 6, 10));
    vdp.setColor(PAL_CH * 16 + 13, gs::rgb4(11, 15, 15));
    vdp.setFogColor(gs::rgb4(5, 8, 9));

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) {
        float h = i * kTau / 16.f;
        art.barge[i] = gs::uploadMipped(vdp, paintBarge(h));
        art.drive[i] = gs::uploadMipped(vdp, paintDrive(h));
    }
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.timber = gs::uploadMipped(vdp, paintTimber());
    art.pile = gs::uploadMipped(vdp, paintPile());
    art.boomArm = gs::uploadMipped(vdp, paintBoomArm());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.bird[0] = gs::uploadMipped(vdp, paintBird(0));
    art.bird[1] = gs::uploadMipped(vdp, paintBird(1));
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.chain = gs::uploadMipped(vdp, paintChain());
    art.title = words(vdp, "BARGE BOOM", 3);
    art.delivered = words(vdp, "DELIVERED", 3);
    art.shortOf = words(vdp, "SHORT", 3);
    art.offBoom = words(vdp, "NOT ON THE BOOM", 2);
    art.broke = words(vdp, "BROKE THE BOOM", 2);
    art.crew = words(vdp, "OTHER CREW", 2);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.paused = words(vdp, "PAUSED", 3);
}

}  // namespace bargeboom
