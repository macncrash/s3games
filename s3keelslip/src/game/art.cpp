#include "art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace keel {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float kScale = 3.1f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap paintHull(float heading) {
    Bitmap b(96, 96);
    const float cx = 48.f, cy = 48.f;
    const float co = std::cos(heading), sn = std::sin(heading);
    auto polyB = [&](std::initializer_list<Pt> meters, int col) {
        std::vector<Pt> p;
        for (const Pt& m : meters) {
            float dx = (m.first * co + m.second * sn) * kScale;
            float dy = (-m.first * sn + m.second * co) * kScale;
            p.push_back({cx + dx, cy + dy});
        }
        b.poly(p, col);
    };
    auto blob = [&](float bx, float by, float rx, float ry, int col) {
        float dx = (bx * co + by * sn) * kScale;
        float dy = (-bx * sn + by * co) * kScale;
        b.ellipse(cx + dx, cy + dy, rx, ry, col);
    };
    // Keelboat: long green hull, cream sail, wood deck, white cabin.
    polyB({{3.4f, 0.f}, {2.2f, 0.85f}, {-2.3f, 0.95f}, {-2.7f, 0.45f}, {-2.7f, -0.45f}, {-2.3f, -0.95f}, {2.2f, -0.85f}}, 2);
    polyB({{3.05f, 0.f}, {1.9f, 0.62f}, {-2.05f, 0.7f}, {-2.35f, 0.28f}, {-2.35f, -0.28f}, {-2.05f, -0.7f}, {1.9f, -0.62f}}, 3);
    polyB({{1.3f, -0.42f}, {1.3f, 0.42f}, {-0.9f, 0.48f}, {-0.9f, -0.48f}}, 5);
    polyB({{0.9f, -0.28f}, {0.9f, 0.28f}, {-0.55f, 0.32f}, {-0.55f, -0.32f}}, 6);
    polyB({{0.15f, 0.f}, {-1.7f, 1.55f}, {-1.7f, -1.55f}}, 8);
    polyB({{-0.05f, 0.f}, {-1.45f, 1.15f}, {-1.45f, -1.15f}}, 9);
    blob(0.35f, 0.f, 1.3f, 1.1f, 4);
    blob(2.55f, 0.f, 1.5f, 1.0f, 1);
    blob(-2.15f, 0.f, 1.4f, 0.9f, 7);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintPlank() {
    Bitmap b(28, 48);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            int c = (x < 3 || x > 24) ? 1 : ((y / 6) & 1) ? 2 : 3;
            if (y % 12 == 0) c = 4;
            b.set(x, y, c);
        }
    return b;
}

Bitmap paintPiling() {
    Bitmap b(10, 14);
    b.rect(2, 2, 6, 11, 2);
    b.rect(3, 1, 4, 3, 3);
    b.ellipse(5, 3, 2.f, 2.f, 4);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(14, 18);
    b.ellipse(7, 9, 5.f, 5.f, 1);
    b.rect(6, 13, 2, 4, 2);
    b.ellipse(7, 5, 2.2f, 2.2f, 3);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(12, 8);
    b.ellipse(6, 4, 5.f, 3.f, 1);
    b.ellipse(3, 4, 2.f, 1.5f, 2);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(10, 22);
    b.rect(4, 8, 2, 13, 2);
    b.ellipse(5, 5, 3.2f, 3.2f, 1);
    b.rect(2, 8, 6, 2, 2);
    return b;
}

Bitmap paintDot() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3.f, 3.f, 1);
    return b;
}

Bitmap paintPin() {
    Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
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
    const uint16_t line = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 10, 11), gs::rgb4(15, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 3, 4), line});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(15, 14, 8), gs::rgb4(1, 5, 4), gs::rgb4(2, 8, 6), gs::rgb4(6, 4, 2), gs::rgb4(10, 8, 5),
            gs::rgb4(14, 13, 11), gs::rgb4(3, 2, 2), gs::rgb4(15, 15, 12), gs::rgb4(12, 13, 14), gs::rgb4(8, 9, 8), 0, 0,
            0, 0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_PIER,
           {0, gs::rgb4(4, 4, 5), gs::rgb4(7, 6, 5), gs::rgb4(10, 8, 6), gs::rgb4(13, 11, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(14, 3, 2), gs::rgb4(14, 14, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(7, 15, 8), gs::rgb4(2, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 3, 2), gs::rgb4(15, 14, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BANNER, {0, ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, 0});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 6), gs::rgb4(4, 4, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SLIP,
           {0, gs::rgb4(3, 8, 9), gs::rgb4(5, 11, 10), gs::rgb4(2, 5, 7), gs::rgb4(8, 10, 6), gs::rgb4(1, 4, 5), 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SHORE,
           {0, gs::rgb4(6, 7, 4), gs::rgb4(4, 5, 3), gs::rgb4(8, 8, 5), gs::rgb4(3, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    vdp.setFogColor(gs::rgb4(2, 4, 6));

    const float tau = 6.2831853f;
    for (int i = 0; i < 8; i++) art.hull[i] = gs::uploadMipped(vdp, paintHull(i * tau / 8.f));
    art.plank = gs::uploadMipped(vdp, paintPlank());
    art.piling = gs::uploadMipped(vdp, paintPiling());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.dot = gs::uploadMipped(vdp, paintDot());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.title = word(vdp, "KEEL SLIP", 3);
    art.berthed = word(vdp, "BERTHED", 3);
    art.held = word(vdp, "IN THE SLIP", 2);
    art.tide = word(vdp, "TIDE TURNED", 2);
    art.scraped = word(vdp, "SCRAPED", 2);
    art.lost = word(vdp, "LOST THE HARBOR", 2);
    art.paused = word(vdp, "PAUSED", 2);
    loadFont(vdp, art);
}

}  // namespace keel
