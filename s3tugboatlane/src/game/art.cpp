#include "art.h"

#include <cmath>
#include <vector>

namespace tuglane {
namespace {

using gs::Bitmap;
using gs::Pt;

const uint16_t kInk = gs::rgb4(1, 1, 2);

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    if (i == 15) vdp.setColor(pal * 16 + 15, kInk);
}

Bitmap paintTug(float heading) {
    Bitmap b(104, 104);
    const float cx = 52.f, cy = 52.f, sc = 5.f;
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

    // Bow is +x. Outer boot, black hull, working deck, house aft, yellow stack.
    polyB({{6.4f, 0.f}, {4.7f, 2.35f}, {-5.1f, 2.7f}, {-6.4f, 1.45f}, {-6.4f, -1.45f}, {-5.1f, -2.7f}, {4.7f, -2.35f}}, 3);
    polyB({{6.05f, 0.f}, {4.4f, 2.05f}, {-4.85f, 2.35f}, {-6.05f, 1.2f}, {-6.05f, -1.2f}, {-4.85f, -2.35f}, {4.4f, -2.05f}}, 2);
    polyB({{5.55f, 0.f}, {4.0f, 1.7f}, {-4.5f, 1.95f}, {-5.6f, 0.95f}, {-5.6f, -0.95f}, {-4.5f, -1.95f}, {4.0f, -1.7f}}, 1);
    polyB({{4.6f, 0.55f}, {2.3f, 0.55f}, {2.3f, -0.55f}, {4.6f, -0.55f}}, 9);
    polyB({{3.7f, 0.22f}, {2.7f, 0.22f}, {2.7f, -0.22f}, {3.7f, -0.22f}}, 12);
    polyB({{-0.4f, 1.55f}, {-4.15f, 1.55f}, {-4.15f, -1.55f}, {-0.4f, -1.55f}}, 4);
    polyB({{-0.7f, 1.2f}, {-2.15f, 1.2f}, {-2.15f, -1.2f}, {-0.7f, -1.2f}}, 5);
    polyB({{-2.45f, 0.85f}, {-3.55f, 0.85f}, {-3.55f, -0.85f}, {-2.45f, -0.85f}}, 10);
    ell(-3.15f, 0.f, 6.4f, 6.4f, 7);
    ell(-3.15f, 0.f, 3.6f, 3.6f, 11);
    ell(-5.35f, 1.15f, 2.5f, 2.5f, 8);
    ell(-5.35f, -1.15f, 2.5f, 2.5f, 8);
    ell(5.35f, 0.f, 2.8f, 2.2f, 13);
    ell(1.6f, 1.85f, 2.6f, 2.6f, 14);
    ell(1.6f, -1.85f, 2.6f, 2.6f, 14);
    b.outline(15, false);
    return b;
}

Bitmap paintShade() {
    Bitmap b(40, 22);
    b.ellipse(20, 11, 18, 8, 1);
    return b;
}

Bitmap paintBuoy(bool green) {
    Bitmap b(18, 26);
    b.ellipse(9, 16, 5, 3, 3);
    b.poly({{9, 2}, {14, 14}, {4, 14}}, green ? 4 : 1);
    b.poly({{9, 5}, {12, 13}, {6, 13}}, green ? 5 : 2);
    b.rect(8, 14, 2, 8, 3);
    b.ellipse(9, 4, 1.4f, 1.4f, 6);
    return b;
}

Bitmap paintShed() {
    Bitmap b(44, 30);
    b.rect(2, 10, 40, 18, 2);
    b.poly({{1, 10}, {22, 2}, {43, 10}}, 4);
    b.rect(6, 16, 8, 8, 7);
    b.rect(28, 15, 10, 6, 5);
    b.rect(18, 18, 6, 10, 6);
    b.outline(15, false);
    return b.cropToContent(0);
}

Bitmap paintHouse() {
    Bitmap b(56, 34);
    b.rect(2, 12, 52, 20, 1);
    b.poly({{0, 12}, {28, 2}, {56, 12}}, 4);
    b.rect(6, 16, 10, 8, 7);
    b.rect(22, 16, 10, 8, 7);
    b.rect(40, 17, 10, 7, 5);
    b.rect(24, 24, 8, 8, 6);
    b.rect(4, 8, 3, 8, 11);
    b.outline(15, false);
    return b.cropToContent(0);
}

Bitmap paintCrate() {
    Bitmap b(22, 18);
    b.rect(1, 1, 20, 16, 9);
    b.rect(2, 2, 18, 14, 10);
    b.line(2, 2, 20, 16, 12, 1.2f);
    b.line(2, 16, 20, 2, 12, 1.2f);
    b.outline(6, false);
    return b;
}

Bitmap paintBollard() {
    Bitmap b(12, 16);
    b.rect(5, 6, 2, 10, 2);
    b.ellipse(6, 5, 4.2f, 3.4f, 1);
    b.ellipse(6, 5, 1.6f, 1.3f, 3);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(12, 24);
    b.rect(5, 8, 2, 16, 2);
    b.ellipse(6, 5, 4.2f, 4.2f, 1);
    b.ellipse(6, 5, 2.1f, 2.1f, 3);
    return b;
}

Bitmap paintPost() {
    Bitmap b(14, 32);
    b.rect(5, 8, 4, 22, 2);
    b.rect(3, 6, 8, 4, 1);
    b.ellipse(7, 4, 4, 3, 3);
    b.rect(6, 28, 2, 4, 4);
    return b;
}

Bitmap paintLine() {
    Bitmap b(32, 6);
    for (int x = 0; x < b.w; x++) {
        int c = ((x / 4) & 1) ? 1 : 2;
        for (int y = 1; y < 5; y++) b.set(x, y, c);
    }
    return b;
}

Bitmap paintFoam() {
    Bitmap b(16, 10);
    b.ellipse(8, 5, 7, 3.4f, 1);
    b.ellipse(8, 5, 3, 1.4f, 2);
    return b;
}

Bitmap paintSmoke() {
    Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 5, 1);
    b.ellipse(6, 6, 3, 2.2f, 2);
    return b;
}

