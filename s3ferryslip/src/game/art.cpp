#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace ferryslip {
namespace {

constexpr float kTau = 6.2831853f;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    return {cx + ly * c + lx * s, cy - (ly * s - lx * c)};
}

Bitmap paintHull(float heading, bool rival) {
    Bitmap b(80, 80);
    const float cx = 40.f, cy = 40.f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, c, s));
        b.poly(w, col);
    };
    int hull = rival ? 2 : 1;
    int deck = rival ? 3 : 4;
    int cabin = rival ? 5 : 6;
    poly({{0.f, 32.f}, {8.f, 24.f}, {10.f, 8.f}, {10.f, -16.f}, {7.f, -26.f}, {-7.f, -26.f}, {-10.f, -16.f}, {-10.f, 8.f}, {-8.f, 24.f}}, hull);
    poly({{0.f, 26.f}, {6.f, 18.f}, {6.f, -18.f}, {-6.f, -18.f}, {-6.f, 18.f}}, deck);
    poly({{-7.f, -4.f}, {7.f, -4.f}, {6.f, 10.f}, {-6.f, 10.f}}, cabin);
    poly({{-5.f, 0.f}, {5.f, 0.f}, {5.f, 6.f}, {-5.f, 6.f}}, 7);
    poly({{-5.f, 22.f}, {5.f, 22.f}, {3.f, 30.f}, {-3.f, 30.f}}, 8);
    poly({{-2.f, -22.f}, {2.f, -22.f}, {1.4f, -8.f}, {-1.4f, -8.f}}, 9);
    b.outline(10, false);
    return b;
}

Bitmap pierArt() {
    Bitmap b(28, 120);
    b.rect(4, 0, 20, 120, 1);
    for (int y = 4; y < 116; y += 10) {
        b.rect(0, y, 6, 4, 2);
        b.rect(22, y, 6, 4, 2);
        b.rect(8, y + 3, 12, 2, 3);
    }
    b.outline(4, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(18, 18);
    b.ellipse(9, 9, 7, 7, 1);
    b.ellipse(9, 9, 3, 3, 2);
    b.outline(3, false);
    return b;
}

Bitmap shedArt() {
    Bitmap b(72, 40);
    b.rect(6, 16, 60, 20, 1);
    b.poly({{4.f, 16.f}, {36.f, 4.f}, {68.f, 16.f}}, 2);
    b.rect(30, 22, 12, 14, 3);
    b.rect(12, 20, 10, 8, 4);
    b.rect(50, 20, 10, 8, 4);
    b.outline(5, false);
    return b;
}

Bitmap clockArt(float ang) {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 14, 14, 1);
    b.ellipse(16, 16, 11, 11, 2);
    float x = 16.f + std::cos(ang) * 8.f;
    float y = 16.f + std::sin(ang) * 8.f;
    b.line(16, 16, x, y, 3, 2.f);
    b.ellipse(16, 16, 2, 2, 4);
    return b;
}

Bitmap foamArt() {
    Bitmap b(12, 8);
    b.ellipse(6, 4, 5, 3, 1);
    b.ellipse(4, 3, 2, 1, 2);
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

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int fill, int edge) {
    gs::TextStyle st{scale, fill, edge, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FERRY, {0, gs::rgb4(2, 9, 6), gs::rgb4(12, 3, 3), gs::rgb4(14, 13, 10), gs::rgb4(15, 15, 15),
                            gs::rgb4(13, 12, 9), gs::rgb4(4, 8, 13), gs::rgb4(15, 12, 3), gs::rgb4(8, 4, 2),
                            gs::rgb4(1, 1, 2), gs::rgb4(1, 3, 3), 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RIVAL, {0, gs::rgb4(8, 8, 9), gs::rgb4(11, 2, 2), gs::rgb4(6, 6, 7), gs::rgb4(10, 10, 11),
                            gs::rgb4(7, 2, 2), gs::rgb4(3, 3, 4), gs::rgb4(13, 11, 8), gs::rgb4(14, 8, 2),
                            gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2), 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PIER, {0, gs::rgb4(9, 7, 4), gs::rgb4(6, 4, 2), gs::rgb4(12, 10, 6), gs::rgb4(3, 2, 1),
                           gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_POST, {0, gs::rgb4(12, 6, 2), gs::rgb4(15, 13, 6), gs::rgb4(4, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(14, 12, 8), gs::rgb4(15, 15, 15), gs::rgb4(12, 2, 2), gs::rgb4(2, 2, 3),
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 8), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(4, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(10, 12, 13), gs::rgb4(2, 3, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    for (int i = 0; i < 12; i++) {
        float h = i * kTau / 12.f;
        art.hull[i] = gs::uploadMipped(vdp, paintHull(h, false));
        art.rival[i] = gs::uploadMipped(vdp, paintHull(h, true));
    }
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    for (int i = 0; i < 6; i++) art.clock[i] = gs::uploadMipped(vdp, clockArt(-1.5708f + i / 5.f * 3.14159f));
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.title = words(vdp, "S3 FERRY SLIP", 3, 1, 2);
    art.berthed = words(vdp, "BERTHED", 4, 1, 2);
    art.turned = words(vdp, "TIDE TURNED", 3, 1, 2);
    art.paused = words(vdp, "PAUSED", 4, 1, 2);
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
}

}  // namespace ferryslip
