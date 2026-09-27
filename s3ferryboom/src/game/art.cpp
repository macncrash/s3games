#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace fboom {
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

Bitmap paintFerry(float heading) {
    Bitmap b(80, 96);
    const float cx = 40.f, cy = 48.f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, c, s));
        b.poly(w, col);
    };
    // Double-ended car ferry. Local +Y is the bow that tows the drive.
    poly({{0, 42}, {11, 30}, {13, 8}, {13, -22}, {10, -36}, {0, -42}, {-10, -36}, {-13, -22}, {-13, 8}, {-11, 30}}, 2);
    poly({{0, 34}, {8, 24}, {8, -28}, {0, -34}, {-8, -28}, {-8, 24}}, 1);
    poly({{-7, -6}, {7, -6}, {7, 16}, {-7, 16}}, 3);
    poly({{-5, -2}, {5, -2}, {5, 4}, {-5, 4}}, 6);
    poly({{-5, 6}, {5, 6}, {5, 12}, {-5, 12}}, 6);
    poly({{-2.2f, 18}, {2.2f, 18}, {1.6f, 30}, {-1.6f, 30}}, 4);
    poly({{-9, 22}, {9, 22}, {7, 28}, {-7, 28}}, 5);
    b.outline(7, false);
    return b;
}

Bitmap paintDrive(float heading) {
    Bitmap b(56, 72);
    const float cx = 28.f, cy = 36.f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, c, s));
        b.poly(w, col);
    };
    poly({{0, 30}, {12, 26}, {12, -26}, {0, -30}, {-12, -26}, {-12, 26}}, 1);
    for (float y : {-16.f, 0.f, 16.f}) {
        poly({{-8, y - 6}, {8, y - 6}, {7, y + 5}, {-7, y + 5}}, 2);
        poly({{-6, y - 3}, {6, y - 3}, {5, y + 1}, {-5, y + 1}}, 4);
        poly({{-2, y + 1}, {2, y + 1}, {2, y + 4}, {-2, y + 4}}, 5);
    }
    b.outline(3, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 28);
    b.rect(5, 2, 6, 24, 1);
    b.rect(3, 0, 10, 5, 2);
    b.rect(4, 18, 8, 4, 3);
    b.outline(4, false);
    return b;
}

Bitmap headArt() {
    Bitmap b(48, 14);
    b.rect(0, 3, 48, 8, 1);
    for (int x = 2; x < 46; x += 8) b.rect(x, 4, 4, 6, 2);
    b.outline(4, false);
    return b;
}

Bitmap bankArt() {
    Bitmap b(20, 40);
    b.rect(0, 0, 20, 40, 1);
    for (int y = 4; y < 36; y += 8) b.rect(2, y, 16, 3, 2);
    return b;
}

Bitmap foamArt() {
    Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 3.2f, 1);
    b.ellipse(6, 6, 2.4f, 1.4f, 2);
    return b;
}

Bitmap gullArt(bool up) {
    Bitmap b(18, 10);
    if (up) {
        b.line(1, 7, 9, 2, 1, 1.4f);
        b.line(9, 2, 17, 7, 1, 1.4f);
    } else {
        b.line(1, 3, 9, 6, 1, 1.4f);
        b.line(9, 6, 17, 3, 1, 1.4f);
    }
    b.set(9, 5, 2);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FERRY, {0, gs::rgb4(15, 15, 15), gs::rgb4(1, 8, 5), gs::rgb4(12, 13, 14), gs::rgb4(14, 4, 3),
                            gs::rgb4(15, 12, 2), gs::rgb4(4, 10, 14), gs::rgb4(1, 2, 3), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DRIVE, {0, gs::rgb4(6, 5, 4), gs::rgb4(12, 3, 3), gs::rgb4(2, 2, 3), gs::rgb4(8, 12, 15),
                            gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOOM, {0, gs::rgb4(10, 7, 3), gs::rgb4(14, 10, 4), gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1),
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(7, 8, 4), gs::rgb4(5, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(14, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(4, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 8), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(10, 12, 13), gs::rgb4(2, 3, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    for (int i = 0; i < 8; i++) {
        float h = i * kTau / 8.f;
        art.hull[i] = gs::uploadMipped(vdp, paintFerry(h));
        art.drive[i] = gs::uploadMipped(vdp, paintDrive(h));
    }
    art.post = gs::uploadMipped(vdp, postArt());
    art.head = gs::uploadMipped(vdp, headArt());
    art.bank = gs::uploadMipped(vdp, bankArt());
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(true));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(false));
    art.title = words(vdp, "S3 FERRY BOOM", 3, 1, 2);
    art.made = words(vdp, "DELIVERED", 3, 1, 2);
    art.missed = words(vdp, "MISSED THE BOOM", 2, 1, 2);
    art.paused = words(vdp, "PAUSED", 3, 1, 2);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
}

}  // namespace fboom
