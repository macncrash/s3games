#include "art.h"

#include <cmath>
#include <vector>

namespace keelbox {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap paintBoat(float heading) {
    Bitmap b(168, 168);
    const float cx = 84.f, cy = 84.f, sc = 6.4f;
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
    // White main to starboard of the mast, so the boat reads as a keel under sail.
    polyB({{1.1f, 0.15f}, {1.1f, 3.55f}, {-3.6f, 0.35f}, {-3.4f, 0.1f}}, 4);
    polyB({{0.85f, 0.35f}, {0.7f, 3.15f}, {-3.15f, 0.45f}}, 5);
    polyB({{5.55f, 0.f}, {3.4f, 1.15f}, {-4.6f, 1.25f}, {-5.35f, 0.45f}, {-5.35f, -0.45f}, {-4.6f, -1.25f}, {3.4f, -1.15f}}, 2);
    polyB({{5.15f, 0.f}, {3.15f, 0.95f}, {-4.3f, 1.02f}, {-5.0f, 0.35f}, {-5.0f, -0.35f}, {-4.3f, -1.02f}, {3.15f, -0.95f}}, 3);
    polyB({{1.6f, -0.72f}, {1.6f, 0.72f}, {-1.15f, 0.78f}, {-1.15f, -0.78f}}, 8);
    polyB({{1.35f, -0.42f}, {1.35f, 0.42f}, {0.15f, 0.42f}, {0.15f, -0.42f}}, 6);
    polyB({{0.15f, -0.08f}, {0.15f, 0.08f}, {-3.55f, 0.08f}, {-3.55f, -0.08f}}, 7);
    ell(4.55f, 0.f, 3.2f, 2.4f, 1);
    ell(-4.85f, 0.f, 2.6f, 2.1f, 9);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintShade() {
    Bitmap b(40, 18);
    b.ellipse(20, 9, 18, 7, 1);
    return b;
}

Bitmap paintCommittee() {
    Bitmap b(36, 22);
    b.poly({{2, 16}, {8, 6}, {28, 6}, {34, 16}}, 2);
    b.rect(10, 8, 16, 7, 3);
    b.rect(14, 3, 3, 6, 4);
    b.rect(22, 10, 5, 4, 5);
    b.outline(1, false);
    return b.cropToContent(0);
}

Bitmap paintPin() {
    Bitmap b(14, 22);
    b.ellipse(7, 8, 5, 6, 1);
    b.rect(6, 4, 2, 10, 2);
    b.rect(6, 14, 2, 7, 3);
    return b;
}

Bitmap paintPost() {
    Bitmap b(12, 26);
    b.rect(5, 6, 3, 18, 2);
    b.ellipse(6, 4, 4, 3, 1);
    b.rect(5, 22, 2, 4, 3);
    return b;
}

Bitmap paintHbar() {
    Bitmap b(28, 6);
    b.rect(0, 1, 28, 4, 1);
    b.rect(1, 2, 26, 2, 2);
    return b;
}

Bitmap paintVbar() {
    Bitmap b(6, 28);
    b.rect(1, 0, 4, 28, 1);
    b.rect(2, 1, 2, 26, 2);
    return b;
}

Bitmap paintHatch() {
    Bitmap b(40, 40);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            bool edge = x < 2 || y < 2 || x >= 38 || y >= 38;
            bool hatch = ((x / 4 + y / 4) & 1) == 0;
            if (edge) b.set(x, y, 1);
            else if (hatch) b.set(x, y, 2);
        }
    }
    return b;
}

Bitmap paintReed() {
    Bitmap b(10, 18);
    b.line(3, 16, 4, 4, 1, 1.2f);
    b.line(6, 16, 5, 2, 2, 1.2f);
    b.ellipse(4, 3, 2, 2, 3);
    return b;
}

Bitmap paintVane() {
    Bitmap b(16, 16);
    b.poly({{8, 1}, {14, 8}, {8, 6}, {2, 8}}, 1);
    b.ellipse(8, 8, 2, 2, 2);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(14, 10);
    b.ellipse(7, 5, 6, 3.4f, 1);
    b.ellipse(7, 5, 2.4f, 1.3f, 2);
    return b;
}

Bitmap paintGull(int flap) {
    Bitmap b(18, 10);
    float tip = flap ? 2.f : 7.f;
    b.line(1, tip, 8, 5, 1, 1.3f);
    b.line(17, tip, 10, 5, 1, 1.3f);
    b.ellipse(9, 5, 1.4f, 1.1f, 2);
    return b;
}

Bitmap paintArrow() {
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 10, 12), gs::rgb4(14, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BOAT,
           {0, gs::rgb4(14, 13, 11), gs::rgb4(2, 3, 5), gs::rgb4(12, 8, 3), gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15),
            gs::rgb4(4, 8, 12), gs::rgb4(6, 4, 2), gs::rgb4(9, 3, 2), gs::rgb4(1, 1, 2), gs::rgb4(15, 12, 4),
            gs::rgb4(3, 6, 8), gs::rgb4(7, 7, 8), gs::rgb4(11, 6, 2), gs::rgb4(8, 10, 6), ink});
    setPal(vdp, PAL_SHORE,
           {0, gs::rgb4(6, 10, 4), gs::rgb4(4, 7, 3), gs::rgb4(9, 12, 5), gs::rgb4(3, 5, 3), gs::rgb4(12, 11, 7),
            gs::rgb4(2, 3, 2), gs::rgb4(8, 8, 6), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 13, 2), gs::rgb4(10, 8, 1), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_END,
           {0, gs::rgb4(12, 3, 2), gs::rgb4(15, 15, 14), gs::rgb4(3, 5, 7), gs::rgb4(14, 12, 4), gs::rgb4(6, 8, 10), 0, 0, 0, 0,
            0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SAIL, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 13, 14), gs::rgb4(8, 9, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 6), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIND, {0, gs::rgb4(14, 15, 15), gs::rgb4(6, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    for (int i = 0; i < 16; i++) vdp.setColor(PAL_CH * 16 + i, 0);
    vdp.setColor(PAL_CH * 16 + 1, gs::rgb4(3, 8, 10));
    vdp.setColor(PAL_CH * 16 + 2, gs::rgb4(2, 6, 9));
    vdp.setColor(PAL_CH * 16 + 11, gs::rgb4(4, 12, 14));
    vdp.setColor(PAL_CH * 16 + 12, gs::rgb4(1, 6, 10));
    vdp.setColor(PAL_CH * 16 + 13, gs::rgb4(10, 15, 15));

    loadFont(vdp, art);
    const float tau = 6.28318530718f;
    for (int i = 0; i < 12; i++) art.boat[i] = gs::uploadMipped(vdp, paintBoat(i * tau / 12.f));
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.committee = gs::uploadMipped(vdp, paintCommittee());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.hbar = gs::uploadMipped(vdp, paintHbar());
    art.vbar = gs::uploadMipped(vdp, paintVbar());
    art.hatch = gs::uploadMipped(vdp, paintHatch());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.vane = gs::uploadMipped(vdp, paintVane());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(0));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(1));
    art.arrow = gs::uploadMipped(vdp, paintArrow());
    art.title = words(vdp, "KEEL BOX", 3);
    art.stopped = words(vdp, "STOPPED", 3);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.outside = words(vdp, "OUTSIDE", 3);
    art.stoppedShort = words(vdp, "SHORT", 3);
    art.paused = words(vdp, "PAUSED", 3);
}

}  // namespace keelbox