Bitmap paintGull(int flap) {
    Bitmap b(20, 12);
    float tip = flap ? 2.f : 8.f;
    b.line(2, tip, 9, 6, 1, 1.4f);
    b.line(18, tip, 11, 6, 1, 1.4f);
    b.ellipse(10, 6, 1.6f, 1.2f, 2);
    return b;
}

Bitmap paintPin() {
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 10, 12), gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_TUG,
           {0, gs::rgb4(12, 11, 8), gs::rgb4(1, 1, 1), gs::rgb4(12, 2, 2), gs::rgb4(15, 15, 13), gs::rgb4(4, 10, 13),
            gs::rgb4(2, 2, 3), gs::rgb4(15, 12, 2), gs::rgb4(3, 3, 4), gs::rgb4(6, 5, 3), gs::rgb4(9, 8, 6),
            gs::rgb4(13, 6, 2), gs::rgb4(14, 14, 14), gs::rgb4(15, 14, 8), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_QUAY,
           {0, gs::rgb4(10, 9, 8), gs::rgb4(6, 6, 5), gs::rgb4(4, 4, 4), gs::rgb4(11, 4, 3), gs::rgb4(7, 3, 2),
            gs::rgb4(2, 2, 2), gs::rgb4(8, 12, 14), gs::rgb4(12, 11, 9), gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 4),
            gs::rgb4(5, 5, 6), gs::rgb4(9, 6, 3), gs::rgb4(14, 14, 13), gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(14, 2, 2), gs::rgb4(15, 8, 6), gs::rgb4(2, 2, 2), gs::rgb4(15, 4, 3), gs::rgb4(12, 12, 12),
                          gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_GREEN,
           {0, gs::rgb4(2, 10, 3), gs::rgb4(8, 15, 8), gs::rgb4(2, 2, 2), gs::rgb4(3, 13, 5), gs::rgb4(12, 15, 12),
            gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 15, 15)});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(12, 12, 12), gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 8, 3)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 6), gs::rgb4(5, 3, 1)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 6), gs::rgb4(3, 3, 3), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 13, 2), gs::rgb4(2, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(14, 8, 2)});
    setPal(vdp, PAL_END, {0, gs::rgb4(15, 15, 14), gs::rgb4(13, 2, 2), gs::rgb4(15, 12, 3), gs::rgb4(3, 3, 3)});

    for (int i = 0; i < 16; i++) vdp.setColor(PAL_LANE * 16 + i, 0);
    vdp.setColor(PAL_LANE * 16 + 1, gs::rgb4(7, 7, 6));
    vdp.setColor(PAL_LANE * 16 + 2, gs::rgb4(5, 5, 4));
    vdp.setColor(PAL_LANE * 16 + 3, gs::rgb4(8, 7, 5));
    vdp.setColor(PAL_LANE * 16 + 4, gs::rgb4(13, 11, 3));
    vdp.setColor(PAL_LANE * 16 + 5, gs::rgb4(14, 13, 9));
    vdp.setColor(PAL_LANE * 16 + 11, gs::rgb4(1, 6, 9));
    vdp.setColor(PAL_LANE * 16 + 12, gs::rgb4(2, 8, 11));
    vdp.setColor(PAL_LANE * 16 + 13, gs::rgb4(10, 14, 14));

    vdp.setFogColor(gs::rgb4(2, 6, 8));
    loadFont(vdp, art);
    const float tau = 6.28318530718f;
    for (int i = 0; i < 16; i++) art.tug[i] = gs::uploadMipped(vdp, paintTug(i * tau / 16.f));
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.buoyR = gs::uploadMipped(vdp, paintBuoy(false));
    art.buoyG = gs::uploadMipped(vdp, paintBuoy(true));
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.house = gs::uploadMipped(vdp, paintHouse());
    art.crate = gs::uploadMipped(vdp, paintCrate());
    art.bollard = gs::uploadMipped(vdp, paintBollard());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.line = gs::uploadMipped(vdp, paintLine());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.smoke = gs::uploadMipped(vdp, paintSmoke());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(0));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(1));
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.title = words(vdp, "TUGBOAT LANE", 3);
    art.held = words(vdp, "HELD", 3);
    art.left = words(vdp, "LEFT THE LANE", 2);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.ranout = words(vdp, "RAN OUT", 3);
    art.paused = words(vdp, "PAUSED", 3);
    art.gate = words(vdp, "END", 2);
}

}  // namespace tuglane
