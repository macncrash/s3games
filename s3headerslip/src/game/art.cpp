#include "art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace headerslip {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap paintSloop(float heading) {
    Bitmap b(96, 96);
    const float cx = 48.f, cy = 50.f, sc = 4.2f;
    const float co = std::cos(heading), sn = std::sin(heading);
    auto polyB = [&](std::initializer_list<Pt> meters, int col) {
        std::vector<Pt> p;
        for (const Pt& m : meters) {
            float dx = (m.first * co + m.second * sn) * sc;
            float dy = (-m.first * sn + m.second * co) * sc;
            p.push_back({cx + dx, cy + dy});
        }
        b.poly(p, col);
    };
    auto blob = [&](float bx, float by, float rx, float ry, int col) {
        float dx = (bx * co + by * sn) * sc;
        float dy = (-bx * sn + by * co) * sc;
        b.ellipse(cx + dx, cy + dy, rx, ry, col);
    };
    polyB({{3.4f, 0.f}, {2.2f, 1.15f}, {-2.6f, 1.25f}, {-3.3f, 0.7f}, {-3.3f, -0.7f}, {-2.6f, -1.25f}, {2.2f, -1.15f}}, 4);
    polyB({{3.0f, 0.f}, {1.9f, 0.85f}, {-2.3f, 0.95f}, {-2.9f, 0.45f}, {-2.9f, -0.45f}, {-2.3f, -0.95f}, {1.9f, -0.85f}}, 3);
    polyB({{1.6f, 0.15f}, {-0.4f, 2.7f}, {-0.55f, 0.15f}}, 1);
    polyB({{1.5f, -0.05f}, {-0.35f, -2.35f}, {-0.5f, -0.05f}}, 2);
    blob(-0.2f, 0.f, 1.3f, 5.6f, 5);
    blob(0.35f, 0.35f, 1.5f, 1.5f, 6);
    blob(2.5f, 0.f, 1.6f, 1.1f, 7);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintVane(float heading) {
    Bitmap b(32, 32);
    const float co = std::cos(heading), sn = std::sin(heading);
    auto P = [&](float x, float y) -> Pt {
        return {16.f + (x * co + y * sn), 16.f + (-x * sn + y * co)};
    };
    b.poly({P(10.f, 0.f), P(-4.f, 5.f), P(-2.f, 0.f), P(-4.f, -5.f)}, 1);
    b.ellipse(16, 16, 2.2f, 2.2f, 2);
    return b.cropToContent(1);
}

Bitmap paintPier() {
    Bitmap b(72, 22);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            int plank = ((x / 8) + (y / 11)) & 1;
            int c = y < 3 ? 4 : (plank ? 2 : 3);
            if (y > 18) c = 1;
            b.set(x, y, c);
        }
    return b;
}

Bitmap paintPost() {
    Bitmap b(10, 28);
    b.rect(3, 6, 4, 22, 2);
    b.rect(1, 2, 8, 6, 1);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(14, 20);
    b.ellipse(7, 9, 5.f, 6.f, 1);
    b.rect(6, 14, 2, 5, 2);
    b.ellipse(7, 4, 2.2f, 2.2f, 3);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(16, 8);
    b.ellipse(5, 4, 4.f, 2.f, 1);
    b.ellipse(11, 4, 3.f, 1.6f, 1);
    return b;
}

Bitmap paintCleat() {
    Bitmap b(16, 8);
    b.rect(1, 3, 14, 2, 2);
    b.rect(3, 1, 2, 6, 1);
    b.rect(11, 1, 2, 6, 1);
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
    const uint16_t ink = gs::rgb4(15, 15, 13);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 10, 9), gs::rgb4(15, 12, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 13, 14), gs::rgb4(10, 6, 3), gs::rgb4(6, 3, 2), gs::rgb4(4, 3, 3),
            gs::rgb4(13, 3, 3), gs::rgb4(14, 12, 6), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(8, 10, 12), gs::rgb4(5, 7, 9), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3), gs::rgb4(3, 3, 4),
            gs::rgb4(15, 12, 3), gs::rgb4(9, 9, 8), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_PIER, {0, gs::rgb4(4, 4, 4), gs::rgb4(9, 8, 6), gs::rgb4(12, 10, 7), gs::rgb4(14, 13, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(14, 4, 3), gs::rgb4(3, 3, 3), gs::rgb4(15, 14, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 9), gs::rgb4(2, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(6, 2, 2), gs::rgb4(15, 14, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BANNER, {0, ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 3), 0});
    setPal(vdp, PAL_WIND, {0, gs::rgb4(15, 14, 8), gs::rgb4(12, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_WATER,
           {0, gs::rgb4(3, 8, 10), gs::rgb4(5, 11, 12), gs::rgb4(2, 5, 7), gs::rgb4(8, 10, 8), gs::rgb4(1, 4, 6), 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0});
    setPal(vdp, PAL_SHORE,
           {0, gs::rgb4(3, 5, 2), gs::rgb4(5, 7, 3), gs::rgb4(7, 6, 3), gs::rgb4(4, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    vdp.setFogColor(gs::rgb4(6, 8, 9));

    const float step = 6.2831853f / 8.f;
    for (int i = 0; i < 8; i++) {
        art.hull[i] = gs::uploadMipped(vdp, paintSloop(i * step));
        art.vane[i] = gs::uploadMipped(vdp, paintVane(i * step));
    }
    art.pier = gs::uploadMipped(vdp, paintPier());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.cleat = gs::uploadMipped(vdp, paintCleat());
    art.title = word(vdp, "HEADER SLIP", 3);
    art.take = word(vdp, "TAKE THE HEADER", 2);
    art.berthed = word(vdp, "BERTHED", 3);
    art.crew = word(vdp, "OTHER CREW", 2);
    art.tide = word(vdp, "TIDE TURNED", 2);
    art.wall = word(vdp, "HEAD WALL", 2);
    art.aground = word(vdp, "AGROUND", 2);
    loadFont(vdp, art);
}

}  // namespace headerslip
