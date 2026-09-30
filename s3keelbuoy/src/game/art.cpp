#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace keelbuoy {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Pt hullPt(float h, float lx, float ly) {
    float wx = std::cos(h) * ly + std::sin(h) * lx;
    float wy = std::sin(h) * ly - std::cos(h) * lx;
    return {24.f + wx, 28.f - wy};
}

void boatFrame(Bitmap& b, float h) {
    auto P = [&](float lx, float ly) { return hullPt(h, lx, ly); };
    Pt bow = P(0, 18), port = P(-7, -4), sternP = P(-5, -16), sternS = P(5, -16), stbd = P(7, -4);
    b.poly({bow, port, sternP, sternS, stbd}, 1);
    b.poly({P(0, 14), P(-4, 2), P(4, 2)}, 2);
    b.poly({P(-1, 6), P(-12, -6), P(-1, -8)}, 3);
    b.poly({P(1, 4), P(9, -8), P(1, -10)}, 4);
    Pt mastT = P(0, 16), mastB = P(0, -12);
    b.line(mastT.first, mastT.second, mastB.first, mastB.second, 5, 1.4f);
    b.ellipse(P(0, -14).first, P(0, -14).second, 2.2f, 1.6f, 6);
}

Bitmap buoyBmp(int band) {
    Bitmap b(24, 36);
    b.ellipse(12, 22, 8, 7, 1);
    b.rect(6, 10, 12, 14, 1);
    b.rect(6, 14, 12, 4, band);
    b.ellipse(12, 10, 8, 4, 2);
    b.line(12, 10, 12, 2, 3, 1.2f);
    b.poly({{12, 3}, {20, 6}, {12, 8}}, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& a) {
    const uint16_t shadow = gs::rgb4(0, 0, 0);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 15, 15), gs::rgb4(2, 4, 6), shadow});
    setPal(vdp, PAL_BOAT, {0, gs::rgb4(6, 3, 1), gs::rgb4(10, 6, 2), gs::rgb4(15, 15, 14), gs::rgb4(12, 13, 14),
                           gs::rgb4(4, 3, 2), gs::rgb4(8, 2, 1), shadow});
    setPal(vdp, PAL_MARK0, {0, gs::rgb4(12, 2, 2), gs::rgb4(15, 6, 4), gs::rgb4(3, 2, 1), gs::rgb4(15, 14, 4), shadow});
    setPal(vdp, PAL_MARK1, {0, gs::rgb4(1, 9, 3), gs::rgb4(6, 14, 6), gs::rgb4(3, 2, 1), gs::rgb4(15, 15, 8), shadow});
    setPal(vdp, PAL_MARK2, {0, gs::rgb4(12, 9, 1), gs::rgb4(15, 13, 4), gs::rgb4(3, 2, 1), gs::rgb4(15, 8, 2), shadow});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(7, 5, 2), gs::rgb4(10, 8, 4), gs::rgb4(4, 3, 2), gs::rgb4(13, 12, 8),
                           gs::rgb4(2, 2, 2), shadow});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 12, 13), shadow});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 8, 9), gs::rgb4(15, 10, 2), shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 7), gs::rgb4(1, 5, 2), shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 12, 3), gs::rgb4(6, 3, 1), shadow});

    for (int i = 0; i < 8; i++) {
        Bitmap b(48, 56);
        boatFrame(b, i * 3.14159265f / 4.f);
        a.boat[i] = gs::uploadMipped(vdp, b);
    }
    for (int i = 0; i < 3; i++) a.buoy[i] = gs::uploadMipped(vdp, buoyBmp(i == 0 ? 2 : 4));

    Bitmap dock(64, 48);
    dock.rect(8, 6, 48, 14, 1);
    dock.rect(8, 6, 48, 3, 2);
    for (int i = 0; i < 5; i++) {
        dock.rect(12 + i * 9, 20, 4, 22, 3);
        dock.ellipse(14 + i * 9, 42, 3, 2, 4);
    }
    dock.rect(26, 8, 12, 8, 4);
    a.dock = gs::uploadMipped(vdp, dock);

    Bitmap foam(16, 10);
    foam.ellipse(5, 5, 4, 2, 1);
    foam.ellipse(11, 4, 3, 2, 2);
    a.foam = gs::uploadMipped(vdp, foam);

    for (int f = 0; f < 2; f++) {
        Bitmap g(20, 12);
        g.ellipse(8, 7, 3, 2, 1);
        g.poly({{8, 6}, {2, f ? 3.f : 8.f}, {8, 8}}, 1);
        g.poly({{8, 6}, {16, f ? 2.f : 5.f}, {9, 8}}, 2);
        g.ellipse(11, 6, 1.2f, 1.f, 3);
        a.gull[f] = gs::uploadMipped(vdp, g);
    }

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    big.outline = 2;
    big.spacing = 1;
    a.title = gs::uploadMipped(vdp, gs::textBitmap("KEELBUOY", big));
    a.done = gs::uploadMipped(vdp, gs::textBitmap("DOCKED", big));

    gs::TileAlloc tiles(vdp, 1);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace keelbuoy
