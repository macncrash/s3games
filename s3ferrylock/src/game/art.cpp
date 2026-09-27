#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace ferrylock {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    return {cx + lx * c - ly * s, cy + lx * s + ly * c};
}

Bitmap paintHull(float heading) {
    Bitmap b(kHullPx, kHullPx);
    const float cx = kHullPx * 0.5f, cy = kHullPx * 0.5f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, c, s));
        b.poly(w, col);
    };
    // Nose is -local Y so heading 0 points up the bitmap.
    poly({{0.f, -34.f}, {14.f, -8.f}, {14.f, 28.f}, {6.f, 34.f}, {-6.f, 34.f}, {-14.f, 28.f}, {-14.f, -8.f}}, 1);
    poly({{0.f, -26.f}, {8.f, -6.f}, {8.f, 22.f}, {-8.f, 22.f}, {-8.f, -6.f}}, 2);
    poly({{0.f, -16.f}, {5.f, -4.f}, {5.f, 8.f}, {-5.f, 8.f}, {-5.f, -4.f}}, 3);
    b.ellipse(cx, cy - 2.f, 3.f, 3.f, 4);
    return b;
}

Bitmap paintLeaf() {
    Bitmap b(kLeafPx, 18);
    b.rect(2, 4, kLeafPx - 4, 10, 1);
    b.rect(2, 4, kLeafPx - 4, 3, 2);
    for (int x = 8; x < kLeafPx - 4; x += 10) b.rect(float(x), 5, 2, 8, 3);
    return b;
}

Bitmap blob(int w, int h, int c) {
    Bitmap b(w, h);
    b.ellipse(w * 0.5f, h * 0.5f, w * 0.45f, h * 0.4f, c);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
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
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(6, 10, 12), gs::rgb4(15, 8, 3), gs::rgb4(15, 4, 3), shadow});
    setPal(vdp, PAL_FERRY, {0, gs::rgb4(12, 4, 3), gs::rgb4(15, 10, 6), gs::rgb4(8, 12, 14), gs::rgb4(15, 14, 6), shadow});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(6, 10, 4), gs::rgb4(10, 8, 4), gs::rgb4(4, 7, 3)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(5, 4, 3), gs::rgb4(9, 8, 6), gs::rgb4(3, 2, 2), ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 12, 14)});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_GULL, {0, ink, gs::rgb4(8, 8, 9)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 12, 3), ink});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(7, 8, 9)});
    setPal(vdp, PAL_HOUSE, {0, gs::rgb4(12, 8, 5), gs::rgb4(6, 4, 3), gs::rgb4(15, 12, 8)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 14, 4), gs::rgb4(8, 6, 2)});

    for (int i = 0; i < 16; i++) art.hull[i] = gs::uploadMipped(vdp, paintHull(i * 3.14159265f / 8.f));
    Bitmap leaf = paintLeaf();
    for (int i = 0; i < kLeafFrames; i++) art.leaf[i] = gs::uploadMipped(vdp, leaf);
    art.wall = gs::uploadMipped(vdp, blob(24, 48, 1));
    art.shore = gs::uploadMipped(vdp, blob(40, 16, 2));
    art.sill = gs::uploadMipped(vdp, blob(28, 10, 1));
    {
        Bitmap h(28, 22);
        h.rect(2, 8, 24, 12, 1);
        h.poly({{2.f, 8.f}, {14.f, 1.f}, {26.f, 8.f}}, 2);
        h.rect(11, 12, 6, 7, 3);
        art.house = gs::uploadMipped(vdp, h);
    }
    art.lamp = gs::uploadMipped(vdp, blob(8, 16, 1));
    art.bollard = gs::uploadMipped(vdp, blob(8, 14, 1));
    art.post = gs::uploadMipped(vdp, blob(8, 20, 1));
    art.buoy[0] = gs::uploadMipped(vdp, blob(12, 16, 1));
    art.buoy[1] = gs::uploadMipped(vdp, blob(12, 16, 2));
    art.dash = gs::uploadMipped(vdp, blob(48, 12, 1));
    art.foam = gs::uploadMipped(vdp, blob(16, 8, 1));
    art.gull[0] = gs::uploadMipped(vdp, blob(16, 8, 1));
    art.gull[1] = gs::uploadMipped(vdp, blob(16, 8, 1));
    art.title = gs::uploadMipped(vdp, gs::textBitmap("FERRY LOCK", gs::TextStyle{3, 1, 0, 0, 1}));
    art.endSign = gs::uploadMipped(vdp, gs::textBitmap("THE END", gs::TextStyle{3, 1, 0, 0, 1}));
    art.clearWord = gs::uploadMipped(vdp, gs::textBitmap("CLEAR", gs::TextStyle{4, 1, 0, 0, 1}));
    art.scraped = gs::uploadMipped(vdp, gs::textBitmap("SCRAPED", gs::TextStyle{3, 1, 0, 0, 1}));
    art.missed = gs::uploadMipped(vdp, gs::textBitmap("MISSED THE END", gs::TextStyle{2, 1, 0, 0, 1}));
    art.paused = gs::uploadMipped(vdp, gs::textBitmap("PAUSED", gs::TextStyle{3, 1, 0, 0, 1}));
    loadFont(vdp, art);
}

}  // namespace ferrylock
